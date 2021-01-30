//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/system/platform.hpp"
#include "vm/system/asserts.hpp"
#include "vm/oops/MethodOopDescriptor.hpp"
#include "vm/interpreter/CodeIterator.hpp"
#include "vm/runtime/Bootstrap.hpp"
#include "vm/interpreter/MethodIterator.hpp"
#include "vm/interpreter/MethodPrinterClosure.hpp"
#include "vm/interpreter/PrettyPrinter.hpp"
#include "vm/memory/vmSymbols.hpp"
#include "vm/memory/oopFactory.hpp"
#include "vm/interpreter/InterpretedInlineCache.hpp"
#include "vm/primitives/primitives.hpp"
#include "vm/system/dll.hpp"
#include "vm/compiler/CostModel.hpp"
#include "vm/lookup/LookupCache.hpp"
#include "vm/oops/BlockClosureOopDescriptor.hpp"
#include "vm/runtime/ResourceMark.hpp"
#include "vm/memory/Scavenge.hpp"
#include "vm/oops/ExpressionStackMapper.hpp"


void MethodOopDescriptor::decay_invocation_count( double decay_factor ) {
    double new_count = (double) invocation_count() / decay_factor;
    set_invocation_count( (std::int32_t) new_count );

    // Take care of the block methods
    CodeIterator c( this );
    do {
        switch ( c.code() ) {
            case ByteCodes::Code::push_new_closure_tos_0:      // fall through
            case ByteCodes::Code::push_new_closure_tos_1:      // fall through
            case ByteCodes::Code::push_new_closure_tos_2:      // fall through
            case ByteCodes::Code::push_new_closure_context_0:  // fall through
            case ByteCodes::Code::push_new_closure_context_1:  // fall through
            case ByteCodes::Code::push_new_closure_context_2: {
                MethodOop block_method = MethodOop( c.oop_at( 1 ) );
                st_assert( block_method->is_method(), "must be method" );
                block_method->decay_invocation_count( decay_factor );
            }
                break;
            case ByteCodes::Code::push_new_closure_tos_n:      // fall through
            case ByteCodes::Code::push_new_closure_context_n: {
                MethodOop block_method = MethodOop( c.oop_at( 2 ) );
                st_assert( block_method->is_method(), "must be method" );
                block_method->decay_invocation_count( decay_factor );
            }
                break;
        }
    } while ( c.advance() );
}


void MethodOopDescriptor::inc_sharing_count() {
    if ( sharing_count() < _sharing_count_max ) {
        set_sharing_count( sharing_count() + 1 );
    }
}


void MethodOopDescriptor::dec_sharing_count() {
    if ( sharing_count() > 0 ) {
        set_sharing_count( sharing_count() - 1 );
    }
}


void MethodOopDescriptor::bootstrap_object( Bootstrap *stream ) {
    MemOopDescriptor::bootstrap_header( stream );
    stream->read_oop( (Oop *) &addr()->_debugInfo );
    stream->read_oop( (Oop *) &addr()->_selector_or_method );
    set_counters( 0, 0 );
    stream->read_oop( (Oop *) &addr()->_size_and_flags );

    for ( std::int32_t i = 1; i <= size_of_codes() * 4; )
        if ( stream->is_byte() ) {
            byte_at_put( i, stream->read_byte() );
            i++;
        } else {
            stream->read_oop( (Oop *) codes( i ) );
            i += 4;
        }
}


std::int32_t MethodOopDescriptor::next_byteCodeIndex_from( std::uint8_t *hp ) const {
    // Computes the next byteCodeIndex
    // hp is the interpreter 'ip' kept in the activation pointing to the next code to execute.

    // Fist the next byteCodeIndex is computed. Note the first index is 1.
    return ( hp - (std::uint8_t *) addr() ) - sizeof( MethodOopDescriptor ) + 1;
}


std::int32_t MethodOopDescriptor::byteCodeIndex_from( std::uint8_t *hp ) const {
    // We find the current byteCodeIndex by searching from the beginning
    return find_byteCodeIndex_from( next_byteCodeIndex_from( hp ) );
}


std::int32_t MethodOopDescriptor::number_of_arguments() const {
    st_assert( is_blockMethod() or selector()->number_of_arguments() == nofArgs(), "just checking" );
    return nofArgs();
}


std::int32_t MethodOopDescriptor::number_of_stack_temporaries() const {
    std::int32_t n  = 1;        // temporary 0 is always there
    std::uint8_t b0 = *codes( 1 );// if there's more than one temporary there's an allocate temp or allocate float at the beginning
    switch ( b0 ) {
        case static_cast<std::int32_t>(ByteCodes::Code::allocate_temp_1):
            n += 1;
            break;
        case static_cast<std::int32_t>(ByteCodes::Code::allocate_temp_2):
            n += 2;
            break;
        case static_cast<std::int32_t>(ByteCodes::Code::allocate_temp_3):
            n += 3;
            break;
        case static_cast<std::int32_t>(ByteCodes::Code::allocate_temp_n): {
            std::uint8_t b1 = *codes( 2 );
            n += ( ( b1 == 0 ) ? 256 : b1 );
        }
            break;

        case static_cast<std::int32_t>(ByteCodes::Code::float_allocate): {
            // One additional temp (temp1) for Floats::magic + additional
            // temps allocated in pairs to match to match one float temp.
            std::uint8_t b1 = *codes( 2 );
            n += 1 + b1 * 2;
        }
            break;
    }


    return
        n;
}


std::int32_t MethodOopDescriptor::float_offset( std::int32_t float_no ) const {
    st_assert( 0 <= float_no and float_no < number_of_float_temporaries(), "float_no out of range" );
    return float_section_start_offset() - float_no * SIZEOF_FLOAT / OOP_SIZE - 1;
}


SymbolOop MethodOopDescriptor::enclosing_method_selector() const {
    st_assert( is_blockMethod(), "must be block method" );
    MethodOop m = parent();
    while ( m->is_blockMethod() )
        m = m->parent();
    return m->selector();
}


void MethodOopDescriptor::print_value_for( KlassOop receiver_klass, ConsoleOutputStream *stream ) {
    ConsoleOutputStream *s = stream ? stream : _console;
    if ( is_blockMethod() ) {
        s->print( "[] in " );
        enclosing_method_selector()->print_symbol_on( s );
    } else {
        selector()->print_symbol_on( s );
    }
    KlassOop holder = receiver_klass->klass_part()->lookup_method_holder_for( this );
    if ( holder ) {
        s->print( " in " );
        holder->klass_part()->print_name_on( s );
    }
}


void MethodOopDescriptor::print_codes() {
    ResourceMark resourceMark;
    selector()->print_symbol_on( _console );
    _console->cr();
    auto           temp = MethodPrinterClosure( _console );
    MethodIterator mi( this, &temp );
    _console->cr();
}


void MethodOopDescriptor::pretty_print() {
    ResourceMark resourceMark;
    PrettyPrinter::print( this );
}


SymbolOop MethodOopDescriptor::selector() const {
    if ( selector_or_method()->is_symbol() )
        return SymbolOop( selector_or_method() );
    return vmSymbols::selector_for_blockMethod();
}


MethodOop MethodOopDescriptor::parent() const {
    Oop t = selector_or_method();
    return t->is_method() ? MethodOop( t ) : nullptr;
}


MethodOop MethodOopDescriptor::home() const {
    MethodOop m = MethodOop( this );
    while ( m->is_blockMethod() )
        m = m->parent();
    return m;
}


ByteArrayOop MethodOopDescriptor::source() {
    return oopFactory::new_symbol( "<no source>" );
}


ObjectArrayOop MethodOopDescriptor::tempInfo() {
    return debugInfo();
}


class methodStream {
public:
    GrowableArray<Oop> *result;


    methodStream() {
        result = new GrowableArray<Oop>( 1000 );
    }


    void put_byte( std::int32_t byte ) {
        result->append( trueObject );
        result->append( smiOopFromValue( byte ) );
    }


    void put_word( std::int32_t word ) {
        const char *p = (const char *) &word;
        put_byte( p[ 0 ] );
        put_byte( p[ 1 ] );
        put_byte( p[ 2 ] );
        put_byte( p[ 3 ] );
    }


    void put_oop( Oop obj ) {
        result->append( falseObject );
        result->append( obj );
    }


    void align( std::uint8_t *hp ) {
        std::uint8_t *end = (std::uint8_t *) ( ( (std::int32_t) hp + 3 ) & ( ~3 ) );
        while ( hp < end ) {
            put_byte( 255 );
            hp++;
        }
    }
};


ObjectArrayOop MethodOopDescriptor::fileout_body() {
    // Convert sends into canonical form
    // Do not uncustomize since we need the mixin to do that.
    BlockScavenge bs;
    ResourceMark  rm;
    methodStream  out;

    CodeIterator c( this );
    do {
        if ( ByteCodes::send_type( c.code() ) not_eq ByteCodes::SendType::no_send ) {
            // Send
            ByteCodes::Code original = ByteCodes::original_send_code_for( c.code() );
            out.put_byte( static_cast<std::int32_t>(original) );
            if ( ByteCodes::format( original ) == ByteCodes::Format::BBOO ) {
                out.put_byte( c.byte_at( 1 ) );
                out.align( c.hp() + 2 );
            } else {
                out.align( c.hp() + 1 );
            }
            out.put_oop( c.ic()->selector() );
            out.put_oop( smiOop_zero );
        } else if ( c.is_primitive_call() ) {
            // Primitive call
            ByteCodes::Code original = ByteCodes::original_primitive_call_code_for( c.code() );
            out.put_byte( static_cast<std::int32_t>(original) );
            out.align( c.hp() + 1 );
            if ( c.code() == ByteCodes::Code::prim_call or c.code() == ByteCodes::Code::primitive_call_failure or c.code() == ByteCodes::Code::primitive_call_self or c.code() == ByteCodes::Code::primitive_call_self_failure ) {
                PrimitiveDescriptor *pdesc = Primitives::lookup( (primitiveFunctionType) c.word_at( 1 ) );
                out.put_oop( pdesc->selector() );
            } else {
                out.put_oop( c.oop_at( 1 ) );
            }
            if ( ByteCodes::format( original ) == ByteCodes::Format::BOL ) {
                out.put_word( c.word_at( 5 ) );
            }
        } else if ( c.is_dll_call() ) {
            // DLL call
            Interpreted_DLLCache *ic = c.dll_cache();
            out.put_byte( static_cast<std::int32_t>( c.code() ) );
            out.align( c.hp() + 1 );
            out.put_oop( ic->dll_name() );
            out.put_oop( ic->funct_name() );
            out.put_oop( smiOop_zero );
            out.put_byte( ic->number_of_arguments() );
        } else {
            // Otherwise
            out.put_byte( static_cast<std::int32_t>( c.code() ) );
            switch ( c.format() ) {
                case ByteCodes::Format::B:
                    break;
                case ByteCodes::Format::BB:
                    out.put_byte( c.byte_at( 1 ) );
                    break;
                case ByteCodes::Format::BBB:
                    out.put_byte( c.byte_at( 1 ) );
                    out.put_byte( c.byte_at( 2 ) );
                    break;
                case ByteCodes::Format::BBBB:
                    out.put_byte( c.byte_at( 1 ) );
                    out.put_byte( c.byte_at( 2 ) );
                    out.put_byte( c.byte_at( 3 ) );
                    break;
                case ByteCodes::Format::BBO:
                    out.put_byte( c.byte_at( 1 ) );
                    out.align( c.hp() + 2 );
                    out.put_oop( c.oop_at( 2 ) );
                    break;
                case ByteCodes::Format::BBL:
                    out.put_byte( c.byte_at( 1 ) );
                    out.align( c.hp() + 2 );
                    out.put_word( c.word_at( 2 ) );
                    break;
                case ByteCodes::Format::BO:
                    out.align( c.hp() + 1 );
                    out.put_oop( c.oop_at( 1 ) );
                    break;
                case ByteCodes::Format::BOL:
                    out.put_oop( c.oop_at( 1 ) );
                    out.align( c.hp() + 2 );
                    out.put_word( c.word_at( 5 ) );
                    break;
                case ByteCodes::Format::BLL:
                    out.align( c.hp() + 1 );
                    out.put_word( c.word_at( 1 ) );
                    out.put_word( c.word_at( 5 ) );
                    break;
                case ByteCodes::Format::BL:
                    out.align( c.hp() + 1 );
                    out.put_word( c.word_at( 1 ) );
                    break;
                case ByteCodes::Format::BBS: {
                    std::int32_t length = c.byte_at( 1 ) == 0 ? 256 : c.byte_at( 1 );
                    out.put_byte( length );
                    for ( std::int32_t i = 0; i < length; i++ ) {
                        out.put_byte( c.byte_at( 2 + i ) );
                    }
                    break;
                }
                default:
                    spdlog::info( "Format unknown %s", ByteCodes::format_as_string( c.format() ) );
                    st_fatal( "aborting" );
            }
        }
    } while ( c.advance() );
    return oopFactory::new_objArray( out.result );
}


MethodOopDescriptor::Method_Inlining_Info MethodOopDescriptor::method_inlining_info() const {
    if ( is_blockMethod() )
        return normal_inline;
    Method_Inlining_Info info = Method_Inlining_Info( get_unsigned_bitfield( flags(), methodInfoFlags, methodInfoSize ) );
    return info;
}


void MethodOopDescriptor::set_method_inlining_info( Method_Inlining_Info info ) {
    if ( is_blockMethod() )
        return;
    set_flags( set_unsigned_bitfield( flags(), methodInfoFlags, methodInfoSize, info ) );
}


MethodOopDescriptor::Block_Info MethodOopDescriptor::block_info() const {
    st_assert( is_blockMethod(), "must be a block" );
    return Block_Info( get_unsigned_bitfield( flags(), blockInfoFlags, blockInfoSize ) );
}


bool MethodOopDescriptor::in_context_allocation( std::int32_t byteCodeIndex ) const {
    CodeIterator c( MethodOop( this ), byteCodeIndex );
    return c.code_type() == ByteCodes::CodeType::new_context;
}


class BlockFinderClosure : public SpecializedMethodClosure {
public:
    bool hasBlock;


    BlockFinderClosure() {
        hasBlock = false;
    }


    void allocate_closure( AllocationType type, std::int32_t nofArgs, MethodOop meth ) {
        hasBlock = true;
    }
};


bool MethodOopDescriptor::hasNestedBlocks() const {
    // should be a bit in the methodOop -- fix this, Robert (delete class above)
    BlockFinderClosure cl;
    MethodIterator     it( MethodOop( this ), &cl );
    return cl.hasBlock;
}


// The following two functions map context numbers (as used in the interpreter
// to access temps in enclosing scopes) to source-level lexical distances, and
// vice versa.
// Definitions: context no = number of indirections through contexts needed to
//			     access temporary (0 -> temp is in current context)
//		lex. dist. = difference in nesting levels between two scopes;
//			     e.g., distance between a scope and its immediately
//			     enclosing scope is 1

std::int32_t MethodOopDescriptor::lexicalDistance( std::int32_t contextNo ) {
    MethodOop    m = this;
    std::int32_t c = -1;
    std::int32_t d = -1;
    while ( c < contextNo ) {
        if ( m->allocatesInterpretedContext() )
            c++;
        m = m->parent();
        d++;
    };
    return d;
}


std::int32_t MethodOopDescriptor::contextNo( std::int32_t lexicalDistance ) {
    MethodOop    m = this;
    std::int32_t c = -1;
    std::int32_t d = -1;
    while ( d < lexicalDistance ) {
        if ( m->allocatesInterpretedContext() )
            c++;
        m = m->parent();
        d++;
    }
    return c;
}


std::int32_t MethodOopDescriptor::context_chain_length() const {
    std::int32_t    length = 0;
    for ( MethodOop method = MethodOop( this ); method; method = method->parent() ) {
        if ( method->allocatesInterpretedContext() )
            length++;
    }
    return length;
}


void MethodOopDescriptor::clear_inline_caches() {
    //    if the method is not customized it has never been executed.
    if ( not is_customized() )
        return;

    CodeIterator c( this );
    do {
        InterpretedInlineCache *ic = c.ic();
        if ( ic ) {
            ic->clear();
        } else {
            // Call it for blocks
            switch ( c.code() ) {
                case ByteCodes::Code::push_new_closure_tos_0:      // fall through
                case ByteCodes::Code::push_new_closure_tos_1:      // fall through
                case ByteCodes::Code::push_new_closure_tos_2:      // fall through
                case ByteCodes::Code::push_new_closure_context_0:  // fall through
                case ByteCodes::Code::push_new_closure_context_1:  // fall through
                case ByteCodes::Code::push_new_closure_context_2: {
                    MethodOop block_method = MethodOop( c.oop_at( 1 ) );
                    st_assert( block_method->is_method(), "must be method" );
                    block_method->clear_inline_caches();
                }
                    break;
                case ByteCodes::Code::push_new_closure_tos_n:      // fall through
                case ByteCodes::Code::push_new_closure_context_n: {
                    MethodOop block_method = MethodOop( c.oop_at( 2 ) );
                    st_assert( block_method->is_method(), "must be method" );
                    block_method->clear_inline_caches();
                }
                    break;
            }
        }
    } while ( c.advance() );
}


void MethodOopDescriptor::cleanup_inline_caches() {
    // if the method is not customized it has never been executed.
    if ( not is_customized() )
        return;

    CodeIterator c( this );
    do {
        InterpretedInlineCache *ic = c.ic();
        if ( ic ) {
            ic->cleanup();
        } else {
            MethodOop bm = c.block_method();
            if ( bm ) {
                bm->cleanup_inline_caches();
            }
        }
    } while ( c.advance() );
}


bool MethodOopDescriptor::was_never_executed() {
    // if the method is not customized it has never been executed.
    if ( not is_customized() )
        return true;

    // return true if method looks like it was never executed
    if ( invocation_count() not_eq 0 or sharing_count() not_eq 0 )
        return false;
    CodeIterator c( this );
    do {
        InterpretedInlineCache *ic = c.ic();
        if ( ic and not ic->is_empty() )
            return false;
    } while ( c.advance() );
    return true;
}


std::int32_t MethodOopDescriptor::estimated_inline_cost( KlassOop receiverKlass ) {
    // the result of this calculation should be cached in the method; 8 bits are enough
    CodeIterator c( this );
    std::int32_t cost = 0;
    do {
        cost += CostModel::cost_for( c.code() );
        switch ( c.code() ) {
            case ByteCodes::Code::push_new_closure_context_0:
            case ByteCodes::Code::push_new_closure_context_1:
            case ByteCodes::Code::push_new_closure_context_2:
            case ByteCodes::Code::push_new_closure_tos_0:
            case ByteCodes::Code::push_new_closure_tos_1:
            case ByteCodes::Code::push_new_closure_tos_2: {
                MethodOop m = MethodOop( c.oop_at( 1 ) );
                st_assert( m->is_method(), "must be method" );
                cost += m->estimated_inline_cost( receiverKlass );
                break;
            }
            case ByteCodes::Code::push_new_closure_tos_n:
            case ByteCodes::Code::push_new_closure_context_n: {
                MethodOop m = MethodOop( c.oop_at( 2 ) );
                st_assert( m->is_method(), "must be method" );
                cost += m->estimated_inline_cost( receiverKlass );
                break;
            }
        }
        extern bool SuperSendsAreAlwaysInlined;
        if ( ByteCodes::is_super_send( c.code() ) and SuperSendsAreAlwaysInlined and receiverKlass ) {
            KlassOop  mh          = receiverKlass->klass_part()->lookup_method_holder_for( this );
            // TODO: the following is wrong. A super send may use a different selector than
            // the containing method. It's bad style, but legal. Need to lookup the selector
            // for the send, not the containing method's selector. slr 13/04/2010
            MethodOop superMethod = mh ? LookupCache::compile_time_super_lookup( mh, selector() ) : nullptr;
            if ( superMethod )
                cost += superMethod->estimated_inline_cost( receiverKlass );
        }
    } while ( c.advance() );
    return cost;
}


std::int32_t MethodOopDescriptor::find_byteCodeIndex_from( std::int32_t nbyteCodeIndex ) const {
    CodeIterator c( MethodOop( this ) );
    std::int32_t prev_byteCodeIndex = 1;
    do {
        if ( c.byteCodeIndex() == nbyteCodeIndex )
            return prev_byteCodeIndex;
        prev_byteCodeIndex = c.byteCodeIndex();
    } while ( c.advance() );
    return -1;
}


std::int32_t MethodOopDescriptor::next_byteCodeIndex( std::int32_t byteCodeIndex ) const {
    CodeIterator c( MethodOop( this ), byteCodeIndex );
    c.advance();
    return c.byteCodeIndex();
}


GrowableArray<std::int32_t> *MethodOopDescriptor::expression_stack_mapping( std::int32_t byteCodeIndex ) {

    GrowableArray<std::int32_t> *mapping = new GrowableArray<std::int32_t>( 10 );
    ExpressionStackMapper       blk( mapping, byteCodeIndex );
    MethodIterator              i( this, &blk );

    // reverse the mapping so the top of the expression stack is first
    // %todo:
    //    move reverse to GrowableArray

    GrowableArray<std::int32_t> *result = new GrowableArray<std::int32_t>( mapping->length() );

    for ( std::int32_t i = mapping->length() - 1; i >= 0; i-- ) {
        result->push( mapping->at( i ) );
    }

    return result;
}


static void lookup_primitive_and_patch( std::uint8_t *p, std::uint8_t byte ) {
    st_assert( (std::int32_t) p % 4 == 0, "first instruction supposed to be aligned" );
    *p = byte;    // patch byte
    p += 4;    // advance to primitive name
    //(*(SymbolOop*)p)->print_symbol_on();
    *(std::int32_t *) p = (std::int32_t) Primitives::lookup( *(SymbolOop *) p )->fn();
}


bool MethodOopDescriptor::is_primitiveMethod() const {
    auto b = static_cast<ByteCodes::Code>(*codes());
    switch ( b ) {
        case ByteCodes::Code::predict_primitive_call:
            return true;
        case ByteCodes::Code::predict_primitive_call_failure:
            return true;
        case ByteCodes::Code::predict_primitive_call_lookup:
            lookup_primitive_and_patch( codes(), static_cast<std::int32_t>( ByteCodes::Code::predict_primitive_call ) );
            return true;
        case ByteCodes::Code::predict_primitive_call_failure_lookup:
            lookup_primitive_and_patch( codes(), static_cast<std::int32_t>( ByteCodes::Code::predict_primitive_call_failure ) );
            return true;
        default:
            return false;
    }
}


ByteCodes::Code MethodOopDescriptor::special_primitive_code() const {
    st_assert( is_special_primitiveMethod(), "should only be called for special primitive methods" );
    ByteCodes::Code code = ByteCodes::Code( *codes( 2 ) );
    st_assert( ByteCodes::send_type( code ) == ByteCodes::SendType::predicted_send, "code or bytecode table inconsistent" );
    return code;
}


MethodOop MethodOopDescriptor::methodOop_from_hcode( std::uint8_t *hp ) {
    MethodOop method = MethodOop( as_memOop( Universe::object_start( (Oop *) hp ) ) );
    st_assert( method->is_method(), "must be method" );
    st_assert( method->codes() <= hp and hp < method->codes() + method->size_of_codes() * sizeof( Oop ), "h-code pointer not contained in method" );
    return method;
}


std::int32_t MethodOopDescriptor::end_byteCodeIndex() const {
    std::int32_t last_entry = this->size_of_codes() * 4;

    for ( std::int32_t i = 0; i < 4; i++ )
        if ( byte_at( last_entry - i ) not_eq static_cast<std::int32_t>( ByteCodes::Code::halt ) )
            return last_entry + 1 - i;

    st_fatal( "should never reach the point" );
    return 0;
}


InterpretedInlineCache *MethodOopDescriptor::ic_at( std::int32_t byteCodeIndex ) const {
    CodeIterator iterator( MethodOop( this ), byteCodeIndex );
    return iterator.ic();
}


MethodOop MethodOopDescriptor::block_method_at( std::int32_t byteCodeIndex ) {

    CodeIterator c( MethodOop( this ), byteCodeIndex );
    switch ( c.code() ) {
        case ByteCodes::Code::push_new_closure_tos_0:      // fall through
        case ByteCodes::Code::push_new_closure_tos_1:      // fall through
        case ByteCodes::Code::push_new_closure_tos_2:      // fall through
        case ByteCodes::Code::push_new_closure_context_0:  // fall through
        case ByteCodes::Code::push_new_closure_context_1:  // fall through
        case ByteCodes::Code::push_new_closure_context_2: {
            MethodOop block_method = MethodOop( c.oop_at( 1 ) );
            st_assert( block_method->is_method(), "must be method" );
            return block_method;
        }
            break;
        case ByteCodes::Code::push_new_closure_tos_n:      // fall through
        case ByteCodes::Code::push_new_closure_context_n: {
            MethodOop block_method = MethodOop( c.oop_at( 2 ) );
            st_assert( block_method->is_method(), "must be method" );
            return block_method;
        }
            break;

        default:
            (void)0;
    }

    return nullptr;
}


std::int32_t MethodOopDescriptor::byteCodeIndex_for_block_method( MethodOop inner ) {

    CodeIterator c( this );
    do {
        if ( inner == block_method_at( c.byteCodeIndex() ) )
            return c.byteCodeIndex();
    } while ( c.advance() );
    ShouldNotReachHere();

    return 0;
}


void MethodOopDescriptor::print_inlining_database_on( ConsoleOutputStream *stream ) {
    if ( is_blockMethod() ) {
        MethodOop o = parent();
        o->print_inlining_database_on( stream );
        stream->print( " %d", o->byteCodeIndex_for_block_method( this ) );
    } else {
        selector()->print_symbol_on( stream );
    }
}


// ContextMethodIterator is used in number_of_context_temporaries to
// get information about context allocation
class ContextMethodIterator : public SpecializedMethodClosure {
private:
    enum {
        sentinel = -1 //
    };

    std::int32_t count;
    bool         _self_in_context;

public:
    ContextMethodIterator() {
        count            = sentinel;
        _self_in_context = false;
    }


    bool self_in_context() {
        return _self_in_context;
    }


    std::int32_t number_of_context_temporaries() {
        st_assert( count not_eq sentinel, "number_of_context_temporaries not set" );
        return count;
    }


    void allocate_context( std::int32_t nofTemps, bool forMethod ) {
        st_assert( count == sentinel, "make sure it is not called more than one" );
        count = nofTemps;
    }


    void copy_self_into_context() {
        _self_in_context = true;
    }
};


std::int32_t MethodOopDescriptor::number_of_context_temporaries( bool *self_in_context ) {
    // Use this for debugging only
    st_assert( allocatesInterpretedContext(), "can only be called if method allocates context" );
    ContextMethodIterator blk;
    MethodIterator        i( this, &blk );
    if ( self_in_context )
        *self_in_context = blk.self_in_context();
    return blk.number_of_context_temporaries();
}


void MethodOopDescriptor::customize_for( KlassOop klass, MixinOop mixin ) {
    st_assert( not is_customized() or klass not_eq mixin->primary_invocation(), "should not recustomize to the same class" );

    CodeIterator c( this );
    do {
        InterpretedInlineCache *ic = c.ic();
        if ( ic )
            ic->clear_without_deallocation_pic();
        switch ( c.code() ) {

            case ByteCodes::Code::push_classVar_name:
            case ByteCodes::Code::store_classVar_pop_name:
            case ByteCodes::Code::store_classVar_name:
                c.customize_class_var_code( klass );
                break;

            case ByteCodes::Code::push_classVar:
            case ByteCodes::Code::store_classVar_pop:
            case ByteCodes::Code::store_classVar:
                c.recustomize_class_var_code( mixin->primary_invocation(), klass );
                break;

            case ByteCodes::Code::push_instVar_name:
            case ByteCodes::Code::store_instVar_pop_name:
            case ByteCodes::Code::store_instVar_name:
            case ByteCodes::Code::return_instVar_name:
                c.customize_inst_var_code( klass );
                break;

            case ByteCodes::Code::push_instVar:
            case ByteCodes::Code::store_instVar_pop:
            case ByteCodes::Code::store_instVar:
            case ByteCodes::Code::return_instVar:
                c.recustomize_inst_var_code( mixin->primary_invocation(), klass );
                break;

            case ByteCodes::Code::push_new_closure_tos_0:      // fall through
            case ByteCodes::Code::push_new_closure_tos_1:      // fall through
            case ByteCodes::Code::push_new_closure_tos_2:      // fall through
            case ByteCodes::Code::push_new_closure_context_0:  // fall through
            case ByteCodes::Code::push_new_closure_context_1:  // fall through
            case ByteCodes::Code::push_new_closure_context_2: {
                MethodOop block_method = MethodOop( c.oop_at( 1 ) );
                st_assert( block_method->is_method(), "must be method" );
                block_method->customize_for( klass, mixin );
            }
                break;
            case ByteCodes::Code::push_new_closure_tos_n:      // fall through
            case ByteCodes::Code::push_new_closure_context_n: {
                MethodOop block_method = MethodOop( c.oop_at( 2 ) );
                st_assert( block_method->is_method(), "must be method" );
                block_method->customize_for( klass, mixin );
            }
                break;
        }
    } while ( c.advance() );
    // set customized flag

    std::int32_t new_flags = addNthBit( flags(), isCustomizedFlag );
    set_size_and_flags( size_of_codes(), nofArgs(), new_flags );
}


void MethodOopDescriptor::uncustomize_for( MixinOop mixin ) {

    if ( not is_customized() )
        return;

    KlassOop klass = mixin->primary_invocation();
    st_assert( klass->is_klass(), "primary invocation muyst be present" );

    CodeIterator c( this );
    do {
        InterpretedInlineCache *ic = c.ic();
        if ( ic )
            ic->clear_without_deallocation_pic();
        switch ( c.code() ) {
            case ByteCodes::Code::push_classVar:
            case ByteCodes::Code::store_classVar_pop:
            case ByteCodes::Code::store_classVar:
                c.uncustomize_class_var_code( mixin->primary_invocation() );
                break;

            case ByteCodes::Code::push_instVar:
            case ByteCodes::Code::store_instVar_pop:
            case ByteCodes::Code::store_instVar:
            case ByteCodes::Code::return_instVar:
                c.uncustomize_inst_var_code( mixin->primary_invocation() );
                break;

            case ByteCodes::Code::push_new_closure_tos_0:      // fall through
            case ByteCodes::Code::push_new_closure_tos_1:      // fall through
            case ByteCodes::Code::push_new_closure_tos_2:      // fall through
            case ByteCodes::Code::push_new_closure_context_0:  // fall through
            case ByteCodes::Code::push_new_closure_context_1:  // fall through
            case ByteCodes::Code::push_new_closure_context_2: {
                MethodOop block_method = MethodOop( c.oop_at( 1 ) );
                st_assert( block_method->is_method(), "must be method" );
                block_method->uncustomize_for( mixin );
            }
                break;
            case ByteCodes::Code::push_new_closure_tos_n:      // fall through
            case ByteCodes::Code::push_new_closure_context_n: {
                MethodOop block_method = MethodOop( c.oop_at( 2 ) );
                st_assert( block_method->is_method(), "must be method" );
                block_method->uncustomize_for( mixin );
            }
                break;
        }
    } while ( c.advance() );

    // set customized flag
    std::int32_t new_flags = subNthBit( flags(), isCustomizedFlag );
    set_size_and_flags( size_of_codes(), nofArgs(), new_flags );
}


MethodOop MethodOopDescriptor::copy_for_customization() const {
    // Copy this method
    std::int32_t len    = size();
    Oop          *clone = Universe::allocate_tenured( len );
    Oop          *to    = clone;
    Oop          *from  = (Oop *) addr();
    Oop          *end   = to + len;
    while ( to < end )
        *to++ = *from++;

    // Do the deep copy
    MethodOop    new_method = MethodOop( as_memOop( clone ) );
    CodeIterator c( new_method );
    do {
        switch ( c.code() ) {
            case ByteCodes::Code::push_new_closure_tos_0:      // fall through
            case ByteCodes::Code::push_new_closure_tos_1:      // fall through
            case ByteCodes::Code::push_new_closure_tos_2:      // fall through
            case ByteCodes::Code::push_new_closure_context_0:  // fall through
            case ByteCodes::Code::push_new_closure_context_1:  // fall through
            case ByteCodes::Code::push_new_closure_context_2: {
                MethodOop block_method = MethodOop( c.oop_at( 1 ) );
                st_assert( block_method->is_method(), "must be method" );
                MethodOop new_block_method = block_method->copy_for_customization();
                new_block_method->set_selector_or_method( new_method );
                Universe::store( c.aligned_oop( 1 ), new_block_method );
            }
                break;
            case ByteCodes::Code::push_new_closure_tos_n:      // fall through
            case ByteCodes::Code::push_new_closure_context_n: {
                MethodOop block_method = MethodOop( c.oop_at( 2 ) );
                st_assert( block_method->is_method(), "must be method" );
                MethodOop new_block_method = block_method->copy_for_customization();
                new_block_method->set_selector_or_method( new_method );
                Universe::store( c.aligned_oop( 2 ), new_block_method );
            }
                break;
        }
    } while ( c.advance() );
    return new_method;
}


void MethodOopDescriptor::verify_context( ContextOop con ) {

    // Check if we should expect a context
    if ( not activation_has_context() ) {
        spdlog::warn( "Activation has no context (0x{0:x})", static_cast<const void *>(con) );
    }

    // Check the static vs. dynamic chain length
    if ( context_chain_length() not_eq con->chain_length() ) {
        spdlog::warn( "Wrong context chain length (got %d expected %d)", con->chain_length(), context_chain_length() );
    }

    // Check the context has no forward reference
    if ( con->unoptimized_context() not_eq nullptr ) {
        spdlog::warn( "Context is optimized (0x{0:x})", static_cast<const void *>(con) );
    }
}


// Traverses over the method including the blocks inside
class TransitiveMethodClosure : public MethodClosure {
public:
    void if_node( IfNode *node );

    void cond_node( CondNode *node );

    void while_node( WhileNode *node );

    void primitive_call_node( PrimitiveCallNode *node );

    void dll_call_node( DLLCallNode *node );

public:
    virtual void inlined_send( SymbolOop selector ) {
    }


public:
    void allocate_temporaries( std::int32_t nofTemps ) {
    }


    void push_self() {
    }


    void push_tos() {
    }


    void push_literal( Oop obj ) {
    }


    void push_argument( std::int32_t no ) {
    }


    void push_temporary( std::int32_t no ) {
    }


    void push_temporary( std::int32_t no, std::int32_t context ) {
    }


    void push_instVar( std::int32_t offset ) {
    }


    void push_instVar_name( SymbolOop name ) {
    }


    void push_classVar( AssociationOop assoc ) {
    }


    void push_classVar_name( SymbolOop name ) {
    }


    void push_global( AssociationOop obj ) {
    }


    void store_temporary( std::int32_t no ) {
    }


    void store_temporary( std::int32_t no, std::int32_t context ) {
    }


    void store_instVar( std::int32_t offset ) {
    }


    void store_instVar_name( SymbolOop name ) {
    }


    void store_classVar( AssociationOop assoc ) {
    }


    void store_classVar_name( SymbolOop name ) {
    }


    void store_global( AssociationOop obj ) {
    }


    void pop() {
    }


    void normal_send( InterpretedInlineCache *ic ) {
    }


    void self_send( InterpretedInlineCache *ic ) {
    }


    void super_send( InterpretedInlineCache *ic ) {
    }


    void double_equal() {
    }


    void double_not_equal() {
    }


    void method_return( std::int32_t nofArgs ) {
    }


    void nonlocal_return( std::int32_t nofArgs ) {
    }


    void allocate_closure( AllocationType type, std::int32_t nofArgs, MethodOop meth );


    void allocate_context( std::int32_t nofTemps, bool forMethod ) {
    }


    void set_self_via_context() {
    }


    void copy_self_into_context() {
    }


    void copy_argument_into_context( std::int32_t argNo, std::int32_t no ) {
    }


    void zap_scope() {
    }


    void predict_primitive_call( PrimitiveDescriptor *pdesc, std::int32_t failure_start ) {
    }


    void float_allocate( std::int32_t nofFloatTemps, std::int32_t nofFloatExprs ) {
    }


    void float_floatify( Floats::Function f, std::int32_t fno ) {
    }


    void float_move( std::int32_t fno, std::int32_t from ) {
    }


    void float_set( std::int32_t fno, DoubleOop value ) {
    }


    void float_nullary( Floats::Function f, std::int32_t fno ) {
    }


    void float_unary( Floats::Function f, std::int32_t fno ) {
    }


    void float_binary( Floats::Function f, std::int32_t fno ) {
    }


    void float_unaryToOop( Floats::Function f, std::int32_t fno ) {
    }


    void float_binaryToOop( Floats::Function f, std::int32_t fno ) {
    }
};


void TransitiveMethodClosure::allocate_closure( AllocationType type, std::int32_t nofArgs, MethodOop meth ) {
    MethodIterator iter( meth, this );
}


void TransitiveMethodClosure::if_node( IfNode *node ) {
    inlined_send( node->selector() );
    MethodIterator iter( node->then_code(), this );
    if ( node->else_code() not_eq nullptr ) {
        MethodIterator iter( node->else_code(), this );
    }
}


void TransitiveMethodClosure::cond_node( CondNode *node ) {
    inlined_send( node->selector() );
    MethodIterator iter( node->expr_code(), this );
}


void TransitiveMethodClosure::while_node( WhileNode *node ) {
    inlined_send( node->selector() );
    MethodIterator iter( node->expr_code(), this );
    if ( node->body_code() not_eq nullptr ) {
        MethodIterator iter( node->body_code(), this );
    }
}


void TransitiveMethodClosure::primitive_call_node( PrimitiveCallNode *node ) {
    inlined_send( node->name() );
    if ( node->failure_code() not_eq nullptr ) {
        MethodIterator iter( node->failure_code(), this );
    }
}


void TransitiveMethodClosure::dll_call_node( DLLCallNode *node ) {
    inlined_send( node->function_name() );
    if ( node->failure_code() not_eq nullptr ) {
        MethodIterator iter( node->failure_code(), this );
    }
}


class ReferencedInstVarNamesClosure : public TransitiveMethodClosure {

private:
    MixinOop _mixin;


    void collect( std::int32_t offset ) {
        SymbolOop name = _mixin->primary_invocation()->klass_part()->inst_var_name_at( offset );
        if ( name )
            _result->append( name );
    }


    void collect( SymbolOop name ) {
        _result->append( name );
    }


public:
    void push_instVar( std::int32_t offset ) {
        collect( offset );
    }


    void push_instVar_name( SymbolOop name ) {
        collect( name );
    }


    void store_instVar( std::int32_t offset ) {
        collect( offset );
    }


    void store_instVar_name( SymbolOop name ) {
        collect( name );
    }


public:
    ReferencedInstVarNamesClosure( std::int32_t size, MixinOop mixin ) {
        _result = new GrowableArray<Oop>( size );
        _mixin  = mixin;
    }


    GrowableArray<Oop> *_result;
};


ObjectArrayOop MethodOopDescriptor::referenced_instance_variable_names( MixinOop mixin ) const {
    ResourceMark                  rm;
    ReferencedInstVarNamesClosure blk( 20, mixin );
    MethodIterator( MethodOop( this ), &blk );
    return oopFactory::new_objArray( blk._result );
}


class ReferencedClassVarNamesClosure : public TransitiveMethodClosure {

private:
    void collect( SymbolOop name ) {
        _result->append( name );
    }


public:
    void push_classVar( AssociationOop assoc ) {
        collect( assoc->key() );
    }


    void push_classVar_name( SymbolOop name ) {
        collect( name );
    }


    void store_classVar( AssociationOop assoc ) {
        collect( assoc->key() );
    }


    void store_classVar_name( SymbolOop name ) {
        collect( name );
    }


public:
    ReferencedClassVarNamesClosure( std::int32_t size ) {
        _result = new GrowableArray<Oop>( size );
    }


    GrowableArray<Oop> *_result;
};


ObjectArrayOop MethodOopDescriptor::referenced_class_variable_names() const {
    ResourceMark                   rm;
    ReferencedClassVarNamesClosure blk( 20 );
    MethodIterator( MethodOop( this ), &blk );
    return oopFactory::new_objArray( blk._result );
}


class ReferencedGlobalsClosure : public TransitiveMethodClosure {
private:
    void collect( SymbolOop selector ) {
        result->append( selector );
    }


public:
    void push_global( AssociationOop obj ) {
        collect( obj->key() );
    }


    void store_global( AssociationOop obj ) {
        collect( obj->key() );
    }


public:
    ReferencedGlobalsClosure( std::int32_t size ) {
        result = new GrowableArray<Oop>( size );
    }


    GrowableArray<Oop> *result;
};


ObjectArrayOop MethodOopDescriptor::referenced_global_names() const {
    ResourceMark             rm;
    ReferencedGlobalsClosure blk( 20 );
    MethodIterator( MethodOop( this ), &blk );
    return oopFactory::new_objArray( blk.result );
}


class SendersClosure : public TransitiveMethodClosure {
private:
    void collect( SymbolOop selector ) {
        result->append( selector );
    }


    void float_op( Floats::Function f ) {
        if ( Floats::has_selector_for( f ) ) {
            collect( Floats::selector_for( f ) );
        }
    }


public:
    void inlined_send( SymbolOop selector ) {
        collect( selector );
    }


    void normal_send( InterpretedInlineCache *ic ) {
        collect( ic->selector() );
    }


    void self_send( InterpretedInlineCache *ic ) {
        collect( ic->selector() );
    }


    void super_send( InterpretedInlineCache *ic ) {
        collect( ic->selector() );
    }


    void double_equal() {
        collect( vmSymbols::double_equal() );
    }


    void double_not_equal() {
        collect( vmSymbols::double_tilde() );
    }


    void float_floatify( Floats::Function f, std::int32_t fno ) {
        float_op( f );
    }


    void float_nullary( Floats::Function f, std::int32_t fno ) {
        float_op( f );
    }


    void float_unary( Floats::Function f, std::int32_t fno ) {
        float_op( f );
    }


    void float_binary( Floats::Function f, std::int32_t fno ) {
        float_op( f );
    }


    void float_unaryToOop( Floats::Function f, std::int32_t fno ) {
        float_op( f );
    }


    void float_binaryToOop( Floats::Function f, std::int32_t fno ) {
        float_op( f );
    }


public:
    SendersClosure( std::int32_t size ) {
        result = new GrowableArray<Oop>( size );
    }


    GrowableArray<Oop> *result;
};


ObjectArrayOop MethodOopDescriptor::senders() const {
    ResourceMark   rm;
    SendersClosure blk( 20 );
    MethodIterator( MethodOop( this ), &blk );
    return oopFactory::new_objArray( blk.result );
}


SymbolOop selectorFrom( Oop method_or_selector ) {

    if ( method_or_selector == nullptr )
        return nullptr;

    if ( method_or_selector->is_symbol() )
        return SymbolOop( method_or_selector );

    if ( not method_or_selector->is_method() )
        return nullptr;

    MethodOop method = MethodOop( method_or_selector );
    if ( method->is_blockMethod() )
        return selectorFrom( MethodOop( method->selector_or_method() ) );

    return method->selector();
}


void stopInSelector( const char *name, MethodOop method ) {
    std::int32_t len      = strlen( name );
    SymbolOop    selector = selectorFrom( method );
    if ( selector == nullptr )
        spdlog::warn( "Selector was nullptr!" );
    else if ( selector->length() == len and strncmp( name, selector->chars(), len ) == 0 ) {
        TraceCanonicalContext = true;
        //method->pretty_print();
        //method->print_codes();
        breakpoint();
    }
}


bool StopInSelector::ignored = false;


SymbolOop className( KlassOop klass ) {
    const std::int32_t class_name_index = 9;

    if ( not klass->is_klass() )
        return nullptr;
    SymbolOop selector = SymbolOop( klass->instVarAt( class_name_index ) );
    if ( selector->is_symbol() )
        return selector;
    if ( not selector->is_klass() )
        return nullptr;
    return className( KlassOop( selector ) );
}


bool selcmp( const char *name, SymbolOop selector ) {
    std::int32_t len = strlen( name );
    if ( selector == nullptr and name == nullptr )
        return true;
    if ( selector == nullptr or not selector->is_symbol() )
        return false;

    return ( ( selector->length() == len and strncmp( name, selector->chars(), len ) == 0 ) );
}


bool shouldStop( const char *name, Oop method_or_selector, const char *class_name, KlassOop klass ) {
    return selcmp( name, selectorFrom( method_or_selector ) ) and selcmp( class_name, className( klass ) );
}


StopInSelector::StopInSelector( const char *class_name, const char *name, KlassOop klass, Oop method_or_selector, bool &fl, bool stop ) :
    enable( shouldStop( name, method_or_selector, class_name, klass ) ), oldFlag( enable ? fl : ignored, true ), stop( stop ) {
    if ( enable and stop )
        breakpoint();
}
