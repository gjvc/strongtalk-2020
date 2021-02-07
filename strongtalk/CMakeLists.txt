
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


# git -------------------------------------------------------------------------

execute_process ( COMMAND git branch --show-current WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}" OUTPUT_VARIABLE GIT_CURRENT_BRANCH ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE )
execute_process ( COMMAND git rev-parse ${GIT_CURRENT_BRANCH} WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}" OUTPUT_VARIABLE GIT_CURRENT_REVISION ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE )


# toolchain -------------------------------------------------------------------

set ( arch i686 )
set ( platform w64-mingw32 )
set ( compiler-family gnu )
set ( compiler-version 10 )
set ( runtime posix )
set ( toolchain-name ${arch}-${platform}-${compiler-family}-${compiler-version}-${runtime} )

set ( cc-compiler /usr/bin/${arch}-${platform}-gcc-${runtime} ) # i686-w64-mingw32-gcc-posix
set ( cxx-compiler /usr/bin/${arch}-${platform}-g++-${runtime} ) # i686-w64-mingw32-g++-posix

set ( build-type debug )


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
set ( ext-dir ${CMAKE_CURRENT_SOURCE_DIR}/ext )
set ( cpp-auto-dir ${cpp-src-dir}/auto )
set ( cpp-main-dir ${cpp-src-dir}/main )
set ( cpp-test-dir ${cpp-src-dir}/test )


# object directories ----------------------------------------------------------

set ( fmt-object-dir ${CMAKE_CURRENT_SOURCE_DIR}/obj/${toolchain-name}/cmake/${build-type}/ext/fmt/ )
set ( googletest-object-dir ${CMAKE_CURRENT_SOURCE_DIR}/obj/${toolchain-name}/cmake/${build-type}/ext/googletest/ )
set ( nasm-object-dir ${CMAKE_CURRENT_SOURCE_DIR}/obj/${toolchain-name}/gnu-make/${build-type}/ext/nasm/ )
set ( spdlog-object-dir ${CMAKE_CURRENT_SOURCE_DIR}/obj/${toolchain-name}/cmake/${build-type}/ext/spdlog/ )


# cmake -----------------------------------------------------------------------

set ( CMAKE_CXX_EXTENSIONS ON )
set ( CMAKE_CXX_STANDARD 20 )
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
message ( STATUS "-----------------------------------------------------------------------------" )
message ( STATUS ">>> toolchain variables" )
message ( STATUS "-----------------------------------------------------------------------------" )
message ( STATUS "" )
message ( STATUS "arch              [${arch}]" )
message ( STATUS "platform          [${platform}]" )
message ( STATUS "compiler-family   [${compiler-family}]" )
message ( STATUS "compiler-version  [${compiler-version}]" )
message ( STATUS "runtime           [${runtime}]" )
message ( STATUS "toolchain-name    [${toolchain-name}]" )
message ( STATUS "" )
message ( STATUS "ext-dir           [${ext-dir}]" )
message ( STATUS "cpp-auto-dir      [${cpp-auto-dir}]" )
message ( STATUS "cpp-main-dir      [${cpp-main-dir}]" )
message ( STATUS "cpp-test-dir      [${cpp-test-dir}]" )
message ( STATUS "" )
message ( STATUS "spdlog-object-dir     [${spdlog-object-dir}]" )
message ( STATUS "nasm-object-dir       [${nasm-object-dir}]" )
message ( STATUS "fmt-object-dir        [${fmt-object-dir}]" )
message ( STATUS "googletest-object-dir [${googletest-object-dir}]" )
message ( STATUS "" )
message ( STATUS "GIT_CURRENT_BRANCH    [${GIT_CURRENT_BRANCH}]" )
message ( STATUS "GIT_CURRENT_REVISION  [${GIT_CURRENT_REVISION}]" )
message ( STATUS "" )
message ( STATUS "" )


# sources ----------------------------------------------------------------------

file ( GLOB_RECURSE st20-main-library-sources ${CMAKE_SOURCE_DIR} "${cpp-main-dir}/vm/*/*.cpp" "${cpp-auto-dir}/vm/*/*.cpp" )
list ( FILTER st20-main-library-sources EXCLUDE REGEX "src/cpp/main/vm/main/main.cpp" )


# library ---------------------------------------------------------------------

include_directories ( PRIVATE ${cpp-main-dir} )
include_directories ( PRIVATE ${cpp-test-dir} )
include_directories ( PRIVATE ${cpp-auto-dir} )

include_directories ( SYSTEM ${ext-dir}/googletest/googletest/include )
include_directories ( SYSTEM ${ext-dir}/nasm/include )
include_directories ( SYSTEM ${ext-dir}/nasm )
include_directories ( SYSTEM ${ext-dir}/nasm/disasm )
include_directories ( SYSTEM ${ext-dir}/nasm/x86 )
include_directories ( SYSTEM ${ext-dir}/spdlog/include )

add_compile_options ( -fpermissive )

# library ----------------------------------------------------------------------

file ( GLOB_RECURSE st20-main-library-sources ${CMAKE_SOURCE_DIR} "${cpp-main-dir}/vm/*/*.cpp" "${cpp-auto-dir}/vm/*/*.cpp" )
list ( FILTER st20-main-library-sources EXCLUDE REGEX "src/cpp/main/vm/main/main.cpp" )
list ( FILTER st20-main-library-sources EXCLUDE REGEX "src/cpp/main/vm/main/version.cpp" )
add_library ( st20-main-library-object OBJECT ${st20-main-library-sources} )
add_library ( st20-main-library-static STATIC $<TARGET_OBJECTS:st20-main-library-object> )


# binary ----------------------------------------------------------------------

file ( GLOB_RECURSE st20-main-binary-sources ${CMAKE_SOURCE_DIR} "${cpp-main-dir}/vm/main/*.cpp" )
add_executable ( st20-main-binary ${st20-main-binary-sources} )
set_target_properties ( st20-main-binary PROPERTIES OUTPUT_NAME st20-main )

target_link_directories ( st20-main-binary PUBLIC ${spdlog-object-dir} )
target_link_options ( st20-main-binary PUBLIC     -static-libgcc  -static-libstdc++ )
target_link_libraries ( st20-main-binary PUBLIC st20-main-library-static spdlog )
add_compile_definitions ( st20-main-binary PRIVATE GIT_CURRENT_BRANCH=${GIT_CURRENT_BRANCH} )
add_compile_definitions ( st20-main-binary PRIVATE GIT_CURRENT_REVISION=${GIT_CURRENT_REVISION} )


# test ------------------------------------------------------------------------

file ( GLOB st20-test-library-sources ${CMAKE_SOURCE_DIR} "${cpp-test-dir}/test/*/*.cpp" )
list ( FILTER st20-test-library-sources EXCLUDE REGEX "src/cpp/test/test/main/main.cpp" )

add_library ( st20-test-library-object OBJECT ${st20-test-library-sources} )
add_library ( st20-test-library-static STATIC $<TARGET_OBJECTS:st20-test-library-object> )

file ( GLOB_RECURSE st20-test-binary-sources ${CMAKE_SOURCE_DIR} "${cpp-test-dir}/test/main/*.cpp" )
add_executable ( st20-test-binary ${st20-test-binary-sources} )
target_link_directories ( st20-test-binary PUBLIC ${spdlog-object-dir} )
target_link_directories ( st20-test-binary PUBLIC ${googletest-object-dir}/lib )
target_link_libraries ( st20-test-binary PUBLIC st20-test-library-static st20-main-library-static spdlog gtest )
