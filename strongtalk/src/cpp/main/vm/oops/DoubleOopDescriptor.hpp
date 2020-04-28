//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/oops/MemOopDescriptor.hpp"
#include "vm/system/sizes.hpp"



// Layout format:
//   [Header]
//   [Klass ]
//   [Double]   (contains untagged values)

class DoubleOopDescriptor : public MemOopDescriptor {
    private:

        double _value;


        // conversion to untagged doubleOopDescriptor*
        DoubleOopDescriptor * addr() const {
            return ( DoubleOopDescriptor * ) MemOopDescriptor::addr();
        }


    public:
        // type conversion
        friend DoubleOop as_doubleOop( void * p );


        // sizing
        static int header_size() {
            return sizeof( DoubleOopDescriptor ) / oopSize;
        }


        static int object_size() {
            return header_size();
        }


        static int value_offset() {
            return sizeof( MemOopDescriptor ) / oopSize;
        }


        // value accessors
        double value() const {
            return addr()->_value;
        }


        void set_value( double v ) {
            addr()->_value = v;
        }


        // bootstrappingInProgress
        void bootstrap_object( Bootstrap * stream );

        friend class DoubleKlass;
};


inline DoubleOop as_doubleOop( void * p ) {
    return DoubleOop( as_memOop( p ) );
}


