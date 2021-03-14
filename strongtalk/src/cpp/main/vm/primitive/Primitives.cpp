
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/interpreter/CodeIterator.hpp"
#include "vm/memory/OopFactory.hpp"
#include "vm/primitive/BehaviorPrimitives.hpp"
#include "vm/primitive/block_primitives.hpp"
#include "vm/primitive/ByteArrayPrimitives.hpp"
#include "vm/primitive/CallBackPrimitives.hpp"
#include "vm/primitive/DebugPrimitives.hpp"
#include "vm/primitive/DoubleByteArray_primitives.hpp"
#include "vm/primitive/DoubleOopPrimitives.hpp"
#include "vm/primitive/MethodOopPrimitives.hpp"
#include "vm/primitive/MixinOopPrimitives.hpp"
#include "vm/primitive/ObjectArrayPrimitives.hpp"
#include "vm/primitive/OopPrimitives.hpp"
#include "vm/primitive/PrimitiveDescriptor.hpp"
#include "vm/primitive/Primitives.hpp"
#include "vm/primitive/primitives_table.hpp"
#include "vm/primitive/ProcessOopPrimitives.hpp"
#include "vm/primitive/ProxyOopPrimitives.hpp"
#include "vm/primitive/SmallIntegerOopPrimitives.hpp"
#include "vm/primitive/SystemPrimitives.hpp"
#include "vm/runtime/DeltaProcess.hpp"
#include "vm/runtime/Process.hpp"
#include "vm/runtime/ResourceMark.hpp"


PrimitiveDescriptor *Primitives::_new0;
PrimitiveDescriptor *Primitives::_new1;
PrimitiveDescriptor *Primitives::_new2;
PrimitiveDescriptor *Primitives::_new3;
PrimitiveDescriptor *Primitives::_new4;
PrimitiveDescriptor *Primitives::_new5;
PrimitiveDescriptor *Primitives::_new6;
PrimitiveDescriptor *Primitives::_new7;
PrimitiveDescriptor *Primitives::_new8;
PrimitiveDescriptor *Primitives::_new9;
PrimitiveDescriptor *Primitives::_equal;
PrimitiveDescriptor *Primitives::_not_equal;
PrimitiveDescriptor *Primitives::_block_allocate;
PrimitiveDescriptor *Primitives::_block_allocate0;
PrimitiveDescriptor *Primitives::_block_allocate1;
PrimitiveDescriptor *Primitives::_block_allocate2;
PrimitiveDescriptor *Primitives::_context_allocate;
PrimitiveDescriptor *Primitives::_context_allocate0;
PrimitiveDescriptor *Primitives::_context_allocate1;
PrimitiveDescriptor *Primitives::_context_allocate2;


const char *name_from_group( const PrimitiveGroup &group ) {

    switch ( group ) {
        case PrimitiveGroup::IntComparisonPrimitive:
            return "IntComparisonPrimitive / small_int_t";
        case PrimitiveGroup::IntArithmeticPrimitive:
            return "IntArithmeticPrimitive / small_int_t";
        case PrimitiveGroup::FloatComparisonPrimitive:
            return "FloatComparisonPrimitive";
        case PrimitiveGroup::FloatArithmeticPrimitive:
            return "FloatArithmeticPrimitive";
        case PrimitiveGroup::ByteArrayPrimitive:
            return "ByteArrayPrimitive";
        case PrimitiveGroup::DoubleByteArrayPrimitive:
            return "DoubleByteArrayPrimitive";
        case PrimitiveGroup::ObjectArrayPrimitive:
            return "ObjectArrayPrimitive";
        case PrimitiveGroup::BlockPrimitive:
            return "BlockPrimitive";
        case PrimitiveGroup::NormalPrimitive:
            return "NormalPrimitive";
        default: st_fatal( "Unknown primitive group" );
            return nullptr;
    }

}


void Primitives::print_table() {

    //
    SPDLOG_INFO( "                                                   P = needs Delta FP code ------------------------------." );
    SPDLOG_INFO( "                                                   I = internal ----------------------------------------.|" );
    SPDLOG_INFO( "                                                   D = can invoke Delta code --------------------------.||" );
    SPDLOG_INFO( "                                                   C = can be constant folded ------------------------.|||" );
    SPDLOG_INFO( "                                                   N = can perform non-local return -----------------.||||" );
    SPDLOG_INFO( "                                                   W = can walk stack (computed) -------------------.|||||" );
    SPDLOG_INFO( "                                                   S = can scavenge -------------------------------.||||||" );
    SPDLOG_INFO( "                                                   F = has failure block -------------------------.|||||||" );
    SPDLOG_INFO( "                                                   R = has receiver -----------------------------.||||||||" );
    SPDLOG_INFO( "                                                                                                 |||||||||" );
    SPDLOG_INFO( "INDEX  NAME                                                                ARGUMENT COUNT ----.  |||||||||  CATEGORY" );

    //
    for ( std::size_t i = 0; i < size_of_primitive_table; i++ ) {
        PrimitiveDescriptor *e = primitive_table[ i ];
        SPDLOG_INFO( "{:5d}  {:<84}  {:2d}  {}{}{}{}{}{}{}{}{}  {}",
                     i,
                     e->name(),
                     e->number_of_parameters(),
                     e->has_receiver() ? 'R' : '_',
                     e->can_fail() ? 'F' : '_',
                     e->can_scavenge() ? 'S' : '_',
                     e->can_walk_stack() ? 'W' : '_',
                     e->can_perform_NonLocalReturn() ? 'N' : '_',
                     e->can_be_constant_folded() ? 'C' : '_',
                     e->can_invoke_delta() ? 'D' : '_',
                     e->is_internal() ? 'I' : '_',
                     e->needs_delta_fp_code() ? 'P' : '_',
                     name_from_group( e->group() )
        );
    }

}


void Primitives::lookup_and_patch() {

    // get primitive call info
    Frame        f              = DeltaProcess::active()->last_frame();
    CodeIterator it( f.hp() );
    Oop          *selector_addr = it.aligned_oop( 1 );

    SymbolOop sel = SymbolOop( *selector_addr );
    st_assert( sel->isSymbol(), "symbol expected" );

    // do lookup
    PrimitiveDescriptor *pdesc = Primitives::lookup( sel );
    if ( pdesc not_eq nullptr and not pdesc->is_internal() ) {
        // primitive found => patch bytecode & cache
        *f.hp()        = std::uint8_t( ByteCodes::primitive_call_code_for( ByteCodes::Code( *f.hp() ) ) );
        *selector_addr = Oop( pdesc->fn() );
    } else {
        // advance hp so that it points to the next instruction
        it.advance();
        f.set_hp( it.hp() );

        {
            ResourceMark resourceMark;
            // primitive not found => process error
            SPDLOG_INFO( "primitive lookup error" );
            sel->print_value();
            SPDLOG_INFO( " not found" );

        }
        if ( DeltaProcess::active()->is_scheduler() ) {
            ResourceMark resourceMark;
            DeltaProcess::active()->trace_stack();
            st_fatal( "primitive lookup error in scheduler" );

        } else {
            DeltaProcess::active()->suspend( ProcessState::primitive_lookup_error );
        }

        ShouldNotReachHere();
    }
}


void primitives_init() {

    Primitives::initialize();
    PrimitiveDescriptor *prev = nullptr;

    for ( std::size_t index = 0; index < size_of_primitive_table; index++ ) {
        PrimitiveDescriptor *e = primitive_table[ index ];
        SPDLOG_INFO( "primitives-init:  primitive_table: {0:3d}  {}", index, e->name() );
        e->verify();
        if ( prev ) {
            guarantee( strcmp( prev->name(), e->name() ) == -1, "primitive table not sorted" );
        }
    }
    Primitives::clear_counters();
    Primitives::print_table();
}


// For debugging/profiling
void Primitives::clear_counters() {

    SPDLOG_INFO( "primitives-init:  primitive_table: clear counters" );

    BehaviorPrimitives::number_of_calls        = 0;
    ByteArrayPrimitives::number_of_calls       = 0;
    CallBackPrimitives::number_of_calls        = 0;
    DoubleByteArrayPrimitives::number_of_calls = 0;
    DebugPrimitives::number_of_calls           = 0;
    DoubleOopPrimitives::number_of_calls       = 0;
    MethodOopPrimitives::number_of_calls       = 0;
    MixinOopPrimitives::number_of_calls        = 0;
    ObjectArrayPrimitives::number_of_calls     = 0;
    OopPrimitives::number_of_calls             = 0;
    ProcessOopPrimitives::number_of_calls      = 0;
    ProxyOopPrimitives::number_of_calls        = 0;
    SmallIntegerOopPrimitives::number_of_calls = 0;
    SystemPrimitives::number_of_calls          = 0;

}


static void print_calls( const char *name, std::int32_t number_of_calls, std::int32_t *total ) {
    if ( number_of_calls > 0 ) {
        SPDLOG_INFO( "{<16}{}", name, number_of_calls );
        *total = *total + number_of_calls;
    }
}


void Primitives::print_counters() {
    std::int32_t total{ 0 };

    SPDLOG_INFO( "Primitive call counters:" );
    print_calls( "behavior", BehaviorPrimitives::number_of_calls, &total );
    print_calls( "byteArray", ByteArrayPrimitives::number_of_calls, &total );
    print_calls( "callBack", CallBackPrimitives::number_of_calls, &total );
    print_calls( "doubleByteArray", DoubleByteArrayPrimitives::number_of_calls, &total );
    print_calls( "debug", DebugPrimitives::number_of_calls, &total );
    print_calls( "double", DoubleOopPrimitives::number_of_calls, &total );
    print_calls( "method", MethodOopPrimitives::number_of_calls, &total );
    print_calls( "mixin", MixinOopPrimitives::number_of_calls, &total );
    print_calls( "objectArray", ObjectArrayPrimitives::number_of_calls, &total );
    print_calls( "oop", OopPrimitives::number_of_calls, &total );
    print_calls( "process", ProcessOopPrimitives::number_of_calls, &total );
    print_calls( "proxy", ProxyOopPrimitives::number_of_calls, &total );
    print_calls( "small_int_t", SmallIntegerOopPrimitives::number_of_calls, &total );
    print_calls( "system", SystemPrimitives::number_of_calls, &total );
    SPDLOG_INFO( "{<16}{}", "total", total );

}


PrimitiveDescriptor *Primitives::lookup( primitiveFunctionType fn ) {
    for ( std::size_t i = 0; i < size_of_primitive_table; i++ ) {
        PrimitiveDescriptor *e = primitive_table[ i ];
        if ( e->fn() == fn )
            return e;
    }
    return nullptr;
}


PrimitiveDescriptor *Primitives::lookup( const char *selector, std::int32_t selector_length ) {
    std::int32_t first{ 0 };
    std::int32_t last{ size_of_primitive_table };
    SPDLOG_INFO( "selector [{}], selector_length [{}]", selector, selector_length );

    PrimitiveDescriptor *element;
    do {
        std::int32_t middle = first + ( last - first ) / 2;
        element = primitive_table[ middle ];

        std::int32_t sign = element->compare( selector, selector_length );
        if ( sign == -1 ) {
            first = middle + 1;
        } else if ( sign == 1 ) {
            last = middle - 1;
        } else {
            return element;
        }
    } while ( first < last );

    // This should not be an assertion as it is possible to compile a reference to a non-existent primitive.
    // For an example, see ProcessPrimitiveLookupError>>provoke()
    // In such a case the lookup should fail and signal a PrimitiveLookupError - slr 24/09/2008
//    st_assert( first == last, "check for one element" );

    element = primitive_table[ first ];

    return element->compare( selector, selector_length ) == 0 ? element : nullptr;
}


PrimitiveDescriptor *Primitives::verified_lookup( const char *selector ) {

    std::int32_t        selector_length = strlen( selector );
    PrimitiveDescriptor *result         = lookup( selector, selector_length );
    if ( result == nullptr ) {
        SPDLOG_INFO( "verified primitive lookup failed: selector [{}]", selector );
        st_fatal( "aborted" );
    }
    return result;
}


void Primitives::initialize() {

    _new0 = verified_lookup( "primitiveNew0:ifFail:" );
    _new1 = verified_lookup( "primitiveNew1:ifFail:" );

    _new2 = verified_lookup( "primitiveNew2:ifFail:" );
    _new3 = verified_lookup( "primitiveNew3:ifFail:" );
    _new4 = verified_lookup( "primitiveNew4:ifFail:" );
    _new5 = verified_lookup( "primitiveNew5:ifFail:" );
    _new6 = verified_lookup( "primitiveNew6:ifFail:" );
    _new7 = verified_lookup( "primitiveNew7:ifFail:" );
    _new8 = verified_lookup( "primitiveNew8:ifFail:" );
    _new9 = verified_lookup( "primitiveNew9:ifFail:" );

    _equal     = verified_lookup( "primitiveEqual:" );
    _not_equal = verified_lookup( "primitiveNotEqual:" );

    _block_allocate  = verified_lookup( "primitiveCompiledBlockAllocate:" );
    _block_allocate0 = verified_lookup( "primitiveCompiledBlockAllocate0" );
    _block_allocate1 = verified_lookup( "primitiveCompiledBlockAllocate1" );
    _block_allocate2 = verified_lookup( "primitiveCompiledBlockAllocate2" );

    _context_allocate  = verified_lookup( "primitiveCompiledContextAllocate:" );
    _context_allocate0 = verified_lookup( "primitiveCompiledContextAllocate0" );
    _context_allocate1 = verified_lookup( "primitiveCompiledContextAllocate1" );
    _context_allocate2 = verified_lookup( "primitiveCompiledContextAllocate2" );

}


void Primitives::patch( const char *name, const char *entry_point ) {
    SPDLOG_INFO( "name [{}], entry_point [0x{0:x}]", name, entry_point );
    st_assert( entry_point, "just checking" );
    PrimitiveDescriptor *desc = verified_lookup( name );
    desc->_fn = (primitiveFunctionType) entry_point;
}
