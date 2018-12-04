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

configure_file("configs/bot0.json" "${CMAKE_CURRENT_BINARY_DIR}/bot0.json" COPYONLY)
configure_file("configs/bot1.json" "${CMAKE_CURRENT_BINARY_DIR}/bot1.json" COPYONLY)
configure_file("configs/bot2.json" "${CMAKE_CURRENT_BINARY_DIR}/bot2.json" COPYONLY)
configure_file("configs/bot_block0.json" "${CMAKE_CURRENT_BINARY_DIR}/bot_block0.json" COPYONLY)
configure_file("configs/bot_block0.json" "${CMAKE_CURRENT_BINARY_DIR}/bot_block0.json" COPYONLY)

file(GLOB files "configs/integration_*.json.in")
foreach(file ${files})
  configure_file(${file} "${CMAKE_CURRENT_BINARY_DIR}/${file}" @ONLY)
endforeach()

configure_file("configs/integration_keepmax0.json.in" "${CMAKE_CURRENT_BINARY_DIR}/integration_keepmax0.json" @ONLY)
configure_file("configs/integration_keepmax1.json.in" "${CMAKE_CURRENT_BINARY_DIR}/integration_keepmax1.json" @ONLY)
configure_file("configs/integration_keepmax2.json.in" "${CMAKE_CURRENT_BINARY_DIR}/integration_keepmax2.json" @ONLY)

file(GLOB files "configs/key_integration*.*")
foreach(file ${files})
  configure_file(${file} "${CMAKE_CURRENT_BINARY_DIR}/${file}" COPYONLY)
endforeach()

configure_file("configs/test_sign.priv" "${CMAKE_CURRENT_BINARY_DIR}/test_sign.priv" COPYONLY)
configure_file("configs/test_sign.pub" "${CMAKE_CURRENT_BINARY_DIR}/test_sign.pub" COPYONLY)

configure_file("configs/key_faucet.priv" "${CMAKE_CURRENT_BINARY_DIR}/key_faucet.priv" COPYONLY)
configure_file("configs/key_faucet.pub" "${CMAKE_CURRENT_BINARY_DIR}/key_faucet.pub" COPYONLY)

file(GLOB files "configs/key*.p*")
foreach(file ${files})
  configure_file(${file} "${CMAKE_CURRENT_BINARY_DIR}/${file}" COPYONLY)
endforeach()

configure_file("configs/data.0.testnet" "${CMAKE_CURRENT_BINARY_DIR}/data.0.testnet" COPYONLY)

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

add_executable(ForkManager
    consensus/ForkManager.cpp
  )
target_link_libraries(ForkManager
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
add_test(ForkManager ForkManager)

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
