//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/runtime/StackChunkBuilder.hpp"
#include "vm/utilities/GrowableArray.hpp"
#include "vm/oops/MethodOopDescriptor.hpp"
#include "vm/oops/SMIOopDescriptor.hpp"
#include "vm/oops/ObjectArrayOopDescriptor.hpp"
#include "vm/runtime/Processes.hpp"
#include "vm/runtime/VirtualFrame.hpp"
#include "vm/memory/OopFactory.hpp"
#include "vm/oops/ContextOopDescriptor.hpp"


GrowableArray<const CompiledVirtualFrame *> *frames;
GrowableArray<ContextOop>                   *contexts;

bool StackChunkBuilder::_is_deoptimizing = false;
std::int32_t *StackChunkBuilder::_framePointer = nullptr;


StackChunkBuilder::StackChunkBuilder( std::int32_t *fp, std::int32_t size ) :
    _virtualFrameCount{ 0 },
    _localExpressionCount{ 0 },
    array{ new GrowableArray<Oop>( size ) } {
    _framePointer = fp; // static
}


StackChunkBuilder::~StackChunkBuilder() {
}


void StackChunkBuilder::append( DeltaVirtualFrame *f ) {
    MethodOop          method;
    std::int32_t       number_of_temps;
    GrowableArray<Oop> *stack;
    {
        //FlagSetting fl(TraceCanonicalContext, false);
        _virtualFrameCount++;

        // Append the frame information to the array
        /*methodOop */method = f->method();
        array->push( f->receiver() );
        array->push( method );
        array->push( smiOopFromValue( f->byteCodeIndex() ) );

        // push locals
        /*std::int32_t*/ number_of_temps = method->number_of_stack_temporaries();
        /*GrowableArray<Oop>* */ stack   = f->expression_stack();

        // push number of locals
        std::int32_t locals = number_of_temps + stack->length();
        array->push( smiOopFromValue( locals ) );
        _localExpressionCount += locals;
    }

    // push context and temporaries
    // if a context is present store the canonical form as temporary 0.
    ContextOop con = f->canonical_context();
    if ( con ) {
        array->push( con );
        method->verify_context( con );

        // If we have pending nlrs (suspended in unwind protect) we have to update
        // the nlr targets if we match deoptimized frames
        if ( not method->is_blockMethod() ) {
            con->set_home_fp( _framePointer );
            if ( f->is_compiled_frame() ) {
                Processes::update_nlr_targets( (CompiledVirtualFrame *) f, con );
            }
        }
    }

    for ( std::int32_t i = con ? 1 : 0; i < number_of_temps; i++ )
        array->push( f->temp_at( i ) );

    // push expression stack
    for ( std::int32_t i = stack->length() - 1; i >= 0; i-- ) {
        array->push( stack->at( i ) );
    }
}


ObjectArrayOop StackChunkBuilder::as_objectArray() {
    std::int32_t length = header_size() + array->length();

    ObjectArrayOop result = OopFactory::new_objectArray( length );
    result->obj_at_put( 1, smiOopFromValue( _virtualFrameCount ) );
    result->obj_at_put( 2, smiOopFromValue( _localExpressionCount ) );

    for ( std::int32_t i = 0; i < array->length(); i++ )
        result->obj_at_put( i + header_size() + 1, array->at( i ) );

    return result;
}


void StackChunkBuilder::context_at_put( const CompiledVirtualFrame *frame, ContextOop con ) {
    // Returns if no StackChunkBuilder is in use
    if ( not is_deoptimizing() ) {
        con->kill();
        return;
    }

    for ( std::int32_t i = 0; i < frames->length(); i++ ) {
        st_assert( not frames->at( i )->equal( frame ), "should not be present" );
    }

    con->set_home_fp( _framePointer );
    frames->push( frame );
    contexts->push( con );
}


ContextOop StackChunkBuilder::context_at( const CompiledVirtualFrame *frame ) {
    // Returns if no StackChunkBuilder is in use
    if ( not is_deoptimizing() )
        return nullptr;
    if ( not frame )
        return nullptr;

    // See if it's stored
    for ( std::int32_t i = 0; i < frames->length(); i++ ) {
        if ( frames->at( i )->equal( frame ) ) {
            return contexts->at( i );
        }
    }
    return nullptr;
}


void StackChunkBuilder::begin_deoptimization() {
    st_assert( not is_deoptimizing(), "just checking" );
    _is_deoptimizing = true;
    frames           = new GrowableArray<const CompiledVirtualFrame *>( 100 );
    contexts         = new GrowableArray<ContextOop>( 100 );
}


void StackChunkBuilder::end_deoptimization() {
    st_assert( is_deoptimizing(), "just checking" );
    _is_deoptimizing = false;
    frames           = nullptr;
    contexts         = nullptr;

}
