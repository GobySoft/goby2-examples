// Copyright 2009-2018 Toby Schneider (http://gobysoft.org/index.wt/people/toby)
//                     GobySoft, LLC (2013-)
//                     Massachusetts Institute of Technology (2007-2014)
//
//
// This file is part of the Goby Underwater Autonomy Project Binaries
// ("The Goby Binaries").
//
// The Goby Binaries are free software: you can redistribute them and/or modify
// them under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// The Goby Binaries are distributed in the hope that they will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Goby.  If not, see <http://www.gnu.org/licenses/>.

#include "pFMSweepExample.h"

#include <goby/moos/moos_protobuf_helpers.h> // for serialize_for_moos
#include <goby/acomms/protobuf/modem_message.pb.h> // for ModemTransmission
#include <goby/acomms/protobuf/mm_driver.pb.h> // for micromodem::protobuf::...

using goby::glog;
using namespace goby::common::logger;
using goby::moos::operator<<;

boost::shared_ptr<FMSweepExampleConfig> master_config;
FMSweepExample* FMSweepExample::inst_ = 0;

int main(int argc, char* argv[]) { return goby::moos::run<FMSweepExample>(argc, argv); }

FMSweepExample* FMSweepExample::get_instance()
{
    if (!inst_)
    {
        master_config.reset(new FMSweepExampleConfig);
        inst_ = new FMSweepExample(*master_config);
    }
    return inst_;
}

void FMSweepExample::delete_instance() { delete inst_; }

FMSweepExample::FMSweepExample(FMSweepExampleConfig& cfg)
    : GobyMOOSApp(&cfg), cfg_(cfg)
{

}

FMSweepExample::~FMSweepExample() {}

void FMSweepExample::loop()
{
    using goby::acomms::protobuf::ModemTransmission;
    using micromodem::protobuf::FMSweepParams;    
    
    ModemTransmission fm_tx;
    fm_tx.set_src(1); // must be vehicle modem id
    fm_tx.set_type(ModemTransmission::DRIVER_SPECIFIC);
    fm_tx.SetExtension(micromodem::protobuf::type, micromodem::protobuf::MICROMODEM_FM_SWEEP);
    FMSweepParams& fm_params = *fm_tx.MutableExtension(micromodem::protobuf::fm_sweep);
    fm_params.set_start_freq(25000); // Hz
    fm_params.set_stop_freq(27000); // Hz
    fm_params.set_duration_ms(500); // ms

    // publish FM Sweep (using GobyMOOSApp methods); preferred
    publish_pb("ACOMMS_MAC_INITIATE_TRANSMISSION", fm_tx);

    // (same thing) publish FM Sweep (using CMOOSComms directly, for use in non-GobyMOOSApp binaries)
    std::string serialized;
    serialize_for_moos(&serialized, fm_tx);
    m_Comms.Notify("ACOMMS_MAC_INITIATE_TRANSMISSION", serialized);
}
