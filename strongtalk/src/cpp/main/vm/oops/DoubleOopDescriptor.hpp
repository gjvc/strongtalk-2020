//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/oops/MemOopDescriptor.hpp"



// Layout format:
//   [Header]
//   [Klass ]
//   [Double]   (contains untagged values)

class DoubleOopDescriptor : public MemOopDescriptor {
private:

    double _value;


    // conversion to untagged doubleOopDescriptor*
    DoubleOopDescriptor *addr() const {
        return (DoubleOopDescriptor *) MemOopDescriptor::addr();
    }


public:
    // type conversion
    friend DoubleOop as_doubleOop( void *p );


    // sizing
    static std::int32_t header_size() {
        return sizeof( DoubleOopDescriptor ) / OOP_SIZE;
    }


    static std::int32_t object_size() {
        return header_size();
    }


    static std::int32_t value_offset() {
        return sizeof( MemOopDescriptor ) / OOP_SIZE;
    }


    // value accessors
    double value() const {
        return addr()->_value;
    }


    void set_value( double v ) {
        addr()->_value = v;
    }


    // bootstrappingInProgress
    void bootstrap_object( Bootstrap *stream );

    friend class DoubleKlass;
};


inline DoubleOop as_doubleOop( void *p ) {
    return DoubleOop( as_memOop( p ) );
}
