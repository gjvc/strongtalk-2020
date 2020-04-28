//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/oops/BlockClosureKlass.hpp"
#include "vm/memory/Universe.hpp"
#include "vm/oops/BlockClosureOopDescriptor.hpp"
#include "vm/memory/MarkSweep.hpp"
#include "vm/runtime/flags.hpp"
#include "vm/oops/MethodOopDescriptor.hpp"
#include "vm/oops/SymbolOopDescriptor.hpp"
#include "vm/oops/KlassOopDescriptor.hpp"
#include "vm/oops/ContextOopDescriptor.hpp"


KlassOop BlockClosureKlass::blockKlassFor( int numberOfArguments ) {
    switch ( numberOfArguments ) {
        case 0:
            return Universe::zeroArgumentBlockKlassObj();
            break;
        case 1:
            return Universe::oneArgumentBlockKlassObj();
            break;
        case 2:
            return Universe::twoArgumentBlockKlassObj();
            break;
        case 3:
            return Universe::threeArgumentBlockKlassObj();
            break;
        case 4:
            return Universe::fourArgumentBlockKlassObj();
            break;
        case 5:
            return Universe::fiveArgumentBlockKlassObj();
            break;
        case 6:
            return Universe::sixArgumentBlockKlassObj();
            break;
        case 7:
            return Universe::sevenArgumentBlockKlassObj();
            break;
        case 8:
            return Universe::eightArgumentBlockKlassObj();
            break;
        case 9:
            return Universe::nineArgumentBlockKlassObj();
            break;
    }
    fatal( "cannot handle block with more than 9 arguments" );
    return nullptr;
}


bool_t BlockClosureKlass::oop_verify( Oop obj ) {
    bool_t flag = Klass::oop_verify( obj );
    // FIX LATER
    return flag;
}


void setKlassVirtualTableFromBlockClosureKlass( Klass * k ) {
    BlockClosureKlass o;
    k->set_vtbl_value( o.vtbl_value() );
}


Oop BlockClosureKlass::allocateObject( bool_t permit_scavenge, bool_t tenured ) {
    KlassOop k = as_klassOop();

    // allocate
    Oop * result = basicAllocate( BlockClosureOopDescriptor::object_size(), &k, permit_scavenge, tenured );
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
    return nullptr;
}


int BlockClosureKlass::oop_scavenge_contents( Oop obj ) {
    // header
    MemOop( obj )->scavenge_header();
    // %note _method can be ignored since methods are tenured
    scavenge_oop( ( Oop * ) &BlockClosureOop( obj )->addr()->_lexical_scope );
    return BlockClosureOopDescriptor::object_size();
}


int BlockClosureKlass::oop_scavenge_tenured_contents( Oop obj ) {
    // header
    MemOop( obj )->scavenge_tenured_header();
    // %note _method can be ignored since methods are tenured
    scavenge_tenured_oop( ( Oop * ) &BlockClosureOop( obj )->addr()->_lexical_scope );
    return BlockClosureOopDescriptor::object_size();
}


void BlockClosureKlass::oop_follow_contents( Oop obj ) {
    // header
    MemOop( obj )->follow_header();
    MarkSweep::reverse_and_push( ( Oop * ) &BlockClosureOop( obj )->addr()->_methodOrJumpAddr );
    MarkSweep::reverse_and_push( ( Oop * ) &BlockClosureOop( obj )->addr()->_lexical_scope );
}


void BlockClosureKlass::oop_layout_iterate( Oop obj, ObjectLayoutClosure * blk ) {
    // header
    MemOop( obj )->layout_iterate_header( blk );
    blk->do_oop( "method", ( Oop * ) &BlockClosureOop( obj )->addr()->_methodOrJumpAddr );
    blk->do_oop( "scope", ( Oop * ) &BlockClosureOop( obj )->addr()->_lexical_scope );
}


void BlockClosureKlass::oop_oop_iterate( Oop obj, OopClosure * blk ) {
    // header
    MemOop( obj )->oop_iterate_header( blk );
    blk->do_oop( ( Oop * ) &BlockClosureOop( obj )->addr()->_methodOrJumpAddr );
    blk->do_oop( ( Oop * ) &BlockClosureOop( obj )->addr()->_lexical_scope );
}


int BlockClosureKlass::number_of_arguments() const {
    KlassOop k = KlassOop( this );        // C++ bogosity alert
    if ( k == Universe::zeroArgumentBlockKlassObj() )
        return 0;
    if ( k == Universe::oneArgumentBlockKlassObj() )
        return 1;
    if ( k == Universe::twoArgumentBlockKlassObj() )
        return 2;
    if ( k == Universe::threeArgumentBlockKlassObj() )
        return 3;
    if ( k == Universe::fourArgumentBlockKlassObj() )
        return 4;
    if ( k == Universe::fiveArgumentBlockKlassObj() )
        return 5;
    if ( k == Universe::sixArgumentBlockKlassObj() )
        return 6;
    if ( k == Universe::sevenArgumentBlockKlassObj() )
        return 7;
    if ( k == Universe::eightArgumentBlockKlassObj() )
        return 8;
    if ( k == Universe::nineArgumentBlockKlassObj() )
        return 9;
    fatal( "unknown block closure class" );
    return 0;
}


void BlockClosureKlass::oop_print_value_on( Oop obj, ConsoleOutputStream * stream ) {
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


