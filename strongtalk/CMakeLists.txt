
####
####  (C) 1994 - 2021, The Strongtalk authors and contributors
####  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
####


# cmake -----------------------------------------------------------------------

cmake_minimum_required ( VERSION 3.0 )
project ( st20 )


# ccache ----------------------------------------------------------------------

if ( "${CMAKE_HOST_SYSTEM_NAME}" STREQUAL "Linux" )

    set ( CMAKE_C_COMPILER_LAUNCHER CCACHE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/.ccache /usr/bin/ccache )
    set ( CMAKE_CXX_COMPILER_LAUNCHER CCACHE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/.ccache /usr/bin/ccache )

endif ( "${CMAKE_HOST_SYSTEM_NAME}" STREQUAL "Linux" )


# toolchain -------------------------------------------------------------------

set ( arch i686 )
set ( platform w64-mingw32 )
set ( compiler-family llvm )
set ( compiler-version 10 )
set ( runtime posix )
set ( runtime-platform linux-gnu )
set ( toolchain-name ${arch}-${platform}-${compiler-family}-${compiler-version}-${runtime} )

set ( cc-compiler /usr/bin/${arch}-${platform}-gcc-${runtime} ) # i686-w64-mingw32-gcc-posix
set ( cxx-compiler /usr/bin/${arch}-${platform}-g++-${runtime} ) # i686-w64-mingw32-g++-posix


# cmake -----------------------------------------------------------------------

set ( object-path ${CMAKE_CURRENT_SOURCE_DIR}/obj/${toolchain-name}-${runtime-variant}/${build-type} )

set ( CMAKE_CXX_COMPILER ${g++} )
set ( CMAKE_CXX_COMPILER ${g++} )
set ( CMAKE_CXX_LINKER_PREFERENCE ${g++} )
set ( CMAKE_C_COMPILER ${gcc} )
set ( CMAKE_C_LINKER_PREFERENCE ${gcc} )
set ( CMAKE_VERBOSE_MAKEFILE ON )


# source directories ----------------------------------------------------------

set ( cpp-src-dir ${CMAKE_CURRENT_SOURCE_DIR}/src/cpp )
set ( cpp-auto-dir ${cpp-src-dir}/auto )
set ( cpp-main-dir ${cpp-src-dir}/main )
set ( cpp-test-dir ${cpp-src-dir}/test )


# cmake -----------------------------------------------------------------------

set ( CMAKE_CXX_STANDARD 20 )
set ( CMAKE_CXX_STANDARD_REQUIRED YES )
set ( CMAKE_CXX_EXTENSIONS ON )

set ( CMAKE_BUILD_TYPE Debug )
set ( CMAKE_EXPORT_COMPILE_COMMANDS YES )
set ( CMAKE_VERBOSE_MAKEFILE YES )


# global ----------------------------------------------------------------------

message ( STATUS "" )
message ( STATUS "" )
message ( STATUS "-----------------------------------------------------------------------------" )
message ( STATUS ">>> CMAKE_ variables" )
message ( STATUS "-----------------------------------------------------------------------------" )
message ( STATUS "" )

message ( STATUS "CMAKE_BUILD_TYPE             [${CMAKE_BUILD_TYPE}]" )
message ( STATUS "CMAKE_CURRENT_BINARY_DIR     [${CMAKE_CURRENT_BINARY_DIR}]" )
message ( STATUS "CMAKE_CURRENT_SOURCE_DIR     [${CMAKE_CURRENT_SOURCE_DIR}]" )
message ( STATUS "CMAKE_CXX_COMPILER           [${CMAKE_CXX_COMPILER}]" )
message ( STATUS "CMAKE_CXX_COMPILER_ID        [${CMAKE_CXX_COMPILER_ID}]" )
message ( STATUS "CMAKE_CXX_COMPILER_LAUNCHER  [${CMAKE_CXX_COMPILER_LAUNCHER}]" )
message ( STATUS "CMAKE_CXX_STANDARD_REQUIRED  [${CMAKE_CXX_STANDARD_REQUIRED}]" )
message ( STATUS "CMAKE_C_COMPILER             [${CMAKE_C_COMPILER}]" )
message ( STATUS "CMAKE_C_COMPILER_LAUNCHER    [${CMAKE_C_COMPILER_LAUNCHER}]" )
message ( STATUS "CMAKE_HOST_SYSTEM_NAME       [${CMAKE_HOST_SYSTEM_NAME}]" )
message ( STATUS "CMAKE_HOST_SYSTEM_VERSION    [${CMAKE_HOST_SYSTEM_VERSION}]" )
message ( STATUS "CMAKE_SYSTEM                 [${CMAKE_SYSTEM}]" )
message ( STATUS "CMAKE_SYSTEM_NAME            [${CMAKE_SYSTEM_NAME}]" )
message ( STATUS "CMAKE_VERBOSE_MAKEFILE       [${CMAKE_VERBOSE_MAKEFILE}]" )
message ( STATUS "CMAKE_VERSION                [${CMAKE_VERSION}]" )
message ( STATUS "" )
message ( STATUS "" )


# sources ----------------------------------------------------------------------

file ( GLOB_RECURSE st20-main-library-sources ${CMAKE_SOURCE_DIR} "${cpp-main-dir}/vm/*/*.cpp" )
list ( FILTER st20-main-library-sources EXCLUDE REGEX "main.cpp" )


# library ---------------------------------------------------------------------

include_directories ( PRIVATE ${cpp-main-dir} )
include_directories ( PRIVATE ${cpp-test-dir} )
include_directories ( PRIVATE ${cpp-auto-dir} )
include_directories ( PRIVATE ${spdlog-include} )
include_directories ( SYSTEM ${CMAKE_SOURCE_DIR}/src/ext/googletest/googletest/include )
include_directories ( SYSTEM ${CMAKE_SOURCE_DIR}/src/ext/nasm/include ${CMAKE_SOURCE_DIR}/src/ext/nasm/disasm ${CMAKE_SOURCE_DIR}/src/ext/nasm ${CMAKE_SOURCE_DIR}/src/ext/nasm/x86 )
include_directories ( SYSTEM ${CMAKE_SOURCE_DIR}/src/ext/spdlog/include )
add_compile_options ( -fpermissive )


# library ----------------------------------------------------------------------

file ( GLOB_RECURSE st20-main-library-sources ${CMAKE_SOURCE_DIR} "${cpp-main-dir}/vm/*/*.cpp" )
list ( FILTER st20-main-library-sources EXCLUDE REGEX "main.cpp" )
add_library ( st20-main-library-object OBJECT ${st20-main-library-sources} )
add_library ( st20-main-library-static STATIC $<TARGET_OBJECTS:st20-main-library-object> )


# binary ----------------------------------------------------------------------

file ( GLOB_RECURSE st20-main-binary-sources ${CMAKE_SOURCE_DIR} "${cpp-main-dir}/vm/main/*.cpp" )
add_executable ( st20-main-binary ${st20-main-binary-sources} )
target_link_libraries( st20-main-binary PRIVATE ${st20-main-library-static} )


# # test ------------------------------------------------------------------------
# 
# file ( GLOB st20-test-library-sources ${CMAKE_SOURCE_DIR} "${cpp-test-dir}/test/*/*.cpp" )
# list ( FILTER st20-test-library-sources EXCLUDE REGEX "main.cpp" )
# 
# add_library ( st20-test-library-object OBJECT ${st20-test-library-sources} )
# add_library ( st20-test-library-static STATIC $<TARGET_OBJECTS:st20-test-library-object> )
# 
# file ( GLOB_RECURSE st20-test-binary-sources ${CMAKE_SOURCE_DIR} "${cpp-test-dir}/test/main/*.cpp" )
# add_executable ( st20-test-binary ${st20-test-binary-sources} )
# target_link_libraries( st20-test-binary ${st20-main-library-static} ${st20-test-library-static} )
# 
