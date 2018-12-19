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

set(files "integration_propagation0.json"
          "integration_propagation1.json"
          "integration_propagation2.json"
          "integration_update0.json"
          "integration_update1.json"
          "integration_update2.json"
          "integration_keepmax0.json"
          "integration_keepmax1.json"
          "integration_keepmax2.json"
          "bot_outsider.40-38.39.json"
          "bot_outsider.41-38.39.json"
          "bot_outsider.42-38.39.json"
          "bot_outsider.50-37.json"
          "bot_outsider.51-37.json"
          "bot_outsider.52-37.json"
          "key_integration0.priv"
          "key_integration0.pub"
          "key_integration1.priv"
          "key_integration1.pub"
          "key_integration2.priv"
          "key_integration2.pub"
          "test_sign.priv"
          "test_sign.pub"
          "key_faucet.priv"
          "key_faucet.pub"
          "key0.pub"
          "key0.priv"
          "key1.pub"
          "key1.priv"
          "key2.pub"
          "key2.priv"
          "bot0.json"
          "bot1.json"
          "bot2.json"
          "bot_block0.json"
          "data.0.testnet"
          "integration_propagation0.peers.json"
          "integration_propagation1.peers.json"
          "integration_propagation2.peers.json"
          "integration_update0.peers.json"
          "integration_update1.peers.json"
          "integration_update2.peers.json"
          "integration_keepmax0.peers.json"
          "integration_keepmax1.peers.json"
          "integration_keepmax2.peers.json"
          "bot_outsider.40-38.39.peers.json"
          "bot_outsider.41-38.39.peers.json"
          "bot_outsider.42-38.39.peers.json"
          "bot_outsider.50-37.peers.json"
          "bot_outsider.51-37.peers.json"
          "bot_outsider.52-37.peers.json"
          "bot0.peers.json"
          "bot1.peers.json"
          "bot2.peers.json"
          "bot_block0.peers.json"
)
foreach(file ${files})
  configure_file("configs/${file}" "${CMAKE_CURRENT_BINARY_DIR}/${file}" COPYONLY)
endforeach()

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


add_executable(LedgerMongodb
  ledger/LedgerMongodb.cpp
  )
target_link_libraries(LedgerMongodb
  GTest::main
  Boost::program_options
  Boost::filesystem
  Boost::system
  Boost::thread
  Boost::log
  cpr::cpr
  ${LIBMONGOCXX_STATIC_LIBRARIES}
  core
  protos
  libonion::onion
  libonion::onioncpp
  )
add_test(LedgerMongodb LedgerMongodb)


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

#add_executable(piitest
#    piitest.cpp
#)
#
#target_link_libraries(piitest
#    GTest::main
#    core
#    cpr::cpr
#)

add_test(integration integration)
#add_test(dbreset  dbreset)
add_test(block0 block0)
#add_test(blocks19 blocks19)
#add_test(piitest  piitest)

include_directories(${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/src ${CMAKE_BINARY_DIR}/src ${LIBMONGOCXX_STATIC_INCLUDE_DIRS})
