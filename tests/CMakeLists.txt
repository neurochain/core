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

set(ProtoFiles "networking/proto/Messages.proto")
PROTOBUF_GENERATE_CPP(ProtoSources ProtoHeaders ${ProtoFiles})

#add_test(integration integration)
#add_executable(integration
#  integration.cpp
#  )
#
#target_link_libraries(integration
#  GTest::main
#  commons
#  networking
#  cryptocc
#  messages
#  cryptopp-static
#  Boost::program_options
#  Boost::system
#  Boost::filesystem
#  Boost::thread
#  Boost::log
#  ${LIBMONGOCXX_STATIC_LIBRARIES}
#  )

include_directories(${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/src ${CMAKE_BINARY_DIR}/src ${LIBMONGOCXX_STATIC_INCLUDE_DIRS})
