
#
#
#
#


# -----------------------------------------------------------------------------

function ( use_gold theTarget )

    if ( LINKER_IS_LLD )
        target_link_libraries ( ${theTarget} PRIVATE "-fuse-ld=lld" )
        if ( LINKER_THREADS_SUPPORTED )
            target_link_libraries ( ${theTarget} PRIVATE "-Wl,--threads" )
        endif ()
    elseif ( LINKER_IS_GOLD )
        if ( WL_ZDEFS_SUPPORTED )
            target_link_libraries ( ${theTarget} PRIVATE "-fuse-ld=gold" "-Wl,-z,defs" )
        endif ()
        if ( LINKER_THREADS_SUPPORTED )
            processorcount ( NUM_CPUS )
            if ( NUM_CPUS GREATER 1 )
                target_link_libraries ( ${theTarget} PRIVATE "-Wl,--threads" "-Wl,--thread-count,${NUM_CPUS}" )
            endif ()
        endif ()
    endif ()

endfunction ()


# -----------------------------------------------------------------------------

function ( setup_lto )
    if ( "${CMAKE_CXX_COMPILER_ID}" MATCHES "MSVC" )
        set ( CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /GL /Gw /Gy" )
        set ( CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /LTCG /OPT:REF /OPT:ICF" )
        set ( CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /LTCG /OPT:REF /OPT:ICF" )
    endif ()

    # Note : http://stackoverflow.com/questions/31355692/cmake-support-for-gccs-link-time-optimization-lto
    if ( "${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU" )
        find_program ( CMAKE_GCC_AR NAMES ${_CMAKE_TOOLCHAIN_PREFIX}gcc-ar${_CMAKE_TOOLCHAIN_SUFFIX} HINTS ${_CMAKE_TOOLCHAIN_LOCATION} )
        find_program ( CMAKE_GCC_NM NAMES ${_CMAKE_TOOLCHAIN_PREFIX}gcc-nm HINTS ${_CMAKE_TOOLCHAIN_LOCATION} )
        find_program ( CMAKE_GCC_RANLIB NAMES ${_CMAKE_TOOLCHAIN_PREFIX}gcc-ranlib HINTS ${_CMAKE_TOOLCHAIN_LOCATION} )

        set ( CMAKE_AR "${CMAKE_GCC_AR}" CACHE INTERNAL "" )
        set ( CMAKE_NM "${CMAKE_GCC_NM}" CACHE INTERNAL "" )
        set ( CMAKE_RANLIB "${CMAKE_GCC_RANLIB}" CACHE INTERNAL "" )
    endif ()
endfunction ()


function ( enable_lto theTarget )
    if ( "${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU" )
        target_compile_options ( ${theTarget} PUBLIC -s -flto -fuse-linker-plugin -fno-fat-lto-objects )
    endif ()
endfunction ()


#
#
#
# https://raw.githubusercontent.com/OSSIA/cmake-modules/master/UseGold.cmake
