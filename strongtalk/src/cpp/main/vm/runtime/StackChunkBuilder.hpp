//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once


#include "vm/memory/allocation.hpp"
#include "vm/oops/MemOopDescriptor.hpp"
#include "vm/utilities/GrowableArray.hpp"
#include "vm/runtime/ResourceObject.hpp"



// The StackChunkBuilder helps generate the canonical form of vframes.
// Used for:
//  - stack deoptimization.
//  - save stacks in heap.

class DeltaVirtualFrame;

class CompiledVirtualFrame;

class StackChunkBuilder : public ResourceObject {

private:
    // These numbers enable calculation of the corresponding deoptimized interpreter stack.
    std::int32_t        _virtualFrameCount;      // Number of VirtualFrame collected
    std::int32_t        _localExpressionCount;   // Sum of all temporaries and expressions in collected VirtualFrame
    static std::int32_t *_framePointer;          // Frame pointer of the resulting frame

    static std::int32_t header_size() {
        return 2;
    }


    static bool _is_deoptimizing;

    GrowableArray<Oop> *array;

public:
    StackChunkBuilder() = default;
    StackChunkBuilder( std::int32_t *fp, std::int32_t size = 100 );
    virtual ~StackChunkBuilder();
    StackChunkBuilder( const StackChunkBuilder & ) = default;
    StackChunkBuilder &operator=( const StackChunkBuilder & ) = default;
    void operator delete( void *ptr ) { (void)(ptr); }

    void append( DeltaVirtualFrame *f );

    // Returns the packed frames as an object array.
    ObjectArrayOop as_objectArray();

    // Constants for the resulting object array
    enum {
        number_of_vframes_index = 1, //
        number_of_locals_index  = 2, //
        first_frame_index       = 3, //
    };

    static void begin_deoptimization();

    static void end_deoptimization();


    static bool is_deoptimizing() {
        return _is_deoptimizing;
    }


    // The context cache
    static void context_at_put( const CompiledVirtualFrame *frame, ContextOop con );

    static ContextOop context_at( const CompiledVirtualFrame *frame );
};
