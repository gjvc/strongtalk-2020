//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/oops/OopDescriptor.hpp"
#include "vm/oops/MemOopDescriptor.hpp"
#include "vm/oops/smiKlass.hpp"
#include "vm/oops/VirtualFrameOopDescriptor.hpp"
#include "vm/runtime/VirtualFrame.hpp"
#include "vm/system/sizes.hpp"



// VirtualFrame objects are handles to process activations.

// memory layout:
//    [header      ]
//    [klass_field ]    in MemOopDescriptor
//    [process     ]    in this class
//    [index       ]    in this class
//    [timestamp   ]    in this class

class VirtualFrameOopDescriptor : public MemOopDescriptor {

    private:
        ProcessOop _process;
        SMIOop     _index;
        SMIOop     _time_stamp;

    protected:
        VirtualFrameOopDescriptor * addr() const {
            return ( VirtualFrameOopDescriptor * ) MemOopDescriptor::addr();
        }


    public:
        // accessors
        ProcessOop process() const {
            return addr()->_process;
        }


        void set_process( ProcessOop p ) {
            STORE_OOP( &addr()->_process, p );
        }


        int index() const {
            return addr()->_index->value();
        }


        void set_index( int i ) {
            STORE_OOP( &addr()->_index, smiOopFromValue( i ) );
        }


        int time_stamp() const {
            return addr()->_time_stamp->value();
        }


        void set_time_stamp( int t ) {
            STORE_OOP( &addr()->_time_stamp, smiOopFromValue( t ) );
        }


        friend VirtualFrameOop as_vframeOop( void * p );


        // sizing
        static int header_size() {
            return sizeof( VirtualFrameOopDescriptor ) / oopSize;
        }


        // get the corresponding VirtualFrame, returns nullptr it fails.
        VirtualFrame * get_vframe();

    private:
        friend class VirtualFrameKlass;
};


inline VirtualFrameOop as_vframeOop( void * p ) {
//    return VirtualFrameOop( as_memOop( p ) );
    return static_cast<VirtualFrameOop>( as_memOop( p ));
}


