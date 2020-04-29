
##
##  (C) 1994 - 2020, The Strongtalk authors and contributors
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

set ( CMAKE_SYSTEM_NAME Windows )

# ccache ----------------------------------------------------------------------

if ( "${CMAKE_HOST_SYSTEM_NAME}" STREQUAL "Linux" )

    set ( CMAKE_C_COMPILER_LAUNCHER CCACHE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/var/ccache /usr/bin/ccache )
    set ( CMAKE_CXX_COMPILER_LAUNCHER CCACHE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/var/ccache /usr/bin/ccache )

endif ( "${CMAKE_HOST_SYSTEM_NAME}" STREQUAL "Linux" )


# mingw-w64 -------------------------------------------------------------------

if ( "${CMAKE_HOST_SYSTEM_NAME}" STREQUAL "Linux" )

    if ( "${CMAKE_SYSTEM_NAME}" STREQUAL "Windows" )

        set ( gcc-version-major 9 )
        set ( gcc-version-minor 3 )
        set ( gcc-version ${gcc-version-major}-${gcc-version-minor} )
        set ( machine-name i686 )
        set ( runtime-platform w64-mingw32 )
        set ( runtime-variant posix )

        set ( platform-link-flags -L /usr/${machine-name}-${runtime-platform}/lib -Wl,-Bstatic,--whole-archive -Wl,-lwinpthread -Wl,-Bdynamic,--no-whole-archive )

    endif ( "${CMAKE_SYSTEM_NAME}" STREQUAL "Windows" )


    if ( "${CMAKE_SYSTEM_NAME}" STREQUAL "Linux" )

        set ( gcc-version-major 9 )
        set ( gcc-version-minor 3 )
        set ( gcc-version ${gcc-version-major}-${gcc-version-minor} )
        set ( machine-name i686 )
        set ( runtime-platform linux-gnu )
        set ( runtime-variant ${gcc-version-major} )

        set ( platform-link-flags -ldl -lpthread -lreadline )

    endif ( "${CMAKE_SYSTEM_NAME}" STREQUAL "Linux" )


    set ( toolchain-name ${machine-name}-${runtime-platform} ) # i686-w64-mingw32
    set ( runtime-prefix /usr/${toolchain-name} ) # i686-w64-mingw32

    set ( gcc /usr/bin/${toolchain-name}-gcc-${runtime-variant} ) # i686-w64-mingw32-gcc-posix
    set ( g++ /usr/bin/${toolchain-name}-g++-${runtime-variant} ) # i686-w64-mingw32-g++-posix

    set ( CMAKE_FIND_ROOT_PATH ${runtime-prefix} )
    set ( CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY )
    set ( CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY )
    set ( CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER )


endif ( "${CMAKE_HOST_SYSTEM_NAME}" STREQUAL "Linux" )

set ( object-path ${CMAKE_CURRENT_SOURCE_DIR}/obj/${toolchain-name}-${runtime-variant}/${build-type} )

set ( CMAKE_C_COMPILER ${gcc} )
set ( CMAKE_CXX_COMPILER ${g++} )

message ( STATUS "runtime-prefix [${runtime-prefix}]" )
message ( STATUS "libgcc-prefix [${libgcc-prefix}]" )


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
message ( STATUS "-----------------------------------------------------------------------------" )
message ( STATUS ">>> ${CMAKE_CXX_COMPILER_ID}" )
message ( STATUS "-----------------------------------------------------------------------------" )
message ( STATUS "" )

if ( CMAKE_CXX_COMPILER_ID STREQUAL "GNU" )
    add_compile_options ( -fpermissive )
    add_compile_options ( -fcheck-new )
    add_compile_options ( -fconcepts )
    add_compile_options ( -O0 )
    add_compile_options ( -g3 )
    add_compile_options ( -gdwarf-2 )
    add_compile_options ( -m32 )

    add_link_options ( -m32 )
    add_link_options ( -static-libgcc )
    add_link_options ( -static-libstdc++ )

elseif ( CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" )
    # ...MSVC...

endif ()


# sources ----------------------------------------------------------------------

file ( GLOB_RECURSE st20-main-library-sources ${CMAKE_SOURCE_DIR} "${cpp-main-dir}/vm/*/*.cpp" )
list ( FILTER st20-main-library-sources EXCLUDE REGEX "main.cpp" )


# library ---------------------------------------------------------------------

add_library ( st20-main-library-object OBJECT ${st20-main-library-sources} )
target_include_directories ( st20-main-library-object PRIVATE ${cpp-main-dir} )
target_include_directories ( st20-main-library-object PUBLIC ${object-path}/debug/ext/nasm/include/ )
target_include_directories ( st20-main-library-object PUBLIC ${object-path}/debug/ext/udis86/include/ )

add_library ( st20-main-library-static STATIC $<TARGET_OBJECTS:st20-main-library-object> )


# binary ----------------------------------------------------------------------

file ( GLOB_RECURSE st20-main-binary-sources ${CMAKE_SOURCE_DIR} "${cpp-main-dir}/vm/main/*.cpp" )
add_executable ( st20-main-binary ${st20-main-binary-sources} )
target_include_directories ( st20-main-binary PRIVATE ${cpp-main-dir} )
target_link_libraries ( st20-main-binary
                        -Wl,--no-as-needed
                            -Wl,-Bstatic
                                -Wl,--whole-archive
                                    -Wl,--start-group
                                        st20-main-library-static
                                        winpthread
                                    -Wl,--end-group
                                -Wl,--no-whole-archive
                            -Wl,-Bdynamic
                        -Wl,--as-needed
                        ${platform-link-flags}
                        )


# test ------------------------------------------------------------------------

file ( GLOB st20-test-library-sources ${CMAKE_SOURCE_DIR} "${cpp-test-dir}/test/*/*.cpp" )
list ( FILTER st20-test-library-sources EXCLUDE REGEX "main.cpp" )

add_library ( st20-test-library-object OBJECT ${st20-test-library-sources} )
target_include_directories ( st20-test-library-object PRIVATE ${cpp-main-dir} )
target_include_directories ( st20-test-library-object PRIVATE ${cpp-test-dir} )
target_include_directories ( st20-test-library-object PUBLIC ${CMAKE_SOURCE_DIR}/ext/googletest/googletest/include/ )

add_library ( st20-test-library-static STATIC $<TARGET_OBJECTS:st20-test-library-object> )


# test ------------------------------------------------------------------------

file ( GLOB st20-test-binary-sources ${CMAKE_SOURCE_DIR} "${cpp-test-dir}/test/main/main.cpp" )

add_executable ( st20-test-binary ${st20-test-binary-sources} )
target_include_directories ( st20-test-binary PRIVATE ${cpp-test-dir} )
target_include_directories ( st20-test-binary PRIVATE ${cpp-main-dir} )
target_include_directories ( st20-test-binary PUBLIC ${CMAKE_SOURCE_DIR}/ext/googletest/googletest/include/ )
target_link_directories ( st20-test-binary PRIVATE obj/${toolchain-name}-${runtime-variant}/debug/ext/googletest/lib/ )

target_link_libraries ( st20-test-binary
                        -Wl,--no-as-needed
                            -Wl,-Bstatic
                                -Wl,--whole-archive
                                    -Wl,--start-group
                                        st20-main-library-static
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

set ( ccache-show-stats-command CCACHE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/var/ccache ccache --show-stats )
set ( ccache-zero-stats-command CCACHE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/var/ccache ccache --zero-stats )
add_custom_command ( TARGET st20-test-binary POST_BUILD COMMAND ${ccache-show-stats-command} VERBATIM )
add_custom_command ( TARGET st20-main-binary POST_BUILD COMMAND ${ccache-show-stats-command} VERBATIM )
