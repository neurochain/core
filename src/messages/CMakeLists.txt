FILE(GLOB ProtoFiles "*.proto")
protobuf_generate_cpp(ProtoSources ProtoHeaders ${ProtoFiles})

add_library(protos
  ${ProtoSources}
  ${ProtoHeaders}
  )
target_link_libraries(protos
  protobuf::libprotobuf
  ${LIBMONGOCXX_STATIC_LIBRARIES}
  )
