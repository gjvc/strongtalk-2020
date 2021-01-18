
##
##  (C) 1994 - 2021, The Strongtalk authors and contributors
##  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
##


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
set ( compiler-family gnu )
set ( compiler-version 10 )
set ( runtime win32 )
set ( runtime-platform linux-gnu )
set ( toolchain-name ${arch}-${platform}-${compiler-family}-${compiler-version}-${runtime} )

set ( cc-compiler /usr/bin/${arch}-${platform}-gcc-${runtime} ) # i686-w64-mingw32-gcc-posix
set ( cxx-compiler /usr/bin/${arch}-${platform}-g++-${runtime} ) # i686-w64-mingw32-g++-posix


# cmake -----------------------------------------------------------------------

set ( object-path ${CMAKE_CURRENT_SOURCE_DIR}/obj/${toolchain-name}-${runtime-variant}/${build-type} )

set ( CMAKE_C_COMPILER ${gcc} )
set ( CMAKE_CXX_COMPILER ${g++} )
set ( CMAKE_C_LINKER_PREFERENCE ${gcc} )
set ( CMAKE_CXX_LINKER_PREFERENCE ${g++} )
set ( CMAKE_CXX_COMPILER ${g++} )
set ( CMAKE_VERBOSE_MAKEFILE ON )


# source directories ----------------------------------------------------------

set ( cpp-src-dir ${CMAKE_CURRENT_SOURCE_DIR}/src/cpp )
set ( cpp-main-dir ${cpp-src-dir}/main )
set ( cpp-test-dir ${cpp-src-dir}/test )


# cmake -----------------------------------------------------------------------

set ( CMAKE_CXX_STANDARD 20 )
set ( CMAKE_CXX_EXTENSIONS ON )
set ( CMAKE_CXX_STANDARD_REQUIRED YES )

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

message ( STATUS "CMAKE_HOST_SYSTEM_NAME        [${CMAKE_HOST_SYSTEM_NAME}]" )
message ( STATUS "CMAKE_HOST_SYSTEM_VERSION     [${CMAKE_HOST_SYSTEM_VERSION}]" )
message ( STATUS "CMAKE_SYSTEM_NAME             [${CMAKE_SYSTEM_NAME}]" )
message ( STATUS "CMAKE_SYSTEM                  [${CMAKE_SYSTEM}]" )
message ( STATUS "CMAKE_VERSION                 [${CMAKE_VERSION}]" )
message ( STATUS "CMAKE_C_COMPILER              [${CMAKE_C_COMPILER}]" )
message ( STATUS "CMAKE_CXX_COMPILER            [${CMAKE_CXX_COMPILER}]" )
message ( STATUS "CMAKE_VERBOSE_MAKEFILE        [${CMAKE_VERBOSE_MAKEFILE}]" )
message ( STATUS "CMAKE_BUILD_TYPE              [${CMAKE_BUILD_TYPE}]" )
message ( STATUS "CMAKE_CXX_STANDARD_REQUIRED   [${CMAKE_CXX_STANDARD_REQUIRED}]" )
message ( STATUS "CMAKE_C_COMPILER_LAUNCHER     [${CMAKE_C_COMPILER_LAUNCHER}]" )


# global ----------------------------------------------------------------------

message ( STATUS "" )
message ( STATUS "" )
message ( STATUS "-----------------------------------------------------------------------------" )
message ( STATUS ">>> ${CMAKE_CXX_COMPILER_ID}" )
message ( STATUS "-----------------------------------------------------------------------------" )
message ( STATUS "" )


# sources ----------------------------------------------------------------------

file ( GLOB_RECURSE st20-main-library-sources ${CMAKE_SOURCE_DIR} "${cpp-main-dir}/vm/*/*.cpp" )
list ( FILTER st20-main-library-sources EXCLUDE REGEX "main.cpp" )


# library ---------------------------------------------------------------------

include_directories ( SYSTEM ${object-path}/src/ext/nasm/include )
add_compile_options ( -fpermissive )


# library ----------------------------------------------------------------------

file ( GLOB_RECURSE st20-main-library-sources ${CMAKE_SOURCE_DIR} "${cpp-main-dir}/vm/*/*.cpp" )
list ( FILTER st20-main-library-sources EXCLUDE REGEX "main.cpp" )
add_library ( st20-main-library-object OBJECT ${st20-main-library-sources} )
target_include_directories ( st20-main-library-object PRIVATE ${cpp-main-dir} )
target_include_directories ( st20-main-library-object INTERFACE ${object-path}/debug/ext/nasm/include/ )
target_include_directories ( st20-main-library-object INTERFACE ${object-path}/debug/ext/udis86/include/ )
add_library ( st20-main-library-static STATIC $<TARGET_OBJECTS:st20-main-library-object> )


# binary ----------------------------------------------------------------------

file ( GLOB_RECURSE st20-main-binary-sources ${CMAKE_SOURCE_DIR} "${cpp-main-dir}/vm/main/*.cpp" )
add_executable ( st20-main-binary ${st20-main-binary-sources} )
target_include_directories ( st20-main-binary PRIVATE ${cpp-main-dir} )


# test ------------------------------------------------------------------------

file ( GLOB st20-test-library-sources ${CMAKE_SOURCE_DIR} "${cpp-test-dir}/test/*/*.cpp" )
list ( FILTER st20-test-library-sources EXCLUDE REGEX "main.cpp" )

add_library ( st20-test-library-object OBJECT ${st20-test-library-sources} )
target_include_directories ( st20-test-library-object PRIVATE ${cpp-main-dir} )
target_include_directories ( st20-test-library-object PRIVATE ${cpp-test-dir} )
target_include_directories ( st20-test-library-object SYSTEM INTERFACE ${CMAKE_SOURCE_DIR}/ext/googletest/googletest/include/ )

add_library ( st20-test-library-static STATIC $<TARGET_OBJECTS:st20-test-library-object> )
