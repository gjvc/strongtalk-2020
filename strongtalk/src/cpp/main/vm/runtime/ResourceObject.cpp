
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//


#include "vm/runtime/ResourceObject.hpp"


void *ResourceObject::operator new( std::size_t size, bool on_cxx_runtime_heap ) {
    return on_cxx_runtime_heap ? (void *) malloc( size ) : allocateResource( size );
}


void ResourceObject::operator delete( void *p, std::int32_t size ) {
    static_cast<void>(p); // unused
    static_cast<void>(size); // unused

}
