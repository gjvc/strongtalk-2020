
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/assembler/Register.hpp"
#include "vm/assembler/Location.hpp"
#include "vm/assembler/x86_registers.hpp"
#include <array>

// Register usage
constexpr int nofArgRegisters   = 0;            // max. number of arguments (excl. receiver) passed in registers
constexpr int nofLocalRegisters = 3;            // max. number of temporaries allocated in registers

// Temporaries on the stack
constexpr int first_temp_offset  = -1;          // offset of first temporary relative to ebp if there are no floats
constexpr int first_float_offset = -4;          // offset of first float relative to 8byte aligned ebp value (= base)


// Mapping specifies the x86 architecture specific constants and code sequences that are valid machine-independently.

class Mapping : AllStatic {

private:
//        static Location _localRegisters[nofLocalRegisters + 1]; // the list of local registers
//        static int      _localRegisterIndex[REGISTER_COUNT + 1];  // the inverse of localRegisters[]
    static std::array<Location, nofLocalRegisters> _localRegisters;
    static std::array<int, REGISTER_COUNT>         _localRegisterIndex;

public:
    // initialization
    static void initialize();

    // register allocation
    static Location localRegister( int i );                 // the i.th local register (i = 0 .. nofLocalRegisters-1)
    static int localRegisterIndex( const Location &l );    // the index of local register l (localRegisterIndex(localRegister(i)) = i)

    // parameter passing
    static Location incomingArg( int i, int nofArgs );      // incoming argument (excluding receiver; i >= 0, 0 = first arg)
    static Location outgoingArg( int i, int nofArgs );      // outgoing argument (excluding receiver; i >= 0, 0 = first arg)

    // stack allocation
    static Location localTemporary( int i );                // the i.th local temporary (i >= 0)
    static int localTemporaryIndex( const Location &l );   // the index of the local temporary l (localTemporaryIndex(localTemporary(i)) = i)
    static Location floatTemporary( int scope_id, int i );  // the i.th float temporary within a scope (i >= 0)

    // context temporaries
    static int contextOffset( int tempNo );                 // the byte offset of temp from the contextOop
    static Location contextTemporary( int contextNo, int i, int scope_id );         // the i.th context temporary (i >= 0)
    static Location *new_contextTemporary( int contextNo, int i, int scope_id );   // ditto, but allocated in resource area

    // conversion functions
    static Location asLocation( const Register &reg ) {
        return Location::registerLocation( reg.number() );
    }


    static Register asRegister( const Location &loc ) {
        return Register( loc.number(), ' ' );
    }


    // predicates
    static bool_t isTemporaryRegister( const Location loc ) {
        return false;
    }    // fix this
    static bool_t isLocalRegister( const Location loc ) {
        return _localRegisterIndex[ loc.number() ] not_eq -1;
    }


    static bool_t isTrashedRegister( const Location loc ) {
        return true;
    }    // fix this

    static bool_t isNormalTemporary( Location loc );

    static bool_t isFloatTemporary( Location loc );

    //
    // helper functions for code generation
    //
    // needsStoreCheck determines whether a store check is needed when storing into a context location
    // (e.g., no storeCheck is needed when initializing individual context fields because there's one store check after context creation).
    //
    static void load( const Location &src, const Register &dst );

    static void store( Register src, const Location &dst, const Register &temp1, const Register &temp2, bool_t needsStoreCheck );

    static void storeO( Oop obj, const Location &dst, const Register &temp1, const Register &temp2, bool_t needsStoreCheck );

    // helper functions for float code
    static void fload( const Location &src, const Register &base );

    static void fstore( const Location &dst, const Register &base );
};


// calls
const Register self_reg     = eax;            // incoming receiver location (in prologue of callee)
const Register receiver_reg = eax;            // outgoing receiver location (before call)
const Register result_reg   = eax;            // outgoing result location (before exit)
const Register frame_reg    = ebp;            // activation frame pointer

const Location selfLoc     = Mapping::asLocation( self_reg );
const Location receiverLoc = Mapping::asLocation( receiver_reg );
const Location resultLoc   = Mapping::asLocation( result_reg );
const Location frameLoc    = Mapping::asLocation( frame_reg );


// non-local returns (make sure to adjust the corresponding constants in interpreter_asm.asm when changing these)
const Register NonLocalReturn_result_reg = eax;            // result being returned
const Register NonLocalReturn_home_reg   = edi;            // frame ptr of home frame (stack)
const Register NonLocalReturn_homeId_reg = esi;            // scope id of home scope (inlining)

const Location NonLocalReturnResultLoc = Mapping::asLocation( NonLocalReturn_result_reg );
const Location NonLocalReturnHomeLoc   = Mapping::asLocation( NonLocalReturn_home_reg );
const Location NonLocalReturnHomeIdLoc = Mapping::asLocation( NonLocalReturn_homeId_reg );


// temporaries for local code generation (within one Node only)
// note: these locations must not intersect with any location used for non-local returns!
const Register temp1 = ebx;
const Register temp2 = ecx;
const Register temp3 = edx;
