set(ProtoFiles
  "${CMAKE_CURRENT_SOURCE_DIR}/messages/common.proto"
  "${CMAKE_CURRENT_SOURCE_DIR}/messages/config.proto"
  "${CMAKE_CURRENT_SOURCE_DIR}/messages/messages.proto"
  "${CMAKE_CURRENT_SOURCE_DIR}/messages/rest.proto"
  )

PROTOBUF_GENERATE_CPP(ProtoSources ProtoHeaders ${ProtoFiles})


add_library(protos
  ${ProtoSources}
  ${ProtoHeaders}
  )


target_link_libraries(protos
  protobuf::libprotobuf
  ${LIBMONGOCXX_STATIC_LIBRARIES}
  )


add_library(core
  ./crypto/Hash.hpp
  ./crypto/EccPub.cpp
  ./crypto/Ecc.cpp
  ./crypto/EccPub.hpp
  ./crypto/Sign.cpp
  ./crypto/Sign.hpp
  ./crypto/EccPriv.hpp
  ./crypto/Ecc.hpp
  ./crypto/Hash.cpp
  ./crypto/EccPriv.cpp
  ./common/Buffer.cpp
  ./common/Buffer.hpp
  ./common/logger.hpp
  ./common/logger.cpp
  ./common/types.hpp
  ./mongo/mongo.hpp
  ./networking/tcp/HeaderPattern.hpp
  ./networking/tcp/Tcp.hpp
  ./networking/tcp/Connection.cpp
  ./networking/tcp/Connection.hpp
  ./networking/tcp/client.hpp
  ./networking/tcp/Tcp.cpp
  ./networking/tcp/Client.hpp
  ./networking/Networking.cpp
  ./networking/TransportLayer.hpp
  ./networking/Networking.hpp
  ./networking/Connection.cpp
  ./networking/Connection.hpp
  ./networking/TransportLayer.cpp
  ./Bot.cpp
  ./tooling/coinbase.cpp

  ./tooling/wallet.cpp
  ./tooling/transaction.cpp
  ./tooling/list.cpp
  ./tooling/keygen.cpp
  ./messages/Queue.hpp
  ./messages/Queue.cpp
  ./messages/Message.hpp
  ./messages/Subscriber.hpp
  ./messages/Message.cpp
  ./messages/Hasher.hpp
  ./ledger/Parser.hpp
  ./ledger/Filter.hpp
  ./ledger/mongo.hpp
  ./ledger/Ledger.hpp
  ./ledger/LedgerMongodb.hpp
  ./ledger/LedgerMongodb.cpp
  ./ledger/Transaction.hpp
  ./Bot.hpp
  )

target_link_libraries(core
  protos

  Boost::log
  Boost::program_options
  Boost::system
  Boost::filesystem
  Boost::thread

  cryptopp-static

  protobuf::libprotobuf
  ${LIBMONGOCXX_STATIC_LIBRARIES}
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

  protobuf::libprotobuf
  ${LIBMONGOCXX_STATIC_LIBRARIES}
  )

add_executable(keygen
  tooling/keygen.cpp
  )

target_link_libraries(keygen
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

add_executable(wallet
  tooling/wallet.cpp
  )
target_link_libraries(wallet
  core
  Boost::program_options
  )

add_executable(transaction
  tooling/transaction.cpp
  )

target_link_libraries(transaction
  core
  Boost::program_options
  )


include_directories(${CMAKE_SOURCE_DIR}/src ${CMAKE_BINARY_DIR}/src ${LIBMONGOCXX_STATIC_INCLUDE_DIRS})
