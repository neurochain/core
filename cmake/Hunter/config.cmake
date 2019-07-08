hunter_config(
    cryptopp
    VERSION
    "5.6.5-neuro"
    URL
    "https://github.com/hunter-packages/cryptopp/archive/v5.6.5-p0.tar.gz"
    SHA1
    4258c9b49c48c433c4aa63629bc896ac9a3902e3
    CMAKE_ARGS "CMAKE_CXX_FLAGS='-march=x86-64';CRYPTOPP_CROSS_COMPILE=On;CRYPTOPP_NATIVE_ARCH=Off;CRYPTOPP_CROSS_COMPILE=On"
)
