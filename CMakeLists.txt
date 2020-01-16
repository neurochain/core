cmake_minimum_required(VERSION 3.0)
set (CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMake")
include(HunterGate)

HunterGate(
    URL "https://github.com/ruslo/hunter/archive/v0.23.176.tar.gz"
    SHA1 "2e9ae973d028660b735ac4c6142725ca36a0048a"
    LOCAL
)

option(CPPCHECK "use cppcheck" OFF)

if (CPPCHECK)
    find_program(CMAKE_CXX_CPPCHECK NAMES cppcheck)
    list(APPEND CMAKE_CXX_CPPCHECK
            --enable=warning,performance
            -DGOOGLE_PROTOBUF_MIN_PROTOC_VERSION=3005000
            -DGOOGLE_PROTOBUF_VERSION=3005001
            --suppressions-list=${CMAKE_SOURCE_DIR}/cppcheck_suppression.txt)
endif ()

set(CPACK_PACKAGE_VERSION_MAJOR "0")
set(CPACK_PACKAGE_VERSION_MINOR "0")
set(CPACK_PACKAGE_VERSION_PATCH "1")
set(CPACK_PACKAGE_VERSION "${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")

project(core VERSION ${CPACK_PACKAGE_VERSION})

execute_process(
  COMMAND git rev-parse HEAD
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
set(CMAKE_CXX_FLAGS "-Wall ${CMAKE_CXX_FLAGS} -std=c++17 -Wno-ignored-qualifiers -pipe -Werror")
set(CMAKE_CXX_FLAGS_RELEASE " -O3")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 --coverage -fsanitize=address")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-g -O2 -fsanitize=address")

option (ENABLE_PROFILING "Enable profiling with google performance" OFF)
if (ENABLE_PROFILING)
  set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -lprofiler")
endif (ENABLE_PROFILING)


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

#hunter_add_package(Pistache)
find_package(Pistache REQUIRED pistache_shared)

hunter_add_package(mongo-cxx-driver)
find_package(libbsoncxx-static CONFIG REQUIRED)
find_package(libmongocxx-static CONFIG REQUIRED)

hunter_add_package(cpr)
find_package(cpr CONFIG REQUIRED)

add_subdirectory(src)
add_subdirectory(tests)


set(CPACK_GENERATOR "TGZ")
set(CPACK_TAR_COMPONENT_INSTALL ON)
set(CPACK_COMPONENTS_IGNORE_GROUPS 1)
set(CPACK_COMPONENTS_ALL dev main)


include(CPack)
