set(ProtoFiles
  "${CMAKE_CURRENT_SOURCE_DIR}/messages/common.proto"
  "${CMAKE_CURRENT_SOURCE_DIR}/messages/config.proto"
  "${CMAKE_CURRENT_SOURCE_DIR}/messages/messages.proto"
  "${CMAKE_CURRENT_SOURCE_DIR}/messages/consensus.proto"
  "${CMAKE_CURRENT_SOURCE_DIR}/messages/rest.proto"
  )

PROTOBUF_GENERATE_CPP(ProtoSources ProtoHeaders ${ProtoFiles})

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/../testnet/data.0.testnet
  DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

configure_file("../testnet/bot.json" "${CMAKE_CURRENT_BINARY_DIR}/bot.json" @ONLY)

add_library(protos
  ${ProtoSources}
  ${ProtoHeaders}
  )


target_link_libraries(protos
  protobuf::libprotobuf
  ${LIBMONGOCXX_STATIC_LIBRARIES}
  )


add_library(core
  ./crypto/KeyPriv.cpp
  ./crypto/KeyPriv.hpp
  ./crypto/Hash.cpp
  ./crypto/Hash.hpp
  ./crypto/KeyPub.cpp
  ./crypto/KeyPub.hpp
  ./crypto/Sign.cpp
  ./crypto/Sign.hpp
  ./crypto/Ecc.hpp
  ./crypto/Ecc.cpp
  ./common/Buffer.cpp
  ./common/Buffer.hpp
  ./common/logger.hpp
  ./common/logger.cpp
  ./common/types.hpp
  ./common/types.cpp
  ./networking/tcp/HeaderPattern.hpp
  ./networking/tcp/Tcp.hpp
  ./networking/tcp/Connection.cpp
  ./networking/tcp/Connection.hpp
  ./networking/tcp/Tcp.cpp
  ./networking/Networking.cpp
  ./networking/TransportLayer.hpp
  ./networking/Networking.hpp
  ./networking/Connection.cpp
  ./networking/Connection.hpp
  ./networking/TransportLayer.cpp
  ./Bot.cpp
  ./tooling/blockgen.hpp
  ./tooling/blockgen.cpp
  ./tooling/Simulator.hpp
  ./tooling/Simulator.cpp
  ./tooling/FullSimulator.hpp
  ./tooling/FullSimulator.cpp
  ./common/Queue.hpp
  ./messages/Queue.hpp
  ./messages/Queue.cpp
  ./messages/Message.hpp
  ./messages/Subscriber.hpp
  ./messages/Message.cpp
  ./messages/Address.cpp
  ./messages/Hasher.hpp
  ./messages/Peers.hpp
  ./messages/Peers.cpp
  ./messages/Peer.hpp
  ./messages/Peer.cpp
  ./messages/NCCAmount.hpp
  ./messages/config/Config.hpp
  ./messages/config/Config.cpp
  ./ledger/Parser.hpp
  ./ledger/Filter.hpp
  ./ledger/mongo.hpp
  ./ledger/Ledger.hpp
  ./ledger/LedgerMongodb.hpp
  ./ledger/LedgerMongodb.cpp
  ./ledger/Transaction.hpp
  ./Bot.hpp
  ./api/Rest.cpp
  ./api/Rest.hpp
  ./api/Api.hpp
  ./api/Api.cpp
  ./api/Monitoring.cpp
  ./api/Monitoring.hpp
  ./consensus/Consensus.hpp
  ./consensus/Consensus.cpp
  ./consensus/Pii.hpp
  ./consensus/Pii.cpp
  ./consensus/Integrities.hpp
  ./consensus/Integrities.cpp
  )

target_link_libraries(core
  protos

  Boost::log
  Boost::program_options
  Boost::system
  Boost::filesystem
  Boost::thread

  cpr::cpr

  cryptopp-static

  protobuf::libprotobuf
  ${LIBMONGOCXX_STATIC_LIBRARIES}

  pistache
  
  mpfr
)

add_executable(main
  main.cpp
  )

target_link_libraries(main
  protos
  core
  Boost::log
  Boost::program_options
  Boost::system
  Boost::filesystem
  Boost::thread

  pistache

  cpr::cpr

  protobuf::libprotobuf
  ${LIBMONGOCXX_STATIC_LIBRARIES}

  mpfr
  )

add_executable(keygen
  tooling/keygen.cpp
  )

target_link_libraries(keygen
  core
  Boost::program_options
  )

add_executable(confgen
  tooling/confgen.cpp
  )

target_link_libraries(confgen
  core
  Boost::program_options
  )

add_executable(showkey
  tooling/Showkey.cpp
  )

target_link_libraries(showkey
  core
  Boost::program_options
  )

add_executable(coinbase
  tooling/coinbase.cpp
  )

target_link_libraries(coinbase
  core
  Boost::program_options
  )

add_executable(blockg
  tooling/blockg.cpp
  )
target_link_libraries(blockg
  core
  Boost::program_options
  )

add_executable(show_block0
  tooling/show_block0.cpp
  )
target_link_libraries(show_block0
  core
  Boost::program_options
  )

#add_executable(wallet
#  tooling/wallet.cpp
#  )
#target_link_libraries(wallet
#  core
#  Boost::program_options
#  )

add_executable(transaction
  tooling/transaction.cpp
  )

target_link_libraries(transaction
  core
  Boost::program_options
  )

add_executable(holes
  tooling/holes.cpp
  )
target_link_libraries(holes
  core
  protos
  )

include_directories(${CMAKE_SOURCE_DIR}/src ${CMAKE_BINARY_DIR}/src ${LIBMONGOCXX_STATIC_INCLUDE_DIRS})

