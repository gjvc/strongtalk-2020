//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/system/platform.hpp"
#include "vm/utilities/ObjectIDTable.hpp"
#include "vm/primitives/primitive_declarations.hpp"
#include "vm/primitives/primitive_tracing.hpp"
#include "vm/runtime/flags.hpp"
#include "vm/memory/vmSymbols.hpp"
#include "vm/memory/SymbolTable.hpp"
#include "vm/interpreter/ByteCodes.hpp"
#include "vm/lookup/LookupKey.hpp"
#include "vm/oops/MethodOopDescriptor.hpp"
#include "vm/oops/MixinOopDescriptor.hpp"
#include "vm/oops/ProcessOopDescriptor.hpp"
#include "vm/memory/oopFactory.hpp"
#include "vm/interpreter/PrettyPrinter.hpp"
#include "vm/code/NativeMethod.hpp"
#include "vm/runtime/VirtualFrame.hpp"
#include "vm/primitives/vframe_primitives.hpp"
#include "vm/runtime/TempDecoder.hpp"
#include "vm/oops/VirtualFrameOopDescriptor.hpp"
#include "vm/runtime/ResourceMark.hpp"
#include "vm/memory/Scavenge.hpp"


TRACE_FUNC( TraceVirtualFramePrims, "VirtualFrame" )


std::int32_t VirtualFrameOopPrimitives::number_of_calls;

#define ASSERT_RECEIVER st_assert(receiver->is_vframe(), "receiver must be VirtualFrame")


PRIM_DECL_1( VirtualFrameOopPrimitives::process, Oop receiver ) {
    PROLOGUE_1( "process", receiver );
    ASSERT_RECEIVER;
    return VirtualFrameOop( receiver )->process();
}


PRIM_DECL_1( VirtualFrameOopPrimitives::index, Oop receiver ) {
    PROLOGUE_1( "index", receiver );
    ASSERT_RECEIVER;
    return smiOopFromValue( VirtualFrameOop( receiver )->index() );
}


PRIM_DECL_1( VirtualFrameOopPrimitives::time_stamp, Oop receiver ) {
    PROLOGUE_1( "time_stamp", receiver );
    ASSERT_RECEIVER;
    return smiOopFromValue( VirtualFrameOop( receiver )->time_stamp() );
}


PRIM_DECL_1( VirtualFrameOopPrimitives::is_smalltalk_activation, Oop receiver ) {
    PROLOGUE_1( "is_smalltalk_activation", receiver );
    ASSERT_RECEIVER;

    ResourceMark resourceMark;

    VirtualFrame *vf = VirtualFrameOop( receiver )->get_vframe();

    if ( vf == nullptr )
        return markSymbol( vmSymbols::activation_is_invalid() );

    return vf->is_delta_frame() ? trueObject : falseObject;
}


PRIM_DECL_1( VirtualFrameOopPrimitives::byte_code_index, Oop receiver ) {
    PROLOGUE_1( "byte_code_index", receiver );
    ASSERT_RECEIVER;

    ResourceMark resourceMark;

    VirtualFrame *vf = VirtualFrameOop( receiver )->get_vframe();

    if ( vf == nullptr )
        return markSymbol( vmSymbols::activation_is_invalid() );

    if ( not vf->is_delta_frame() )
        return markSymbol( vmSymbols::external_activation() );

    return smiOopFromValue( ( (DeltaVirtualFrame *) vf )->byteCodeIndex() );
}


PRIM_DECL_1( VirtualFrameOopPrimitives::expression_stack, Oop receiver ) {
    PROLOGUE_1( "expression_stack", receiver );
    ASSERT_RECEIVER;

    ResourceMark resourceMark;

    VirtualFrame *vf = VirtualFrameOop( receiver )->get_vframe();

    if ( vf == nullptr )
        return markSymbol( vmSymbols::activation_is_invalid() );

    if ( not vf->is_delta_frame() )
        return markSymbol( vmSymbols::external_activation() );

    BlockScavenge bs;

    GrowableArray<Oop> *stack = ( (DeltaVirtualFrame *) vf )->expression_stack();

    return oopFactory::new_objArray( stack );
}


PRIM_DECL_1( VirtualFrameOopPrimitives::method, Oop receiver ) {
    PROLOGUE_1( "method", receiver );
    ASSERT_RECEIVER;

    ResourceMark resourceMark;

    VirtualFrame *vf = VirtualFrameOop( receiver )->get_vframe();

    if ( vf == nullptr )
        return markSymbol( vmSymbols::activation_is_invalid() );

    if ( not vf->is_delta_frame() )
        return markSymbol( vmSymbols::external_activation() );

    return ( (DeltaVirtualFrame *) vf )->method();
}


PRIM_DECL_1( VirtualFrameOopPrimitives::receiver, Oop receiver ) {
    PROLOGUE_1( "receiver", receiver );

    st_assert( receiver->is_vframe(), "receiver must be VirtualFrame" )

    ResourceMark resourceMark;

    VirtualFrame *vf = VirtualFrameOop( receiver )->get_vframe();

    if ( vf == nullptr )
        return markSymbol( vmSymbols::activation_is_invalid() );

    if ( not vf->is_delta_frame() )
        return markSymbol( vmSymbols::external_activation() );

    return ( (DeltaVirtualFrame *) vf )->receiver();
}


PRIM_DECL_1( VirtualFrameOopPrimitives::temporaries, Oop receiver ) {
    PROLOGUE_1( "temporaries", receiver );

    st_assert( receiver->is_vframe(), "receiver must be VirtualFrame" );

    BlockScavenge bs;
    ResourceMark  rm;

    VirtualFrame *vf = VirtualFrameOop( receiver )->get_vframe();

    if ( vf == nullptr )
        return markSymbol( vmSymbols::activation_is_invalid() );

    if ( not vf->is_delta_frame() )
        return markSymbol( vmSymbols::external_activation() );

    DeltaVirtualFrame  *df       = (DeltaVirtualFrame *) vf;
    GrowableArray<Oop> *temps    = new GrowableArray<Oop>( 10 );
    MethodOop          method    = df->method();
    std::int32_t       tempCount = method->number_of_stack_temporaries();

    for ( std::int32_t offset = ( method->activation_has_context() ? 1 : 0 ); offset < tempCount; offset++ ) {
        ByteArrayOop name = find_stack_temp( method, df->byteCodeIndex(), offset );
        if ( name )
            temps->append( oopFactory::new_association( oopFactory::new_symbol( name ), df->temp_at( offset ), false ) );
    }

    while ( df ) {
        if ( method->allocatesInterpretedContext() ) {
            std::int32_t contextTempCount = method->number_of_context_temporaries();

            for ( std::int32_t offset = 0; offset < contextTempCount; offset++ ) {
                ByteArrayOop name = find_heap_temp( method, df->byteCodeIndex(), offset );
                if ( name )
                    temps->append( oopFactory::new_association( oopFactory::new_symbol( name ), df->context_temp_at( offset ), false ) );
            }
        }

        df         = df->parent();
        if ( df )
            method = df->method();
    }

    return oopFactory::new_objArray( temps );
}


PRIM_DECL_1( VirtualFrameOopPrimitives::arguments, Oop receiver ) {
    PROLOGUE_1( "arguments", receiver );
    ASSERT_RECEIVER;

    ResourceMark resourceMark;

    VirtualFrame *vf = VirtualFrameOop( receiver )->get_vframe();

    if ( vf == nullptr )
        return markSymbol( vmSymbols::activation_is_invalid() );

    if ( not vf->is_delta_frame() )
        return markSymbol( vmSymbols::external_activation() );

    BlockScavenge bs;

    GrowableArray<Oop> *stack = ( (DeltaVirtualFrame *) vf )->arguments();

    return oopFactory::new_objArray( stack );
}


class vframeStream : public ByteArrayPrettyPrintStream {
    void begin_highlight() {
        set_highlight( true );
        print_char( 27 );
    }


    void end_highlight() {
        set_highlight( false );
        print_char( 27 );
    }
};


PRIM_DECL_1( VirtualFrameOopPrimitives::pretty_print, Oop receiver ) {
    PROLOGUE_1( "receiver", receiver );
    ASSERT_RECEIVER;

    ResourceMark  rm;
    BlockScavenge bs;

    VirtualFrame *vf = VirtualFrameOop( receiver )->get_vframe();

    if ( vf == nullptr )
        return markSymbol( vmSymbols::activation_is_invalid() );

    if ( not vf->is_delta_frame() )
        return markSymbol( vmSymbols::external_activation() );

    ByteArrayPrettyPrintStream *stream = new vframeStream;
    PrettyPrinter::print_body( (DeltaVirtualFrame *) vf, stream );

    return stream->asByteArray();
}


class DeoptimizeProcess : public FrameClosure {
private:
    DeltaProcess *theProcess;
public:
    void begin_process( Process *process ) {
        if ( process->is_deltaProcess() )
            theProcess = (DeltaProcess *) process;
        else
            theProcess = nullptr;
    }


    void end_process( Process *process ) {
        theProcess = nullptr;
    }


    void do_frame( Frame *fr ) {
        if ( theProcess and fr->is_compiled_frame() )
            theProcess->deoptimize_stretch( fr, fr );
    }
};


void deoptimize( DeltaProcess *process ) {
    ResourceMark      rm;
    DeoptimizeProcess op;
    process->frame_iterate( &op );
}


PRIM_DECL_1( VirtualFrameOopPrimitives::single_step, Oop activation ) {
    PROLOGUE_1( "single_step", activation );

    ProcessOop process = VirtualFrameOop( activation )->process();

    // Check if argument is a processOop
    if ( not process->is_process() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );

    // Make sure process is not dead
    if ( not ProcessOop( process )->is_live() )
        return markSymbol( vmSymbols::dead() );

    DeltaProcess *proc = ProcessOop( process )->process();
    deoptimize( proc );
    proc->setupSingleStep();

    return process;
}


PRIM_DECL_1( VirtualFrameOopPrimitives::step_next, Oop activation ) {
    PROLOGUE_1( "step_next", activation );

    ResourceMark resourceMark;
    if ( VirtualFrameOop( activation )->get_vframe() == nullptr )
        return markSymbol( vmSymbols::activation_is_invalid() );

    ProcessOop process = VirtualFrameOop( activation )->process();

    // Check if argument is a processOop
    if ( not process->is_process() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );

    // Make sure process is not dead
    if ( not ProcessOop( process )->is_live() )
        return markSymbol( vmSymbols::dead() );

    DeltaProcess *proc = ProcessOop( process )->process();
    deoptimize( proc );

    VirtualFrame *vf = VirtualFrameOop( activation )->get_vframe();

    proc->setupStepNext( vf->fr().fp() );

    return process;
}


PRIM_DECL_1( VirtualFrameOopPrimitives::step_return, Oop activation ) {
    PROLOGUE_1( "step_return", activation );

    ResourceMark resourceMark;
    if ( VirtualFrameOop( activation )->get_vframe() == nullptr )
        return markSymbol( vmSymbols::activation_is_invalid() );

    ProcessOop process = VirtualFrameOop( activation )->process();

    // Check if argument is a processOop
    if ( not process->is_process() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );

    // Make sure process is not dead
    if ( not ProcessOop( process )->is_live() )
        return markSymbol( vmSymbols::dead() );

    {
        HandleMark hm;
        Handle     activationHandle( activation );

        DeltaProcess *proc = ProcessOop( process )->process();
        deoptimize( proc );

        VirtualFrame *vf           = VirtualFrameOop( activationHandle.as_oop() )->get_vframe();
        std::int32_t *framePointer = vf->fr().fp();

        proc->setupStepReturn( framePointer );

        return activationHandle.as_oop();
    }
}
