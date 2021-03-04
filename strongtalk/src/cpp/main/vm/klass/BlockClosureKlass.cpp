//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/klass/BlockClosureKlass.hpp"
#include "vm/memory/Universe.hpp"
#include "vm/oop/BlockClosureOopDescriptor.hpp"
#include "vm/memory/MarkSweep.hpp"
#include "vm/runtime/flags.hpp"
#include "vm/oop/MethodOopDescriptor.hpp"
#include "vm/oop/SymbolOopDescriptor.hpp"
#include "vm/oop/KlassOopDescriptor.hpp"
#include "vm/oop/ContextOopDescriptor.hpp"
#include "vm/utility/ConsoleOutputStream.hpp"


KlassOop BlockClosureKlass::blockKlassFor( std::int32_t numberOfArguments ) {
    switch ( numberOfArguments ) {
        case 0:
            return Universe::zeroArgumentBlockKlassObject();
            break;
        case 1:
            return Universe::oneArgumentBlockKlassObject();
            break;
        case 2:
            return Universe::twoArgumentBlockKlassObject();
            break;
        case 3:
            return Universe::threeArgumentBlockKlassObject();
            break;
        case 4:
            return Universe::fourArgumentBlockKlassObject();
            break;
        case 5:
            return Universe::fiveArgumentBlockKlassObject();
            break;
        case 6:
            return Universe::sixArgumentBlockKlassObject();
            break;
        case 7:
            return Universe::sevenArgumentBlockKlassObject();
            break;
        case 8:
            return Universe::eightArgumentBlockKlassObject();
            break;
        case 9:
            return Universe::nineArgumentBlockKlassObject();
            break;
    }
    st_fatal( "cannot handle block with more than 9 arguments" );
    return nullptr;
}


bool BlockClosureKlass::oop_verify( Oop obj ) {
    bool flag = Klass::oop_verify( obj );
    // FIX LATER
    return flag;
}


void setKlassVirtualTableFromBlockClosureKlass( Klass *k ) {
    BlockClosureKlass o;
    k->set_vtbl_value( o.vtbl_value() );
}


Oop BlockClosureKlass::allocateObject( bool permit_scavenge, bool tenured ) {
    KlassOop k = as_klassOop();

    // allocate
    Oop *result = basicAllocate( BlockClosureOopDescriptor::object_size(), &k, permit_scavenge, tenured );
    if ( result == nullptr )
        return nullptr;

    BlockClosureOop obj = as_blockClosureOop( result );
    // header
    obj->initialize_header( false, k );
    // %not initialized by the interpreter
    // obj->addr()->_method         = (methodOop)      smiOop_zero;
    // obj->addr()->_lexical_scope  = (heapContextOop) smiOop_zero;
    return obj;
}


KlassOop BlockClosureKlass::create_subclass( MixinOop mixin, Format format ) {
    static_cast<void>(mixin); // unused
    static_cast<void>(format); // unused
    return nullptr;
}


std::int32_t BlockClosureKlass::oop_scavenge_contents( Oop obj ) {
    // header
    MemOop( obj )->scavenge_header();
    // %note _method can be ignored since methods are tenured
    scavenge_oop( (Oop *) &BlockClosureOop( obj )->addr()->_lexical_scope );
    return BlockClosureOopDescriptor::object_size();
}


std::int32_t BlockClosureKlass::oop_scavenge_tenured_contents( Oop obj ) {
    // header
    MemOop( obj )->scavenge_tenured_header();
    // %note _method can be ignored since methods are tenured
    scavenge_tenured_oop( (Oop *) &BlockClosureOop( obj )->addr()->_lexical_scope );
    return BlockClosureOopDescriptor::object_size();
}


void BlockClosureKlass::oop_follow_contents( Oop obj ) {
    // header
    MemOop( obj )->follow_header();
    MarkSweep::reverse_and_push( (Oop *) &BlockClosureOop( obj )->addr()->_methodOrJumpAddr );
    MarkSweep::reverse_and_push( (Oop *) &BlockClosureOop( obj )->addr()->_lexical_scope );
}


void BlockClosureKlass::oop_layout_iterate( Oop obj, ObjectLayoutClosure *blk ) {
    // header
    MemOop( obj )->layout_iterate_header( blk );
    blk->do_oop( "method", (Oop *) &BlockClosureOop( obj )->addr()->_methodOrJumpAddr );
    blk->do_oop( "scope", (Oop *) &BlockClosureOop( obj )->addr()->_lexical_scope );
}


void BlockClosureKlass::oop_oop_iterate( Oop obj, OopClosure *blk ) {
    // header
    MemOop( obj )->oop_iterate_header( blk );
    blk->do_oop( (Oop *) &BlockClosureOop( obj )->addr()->_methodOrJumpAddr );
    blk->do_oop( (Oop *) &BlockClosureOop( obj )->addr()->_lexical_scope );
}


std::int32_t BlockClosureKlass::number_of_arguments() const {
    KlassOop k = KlassOop( this );        // C++ bogosity alert
    if ( k == Universe::zeroArgumentBlockKlassObject() )
        return 0;
    if ( k == Universe::oneArgumentBlockKlassObject() )
        return 1;
    if ( k == Universe::twoArgumentBlockKlassObject() )
        return 2;
    if ( k == Universe::threeArgumentBlockKlassObject() )
        return 3;
    if ( k == Universe::fourArgumentBlockKlassObject() )
        return 4;
    if ( k == Universe::fiveArgumentBlockKlassObject() )
        return 5;
    if ( k == Universe::sixArgumentBlockKlassObject() )
        return 6;
    if ( k == Universe::sevenArgumentBlockKlassObject() )
        return 7;
    if ( k == Universe::eightArgumentBlockKlassObject() )
        return 8;
    if ( k == Universe::nineArgumentBlockKlassObject() )
        return 9;
    st_fatal( "unknown block closure class" );
    return 0;
}


void BlockClosureKlass::oop_print_value_on( Oop obj, ConsoleOutputStream *stream ) {
    if ( PrintObjectID ) {
        MemOop( obj )->print_id_on( stream );
        stream->print( "-" );
    }
    stream->print( "[] in " );
    MethodOop method = BlockClosureOop( obj )->method();
    method->home()->selector()->print_symbol_on( stream );
    stream->print( "(scope = " );
    BlockClosureOop( obj )->lexical_scope()->print_value_on( stream );
    stream->print( ")" );
}
