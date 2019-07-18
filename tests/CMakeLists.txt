file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/configs/test_sign.priv
  DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/configs/test_sign.pub
  DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

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
  get_filename_component(COIN ${file} NAME_WE)
  configure_file("${file}" "${CMAKE_CURRENT_BINARY_DIR}/${COIN}.json" @ONLY)
endforeach()

#configure_file("configs/key_faucet.priv" "${CMAKE_CURRENT_BINARY_DIR}/key_faucet.priv" COPYONLY)
#configure_file("configs/key_faucet.pub" "${CMAKE_CURRENT_BINARY_DIR}/key_faucet.pub" COPYONLY)

file(GLOB files "configs/key*.pub")
foreach(file ${files})
  get_filename_component(COIN ${file} NAME_WE)
  configure_file(${file} "${CMAKE_CURRENT_BINARY_DIR}/${COIN}.pub" COPYONLY)
endforeach()

file(GLOB files "configs/key*.priv")
foreach(file ${files})
  get_filename_component(COIN ${file} NAME_WE)
  configure_file(${file} "${CMAKE_CURRENT_BINARY_DIR}/${COIN}.priv" COPYONLY)
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
  cpr::cpr
  ${LIBMONGOCXX_STATIC_LIBRARIES}
  )
add_test(integration integration)

add_executable(full_simulator
    tooling/FullSimulator.cpp
  )

target_link_libraries(full_simulator
  GTest::main
  core
  protos
  Boost::program_options
  Boost::system
  Boost::filesystem
  Boost::thread
  Boost::log
  cpr::cpr
  ${LIBMONGOCXX_STATIC_LIBRARIES}
  )
add_test(full_simulator full_simulator)

add_executable(ut
  ./common/Buffer.cpp
  ./crypto/Hash.cpp
  ./crypto/Ecc.cpp
  ./crypto/Sign.cpp
  ./messages/Address.cpp
  ./messages/Config.cpp
#  ./messages/Queue.cpp
  ./messages/Subscriber.cpp
  ./messages/Peers.cpp
  ./networking/TransportLayer.cpp
  ./networking/tcp/Tcp.cpp
  ./networking/tcp/Connection.cpp
  ./api/Rest.cpp)

target_link_libraries(ut
  GTest::main
  core
  protos
  Boost::log
  Boost::program_options
  Boost::system
  Boost::filesystem
  Boost::thread
  cpr::cpr
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
  )
add_test(LedgerMongodb LedgerMongodb)

add_executable(Simulator
  tooling/Simulator.cpp
  )
target_link_libraries(Simulator
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
  )
add_test(Simulator Simulator)

add_executable(Pii
  consensus/Pii.cpp
  )
target_link_libraries(Pii
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
  )
add_test(Pii Pii)

add_executable(Consensus
  consensus/Consensus.cpp
  )
target_link_libraries(Consensus
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
  )
add_test(Consensus Consensus)

add_executable(RealtimeConsensus
  consensus/RealtimeConsensus.cpp
  )
target_link_libraries(RealtimeConsensus
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
  )
add_test(RealtimeConsensus RealtimeConsensus)

add_executable(RealtimeSimulator
    tooling/RealtimeSimulator.cpp
  )
target_link_libraries(RealtimeSimulator
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
  )
add_test(RealtimeSimulator RealtimeSimulator)

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
