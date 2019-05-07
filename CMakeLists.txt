cmake_minimum_required(VERSION 3.0)
set (CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMake")
include(HunterGate)

HunterGate(
  URL "https://gitlab.com/traxncc/hunter/-/archive/pr.onion/hunter-pr.onion.tar.bz2"
  SHA1 "dfc0c230e491ef7abf6a628d7d5c71d32dd05b6e"
  )

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CPACK_PACKAGE_VERSION_MAJOR "0")
set(CPACK_PACKAGE_VERSION_MINOR "0")
set(CPACK_PACKAGE_VERSION_PATCH "1")
set(CPACK_PACKAGE_VERSION "${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")

project(core VERSION ${CPACK_PACKAGE_VERSION})

execute_process(
  COMMAND git log -1 --format=%h
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE GIT_COMMIT_HASH
  OUTPUT_STRIP_TRAILING_WHITESPACE
  )

execute_process(
  COMMAND git rev-parse --abbrev-ref HEAD
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE GIT_BRANCH
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

configure_file(
  ${CMAKE_SOURCE_DIR}/version.h.in
  ${CMAKE_BINARY_DIR}/src/version.h
)

enable_testing ()


set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH};~/root/lib/cmake/pistache")
set(CMAKE_CXX_FLAGS "-Wall  ${CMAKE_CXX_FLAGS} -std=c++17 -Wno-ignored-qualifiers -pipe -Werror")
set(CMAKE_CXX_FLAGS_RELEASE " -O3")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 --coverage -fsanitize=address")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-g -O2 -fsanitize=address")

include(GNUInstallDirs)
include(FindPkgConfig)

hunter_add_package(Boost COMPONENTS program_options system filesystem log thread)
find_package(Boost 1.68 CONFIG REQUIRED program_options system filesystem log thread)

hunter_add_package(GTest)
find_package(GTest CONFIG REQUIRED)

hunter_add_package(Protobuf)
include(FindProtobuf)
find_package(Protobuf CONFIG REQUIRED)

hunter_add_package(cryptopp)
find_package(cryptopp CONFIG REQUIRED)

find_package(Pistache REQUIRED pistache_shared)

hunter_add_package(mongo-cxx-driver)
find_package(libbsoncxx-static CONFIG REQUIRED)
find_package(libmongocxx-static CONFIG REQUIRED)

hunter_add_package(libonion)
find_package(libonion CONFIG REQUIRED)

hunter_add_package(cpr)
find_package(cpr CONFIG REQUIRED)

add_subdirectory(src)
add_subdirectory(tests)


set(CPACK_GENERATOR "TGZ")
set(CPACK_TAR_COMPONENT_INSTALL ON)
set(CPACK_COMPONENTS_IGNORE_GROUPS 1)
set(CPACK_COMPONENTS_ALL dev main)


include(CPack)
