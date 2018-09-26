set(ProtoFiles
  "${CMAKE_CURRENT_SOURCE_DIR}/messages/common.proto"
  "${CMAKE_CURRENT_SOURCE_DIR}/messages/config.proto"
  "${CMAKE_CURRENT_SOURCE_DIR}/messages/messages.proto"
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
  ./messages/Queue.hpp
  ./messages/Subscriber.hpp
  ./messages/Queue.cpp
  ./messages/Message.hpp
  ./mongo/mongo.hpp
  ./Bot.hpp
  ./common/logger.cpp
  ./common/Buffer.hpp
  ./common/logger.hpp
  ./common/Buffer.cpp
  ./common/types.hpp
  ./networking/Networking.hpp
  ./networking/TransportLayer.cpp
  ./networking/Peer.cpp
  ./networking/Networking.cpp
  ./networking/Connection.hpp
  ./networking/Peer.hpp
  ./networking/tcp/client.hpp
  ./networking/tcp/Client.hpp
  ./networking/tcp/HeaderPattern.hpp
  ./networking/tcp/Tcp.hpp
  ./networking/tcp/Connection.hpp
  ./networking/tcp/Connection.cpp
  ./networking/tcp/Tcp.cpp
  ./networking/TransportLayer.hpp
  ./crypto/Ecc.hpp
  ./crypto/EccPriv.hpp
  ./crypto/EccPriv.cpp
  ./crypto/EccPub.hpp
  ./crypto/Hash.hpp
  ./crypto/Hash.cpp
  ./crypto/EccPub.cpp
  ./crypto/Ecc.cpp
  ./ledger/Parser.hpp
  ./ledger/Filter.hpp
  ./ledger/mongo.hpp
  ./ledger/LedgerMongodb.hpp
  ./ledger/LedgerMongodb.cpp
  )

target_link_libraries(core
  protos

  Boost::program_options
  Boost::system
  Boost::filesystem
  Boost::thread
  Boost::log

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
  Boost::program_options
  Boost::system
  Boost::filesystem
  Boost::thread
  Boost::log
  
  protobuf::libprotobuf
  ${LIBMONGOCXX_STATIC_LIBRARIES}  
  )


include_directories(${CMAKE_SOURCE_DIR}/src ${CMAKE_BINARY_DIR}/src ${LIBMONGOCXX_STATIC_INCLUDE_DIRS})
