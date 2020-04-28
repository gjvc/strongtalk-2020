//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once


#include "vm/system/platform.hpp"
#include "vm/memory/allocation.hpp"
#include "vm/oops/MemOopDescriptor.hpp"
#include "vm/utilities/GrowableArray.hpp"
#include "ResourceObject.hpp"



// The StackChunkBuilder helps generate the canonical form of vframes.
// Used for:
//  - stack deoptimization.
//  - save stacks in heap.

class DeltaVirtualFrame;
class CompiledVirtualFrame;

class StackChunkBuilder : public ResourceObject {

    private:
        // These numbers enable calculation of the corresponding deoptimized interpreter stack.
        int        _virtualFrameCount;      // Number of VirtualFrame collected
        int        _localExpressionCount;   // Sum of all temporaries and expressions in collected VirtualFrame
        static int * _framePointer;          // Frame pointer of the resulting frame

        static int header_size() {
            return 2;
        }


        static bool_t _is_deoptimizing;

        GrowableArray <Oop> * array;

    public:
        StackChunkBuilder( int * fp, int size = 100 );

        ~StackChunkBuilder();

        void append( DeltaVirtualFrame * f );

        // Returns the packed frames as an object array.
        ObjectArrayOop as_objArray();

        // Constants for the resulting object array
        enum {
            number_of_vframes_index = 1, //
            number_of_locals_index  = 2, //
            first_frame_index       = 3, //
        };

        static void begin_deoptimization();

        static void end_deoptimization();


        static bool_t is_deoptimizing() {
            return _is_deoptimizing;
        }


        // The context cache
        static void context_at_put( const CompiledVirtualFrame * frame, ContextOop con );

        static ContextOop context_at( const CompiledVirtualFrame * frame );
};

