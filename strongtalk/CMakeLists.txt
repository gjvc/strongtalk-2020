
##
##  (C) 1994 - 2020, The st20 authors and contributors
##  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
##

cmake_minimum_required ( VERSION 3.6 )
project ( st20 )

message ( STATUS "" )
message ( STATUS "-----------------------------------------------------------------------------" )
message ( STATUS ">>> cmake" )
message ( STATUS "-----------------------------------------------------------------------------" )
message ( STATUS "" )

message ( STATUS "CMAKE_HOST_SYSTEM_NAME = [${CMAKE_HOST_SYSTEM_NAME}]" )
message ( STATUS "CMAKE_HOST_SYSTEM_VERSION = [${CMAKE_HOST_SYSTEM_VERSION}]" )
message ( STATUS "CMAKE_SYSTEM_NAME = [${CMAKE_SYSTEM_NAME}]" )
message ( STATUS "CMAKE_SYSTEM = [${CMAKE_SYSTEM}]" )
message ( STATUS "CMAKE_VERSION = [${CMAKE_VERSION}]" )
message ( STATUS "CMAKE_C_COMPILER = [${CMAKE_C_COMPILER}]" )
message ( STATUS "CMAKE_CXX_COMPILER = [${CMAKE_CXX_COMPILER}]" )


# ccache ----------------------------------------------------------------------

if ( "${CMAKE_HOST_SYSTEM_NAME}" STREQUAL "Linux" )

    message ( STATUS "" )
    message ( STATUS "" )
    message ( STATUS "-----------------------------------------------------------------------------" )
    message ( STATUS ">>> ccache" )
    message ( STATUS "-----------------------------------------------------------------------------" )
    message ( STATUS "" )

    set ( CMAKE_C_COMPILER_LAUNCHER CCACHE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/var/ccache /usr/bin/ccache )
    set ( CMAKE_CXX_COMPILER_LAUNCHER CCACHE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/var/ccache /usr/bin/ccache )
    execute_process ( COMMAND CCACHE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/var/ccache ccache --zero-stats OUTPUT_STRIP_TRAILING_WHITESPACE )

endif ( "${CMAKE_HOST_SYSTEM_NAME}" STREQUAL "Linux" )


# mingw-w64 -------------------------------------------------------------------

if ( "${CMAKE_HOST_SYSTEM_NAME}" STREQUAL "Linux" )

    if ( "${CMAKE_SYSTEM_NAME}" STREQUAL "Linux" )
        set ( machine-name i686 )
        set ( runtime-platform linux-gnu )
        set ( gcc-version-major 9 )
        set ( gcc-version-minor 3 )
        set ( gcc-version ${gcc-version-major}-${gcc-version-minor} )
        set ( runtime-variant 9 )
        set ( word-size 32 )

#        /usr/lib/gcc/x86_64-linux-gnu/9/32/libstdc++.a
        set ( libgcc-prefix /usr/lib/gcc/${toolchain-name}/${gcc-version-major}/${word-size} ) # i686-w64-mingw323 9.3 posix
        set( platform-link-flags -ldl -lpthread -lreadline )

    endif ( "${CMAKE_SYSTEM_NAME}" STREQUAL "Linux" )


    if ( "${CMAKE_SYSTEM_NAME}" STREQUAL "Windows" )

        set ( machine-name i686 )
        set ( runtime-platform w64-mingw32 )
        set ( gcc-version 9.3 )
        set ( runtime-variant posix )

        set ( windres /usr/bin/${toolchain-name}-windres ) # i686-w64-mingw32
        set ( dlltool /usr/bin/${toolchain-name}-dlltool ) # i686-w64-mingw32

        set ( CMAKE_RC_COMPILER ${windres} )

        set( mingw-w64-link-flags -L /usr/${machine-name}-${runtime-platform}/lib -Wl,-Bstatic,--whole-archive -Wl,-lwinpthread -Wl,-Bdynamic,--no-whole-archive )
        set( platform-link-flags ${mingw-w64-link-flags} )

    endif ( "${CMAKE_SYSTEM_NAME}" STREQUAL "Windows" )


    set ( toolchain-name ${machine-name}-${runtime-platform} ) # i686-w64-mingw32
    set ( runtime-prefix /usr/${toolchain-name} ) # i686-w64-mingw32
    set ( libgcc-prefix /usr/lib/gcc/${toolchain-name}/${gcc-version}-${runtime-variant} ) # i686-w64-mingw323 9.3 posix

    set ( gcc /usr/bin/${toolchain-name}-gcc-${runtime-variant} ) # i686-w64-mingw32 posix
    set ( g++ /usr/bin/${toolchain-name}-g++-${runtime-variant} ) # i686-w64-mingw32 posix


    set ( CMAKE_FIND_ROOT_PATH ${runtime-prefix} )
    set ( CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY )
    set ( CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY )
    set ( CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER )


endif ( "${CMAKE_HOST_SYSTEM_NAME}" STREQUAL "Linux" )

set ( CMAKE_C_COMPILER ${gcc} )
set ( CMAKE_CXX_COMPILER ${g++} )

message ( STATUS "runtime-prefix [${runtime-prefix}]" )
message ( STATUS "libgcc-prefix [${libgcc-prefix}]" )


# source directories ----------------------------------------------------------

set ( cpp-src-dir ${CMAKE_CURRENT_SOURCE_DIR}/src/cpp )
set ( cpp-main-dir ${cpp-src-dir}/main )
set ( cpp-test-dir ${cpp-src-dir}/test )


# object directories ----------------------------------------------------------

set ( st20-obj ${CMAKE_CURRENT_SOURCE_DIR}/obj/${toolchain-name}-${runtime-variant}/${build-type} )


# cmake -----------------------------------------------------------------------

set ( CMAKE_CXX_STANDARD 20 )
set ( CMAKE_CXX_EXTENSIONS ON )
set ( CMAKE_CXX_STANDARD_REQUIRED YES )

set ( CMAKE_BUILD_TYPE Debug )
set ( CMAKE_EXPORT_COMPILE_COMMANDS YES )
set ( CMAKE_VERBOSE_MAKEFILE YES )


# global ----------------------------------------------------------------------

message ( STATUS "" )
message ( STATUS "-----------------------------------------------------------------------------" )
message ( STATUS ">>> ${CMAKE_CXX_COMPILER_ID}" )
message ( STATUS "-----------------------------------------------------------------------------" )
message ( STATUS "" )

if ( CMAKE_CXX_COMPILER_ID STREQUAL "Clang" )
    # ...clang...

elseif ( CMAKE_CXX_COMPILER_ID STREQUAL "GNU" )
    add_compile_options ( -fpermissive )
    add_compile_options ( -fcheck-new )
    add_compile_options ( -fconcepts )
    add_compile_options ( -O0 )
    add_compile_options ( -g3 )
    add_compile_options ( -ggdb )
    add_compile_options ( -gdwarf-2 )
    add_compile_options ( -m32 )

    add_link_options ( -m32 )
    add_link_options ( -static-libgcc )
    add_link_options ( -static-libstdc++ )

elseif ( CMAKE_CXX_COMPILER_ID STREQUAL "Intel" )
    # ...Intel...

elseif ( CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" )
    # ...MSVC...

endif ()


# sources ----------------------------------------------------------------------

file ( GLOB_RECURSE st20-vm-sources ${CMAKE_SOURCE_DIR} "${cpp-main-dir}/vm/*/*.cpp" )
file ( GLOB_RECURSE st20-test-sources ${CMAKE_SOURCE_DIR} "${cpp-test-dir}/test/*/*.cpp" )


# library ---------------------------------------------------------------------

set ( st20-vm-library-sources ${st20-vm-sources} )
list ( FILTER st20-vm-library-sources EXCLUDE REGEX "(posix|win32).cpp" )

add_library ( st20-vm-library-object OBJECT ${st20-vm-library-sources} )
target_include_directories ( st20-vm-library-object PRIVATE ${cpp-main-dir} )
target_include_directories ( st20-vm-library-object PUBLIC obj/${toolchain-name}-${runtime-variant}/debug/ext/nasm/include/ )
target_include_directories ( st20-vm-library-object PUBLIC obj/${toolchain-name}-${runtime-variant}/debug/ext/udis86/include/ )

add_library ( st20-vm-library-static STATIC $<TARGET_OBJECTS:st20-vm-library-object> )


# binary ----------------------------------------------------------------------

set ( st20-runtime-sources ${st20-vm-sources} )
if ( "${CMAKE_SYSTEM_NAME}" STREQUAL "Linux" )
    list ( FILTER st20-runtime-sources INCLUDE REGEX "posix.cpp" )
endif ( "${CMAKE_SYSTEM_NAME}" STREQUAL "Linux" )
if ( "${CMAKE_SYSTEM_NAME}" STREQUAL "Windows" )
    list ( FILTER st20-runtime-sources INCLUDE REGEX "win32.cpp" )
endif ( "${CMAKE_SYSTEM_NAME}" STREQUAL "Windows" )

add_executable ( st20-main-binary ${st20-runtime-sources} )
target_include_directories ( st20-main-binary PRIVATE ${cpp-main-dir} )
target_link_libraries ( st20-main-binary
                        -Wl,--no-as-needed
                        -Wl,-Bstatic
                        -Wl,--whole-archive
                        -Wl,--start-group
                        st20-vm-library-static
                        -Wl,--end-group
                        -Wl,--no-whole-archive
                        -Wl,-Bdynamic
                        -Wl,--as-needed
                        ${platform-link-flags}
                        )


# test ------------------------------------------------------------------------

set ( st20-test-library-sources ${st20-test-sources} )
list ( FILTER st20-test-library-sources EXCLUDE REGEX "main.cpp" )

add_library ( st20-test-library-object OBJECT ${st20-test-library-sources} )
target_include_directories ( st20-test-library-object PRIVATE ${cpp-main-dir} )
target_include_directories ( st20-test-library-object PRIVATE ${cpp-test-dir} )
target_include_directories ( st20-test-library-object PUBLIC ${CMAKE_SOURCE_DIR}/ext/googletest/googletest/include )

add_library ( st20-test-library-static STATIC $<TARGET_OBJECTS:st20-test-library-object> )


# test ------------------------------------------------------------------------

set ( st20-test-binary-sources ${st20-test-sources} )
list ( FILTER st20-test-binary-sources INCLUDE REGEX "main.cpp" )

set ( st20-runtime-sources ${st20-vm-sources} )
if ( "${CMAKE_SYSTEM_NAME}" STREQUAL "Windows" )
    list ( FILTER st20-runtime-sources INCLUDE REGEX "win32.cpp" )
endif ( "${CMAKE_SYSTEM_NAME}" STREQUAL "Windows" )
if ( "${CMAKE_SYSTEM_NAME}" STREQUAL "Linux" )
    list ( FILTER st20-runtime-sources INCLUDE REGEX "posix.cpp" )
endif ( "${CMAKE_SYSTEM_NAME}" STREQUAL "Linux" )


add_executable ( st20-test-binary ${st20-test-binary-sources} ${st20-runtime-sources} )
target_include_directories ( st20-test-binary PRIVATE ${cpp-test-dir} )
target_include_directories ( st20-test-binary PRIVATE ${cpp-main-dir} )
target_include_directories ( st20-test-binary PUBLIC ${CMAKE_SOURCE_DIR}/ext/googletest/googletest/include )
target_link_directories ( st20-test-binary PRIVATE obj/${toolchain-name}-${runtime-variant}/debug/ext/googletest/lib )

target_link_libraries ( st20-test-binary
                        -Wl,--no-as-needed
                        -Wl,-Bstatic
                        -Wl,--whole-archive
                        -Wl,--start-group
                        st20-vm-library-static
                        st20-test-library-static
                        gtest
                        -Wl,--end-group
                        -Wl,--no-whole-archive
                        -Wl,-Bdynamic
                        -Wl,--as-needed
                        ${platform-link-flags}
                        )


# done ------------------------------------------------------------------------

message ( STATUS "" )
message ( STATUS "-----------------------------------------------------------------------------" )
message ( STATUS ">>> done" )
message ( STATUS "-----------------------------------------------------------------------------" )
message ( STATUS "" )


# ccache ----------------------------------------------------------------------

add_custom_command ( TARGET st20-test-binary
                     POST_BUILD
                     COMMAND CCACHE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/var/ccache ccache --show-stats
                     VERBATIM
                     )

add_custom_command ( TARGET st20-main-binary
                     POST_BUILD
                     COMMAND CCACHE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/var/ccache ccache --show-stats
                     VERBATIM
                     )
