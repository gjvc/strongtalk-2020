//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/system/asserts.hpp"
#include "vm/memory/allocation.hpp"
#include "vm/utilities/OutputStream.hpp"
#include "vm/runtime/ReservedSpace.hpp"
#include "vm/system/os.hpp"


ReservedSpace::ReservedSpace( std::int32_t size ) :
    _base{ nullptr },
    _size{ size } {
    st_assert( ( size % os::vm_page_size() ) == 0, "size not page aligned" );
    _base = os::reserve_memory( size );
}


ReservedSpace ReservedSpace::first_part( std::int32_t partition_size ) {
    if ( partition_size > size() ) {
        st_fatal( "partition failed" );
    }
    ReservedSpace result( base(), partition_size );
    return result;
}


ReservedSpace ReservedSpace::last_part( std::int32_t partition_size ) {
    if ( partition_size > size() ) {
        st_fatal( "partition failed" );
    }
    ReservedSpace result( base() + partition_size, size() - partition_size );
    return result;
}


std::int32_t ReservedSpace::page_align_size( std::int32_t size ) {
    return align_size( size, os::vm_page_size() );
}


std::int32_t ReservedSpace::align_size( std::int32_t size, std::int32_t page_size ) {
    std::int32_t adjust = size == 0 ? page_size : ( page_size - ( size % page_size ) ) % page_size;
    return size + adjust;
}
