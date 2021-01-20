//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/primitives/method_primitives.hpp"
#include "vm/memory/vmSymbols.hpp"
#include "vm/memory/oopFactory.hpp"
#include "vm/memory/Reflection.hpp"
#include "vm/oops/KlassOopDescriptor.hpp"
#include "vm/oops/DoubleOopDescriptor.hpp"
#include "vm/oops/ProxyOopDescriptor.hpp"
#include "vm/oops/ProcessOopDescriptor.hpp"
#include "vm/oops/ObjectArrayOopDescriptor.hpp"
#include "vm/utilities/Integer.hpp"
#include "vm/runtime/vmOperations.hpp"
#include "vm/code/StubRoutines.hpp"
#include "vm/primitives/primitives.hpp"
#include "vm/code/Zone.hpp"
#include "vm/code/InliningDatabase.hpp"
#include "vm/compiler/RecompilationScope.hpp"
#include "vm/interpreter/PrettyPrinter.hpp"
#include "vm/oops/MethodKlass.hpp"
#include "vm/oops/BlockClosureKlass.hpp"
#include "vm/runtime/ResourceMark.hpp"
#include "vm/oops/ContextOopDescriptor.hpp"
#include "vm/oops/ContextKlass.hpp"
#include "vm/memory/Scavenge.hpp"


TRACE_FUNC( TraceMethodPrims, "method" )


std::size_t methodOopPrimitives::number_of_calls;

#define ASSERT_RECEIVER st_assert(receiver->is_method(), "receiver must be method")


PRIM_DECL_1( methodOopPrimitives::selector, Oop receiver ) {
    PROLOGUE_1( "selector", receiver );
    ASSERT_RECEIVER;
    return MethodOop( receiver )->selector();
}


PRIM_DECL_1( methodOopPrimitives::debug_info, Oop receiver ) {
    PROLOGUE_1( "debug_info", receiver );
    ASSERT_RECEIVER;
    return MethodOop( receiver )->debugInfo();
}


PRIM_DECL_1( methodOopPrimitives::size_and_flags, Oop receiver ) {
    PROLOGUE_1( "size_and_flags", receiver );
    ASSERT_RECEIVER;
    return MethodOop( receiver )->size_and_flags();
}


PRIM_DECL_1( methodOopPrimitives::fileout_body, Oop receiver ) {
    PROLOGUE_1( "fileout_body", receiver );
    ASSERT_RECEIVER;
    return MethodOop( receiver )->fileout_body();
}


PRIM_DECL_2( methodOopPrimitives::setSelector, Oop receiver, Oop name ) {
    PROLOGUE_2( "setSelector", receiver, name );
    ASSERT_RECEIVER;
    if ( not name->is_symbol() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );
    MethodOop( receiver )->set_selector_or_method( Oop( name ) );
    return receiver;
}


PRIM_DECL_1( methodOopPrimitives::numberOfArguments, Oop receiver ) {
    PROLOGUE_1( "numberOfArguments", receiver );
    ASSERT_RECEIVER;
    return smiOopFromValue( MethodOop( receiver )->number_of_arguments() );
}


PRIM_DECL_1( methodOopPrimitives::outer, Oop receiver ) {
    PROLOGUE_1( "outer", receiver );
    ASSERT_RECEIVER;
    return MethodOop( receiver )->selector_or_method();
}


PRIM_DECL_2( methodOopPrimitives::setOuter, Oop receiver, Oop method ) {
    PROLOGUE_2( "setOuter", receiver, method );
    ASSERT_RECEIVER;
    MethodOop( receiver )->set_selector_or_method( Oop( method ) );
    return receiver;
}


PRIM_DECL_2( methodOopPrimitives::referenced_instance_variable_names, Oop receiver, Oop mixin ) {
    PROLOGUE_2( "referenced_instance_variable_names", receiver, mixin );
    ASSERT_RECEIVER;
    if ( not mixin->is_mixin() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );

    return MethodOop( receiver )->referenced_instance_variable_names( MixinOop( mixin ) );
}


PRIM_DECL_1( methodOopPrimitives::referenced_class_variable_names, Oop receiver ) {
    PROLOGUE_1( "referenced_class_variable_names", receiver );
    ASSERT_RECEIVER;
    return MethodOop( receiver )->referenced_class_variable_names();
}


PRIM_DECL_1( methodOopPrimitives::referenced_global_names, Oop receiver ) {
    PROLOGUE_1( "referenced_global_names", receiver );
    ASSERT_RECEIVER;
    return MethodOop( receiver )->referenced_global_names();
}


PRIM_DECL_1( methodOopPrimitives::senders, Oop receiver ) {
    PROLOGUE_1( "senders", receiver );
    ASSERT_RECEIVER;
    return MethodOop( receiver )->senders();
}


PRIM_DECL_2( methodOopPrimitives::prettyPrint, Oop receiver, Oop klass ) {
    PROLOGUE_2( "prettyPrint", receiver, klass );
    ASSERT_RECEIVER;
    if ( klass == nilObj ) {
        PrettyPrinter::print( MethodOop( receiver ) );
    } else {
        if ( not klass->is_klass() )
            return markSymbol( vmSymbols::first_argument_has_wrong_type() );
        PrettyPrinter::print( MethodOop( receiver ), KlassOop( klass ) );
    }
    return receiver;
}


PRIM_DECL_2( methodOopPrimitives::prettyPrintSource, Oop receiver, Oop klass ) {
    PROLOGUE_2( "prettyPrintSource", receiver, klass );
    ASSERT_RECEIVER;
    ByteArrayOop a;
    if ( klass == nilObj ) {
        a = PrettyPrinter::print_in_byteArray( MethodOop( receiver ) );
    } else {
        if ( not klass->is_klass() )
            return markSymbol( vmSymbols::first_argument_has_wrong_type() );
        a = PrettyPrinter::print_in_byteArray( MethodOop( receiver ), KlassOop( klass ) );
    }
    return a;
}


PRIM_DECL_1( methodOopPrimitives::printCodes, Oop receiver ) {
    PROLOGUE_1( "printCodes", receiver );
    ASSERT_RECEIVER;
    {
        ResourceMark resourceMark;
        MethodOop( receiver )->print_codes();
    }
    return receiver;
}


PRIM_DECL_6( methodOopPrimitives::constructMethod, Oop selector_or_method, Oop flags, Oop nofArgs, Oop debugInfo, Oop bytes, Oop oops ) {
    PROLOGUE_6( "constructMethod", selector_or_method, flags, nofArgs, debugInfo, bytes, oops );
    if ( not selector_or_method->is_symbol() and ( selector_or_method not_eq nilObj ) )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );
    if ( not flags->is_smi() )
        return markSymbol( vmSymbols::second_argument_has_wrong_type() );
    if ( not nofArgs->is_smi() )
        return markSymbol( vmSymbols::third_argument_has_wrong_type() );
    if ( not debugInfo->is_objArray() )
        return markSymbol( vmSymbols::fourth_argument_has_wrong_type() );
    if ( not bytes->is_byteArray() )
        return markSymbol( vmSymbols::fifth_argument_has_wrong_type() );
    if ( not oops->is_objArray() )
        return markSymbol( vmSymbols::sixth_argument_has_wrong_type() );

    if ( ObjectArrayOop( oops )->length() * oopSize not_eq ByteArrayOop( bytes )->length() ) {
        return markSymbol( vmSymbols::method_construction_failed() );
    }

    MethodKlass *k = (MethodKlass *) Universe::methodKlassObj()->klass_part();
    MethodOop result = k->constructMethod( selector_or_method, SMIOop( flags )->value(), SMIOop( nofArgs )->value(), ObjectArrayOop( debugInfo ), ByteArrayOop( bytes ), ObjectArrayOop( oops ) );
    if ( result )
        return result;
    return markSymbol( vmSymbols::method_construction_failed() );
}


extern "C" BlockClosureOop allocateBlock( SMIOop nof );


static Oop allocate_block_for( MethodOop method, Oop self ) {
    BlockScavenge bs;

    if ( not method->is_customized() ) {
        method->customize_for( self->klass(), self->blueprint()->mixin() );
    }

    // allocate the context for the block (make room for the self)
    ContextKlass *ok = (ContextKlass *) contextKlassObj->klass_part();
    ContextOop con = ContextOop( ok->allocateObjectSize( 1 ) );
    con->kill();
    con->obj_at_put( 0, self );

    // allocate the resulting block
    BlockClosureOop blk = allocateBlock( smiOopFromValue( method->number_of_arguments() ) );
    blk->set_method( method );
    blk->set_lexical_scope( con );

    return blk;
}


PRIM_DECL_1( methodOopPrimitives::allocate_block, Oop receiver ) {
    PROLOGUE_1( "allocate_block", receiver );
    ASSERT_RECEIVER;

    if ( not MethodOop( receiver )->is_blockMethod() )
        return markSymbol( vmSymbols::conversion_failed() );

    return allocate_block_for( MethodOop( receiver ), nilObj );
}


PRIM_DECL_2( methodOopPrimitives::allocate_block_self, Oop receiver, Oop self ) {
    PROLOGUE_2( "allocate_block_self", receiver, self );
    ASSERT_RECEIVER;

    if ( not MethodOop( receiver )->is_blockMethod() )
        return markSymbol( vmSymbols::conversion_failed() );

    return allocate_block_for( MethodOop( receiver ), self );
}


static SymbolOop symbol_from_method_inlining_info( MethodOopDescriptor::Method_Inlining_Info info ) {
    if ( info == MethodOopDescriptor::normal_inline )
        return oopFactory::new_symbol( "Normal" );
    if ( info == MethodOopDescriptor::never_inline )
        return oopFactory::new_symbol( "Never" );
    if ( info == MethodOopDescriptor::always_inline )
        return oopFactory::new_symbol( "Always" );
    ShouldNotReachHere();
    return nullptr;
}


PRIM_DECL_2( methodOopPrimitives::set_inlining_info, Oop receiver, Oop info ) {
    PROLOGUE_2( "set_inlining_info", receiver, info );
    ASSERT_RECEIVER;
    if ( not info->is_symbol() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );

    // Check argument value
    MethodOopDescriptor::Method_Inlining_Info in;
    if ( SymbolOop( info )->equals( "Never" ) ) {
        in = MethodOopDescriptor::never_inline;
    } else if ( SymbolOop( info )->equals( "Always" ) ) {
        in = MethodOopDescriptor::always_inline;
    } else if ( SymbolOop( info )->equals( "Normal" ) ) {
        in = MethodOopDescriptor::normal_inline;
    } else {
        return markSymbol( vmSymbols::argument_is_invalid() );
    }
    MethodOopDescriptor::Method_Inlining_Info old = MethodOop( receiver )->method_inlining_info();
    MethodOop( receiver )->set_method_inlining_info( in );
    return symbol_from_method_inlining_info( old );
}


PRIM_DECL_1( methodOopPrimitives::inlining_info, Oop receiver ) {
    PROLOGUE_1( "inlining_info", receiver );
    ASSERT_RECEIVER;
    return symbol_from_method_inlining_info( MethodOop( receiver )->method_inlining_info() );
}
