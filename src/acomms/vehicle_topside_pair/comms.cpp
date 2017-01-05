// basic example that illustrates sending a command message every time we get our chance to send

#include <iostream>

#include "dccl.h"
#include "goby/acomms/modem_driver.h"
#include "goby/acomms/amac.h"
#include "goby/acomms/bind.h"

#include "goby/util/as.h"
#include "goby/common/time.h"

#include <boost/lexical_cast.hpp>

#include "messages/nav.pb.h"
#include "messages/command.pb.h"


int startup_failure(const char* name);

void handle_modem_data_request(goby::acomms::protobuf::ModemTransmission* msg);
void handle_modem_receive(const goby::acomms::protobuf::ModemTransmission& msg);

const int topside_id = 1;
const int vehicle_id = 2;


// I wouldn't normally use precompiler definitions
// like this, but it makes the example code short
// and easy to read. In production code, create common library classes
// and functions 
#ifdef TOPSIDE
const int my_id = topside_id;
#endif

#ifdef VEHICLE
const int my_id = vehicle_id;
#endif

// for UDPDriver
boost::shared_ptr<boost::asio::io_service> asio_service;    
    
dccl::Codec dccl_codec;

int main(int argc, char* argv[])
{
    if(argc != 4) return startup_failure(argv[0]);

    std::string serial_port = argv[1];
    std::string log_file = argv[3];
    std::ofstream fout(log_file.c_str());
    if(!fout.is_open())
    {
        std::cerr << "bad value for debug_log_file: " << log_file << std::endl;
        return startup_failure(argv[0]);
    }
    
    //
    //  Initialize Goby/DCCL debug logging
    //
    goby::glog.add_stream(goby::common::logger::DEBUG2, &fout);
    goby::glog.set_name(argv[0]);

    dccl::dlog.connect(dccl::logger::DEBUG1, &fout, true);

    //
    // Create Goby objects
    //
    boost::scoped_ptr<goby::acomms::ModemDriverBase> driver;
    // hard coded here, but could be read by configuration file or command line
    goby::acomms::protobuf::DriverType driver_type;

    if(!goby::acomms::protobuf::DriverType_Parse(argv[2], &driver_type))
    {
        std::cerr << "Invalid DRIVER_TYPE_ENUM " << argv[2] << std::endl;
        std::cerr << "Valid options are: ";
        for(int i = goby::acomms::protobuf::DriverType_MIN+1, n = goby::acomms::protobuf::DriverType_MAX; i <= n; ++i)
        {
            if(goby::acomms::protobuf::DriverType_IsValid(i))
                std::cerr << goby::acomms::protobuf::DriverType_Name(static_cast<goby::acomms::protobuf::DriverType>(i)) << std::endl;
        }
        
        return startup_failure(argv[0]);
    }
    
        
    switch(driver_type)
    {
        default:
            std::cerr << "Driver type not configured for this example: " << goby::acomms::protobuf::DriverType_Name(driver_type) << std::endl;
            return startup_failure(argv[0]);

        case goby::acomms::protobuf::DRIVER_WHOI_MICROMODEM:
            driver.reset(new goby::acomms::MMDriver);
            break;
            
        case goby::acomms::protobuf::DRIVER_BENTHOS_ATM900:
            driver.reset(new goby::acomms::BenthosATM900Driver);
            break;

        case goby::acomms::protobuf::DRIVER_UDP:
            asio_service.reset(new boost::asio::io_service);
            driver.reset(new goby::acomms::UDPDriver(asio_service.get()));
            break;
    }
    
    
    goby::acomms::MACManager mac;

    //
    // Connect slots
    //
    // connect slots for mac and driver
    goby::acomms::bind(mac, *driver);
    // our callback for received data from the modem
    goby::acomms::connect(&driver->signal_receive, &handle_modem_receive);
    // our callback for requested data for transfer to the modem
    goby::acomms::connect(&driver->signal_data_request, &handle_modem_data_request);
    
    //
    // Initialize DCCL (marshalling)
    //
    dccl_codec.load<NavigationReport>();
    dccl_codec.load<AUVCommand>();
    
    //
    // Initialize medium access control
    // [ 64 bytes ] topside -> vehicle [ 20 sec ]
    // [ 64 bytes ] vehicle -> topside [ 20 sec ]
    // [ 64 bytes ] vehicle -> topside [ 20 sec ]
    goby::acomms::protobuf::MACConfig mac_cfg;
    mac_cfg.set_type(goby::acomms::protobuf::MAC_FIXED_DECENTRALIZED);
    mac_cfg.set_modem_id(my_id);

    goby::acomms::protobuf::ModemTransmission topside_slot;
    topside_slot.set_src(topside_id);
    topside_slot.set_max_frame_bytes(64);
    switch(driver_type)
    {
        default: break;
            
        case goby::acomms::protobuf::DRIVER_WHOI_MICROMODEM:
            topside_slot.set_rate(1);
            break;
            
        case goby::acomms::protobuf::DRIVER_BENTHOS_ATM900:
            topside_slot.set_rate(3);
            break;

    }    
    topside_slot.set_type(goby::acomms::protobuf::ModemTransmission::DATA);
    topside_slot.set_slot_seconds(20);

    goby::acomms::protobuf::ModemTransmission vehicle_slot = topside_slot;
    vehicle_slot.set_src(vehicle_id);
    
    // copy into mac configuration
    *mac_cfg.add_slot() = topside_slot;
    *mac_cfg.add_slot() = vehicle_slot;
    *mac_cfg.add_slot() = vehicle_slot;

    //
    // Initialize modem driver
    //
    goby::acomms::protobuf::DriverConfig driver_cfg;
    driver_cfg.set_modem_id(my_id);
    driver_cfg.set_serial_port(serial_port);

    // driver specific configuration
    switch(driver_type)
    {
        default: break;

        case goby::acomms::protobuf::DRIVER_BENTHOS_ATM900:
            // add some simulated delay
            driver_cfg.AddExtension(benthos::protobuf::BenthosATM900DriverConfig::config, "@SimAcDly=1000");
            // turn down TxPower to minimum
            driver_cfg.AddExtension(benthos::protobuf::BenthosATM900DriverConfig::config, "@TxPower=1");
            // go to lowpower after 1 minute idle
            driver_cfg.AddExtension(benthos::protobuf::BenthosATM900DriverConfig::config, "@IdleTimer=00:01:00");
            break;

        case goby::acomms::protobuf::DRIVER_WHOI_MICROMODEM:
            break;
            
        case goby::acomms::protobuf::DRIVER_UDP:
#ifdef TOPSIDE
            driver_cfg.MutableExtension(UDPDriverConfig::local)->set_port(50001);
            driver_cfg.MutableExtension(UDPDriverConfig::remote)->set_port(50002);
#endif
#ifdef VEHICLE
            driver_cfg.MutableExtension(UDPDriverConfig::local)->set_port(50002);
            driver_cfg.MutableExtension(UDPDriverConfig::remote)->set_port(50001);
#endif
            break;
    }


    //
    // Start up everything
    //
    try
    {
        std::cout << "Starting up MAC..." << std::endl;
        mac.startup(mac_cfg);
        std::cout << "...success" << std::endl;
        std::cout << "Starting up Modem..." << std::endl;
        driver->startup(driver_cfg);
        std::cout << "...success" << std::endl;
    }
    catch(std::runtime_error& e)
    {
        std::cerr << "exception at startup: " << e.what() << std::endl;
        return startup_failure(argv[0]);
    }

    //
    // Loop until terminated (CTRL-C)
    //
    for(;;)
    {
        try
        {
            driver->do_work();
            mac.do_work();
        }
        catch(std::runtime_error& e)
        {
            std::cerr << "exception while running: " << e.what() << std::endl;
            return 1;
        }
        // nominally 10 Hz works well
        // you can reduce this to save CPU (at the slight cost of responsiveness)
        usleep(10000);
    }    
}


int startup_failure(const char* name)
{
    std::cerr << "usage: " << name << "  /dev/tty_modem_A DRIVER_TYPE_ENUM debug_log_file" << std::endl;
    return 1;
}


#ifdef TOPSIDE
void handle_modem_data_request(goby::acomms::protobuf::ModemTransmission* msg)
{
    static int req_count = 0;
    // for this example, only send data on every other request as a simple proxy for whether we have a command to send
    if(req_count & 1)
    {
        std::cout << "No data to send." << std::endl;
    }
    else
    {
        using namespace boost::units;
        
        // create the DCCL command
        AUVCommand cmd;
        cmd.set_waypoint_x_with_units((rand() % 10000 - 5000)*si::meters); 
        cmd.set_waypoint_y_with_units((rand() % 10000 - 5000)*si::meters); 
        cmd.set_survey_depth_with_units((rand() % 1000)*si::meters); 
        cmd.set_speed_with_units(1.5*si::meters_per_second);

        // encode message and add it to the data request
        dccl_codec.encode(msg->add_frame(), cmd);

        // pick a destination for our message (here it's always the vehicle).
        msg->set_dest(vehicle_id);

        // we want an acknowledgment for commands
        msg->set_ack_requested(true);
        std::cout << "<< Sending command [" << msg->frame(0).size() << " B]: " << cmd.ShortDebugString() << std::endl;
    }
    
    ++req_count;
}
#endif

#ifdef VEHICLE
void handle_modem_data_request(goby::acomms::protobuf::ModemTransmission* msg)
{
    using namespace boost::units;

    NavigationReport report;
    report.set_x_with_units((rand() % 10000 - 5000)*si::meters);
    report.set_y_with_units((rand() % 10000 - 5000)*si::meters);
    report.set_z_with_units(-(rand() % 1000)*si::meters);
    report.set_veh_class(NavigationReport::AUV);
    report.set_battery_ok(true);
    
    dccl_codec.encode(msg->add_frame(), report);    
    msg->set_dest(topside_id);
    // reports are superceded, so no need for ack.
    msg->set_ack_requested(false);

    std::cout << "<< Sending report [" << msg->frame(0).size() << " B]: " << report.ShortDebugString() << std::endl;

}
#endif

void handle_modem_receive(const goby::acomms::protobuf::ModemTransmission& msg)
{
    switch(msg.type())
    {
        default: break;
        case goby::acomms::protobuf::ModemTransmission::DATA:
            if(msg.frame_size() != 1)
            {
                std::cerr << ">> Received data with wrong number of frames, expected 1" << std::endl;
                return;
            }

            if(dccl_codec.id(msg.frame(0)) == dccl_codec.id<NavigationReport>())
            {
                NavigationReport report;
                dccl_codec.decode(msg.frame(0), &report);
                std::cout << ">> Received NavigationReport: " << report.ShortDebugString() << std::endl;
            }
            else if(dccl_codec.id(msg.frame(0)) == dccl_codec.id<AUVCommand>())
            {
                AUVCommand command;
                dccl_codec.decode(msg.frame(0), &command);
                std::cout << ">> Received AUVCommand: " << command.ShortDebugString() << std::endl;
            }            
            break;
        case goby::acomms::protobuf::ModemTransmission::ACK:
            std::cout << ">> Received ACK" << std::endl;
            break;
    }

    if(msg.HasExtension(benthos::protobuf::receive_stat))
    {
        const benthos::protobuf::ReceiveStatistics& rx_stat = msg.GetExtension(benthos::protobuf::receive_stat);
        std::cout << "Rx stats: " << rx_stat.ShortDebugString() << std::endl;
    }
    
}

