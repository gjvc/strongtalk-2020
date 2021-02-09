
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/system/platform.hpp"
#include "vm/system/asserts.hpp"
#include "vm/memory/util.hpp"
#include "vm/utilities/OutputStream.hpp"
#include "vm/oops/MethodKlass.hpp"
#include "vm/interpreter/CodeIterator.hpp"
#include "vm/oops/KlassOopDescriptor.hpp"
#include "vm/oops/ObjectArrayOopDescriptor.hpp"
#include "vm/oops/SymbolOopDescriptor.hpp"
#include "vm/memory/MarkSweep.hpp"
#include "vm/utilities/ConsoleOutputStream.hpp"


void setKlassVirtualTableFromMethodKlass( Klass *k ) {
    MethodKlass o;
    k->set_vtbl_value( o.vtbl_value() );
}


KlassOop MethodKlass::create_subclass( MixinOop mixin, Format format ) {
    static_cast<void>(mixin); // unused
    static_cast<void>(format); // unused
    return nullptr;
}


void MethodKlass::oop_layout_iterate( Oop obj, ObjectLayoutClosure *blk ) {

    // header
    MemOop( obj )->layout_iterate_header( blk );
    MethodOop m = MethodOop( obj );
    blk->do_oop( "debugInfo", (Oop *) &m->addr()->_debugInfo );
    blk->do_oop( "selector", (Oop *) &m->addr()->_selector_or_method );
    blk->do_oop( "sizeCodes", (Oop *) &m->addr()->_size_and_flags );

    // indexables
    SPDLOG_INFO( "MethodKlass::oop_layout_iterate not implemented yet" );
    CodeIterator c( m );
    do {
        // Put in the meat here.
    } while ( c.advance() );

}


void MethodKlass::oop_oop_iterate( Oop obj, OopClosure *blk ) {

    // header
    MemOop( obj )->oop_iterate_header( blk );
    MethodOop m = MethodOop( obj );
    blk->do_oop( (Oop *) &m->addr()->_debugInfo );
    blk->do_oop( (Oop *) &m->addr()->_selector_or_method );
    blk->do_oop( (Oop *) &m->addr()->_size_and_flags );

    // codes
    CodeIterator c( m );
    do {
        switch ( c.format() ) {
            case ByteCodes::Format::BBO:
                blk->do_oop( c.aligned_oop( 2 ) );
                break; // BBO
            case ByteCodes::Format::BBOO:
                blk->do_oop( c.aligned_oop( 2 ) );
            case ByteCodes::Format::BBLO:
                blk->do_oop( c.aligned_oop( 2 ) + 1 );
                break; // BBOO, BBLO
            case ByteCodes::Format::BOL:
            case ByteCodes::Format::BO:
                blk->do_oop( c.aligned_oop( 1 ) );
                break; // BOL, BO
            case ByteCodes::Format::BOO:
            case ByteCodes::Format::BOOLB:
                blk->do_oop( c.aligned_oop( 1 ) );
            case ByteCodes::Format::BLO:
                blk->do_oop( c.aligned_oop( 1 ) + 1 );
                break; // BOO, BOOLB, BLO
            default: nullptr;
        }
    } while ( c.advance() );

}


void MethodKlass::oop_print_layout( Oop obj ) {
    MethodOop( obj )->print_codes();
}


void MethodKlass::oop_print_on( Oop obj, ConsoleOutputStream *stream ) {
    st_assert( obj->is_method(), "must be method" );
    MethodOop method = MethodOop( obj );

    std::int32_t indent_col = 3;
    std::int32_t value_col  = 16;

    // header
    MemOopKlass::oop_print_value_on( obj, stream );
    stream->cr();

    // selector/outer method
    stream->fill_to( indent_col );
    stream->print( "%s:", method->is_blockMethod() ? "outer" : "selector" );
    stream->fill_to( value_col );
    method->selector_or_method()->print_value_on( stream );
    stream->cr();

    // holder
    KlassOop k = Universe::method_holder_of( method->home() );
    if ( k ) {
        stream->fill_to( indent_col );
        stream->print( "holder:" );
        stream->fill_to( value_col );
        k->print_value_on( stream );
        stream->cr();
    }

    // incovation counter
    stream->fill_to( indent_col );
    stream->print( "invocation:" );
    stream->fill_to( value_col );
    stream->print_cr( "%d", method->invocation_count() );

    // sharing counter
    stream->fill_to( indent_col );
    stream->print( "sharing:" );
    stream->fill_to( value_col );
    stream->print_cr( "%d", method->sharing_count() );

    // code size
    stream->fill_to( indent_col );
    stream->print( "code size:" );
    stream->fill_to( value_col );
    stream->print_cr( "%d", method->size_of_codes() );

    // arguments
    stream->fill_to( indent_col );
    stream->print( "arguments:" );
    stream->fill_to( value_col );
    stream->print_cr( "%d", method->nofArgs() );

    // debug array
    stream->fill_to( indent_col );
    stream->print( "debug info:" );
    stream->fill_to( value_col );
    method->debugInfo()->print_value_on( stream );
    stream->cr();

    // flags
    stream->fill_to( indent_col );
    stream->print( "flags:" );
    stream->fill_to( value_col );
    stream->print( "%s", method->is_customized() ? "customized" : "not_customized" );
    if ( method->allocatesInterpretedContext() ) {
        stream->print( " allocates_context" );
    }
    if ( method->mustBeCustomizedToClass() ) {
        stream->print( " class_specific" );
    }
    if ( method->containsNonLocalReturn() ) {
        stream->print( " NonLocalReturn" );
    }

    // flags for blocks
    if ( method->is_blockMethod() ) {
        switch ( method->block_info() ) {
            case MethodOopDescriptor::expects_nil:
                stream->print( " pure_block" );
                break;
            case MethodOopDescriptor::expects_self:
                stream->print( " self_copying_block" );
                break;
            case MethodOopDescriptor::expects_parameter:
                stream->print( " parameter_copying_block" );
                break;
            case MethodOopDescriptor::expects_context:
                stream->print( " full_block" );
                break;
        }
    } else {
        switch ( method->method_inlining_info() ) {
            case MethodOopDescriptor::normal_inline:
                stream->print( " normal inline" );
                break;
            case MethodOopDescriptor::never_inline:
                stream->print( " never inline" );
                break;
            case MethodOopDescriptor::always_inline:
                stream->print( " aways inline" );
                break;
        }
    }
    stream->cr();
}


void MethodKlass::oop_print_value_on( Oop obj, ConsoleOutputStream *stream ) {
    st_assert( obj->is_method(), "must be method" );
    MethodOop method = MethodOop( obj );
    if ( PrintObjectID ) {
        MemOop( obj )->print_id_on( stream );
        stream->print( "-" );
    }
    if ( method->is_blockMethod() ) {
        stream->print( "BlockMethod in " );
        method->enclosing_method_selector()->print_symbol_on( stream );
    } else {
        stream->print( "Method " );
        method->selector()->print_symbol_on( stream );
    }
    if ( ProfilerShowMethodHolder ) {
        KlassOop k = Universe::method_holder_of( method );
        stream->print( " in " );
        if ( k ) {
            k->print_value_on( stream );
        } else {
            stream->print( "(unknown)" );
        }
    }
}


std::int32_t MethodKlass::oop_scavenge_contents( Oop obj ) {
    static_cast<void>(obj); // unused
    // Methods must reside in old Space
    ShouldNotCallThis();
    return -1;
}


std::int32_t MethodKlass::oop_scavenge_tenured_contents( Oop obj ) {
    // There should be no new objects referred insinde a methodOop
    return object_size( MethodOop( obj )->size_of_codes() );
}


void MethodKlass::oop_follow_contents( Oop obj ) {
    MethodOop    m = MethodOop( obj );
    CodeIterator c( m );
    do {
        switch ( c.format() ) {
            case ByteCodes::Format::BBO:
                MarkSweep::reverse_and_push( c.aligned_oop( 2 ) );
                break; // BBO

            case ByteCodes::Format::BBOO:
                MarkSweep::reverse_and_push( c.aligned_oop( 2 ) );
                [[fallthrough]];

            case ByteCodes::Format::BBLO:
                MarkSweep::reverse_and_push( c.aligned_oop( 2 ) + 1 );
                break; // BBOO, BBLO

            case ByteCodes::Format::BOL:
                [[fallthrough]];
            case ByteCodes::Format::BO:
                MarkSweep::reverse_and_push( c.aligned_oop( 1 ) );
                break; // BOL, BO

            case ByteCodes::Format::BOO:
                [[fallthrough]];
            case ByteCodes::Format::BOOLB:
                MarkSweep::reverse_and_push( c.aligned_oop( 1 ) );

            case ByteCodes::Format::BLO:
                MarkSweep::reverse_and_push( c.aligned_oop( 1 ) + 1 );
                break; // BOO, BOOLB, BLO

            default:
                (void)0;
        }
    } while ( c.advance() );

    MarkSweep::reverse_and_push( (Oop *) &m->addr()->_debugInfo );
    MarkSweep::reverse_and_push( (Oop *) &m->addr()->_selector_or_method );
    m->follow_header();
}


static Oop tenured( Oop obj ) {
    return obj->is_old() ? obj : obj->shallow_copy( true );
}


MethodOop MethodKlass::constructMethod( Oop selector_or_method, std::int32_t flags, std::int32_t nofArgs, ObjectArrayOop debugInfo, ByteArrayOop bytes, ObjectArrayOop oops ) {
    KlassOop     k        = as_klassOop();
    std::int32_t obj_size = MethodOopDescriptor::header_size() + oops->length();

    st_assert( oops->length() * OOP_SIZE == bytes->length(), "Invalid array sizes" );

    // allocate
    MethodOop method = as_methodOop( Universe::allocate_tenured( obj_size ) );
    MemOop( method )->initialize_header( has_untagged_contents(), k );

    // initialize the header
    method->set_debugInfo( ObjectArrayOop( tenured( debugInfo ) ) );
    method->set_selector_or_method( tenured( selector_or_method ) );

    method->set_invocation_count( 0 );
    method->set_sharing_count( 0 );
    method->set_size_and_flags( oops->length(), nofArgs, flags );

    // merge the bytes and the oops

    // first copy the byte array into the method
    for ( std::int32_t i = 1; i <= bytes->length(); i++ ) {
        method->byte_at_put( i, bytes->byte_at( i ) );
    }

    // then merge in the oops
    for ( std::int32_t i = 1; i <= oops->length(); i++ ) {
        bool         copyOop  = true;
        std::int32_t bc_index = i * OOP_SIZE - ( OOP_SIZE - 1 );

        for ( std::int32_t j = 0; j < OOP_SIZE; j++ ) {
            // copy Oop if bytearray holds 4 consecutive aligned zeroes
            if ( bytes->byte_at( bc_index + j ) not_eq 0 ) {
                copyOop = false;
            }
        }

        if ( copyOop ) {
            Oop value = tenured( oops->obj_at( i ) );
            st_assert( value->isSmallIntegerOop() or value->is_old(), "literal must be tenured" );
            method->oop_at_put( bc_index, value );
        }
    }

    st_assert( method->is_method(), "must be method" );
    return method;
}
