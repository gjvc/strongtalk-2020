//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/primitives/primitives.hpp"
#include "vm/memory/oopFactory.hpp"
#include "vm/compiler/Expression.hpp"
#include "vm/runtime/Process.hpp"
#include "vm/interpreter/CodeIterator.hpp"
#include "vm/primitives/behavior_primitives.hpp"
#include "vm/primitives/block_primitives.hpp"
#include "vm/primitives/byteArray_primitives.hpp"
#include "vm/primitives/callBack_primitives.hpp"
#include "vm/primitives/dByteArray_primitives.hpp"
#include "vm/primitives/debug_primitives.hpp"
#include "vm/primitives/double_primitives.hpp"
#include "vm/primitives/method_primitives.hpp"
#include "vm/primitives/mixin_primitives.hpp"
#include "vm/primitives/objArray_primitives.hpp"
#include "vm/primitives/oop_primitives.hpp"
#include "vm/primitives/process_primitives.hpp"
#include "vm/primitives/proxy_primitives.hpp"
#include "vm/primitives/smi_primitives.hpp"
#include "vm/primitives/system_primitives.hpp"
#include "vm/runtime/ResourceMark.hpp"


// The primitive_table is generated from prims.src.
// The output has the following format:
//   static int size_of_primitive_table
//   static PrimitiveDescriptor* primitive_table;
#include "vm/primitives/primitives_table.cpp"


// the typedefs below are necessary to ensure that args are passed correctly when calling a primitive through a function pointer
// NB: there's no general n-argument primitive because some calling conventions can't handle vararg functions

typedef Oop (__CALLING_CONVENTION *prim_fntype0)();

typedef Oop (__CALLING_CONVENTION *prim_fntype1)( Oop );

typedef Oop (__CALLING_CONVENTION *prim_fntype2)( Oop, Oop );

typedef Oop (__CALLING_CONVENTION *prim_fntype3)( Oop, Oop, Oop );

typedef Oop (__CALLING_CONVENTION *prim_fntype4)( Oop, Oop, Oop, Oop );

typedef Oop (__CALLING_CONVENTION *prim_fntype5)( Oop, Oop, Oop, Oop, Oop );

typedef Oop (__CALLING_CONVENTION *prim_fntype6)( Oop, Oop, Oop, Oop, Oop, Oop );

typedef Oop (__CALLING_CONVENTION *prim_fntype7)( Oop, Oop, Oop, Oop, Oop, Oop, Oop );

typedef Oop (__CALLING_CONVENTION *prim_fntype8)( Oop, Oop, Oop, Oop, Oop, Oop, Oop, Oop );

typedef Oop (__CALLING_CONVENTION *prim_fntype9)( Oop, Oop, Oop, Oop, Oop, Oop, Oop, Oop, Oop );


Oop PrimitiveDescriptor::eval( Oop *a ) {

    const bool_t reverseArgs = true;    // change this when changing primitive calling convention
    Oop          res;
    int          ebx_on_stack;


// %hack: see below
#ifndef __GNUC__
    __asm mov ebx_on_stack, ebx
#else
    __asm__("pushl %%eax;"
            "movl %%ebx, %%eax;"
            "movl %%eax, %0;"
            "popl %%eax;"
    : "=a"(ebx_on_stack));
#endif
    if ( reverseArgs ) {
        switch ( number_of_parameters() ) {
            case 0:
                res = ( (prim_fntype0) _fn )();
                break;
            case 1:
                res = ( (prim_fntype1) _fn )( a[ 0 ] );
                break;
            case 2:
                res = ( (prim_fntype2) _fn )( a[ 1 ], a[ 0 ] );
                break;
            case 3:
                res = ( (prim_fntype3) _fn )( a[ 2 ], a[ 1 ], a[ 0 ] );
                break;
            case 4:
                res = ( (prim_fntype4) _fn )( a[ 3 ], a[ 2 ], a[ 1 ], a[ 0 ] );
                break;
            case 5:
                res = ( (prim_fntype5) _fn )( a[ 4 ], a[ 3 ], a[ 2 ], a[ 1 ], a[ 0 ] );
                break;
            case 6:
                res = ( (prim_fntype6) _fn )( a[ 5 ], a[ 4 ], a[ 3 ], a[ 2 ], a[ 1 ], a[ 0 ] );
                break;
            case 7:
                res = ( (prim_fntype7) _fn )( a[ 6 ], a[ 5 ], a[ 4 ], a[ 3 ], a[ 2 ], a[ 1 ], a[ 0 ] );
                break;
            case 8:
                res = ( (prim_fntype8) _fn )( a[ 7 ], a[ 6 ], a[ 5 ], a[ 4 ], a[ 3 ], a[ 2 ], a[ 1 ], a[ 0 ] );
                break;
            case 9:
                res = ( (prim_fntype9) _fn )( a[ 8 ], a[ 7 ], a[ 6 ], a[ 5 ], a[ 4 ], a[ 3 ], a[ 2 ], a[ 1 ], a[ 0 ] );
                break;
            default: ShouldNotReachHere();
        }
    } else {
        switch ( number_of_parameters() ) {
            case 0:
                res = ( (prim_fntype0) _fn )();
                break;
            case 1:
                res = ( (prim_fntype1) _fn )( a[ 0 ] );
                break;
            case 2:
                res = ( (prim_fntype2) _fn )( a[ 0 ], a[ 1 ] );
                break;
            case 3:
                res = ( (prim_fntype3) _fn )( a[ 0 ], a[ 1 ], a[ 2 ] );
                break;
            case 4:
                res = ( (prim_fntype4) _fn )( a[ 0 ], a[ 1 ], a[ 2 ], a[ 3 ] );
                break;
            case 5:
                res = ( (prim_fntype5) _fn )( a[ 0 ], a[ 1 ], a[ 2 ], a[ 3 ], a[ 4 ] );
                break;
            case 6:
                res = ( (prim_fntype6) _fn )( a[ 0 ], a[ 1 ], a[ 2 ], a[ 3 ], a[ 4 ], a[ 5 ] );
                break;
            case 7:
                res = ( (prim_fntype7) _fn )( a[ 0 ], a[ 1 ], a[ 2 ], a[ 3 ], a[ 4 ], a[ 5 ], a[ 6 ] );
                break;
            case 8:
                res = ( (prim_fntype8) _fn )( a[ 0 ], a[ 1 ], a[ 2 ], a[ 3 ], a[ 4 ], a[ 5 ], a[ 6 ], a[ 7 ] );
                break;
            case 9:
                res = ( (prim_fntype9) _fn )( a[ 0 ], a[ 1 ], a[ 2 ], a[ 3 ], a[ 4 ], a[ 5 ], a[ 6 ], a[ 7 ], a[ 8 ] );
                break;
            default: ShouldNotReachHere();
        }
    }

    // %hack: some primitives alter EBX and crash the compiler's constant propagation
    int ebx_now;
#ifndef __GNUC__
    __asm mov ebx_now, ebx
    __asm mov ebx, ebx_on_stack
#else
    __asm__("pushl %%eax;"
            "movl %%ebx, %%eax;"
            "movl %%eax, %0;"
            "movl %1, %%eax;"
            "movl %%eax, %%ebx;"
            "popl %%eax;" : "=a"(ebx_now) : "a"(ebx_on_stack));
#endif

    if ( ebx_now not_eq ebx_on_stack ) {
        _console->print_cr( "ebx changed (%X -> %X) in :", ebx_on_stack, ebx_now );
        print();
    }

    return res;
}


void Primitives::print_table() {

    _console->print_cr( "Primitive table:" );
    for ( std::size_t i = 0; i < size_of_primitive_table; i++ ) {
        PrimitiveDescriptor *e = primitive_table[ i ];
        _console->print( "%3d ", i );
        e->print();
    }
    _console->print_cr( " - format: <index> <name> <number_of_parameters> <flags> [category]" );
    _console->print_cr( "    flags:  R = has receiver            F = has failure block" );
    _console->print_cr( "            S = can scavenge            N = can perform NonLocalReturn" );
    _console->print_cr( "            C = can be constant folded  D = can invoke Delta code" );
    _console->print_cr( "            I = internal                P = needs Delta fp code" );
    _console->print_cr( "            W = can walk stack (computed)" );
}


bool_t PrimitiveDescriptor::can_walk_stack() const {
    return can_scavenge() or can_invoke_delta() or can_perform_NonLocalReturn();
}


SymbolOop PrimitiveDescriptor::selector() const {
    return oopFactory::new_symbol( name() );
}


void PrimitiveDescriptor::print() {
    _console->print( "%48s %d %s%s%s%s%s%s%s%s%s", name(), number_of_parameters(), has_receiver() ? "R" : "_", can_fail() ? "F" : "_", can_scavenge() ? "S" : "_", can_walk_stack() ? "W" : "_", can_perform_NonLocalReturn() ? "N" : "_", can_be_constant_folded() ? "C" : "_", can_invoke_delta() ? "D" : "_", is_internal() ? "I" : "_", needs_delta_fp_code() ? "P" : "_" );
    switch ( group() ) {
        case PrimitiveGroup::IntComparisonPrimitive:
            _console->print( ", smi_t compare" );
            break;
        case PrimitiveGroup::IntArithmeticPrimitive:
            _console->print( ", smi_t arith" );
            break;
        case PrimitiveGroup::FloatComparisonPrimitive:
            _console->print( ", double compare" );
            break;
        case PrimitiveGroup::FloatArithmeticPrimitive:
            _console->print( ", double arith" );
            break;
        case PrimitiveGroup::ByteArrayPrimitive:
            _console->print( ", byte array op." );
            break;
        case PrimitiveGroup::DoubleByteArrayPrimitive:
            _console->print( ", double-byte array op." );
            break;
        case PrimitiveGroup::ObjArrayPrimitive:
            _console->print( ", array op." );
            break;
        case PrimitiveGroup::BlockPrimitive:
            _console->print( ", block/context" );
            break;
        case PrimitiveGroup::NormalPrimitive:
            break;
        default: st_fatal( "Unknown primitive group" );
    }
    _console->cr();
}


const char *PrimitiveDescriptor::parameter_type( int index ) const {
    st_assert( ( 0 <= index ) and ( index < number_of_parameters() ), "illegal parameter index" );
    return _types[ 1 + index ];
}


const char *PrimitiveDescriptor::return_type() const {
    return _types[ 0 ];
}


Expression *PrimitiveDescriptor::convertToKlass( const char *type, PseudoRegister *p, Node *n ) const {
    if ( 0 == strcmp( type, "SmallInteger" ) )
        return new KlassExpression( Universe::smiKlassObj(), p, n );
    if ( 0 == strcmp( type, "Double" ) )
        return new KlassExpression( Universe::doubleKlassObj(), p, n );
    if ( 0 == strcmp( type, "Float" ) )
        return new KlassExpression( Universe::doubleKlassObj(), p, n );
    if ( 0 == strcmp( type, "Symbol" ) )
        return new KlassExpression( Universe::symbolKlassObj(), p, n );
    if ( 0 == strcmp( type, "Boolean" ) ) {
        // NB: set expression node to nullptr, not n -- MergeExpression cannot be split
        Expression *t = new ConstantExpression( Universe::trueObj(), p, nullptr );
        Expression *f = new ConstantExpression( Universe::falseObj(), p, nullptr );
        return new MergeExpression( t, f, p, nullptr );
    }

    // should extend:
    // - looking up klassName in global class dictionary would cover many other cases
    // - for these, need to agree what prim info means: "exact match" or "any subclass of"
    // fix this later  -Urs 11/95
    return nullptr;
}


Expression *PrimitiveDescriptor::parameter_klass( int index, PseudoRegister *p, Node *n ) const {
    return convertToKlass( parameter_type( index ), p, n );
}


Expression *PrimitiveDescriptor::return_klass( PseudoRegister *p, Node *n ) const {
    return convertToKlass( return_type(), p, n );
}


void PrimitiveDescriptor::error( const char *msg ) {
    print();
    ::error( msg );
}


void PrimitiveDescriptor::verify() {
    bool_t ok = true;
    if ( can_invoke_delta() ) {
        if ( not can_scavenge() )
            error( "canInvokeDelta implies canScavenge" );
        if ( not can_walk_stack() )
            error( "canInvokeDelta implies can_walk_stack" );
        if ( can_be_constant_folded() )
            error( "canInvokeDelta implies not canbeConstantFolded" );
        if ( not can_perform_NonLocalReturn() )
            error( "canInvokeDelta implies canPerformNonLocalReturn" );
    }
    if ( can_be_constant_folded() ) {
        if ( can_perform_NonLocalReturn() )
            error( "canbeConstantFolded implies not canPerformNonLocalReturn" );
    }
    if ( group() == PrimitiveGroup::BlockPrimitive ) {
        if ( not can_walk_stack() )
            error( "blocks must have can_walk_stack" );
    }
}


int PrimitiveDescriptor::compare( const char *str, int len ) {
    int src_len = strlen( name() );
    int sign    = strncmp( name(), str, min( src_len, len ) );
    // if (sign not_eq 0 or src_len == len) return sign;
    if ( sign not_eq 0 )
        return sign < 0 ? -1 : 1;
    if ( src_len == len )
        return 0;
    return src_len < len ? -1 : 1;
}


PrimitiveDescriptor *Primitives::lookup( const char *selector, int len ) {
    int first = 0;
    int last  = size_of_primitive_table;

    PrimitiveDescriptor *element;
    do {
        int middle = first + ( last - first ) / 2;
        element = primitive_table[ middle ];
        int sign = element->compare( selector, len );
        if ( sign == -1 )
            first = middle + 1;
        else if ( sign == 1 )
            last = middle - 1;
        else
            return element;
    } while ( first < last );

    // This should not be an assertion as it is possible to compile Aa reference to a non-existent primitive.
    // For an example, see ProcessPrimitiveLookupError>>provoke()
    // In such a case the lookup should fail and signal a PrimitiveLookupError - slr 24/09/2008
    // st_assert(first == last, "check for one element");

    element = primitive_table[ first ];

    return element->compare( selector, len ) == 0 ? element : nullptr;
}


PrimitiveDescriptor *Primitives::lookup( const char *selector ) {
    return lookup( selector, strlen( selector ) );
}


PrimitiveDescriptor *Primitives::lookup( primitiveFunctionType fn ) {
    for ( std::size_t i = 0; i < size_of_primitive_table; i++ ) {
        PrimitiveDescriptor *e = primitive_table[ i ];
        if ( e->fn() == fn )
            return e;
    }
    return nullptr;
}


void Primitives::lookup_and_patch() {

    // get primitive call info
    Frame        f = DeltaProcess::active()->last_frame();
    CodeIterator it( f.hp() );
    Oop *selector_addr = it.aligned_oop( 1 );

    SymbolOop sel = SymbolOop( *selector_addr );
    st_assert( sel->is_symbol(), "symbol expected" );

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
            _console->print_cr( "primitive lookup error" );
            sel->print_value();
            _console->print_cr( " not found" );

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
    _console->print_cr( "%%system-init:  primitives_init" );

    Primitives::initialize();
    PrimitiveDescriptor *prev = nullptr;

    for ( std::size_t index = 0; index < size_of_primitive_table; index++ ) {
        PrimitiveDescriptor *e = primitive_table[ index ];
        e->verify();
        if ( prev ) {
            guarantee( strcmp( prev->name(), e->name() ) == -1, "primitive table not sorted" );
        }
    }
    Primitives::clear_counters();
}


// For debugging/profiling
void Primitives::clear_counters() {

    behaviorPrimitives::_numberOfCalls         = 0;
    byteArrayPrimitives::number_of_calls       = 0;
    callBackPrimitives::number_of_calls        = 0;
    doubleByteArrayPrimitives::number_of_calls = 0;
    debugPrimitives::number_of_calls           = 0;
    doubleOopPrimitives::number_of_calls       = 0;
    methodOopPrimitives::number_of_calls       = 0;
    mixinOopPrimitives::number_of_calls        = 0;
    objArrayPrimitives::number_of_calls        = 0;
    oopPrimitives::number_of_calls             = 0;
    processOopPrimitives::number_of_calls      = 0;
    proxyOopPrimitives::number_of_calls        = 0;
    smiOopPrimitives::number_of_calls          = 0;
    SystemPrimitives::number_of_calls          = 0;

}


static void print_calls( const char *name, int inc, int *total ) {
    if ( inc > 0 ) {
        lprintf( " %s:\t%6d\n", name, inc );
        *total = *total + inc;
    }
}


void Primitives::print_counters() {
    int total = 0;
    lprintf( "Primitive call counters:\n" );
    print_calls( "Behavior", behaviorPrimitives::_numberOfCalls, &total );
    print_calls( "byteArray", byteArrayPrimitives::number_of_calls, &total );
    print_calls( "callBack", callBackPrimitives::number_of_calls, &total );
    print_calls( "doubleByteArray", doubleByteArrayPrimitives::number_of_calls, &total );
    print_calls( "debug", debugPrimitives::number_of_calls, &total );
    print_calls( "double", doubleOopPrimitives::number_of_calls, &total );
    print_calls( "method", methodOopPrimitives::number_of_calls, &total );
    print_calls( "mixin", mixinOopPrimitives::number_of_calls, &total );
    print_calls( "objArray", objArrayPrimitives::number_of_calls, &total );
    print_calls( "Oop", oopPrimitives::number_of_calls, &total );
    print_calls( "process", processOopPrimitives::number_of_calls, &total );
    print_calls( "proxy", proxyOopPrimitives::number_of_calls, &total );
    print_calls( "smi_t", smiOopPrimitives::number_of_calls, &total );
    print_calls( "system", SystemPrimitives::number_of_calls, &total );
    lprintf( "Total:\t%6d\n", total );

}


PrimitiveDescriptor *InterpretedPrimitiveCache::pdesc() const {

    CodeIterator c( hp() );
    switch ( c.code() ) {
        case ByteCodes::Code::prim_call:
        case ByteCodes::Code::primitive_call_failure:
        case ByteCodes::Code::primitive_call_self:
        case ByteCodes::Code::primitive_call_self_failure:
            return Primitives::lookup( (primitiveFunctionType) c.word_at( 1 ) );

        case ByteCodes::Code::primitive_call_lookup:
        case ByteCodes::Code::primitive_call_failure_lookup:
        case ByteCodes::Code::primitive_call_self_lookup:
        case ByteCodes::Code::primitive_call_self_failure_lookup:
            return Primitives::lookup( SymbolOop( c.oop_at( 1 ) ) );

        default: st_fatal( "Wrong bytecode" );
    }
    return nullptr;
}


bool_t InterpretedPrimitiveCache::has_receiver() const {
    CodeIterator c( hp() );
    switch ( c.code() ) {
        case ByteCodes::Code::primitive_call_self:
        case ByteCodes::Code::primitive_call_self_failure:
        case ByteCodes::Code::primitive_call_self_lookup:
        case ByteCodes::Code::primitive_call_self_failure_lookup:
            return true;

        case ByteCodes::Code::prim_call:
        case ByteCodes::Code::primitive_call_failure:
        case ByteCodes::Code::primitive_call_lookup:
        case ByteCodes::Code::primitive_call_failure_lookup:
            return false;

        default: st_fatal( "Wrong bytecode" );
    }
    return false;
}


SymbolOop InterpretedPrimitiveCache::name() const {
    CodeIterator c( hp() );
    switch ( c.code() ) {
        case ByteCodes::Code::prim_call:
        case ByteCodes::Code::primitive_call_failure:
        case ByteCodes::Code::primitive_call_self:
        case ByteCodes::Code::primitive_call_self_failure:
            return Primitives::lookup( (primitiveFunctionType) c.word_at( 1 ) )->selector();

        case ByteCodes::Code::primitive_call_lookup:
        case ByteCodes::Code::primitive_call_failure_lookup:
        case ByteCodes::Code::primitive_call_self_lookup:
        case ByteCodes::Code::primitive_call_self_failure_lookup:
            return SymbolOop( c.oop_at( 1 ) );

        default: st_fatal( "Wrong bytecode" );
    }
    return nullptr;
}


int InterpretedPrimitiveCache::number_of_parameters() const {
    int result = name()->number_of_arguments() + ( has_receiver() ? 1 : 0 ) - ( has_failure_code() ? 1 : 0 );
    st_assert( pdesc() == nullptr or pdesc()->number_of_parameters() == result, "checking result" );
    return result;
}


bool_t InterpretedPrimitiveCache::has_failure_code() const {
    CodeIterator c( hp() );
    switch ( c.code() ) {
        case ByteCodes::Code::primitive_call_failure:
        case ByteCodes::Code::primitive_call_failure_lookup:
        case ByteCodes::Code::primitive_call_self_failure:
        case ByteCodes::Code::primitive_call_self_failure_lookup:
            return true;

        case ByteCodes::Code::prim_call:
        case ByteCodes::Code::primitive_call_lookup:
        case ByteCodes::Code::primitive_call_self_lookup:
        case ByteCodes::Code::primitive_call_self:
            return false;

        default: st_fatal( "Wrong bytecode" );
    }
    return false;
}


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


PrimitiveDescriptor *Primitives::verified_lookup( const char *selector ) {
    PrimitiveDescriptor *result = lookup( selector );
    if ( result == nullptr ) {
        _console->print_cr( "Verified primitive lookup failed" );
        _console->print_cr( " selector = %s", selector );
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
    _console->print_cr( "%%primitives-init:  name [%s], entry_point [0x%0x]", name, entry_point );
    st_assert( entry_point, "just checking" );
    PrimitiveDescriptor *pdesc = verified_lookup( name );
    pdesc->_fn = (primitiveFunctionType) entry_point;
}
