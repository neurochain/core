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

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/../testnet/data.0.testnet
  DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

configure_file("configs/bot0.json.in" "${CMAKE_CURRENT_BINARY_DIR}/bot0.json" @ONLY)
configure_file("configs/bot1.json.in" "${CMAKE_CURRENT_BINARY_DIR}/bot1.json" @ONLY)
configure_file("configs/bot2.json.in" "${CMAKE_CURRENT_BINARY_DIR}/bot2.json" @ONLY)

configure_file("configs/integration_propagation0.json.in" "${CMAKE_CURRENT_BINARY_DIR}/integration_propagation0.json" @ONLY)
configure_file("configs/integration_propagation1.json.in" "${CMAKE_CURRENT_BINARY_DIR}/integration_propagation1.json" @ONLY)
configure_file("configs/integration_propagation2.json.in" "${CMAKE_CURRENT_BINARY_DIR}/integration_propagation2.json" @ONLY)
configure_file("configs/integration_update0.json.in" "${CMAKE_CURRENT_BINARY_DIR}/integration_update0.json" @ONLY)
configure_file("configs/integration_update1.json.in" "${CMAKE_CURRENT_BINARY_DIR}/integration_update1.json" @ONLY)
configure_file("configs/integration_update2.json.in" "${CMAKE_CURRENT_BINARY_DIR}/integration_update2.json" @ONLY)

configure_file("configs/integration_keepmax0.json.in" "${CMAKE_CURRENT_BINARY_DIR}/integration_keepmax0.json" @ONLY)
configure_file("configs/integration_keepmax1.json.in" "${CMAKE_CURRENT_BINARY_DIR}/integration_keepmax1.json" @ONLY)
configure_file("configs/integration_keepmax2.json.in" "${CMAKE_CURRENT_BINARY_DIR}/integration_keepmax2.json" @ONLY)

configure_file("configs/key_integration0.priv" "${CMAKE_CURRENT_BINARY_DIR}/key_integration0.priv" COPYONLY)
configure_file("configs/key_integration1.priv" "${CMAKE_CURRENT_BINARY_DIR}/key_integration1.priv" COPYONLY)
configure_file("configs/key_integration2.priv" "${CMAKE_CURRENT_BINARY_DIR}/key_integration2.priv" COPYONLY)
configure_file("configs/key_integration0.pub" "${CMAKE_CURRENT_BINARY_DIR}/key_integration0.pub" COPYONLY)
configure_file("configs/key_integration1.pub" "${CMAKE_CURRENT_BINARY_DIR}/key_integration1.pub" COPYONLY)
configure_file("configs/key_integration2.pub" "${CMAKE_CURRENT_BINARY_DIR}/key_integration2.pub" COPYONLY)

configure_file("configs/test_sign.priv" "${CMAKE_CURRENT_BINARY_DIR}/test_sign.priv" COPYONLY)
configure_file("configs/test_sign.pub" "${CMAKE_CURRENT_BINARY_DIR}/test_sign.pub" COPYONLY)

configure_file("../testnet/data.0.testnet" "${CMAKE_CURRENT_BINARY_DIR}/../data.0.testnet" COPYONLY)

set(ProtoFiles "networking/proto/Messages.proto")
PROTOBUF_GENERATE_CPP(ProtoSources ProtoHeaders ${ProtoFiles})

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
  cpr::cpr
  libonion::onioncpp
  ${LIBMONGOCXX_STATIC_LIBRARIES}
  )
add_test(integration integration)

add_executable(ut
  ./common/Buffer.cpp
  ./crypto/Hash.cpp
  ./crypto/Ecc.cpp
  ./messages/Config.cpp
  ./messages/Queue.cpp
  ./messages/Subscriber.cpp
  ./networking/TransportLayer.cpp
  ./networking/tcp/Tcp.cpp
  ./networking/tcp/Connection.cpp
  )

target_link_libraries(ut
  GTest::main
  core
  protos
  Boost::log
  Boost::program_options
  Boost::system
  Boost::filesystem
  Boost::thread
  libonion::onion
  cpr::cpr
  libonion::onioncpp
  ${LIBMONGOCXX_STATIC_LIBRARIES}
  )

add_test(ut ut)


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
#[[
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
    core
    cpr::cpr
)]]

add_test(integration integration)
#add_test(dbreset  dbreset)
add_test(block0 block0)
#add_test(blocks19 blocks19)
#add_test(piitest  piitest)

include_directories(${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/src ${CMAKE_BINARY_DIR}/src ${LIBMONGOCXX_STATIC_INCLUDE_DIRS})
