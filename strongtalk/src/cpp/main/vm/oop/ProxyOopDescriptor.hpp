//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/oop/MemOopDescriptor.hpp"
#include "vm/runtime/Process.hpp"


// proxy objects are handles to external data.

// memory layout:
//    [header      ]
//    [klass_field ]
//    [pointer     ]

const std::int32_t pointer_no     = sizeof( MemOopDescriptor ) / OOP_SIZE;
const std::int32_t pointer_offset = sizeof( MemOopDescriptor ) - MEMOOP_TAG;

class ProxyOopDescriptor : public MemOopDescriptor {

private:
    void *_pointer;

protected:
    ProxyOopDescriptor *addr() const {
        return (ProxyOopDescriptor *) MemOopDescriptor::addr();
    }


public:
    // field offsets for code generation
    static std::int32_t pointer_byte_offset() {
        return ( 2 * OOP_SIZE ) - MEMOOP_TAG;
    }


    friend ProxyOop as_proxyOop( void *p );


    // sizing
    static std::int32_t header_size() {
        return sizeof( ProxyOopDescriptor ) / OOP_SIZE;
    }


    void *get_pointer() const {
        return addr()->_pointer;
    }


    void set_pointer( void *ptr ) {
        addr()->_pointer = ptr;
    }


    void null_pointer() {
        set_pointer( nullptr );
    }


    bool is_null() const {
        return get_pointer() == nullptr;
    }


    bool is_allOnes() const {
        return get_pointer() == (void *) -1;
    }


    small_int_t foreign_hash() const {
        return small_int_t( get_pointer() ) >> TAG_SIZE;
    }


    bool same_pointer_as( ProxyOop x ) const {
        return get_pointer() == x->get_pointer();
    }


    void bootstrap_object( Bootstrap *stream );

private:
    std::uint8_t *addr_at( std::int32_t offset ) const {
        return ( (std::uint8_t *) get_pointer() ) + offset;
    }


public:
    std::uint8_t byte_at( std::int32_t offset ) const {
        return *addr_at( offset );
    }


    void byte_at_put( std::int32_t offset, std::uint8_t c ) {
        *addr_at( offset ) = c;
    }


    std::uint16_t doubleByte_at( std::int32_t offset ) const {
        return *( (std::uint16_t *) addr_at( offset ) );
    }


    void doubleByte_at_put( std::int32_t offset, std::uint16_t db ) {
        *( (std::uint16_t *) addr_at( offset ) ) = db;
    }


    std::int32_t long_at( std::int32_t offset ) const {
        return *( (std::int32_t *) addr_at( offset ) );
    }


    void long_at_put( std::int32_t offset, std::int32_t l ) {
        *( (std::int32_t *) addr_at( offset ) ) = l;
    }


    float float_at( std::int32_t offset ) const {
        return *( (float *) addr_at( offset ) );
    }


    void float_at_put( std::int32_t offset, float f ) {
        *( (float *) addr_at( offset ) ) = f;
    }


    double double_at( std::int32_t offset ) const {
        return *( (double *) addr_at( offset ) );
    }


    void double_at_put( std::int32_t offset, double d ) {
        *( (double *) addr_at( offset ) ) = d;
    }


    friend class ProxyKlass;
};


inline ProxyOop as_proxyOop( void *p ) {
    return ProxyOop( as_memOop( p ) );
}
