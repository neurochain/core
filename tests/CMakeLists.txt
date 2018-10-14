#file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/crypto/test_sign.priv
#  DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
#
#file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/crypto/test_sign.pub
#  DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
#
#file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/config/bot.json
#  DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
#
#file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/config/peers.json
#  DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/../block.0.bp
  DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

configure_file("configs/bot0.json.in" "${CMAKE_CURRENT_BINARY_DIR}/bot0.json" @ONLY)
configure_file("configs/bot1.json.in" "${CMAKE_CURRENT_BINARY_DIR}/bot1.json" @ONLY)
configure_file("configs/bot2.json.in" "${CMAKE_CURRENT_BINARY_DIR}/bot2.json" @ONLY)


set(ProtoFiles "networking/proto/Messages.proto")
PROTOBUF_GENERATE_CPP(ProtoSources ProtoHeaders ${ProtoFiles})

add_test(integration integration)
add_executable(integration
  integration.cpp
  )

target_link_libraries(integration
  GTest::main
  core
  protos
  Boost::program_options
  Boost::system
  Boost::filesystem
  Boost::thread
  Boost::log
  libonion::onion
  libonion::onioncpp
  ${LIBMONGOCXX_STATIC_LIBRARIES}
  )

add_executable(dbreset
    dbreset.cpp
)

target_link_libraries(dbreset
    GTest::main
    core
)

add_executable(block0
    block0.cpp
)

target_link_libraries(block0
    GTest::main
    core
)

add_executable(blocks19
    blocks19.cpp
)

target_link_libraries(blocks19
    GTest::main
    core
)

add_executable(piitest
    piitest.cpp
)

target_link_libraries(piitest
    GTest::main
    consensus
)

include_directories(${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/src ${CMAKE_BINARY_DIR}/src ${LIBMONGOCXX_STATIC_INCLUDE_DIRS})
