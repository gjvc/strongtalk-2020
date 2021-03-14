//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/platform/platform.hpp"
#include "vm/oop/OopDescriptor.hpp"
#include "vm/oop/MemOopDescriptor.hpp"
#include "vm/klass/SmallIntegerKlass.hpp"
#include "vm/oop/VirtualFrameOopDescriptor.hpp"
#include "vm/runtime/VirtualFrame.hpp"


// VirtualFrame objects are handles to process activations.

// memory layout:
//    [header      ]
//    [klass_field ]    in MemOopDescriptor
//    [process     ]    in this class
//    [index       ]    in this class
//    [timestamp   ]    in this class

class VirtualFrameOopDescriptor : public MemOopDescriptor {

private:
    ProcessOop      _process;
    SmallIntegerOop _index;
    SmallIntegerOop _time_stamp;

protected:
    VirtualFrameOopDescriptor *addr() const {
        return (VirtualFrameOopDescriptor *) MemOopDescriptor::addr();
    }


public:
    // accessors
    ProcessOop process() const {
        return addr()->_process;
    }


    void set_process( ProcessOop p ) {
        STORE_OOP( &addr()->_process, p );
    }


    std::int32_t index() const {
        return addr()->_index->value();
    }


    void set_index( std::int32_t i ) {
        STORE_OOP( &addr()->_index, smiOopFromValue( i ) );
    }


    std::int32_t time_stamp() const {
        return addr()->_time_stamp->value();
    }


    void set_time_stamp( std::int32_t t ) {
        STORE_OOP( &addr()->_time_stamp, smiOopFromValue( t ) );
    }


    friend VirtualFrameOop as_vframeOop( void *p );


    // sizing
    static std::int32_t header_size() {
        return sizeof( VirtualFrameOopDescriptor ) / OOP_SIZE;
    }


    // get the corresponding VirtualFrame, returns nullptr it fails.
    VirtualFrame *get_vframe();

private:
    friend class VirtualFrameKlass;
};


inline VirtualFrameOop as_vframeOop( void *p ) {
//    return VirtualFrameOop( as_memOop( p ) );
    return static_cast<VirtualFrameOop>( as_memOop( p ));
}
