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
)


add_library(core
  ./messages/Queue.hpp
  ./messages/Queue.cpp
  ./messages/Subscriber.hpp
  ./messages/Message.hpp
  ./messages/Message.cpp
  ./mongo/mongo.hpp
  ./Bot.hpp
  ./Bot.cpp
  ./common/Buffer.hpp
  ./common/Buffer.cpp
  ./common/logger.hpp
  ./common/logger.cpp
  ./common/types.hpp
  ./networking/Peer.hpp
  ./networking/Peer.cpp
  ./networking/Networking.hpp
  ./networking/Networking.cpp
  ./networking/Connection.hpp
  ./networking/tcp/client.hpp
  ./networking/tcp/Client.hpp
  ./networking/tcp/HeaderPattern.hpp
  ./networking/tcp/Tcp.cpp
  ./networking/tcp/Tcp.hpp
  ./networking/tcp/Connection.hpp
  ./networking/tcp/Connection.cpp
  ./networking/TransportLayer.hpp
  ./networking/TransportLayer.cpp
  ./crypto/EccPriv.hpp
  ./crypto/EccPriv.cpp
  ./crypto/EccPub.hpp
  ./crypto/EccPub.cpp
  ./crypto/Hash.hpp
  ./crypto/Hash.cpp
  ./crypto/Ecc.hpp
  ./crypto/Ecc.cpp
  )

target_link_libraries(core
  protos

  Boost::system
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
  core
  protos
  Boost::log
  Boost::program_options
  Boost::filesystem
  Boost::system
  Boost::thread
  cryptopp-static
  
  protobuf::libprotobuf
  ${LIBMONGOCXX_STATIC_LIBRARIES}
  )


include_directories(${CMAKE_SOURCE_DIR}/src ${CMAKE_BINARY_DIR}/src ${LIBMONGOCXX_STATIC_INCLUDE_DIRS})
