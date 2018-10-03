cmake_minimum_required(VERSION 3.0)
set (CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMake")
include(HunterGate)

HunterGate(
  URL "https://gitlab.com/traxncc/hunter/-/archive/pr.onion/hunter-pr.onion.tar.bz2"
  SHA1 "89e5c4616ad9d1e38c745ea8b30641bd64e1ec83"
  )

set(CPACK_PACKAGE_VERSION_MAJOR "0")
set(CPACK_PACKAGE_VERSION_MINOR "0")
set(CPACK_PACKAGE_VERSION_PATCH "1")
set(CPACK_PACKAGE_VERSION "${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")

project(core VERSION ${CPACK_PACKAGE_VERSION})

enable_testing ()


set(CMAKE_CXX_FLAGS "-Wall  ${CMAKE_CXX_FLAGS} -std=c++17 ")
set(CMAKE_CXX_FLAGS_RELEASE " -O3 ${CMAKE_CXX_FLAGS} -Werror")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0  ${CMAKE_CXX_FLAGS} --coverage -fsanitize=address ")

include(GNUInstallDirs)
include(FindPkgConfig)

hunter_add_package(Boost COMPONENTS program_options system filesystem log thread )
find_package(Boost 1.65 CONFIG REQUIRED program_options system filesystem log thread )

hunter_add_package(GTest)
find_package(GTest CONFIG REQUIRED)

hunter_add_package(Protobuf)
include(FindProtobuf)
find_package(Protobuf CONFIG REQUIRED)

hunter_add_package(cryptopp)
find_package(cryptopp CONFIG REQUIRED)

hunter_add_package(mongo-cxx-driver)
find_package(libbsoncxx-static CONFIG REQUIRED)
find_package(libmongocxx-static CONFIG REQUIRED)

hunter_add_package(libonion)
find_package(libonion CONFIG REQUIRED)

add_subdirectory(src)
add_subdirectory(tests)


set(CPACK_GENERATOR "TGZ")
set(CPACK_TAR_COMPONENT_INSTALL ON)
set(CPACK_COMPONENTS_IGNORE_GROUPS 1)
set(CPACK_COMPONENTS_ALL dev main)


include(CPack)
