FILE(GLOB ProtoFiles "*.proto")
protobuf_generate_cpp(ProtoSources ProtoHeaders ${ProtoFiles})
protobuf_generate_grpc_cpp(_ProtoSources _ProtoHeaders ${CMAKE_CURRENT_BINARY_DIR} ${ProtoFiles})

add_library(protos
  ${ProtoSources}
  ${ProtoHeaders}
  ${_ProtoSources}
  ${_ProtoHeaders})

target_link_libraries(protos
  protobuf::libprotobuf
  gRPC::grpc++_reflection
  ${LIBMONGOCXX_STATIC_LIBRARIES})
