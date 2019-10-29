protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${CMAKE_CURRENT_BINARY_DIR} config.proto)

add_executable(pFMSweepExample
  pFMSweepExample.cpp
  ${PROTO_SRCS} ${PROTO_HDRS})

target_link_libraries(pFMSweepExample
  goby_moos
  ${MOOS_LIBRARIES}
  ${MOOSGeodesy_LIBRARIES}
  ${Boost_LIBRARIES}
  ${PROTOBUF_LIBRARY}
  ${CMAKE_DL_LIBS})
