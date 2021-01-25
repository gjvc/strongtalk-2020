
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//


#include "ResourceObject.hpp"


void *ResourceObject::operator new( std::int32_t size, bool_t on_C_heap ) {
    return on_C_heap ? (void *) malloc( size ) : allocateResource( size );
}


void ResourceObject::operator delete( void *p, std::int32_t ) {
}
