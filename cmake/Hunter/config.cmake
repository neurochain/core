hunter_config(
    cryptopp
    VERSION
    "8.2.0-neuro"
    URL
    "https://github.com/hunter-packages/cryptopp/archive/v8.2.0-p0.tar.gz"
    SHA1
    38a70c9ba970cc862b5cca0010fffdd4e56afcae
)

hunter_config(
    mongo-c-driver
    VERSION
    1.12.0
    URL
    "https://github.com/mongodb/mongo-c-driver/archive/1.12.0.tar.gz"
    SHA1
    579c0f0663cd5d7fa2b79fcea99f39dfbac5fb81
)

hunter_config(
    mongo-cxx-driver
    VERSION
    3.3.2
    URL
    "https://github.com/trax44/mongo-cxx-driver/archive/releases/v3.3.zip"
    SHA1
    1cd204525f3c6a9bc58f2df4db8c53b0225b21e3
)


hunter_config(
  pistache
  VERSION
  97d0cb3b751f690460b9ceb6d39464ac23d45f1e
  URL
  "https://github.com/oktal/pistache/archive/97d0cb3b751f690460b9ceb6d39464ac23d45f1e.zip"
  SHA1
  346cb785fc90bf062b42246e87cfe35a26192275
  )

hunter_config(
  gRPC
  VERSION
  a7a6e3aa02a420d2b40713d3c0cda5734d70842b
  URL
  "https://gitlab.com/neurochaintech/grpc/-/archive/hunter-1.17.2/grpc-hunter-1.17.2.zip"
  SHA1
  1bd175ab0edab1f65e5a96418796e73203c53be0
)

hunter_config(
  c-ares
  VERSION
  1.14.0-p0
  CMAKE_ARGS
  CARES_SHARED=ON CARES_STATIC=OFF CARES_STATIC_PIC=ON CARES_STATICLIB=OFF
)

hunter_config(
  ZLIB
  VERSION
  8ce5d756dc79882b8dcc418c57815fdd586d319c
  URL
  "https://gitlab.com/neurochaintech/zlib/-/archive/hunter-1.2.11/zlib-hunter-1.2.11.zip"
  SHA1
  dd869cdce074872d0a02dfc8497e43c73dd8bbb6
)

hunter_config(
  GTest
  VERSION
  1.10.0-p0
)
