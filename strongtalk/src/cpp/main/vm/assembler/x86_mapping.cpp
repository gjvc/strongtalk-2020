
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/assembler/x86_mapping.hpp"
#include "vm/system/asserts.hpp"
#include "vm/compiler/Compiler.hpp"
#include "vm/oops/BlockClosureOopDescriptor.hpp"
#include "vm/oops/ContextOopDescriptor.hpp"



// stack mapping

void Mapping::initialize() {
    _localRegisters[ 0 ] = asLocation( eax );
    _localRegisters[ 1 ] = asLocation( edi );
    _localRegisters[ 2 ] = asLocation( esi );

    for ( std::size_t i = 0; i < REGISTER_COUNT; i++ )
        _localRegisterIndex[ i ] = -1;

    for ( std::size_t i = 0; i < nofLocalRegisters; i++ )
        _localRegisterIndex[ _localRegisters[ i ].number() ] = i;

    for ( std::size_t i = 0; i < nofLocalRegisters; i++ ) {
        Register r = asRegister( _localRegisters[ i ] );
        st_assert( ( r not_eq temp1 ) and ( r not_eq temp2 ) and ( r not_eq temp3 ), "local registers must be disjoint from temporary registers" );
    }
}


// local registers
//Location Mapping::_localRegisters[nofLocalRegisters + 1]; // allow for 0 local registers

// C++ won't compile array with 0 elements
//std::int32_t      Mapping::_localRegisterIndex[REGISTER_COUNT + 1];

std::array<Location, nofLocalRegisters>      Mapping::_localRegisters;      //
std::array<std::int32_t, REGISTER_COUNT>     Mapping::_localRegisterIndex;  //


Location Mapping::localRegister( std::int32_t i ) {
    st_assert( 0 <= i and i < nofLocalRegisters, "illegal local register index" );
    return _localRegisters[ i ];
}


std::int32_t Mapping::localRegisterIndex( const Location &l ) {
    st_assert( 0 <= l.number() and l.number() < REGISTER_COUNT, "illegal local register" );
    std::int32_t res = _localRegisterIndex[ l.number() ];
    st_assert( res >= 0, "not a local register" );
    st_assert( localRegister( res ) == l, "incorrect mapping" );
    return res;
}


// parameter passing
Location Mapping::incomingArg( std::int32_t i, std::int32_t nofArgs ) {
    st_assert( ( 0 <= i ) and ( i < nofArgs ), "illegal arg number" );
    return Location::stackLocation( nofArgs - i + 1 );
}


Location Mapping::outgoingArg( std::int32_t i, std::int32_t nofArgs ) {
    st_assert( ( 0 <= i ) and ( i < nofArgs ), "illegal arg number" );
    return Location::TOP_OF_STACK;
}


// stack allocation (Note: offsets are always in oops!)
Location Mapping::localTemporary( std::int32_t i ) {
    st_assert( i >= 0, "illegal temporary number" );
    std::int32_t floats = theCompiler->totalNofFloatTemporaries();
    std::int32_t offset = ( floats > 0 ? first_float_offset - floats * ( SIZEOF_FLOAT / OOP_SIZE ) : first_temp_offset ) - i;
    return Location::stackLocation( offset );
}


std::int32_t Mapping::localTemporaryIndex( const Location &l ) {
    std::int32_t floats = theCompiler->totalNofFloatTemporaries();
    std::int32_t i      = ( floats > 0 ? first_float_offset - floats * ( SIZEOF_FLOAT / OOP_SIZE ) : first_temp_offset ) - l.offset();
    st_assert( localTemporary( i ) == l, "incorrect mapping" );
    return i;
}


Location Mapping::floatTemporary( std::int32_t scope_id, std::int32_t i ) {
    InlinedScope *scope = theCompiler->scopes->at( scope_id );
    st_assert( scope->firstFloatIndex() >= 0, "firstFloatIndex not computed yet" );
    //
    // Floats must be 8byte aligned in order to a void massive time penalties.
    // They're accessed via a base register which holds the 8byte aligned value of ebp.
    // The byte-offset of the first float must be a multiple of FLOAT_SIZE
    // => need an extra filler word besides the Floats::magic value.
    //
    // base - 1: Floats::magic
    // base - 2: filler word - undefined
    // base - 3: (global) float 0 hi word
    // base - 4: (global) float 0 lo word
    //
    st_assert( SIZEOF_FLOAT == 2 * OOP_SIZE, "check this code" );
    Location loc = Location::stackLocation( first_float_offset - ( scope->firstFloatIndex() + i ) * ( SIZEOF_FLOAT / OOP_SIZE ) );
    st_assert( ( loc.offset() * OOP_SIZE ) % SIZEOF_FLOAT == 0, "offset is not correctly aligned" );
    return loc;
}


// context temporaries
Location Mapping::contextTemporary( std::int32_t contextNo, std::int32_t i, std::int32_t scope_offset ) {
    st_assert( ( 0 <= contextNo ) and ( 0 <= i ), "illegal context or temporary no" );
    return Location::compiledContextLocation( contextNo, i, scope_offset );
}


Location *Mapping::new_contextTemporary( std::int32_t contextNo, std::int32_t i, std::int32_t scope_id ) {
    st_assert( ( 0 <= contextNo ) and ( 0 <= i ), "illegal context or temporary no" );
    return new Location( LocationMode::CONTEXT_LOCATIION_1, contextNo, i, scope_id );
}


std::int32_t Mapping::contextOffset( std::int32_t tempNo ) {
    // computes the byte offset within the context object
    return tempNo * OOP_SIZE + ContextOopDescriptor::temp0_byte_offset();
}


// predicates
bool Mapping::isNormalTemporary( Location loc ) {
    st_assert( not loc.isFloatLocation(), "must have been converted into STACK_LOCATION by register allocation" );
    return loc.isStackLocation() and not isFloatTemporary( loc );
}


bool Mapping::isFloatTemporary( Location loc ) {
    st_assert( not loc.isFloatLocation(), "must have been converted into STACK_LOCATION by register allocation" );
    if ( not loc.isStackLocation() ) {
        return false;
    }
    std::int32_t floats = theCompiler->totalNofFloatTemporaries();
    std::int32_t offset = loc.offset();
    return floats > 0 and first_float_offset + 2 >= offset and offset > first_float_offset - floats * ( SIZEOF_FLOAT / OOP_SIZE );
}


// helper functions for code generation
void Mapping::load( const Location &src, const Register &dst ) {

    switch ( src.mode() ) {
        case LocationMode::SPECIAL_LOCATION: {
            if ( src == Location::RESULT_OF_NON_LOCAL_RETURN ) {
                // treat as NonLocalReturn_result_reg
                if ( NonLocalReturn_result_reg not_eq dst )
                    theMacroAssembler->movl( dst, NonLocalReturn_result_reg );
            } else {
                ShouldNotReachHere();
            }
            break;
        }
        case LocationMode::REGISTER_LOCATION: {
            Register s = asRegister( src );
            if ( s not_eq dst )
                theMacroAssembler->movl( dst, s );
            break;
        }
        case LocationMode::STACK_LOCATION: {
            st_assert( isNormalTemporary( src ), "must be a normal temporary location" );
            theMacroAssembler->Load( ebp, src.offset() * OOP_SIZE, dst );
            break;
        }
        case LocationMode::CONTEXT_LOCATIION_1: {
            PseudoRegister *base = theCompiler->contextList->at( src.contextNo() )->context();
            load( base->_location, dst );
            theMacroAssembler->Load( dst, contextOffset( src.tempNo() ), dst );
            break;
        }
        default: {
            ShouldNotReachHere();
            break;
        }
    }
}


void Mapping::store( Register src, const Location &dst, const Register &temp1, const Register &temp2, bool needsStoreCheck ) {

    st_assert( src not_eq temp1 and src not_eq temp2 and temp1 not_eq temp2, "registers must be different" );
    switch ( dst.mode() ) {
        case LocationMode::SPECIAL_LOCATION: {
            if ( dst == Location::TOP_OF_STACK ) {
                theMacroAssembler->pushl( src );
            } else {
                ShouldNotReachHere();
            }
            break;
        }
        case LocationMode::REGISTER_LOCATION: {
            Register d = asRegister( dst );
            if ( d not_eq src )
                theMacroAssembler->movl( d, src );
            break;
        }
        case LocationMode::STACK_LOCATION: {
            st_assert( isNormalTemporary( dst ), "must be a normal temporary location" );
            theMacroAssembler->Store( src, ebp, dst.offset() * OOP_SIZE );
            break;
        }
        case LocationMode::CONTEXT_LOCATIION_1: {
            PseudoRegister *base = theCompiler->contextList->at( dst.contextNo() )->context();
            load( base->_location, temp1 );
            theMacroAssembler->Store( src, temp1, contextOffset( dst.tempNo() ) );
            if ( needsStoreCheck )
                theMacroAssembler->store_check( temp1, temp2 );
            break;
        }
        default: {
            ShouldNotReachHere();
            break;
        }
    }
}


void Mapping::storeO( Oop obj, const Location &dst, const Register &temp1, const Register &temp2, bool needsStoreCheck ) {

    st_assert( temp1 not_eq temp2, "registers must be different" );
    switch ( dst.mode() ) {
        case LocationMode::SPECIAL_LOCATION: {
            if ( dst == Location::TOP_OF_STACK ) {
                theMacroAssembler->pushl( obj );
            } else {
                ShouldNotReachHere();
            }
            break;
        }
        case LocationMode::REGISTER_LOCATION: {
            theMacroAssembler->movl( asRegister( dst ), obj );
            break;
        }
        case LocationMode::STACK_LOCATION: {
            st_assert( isNormalTemporary( dst ), "must be a normal temporary location" );
            theMacroAssembler->movl( Address( ebp, dst.offset() * OOP_SIZE ), obj );
            break;
        }
        case LocationMode::CONTEXT_LOCATIION_1: {
            PseudoRegister *base = theCompiler->contextList->at( dst.contextNo() )->context();
            load( base->_location, temp1 );
            theMacroAssembler->movl( Address( temp1, contextOffset( dst.tempNo() ) ), obj );
            if ( needsStoreCheck )
                theMacroAssembler->store_check( temp1, temp2 );
            break;
        }
        default: {
            ShouldNotReachHere();
            break;
        }
    }
}


void Mapping::fload( const Location &src, const Register &base ) {

    if ( src == Location::TOP_OF_FLOAT_STACK ) {
        if ( UseFPUStack ) {
            // nothing to do, value is on stack already
        } else {
            ShouldNotReachHere();
        }
    } else {
        st_assert( isFloatTemporary( src ), "must be a float location" );
        st_assert( ( src.offset() * OOP_SIZE ) % SIZEOF_FLOAT == 0, "float is not aligned" );
        theMacroAssembler->fld_d( Address( base, src.offset() * OOP_SIZE ) );
    }
}


void Mapping::fstore( const Location &dst, const Register &base ) {

    if ( dst == Location::TOP_OF_FLOAT_STACK ) {
        if ( UseFPUStack ) {
            // nothing to do, value is on stack already
        } else {
            ShouldNotReachHere();
        }
    } else {
        st_assert( isFloatTemporary( dst ), "must be a float location" );
        st_assert( ( dst.offset() * OOP_SIZE ) % SIZEOF_FLOAT == 0, "float is not aligned" );
        theMacroAssembler->fstp_d( Address( base, dst.offset() * OOP_SIZE ) );
    }
}


void mapping_init() {
    SPDLOG_INFO( "system-init:  mapping_init" );

    Mapping::initialize();
}
