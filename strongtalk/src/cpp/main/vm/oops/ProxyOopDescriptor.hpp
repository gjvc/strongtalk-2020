//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/oops/MemOopDescriptor.hpp"
#include "vm/runtime/Process.hpp"
#include "vm/system/sizes.hpp"




// proxy objects are handles to external data.

// memory layout:
//    [header      ]
//    [klass_field ]
//    [pointer     ]

const int pointer_no     = sizeof( MemOopDescriptor ) / oopSize;
const int pointer_offset = sizeof( MemOopDescriptor ) - MEMOOP_TAG;

class ProxyOopDescriptor : public MemOopDescriptor {

    private:
        void * _pointer;

    protected:
        ProxyOopDescriptor * addr() const {
            return ( ProxyOopDescriptor * ) MemOopDescriptor::addr();
        }


    public:
        // field offsets for code generation
        static int pointer_byte_offset() {
            return ( 2 * oopSize ) - MEMOOP_TAG;
        }


        friend ProxyOop as_proxyOop( void * p );


        // sizing
        static int header_size() {
            return sizeof( ProxyOopDescriptor ) / oopSize;
        }


        void * get_pointer() const {
            return addr()->_pointer;
        }


        void set_pointer( void * ptr ) {
            addr()->_pointer = ptr;
        }


        void null_pointer() {
            set_pointer( nullptr );
        }


        bool_t is_null() const {
            return get_pointer() == nullptr;
        }


        bool_t is_allOnes() const {
            return get_pointer() == ( void * ) -1;
        }


        smi_t foreign_hash() const {
            return smi_t( get_pointer() ) >> TAG_SIZE;
        }


        bool_t same_pointer_as( ProxyOop x ) const {
            return get_pointer() == x->get_pointer();
        }


        void bootstrap_object( Bootstrap * stream );

    private:
        std::uint8_t * addr_at( int offset ) const {
            return ( ( std::uint8_t * ) get_pointer() ) + offset;
        }


    public:
        std::uint8_t byte_at( int offset ) const {
            return *addr_at( offset );
        }


        void byte_at_put( int offset, std::uint8_t c ) {
            *addr_at( offset ) = c;
        }


        std::uint16_t doubleByte_at( int offset ) const {
            return *( ( std::uint16_t * ) addr_at( offset ) );
        }


        void doubleByte_at_put( int offset, std::uint16_t db ) {
            *( ( std::uint16_t * ) addr_at( offset ) ) = db;
        }


        std::int32_t long_at( int offset ) const {
            return *( ( std::int32_t * ) addr_at( offset ) );
        }


        void long_at_put( int offset, std::int32_t l ) {
            *( ( std::int32_t * ) addr_at( offset ) ) = l;
        }


        float float_at( int offset ) const {
            return *( ( float * ) addr_at( offset ) );
        }


        void float_at_put( int offset, float f ) {
            *( ( float * ) addr_at( offset ) ) = f;
        }


        double double_at( int offset ) const {
            return *( ( double * ) addr_at( offset ) );
        }


        void double_at_put( int offset, double d ) {
            *( ( double * ) addr_at( offset ) ) = d;
        }


        friend class ProxyKlass;
};


inline ProxyOop as_proxyOop( void * p ) {
    return ProxyOop( as_memOop( p ) );
}
