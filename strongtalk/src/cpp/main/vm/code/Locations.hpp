//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/memory/allocation.hpp"
#include "vm/utilities/GrowableArray.hpp"
#include "vm/assembler/Address.hpp"
#include "vm/assembler/Register.hpp"
#include "vm/runtime/ResourceObject.hpp"


// Locations manages register and stack locations. All locations are numbered. A fixed range
// of indices >= 0 is assigned to arguments & registers and a growable range of indices >= 0
// is assigned to stack locations.
//
// Note: Do not confuse Locations with the (old) class Location, which should disappear
//       as soon as the new backend is up and running.

class Locations : public PrintableResourceObject {

private:
    std::int32_t                _nofArguments;          // the number of arguments
    std::int32_t                _nofRegisters;          // the maximum number of available registers
    GrowableArray<std::int32_t> *_freeList;             // the list of free locations
    std::int32_t                _firstFreeRegister;     // the index of the first free register in _freeList
    std::int32_t                _firstFreeStackTmp;     // the index of the first free stack temporary in _freeList

    std::int32_t argumentsBeg() const {
        return 0;
    }


    std::int32_t argumentsEnd() const {
        return _nofArguments;
    }


    std::int32_t registersBeg() const {
        return _nofArguments;
    }


    std::int32_t registersEnd() const {
        return _nofArguments + _nofRegisters;
    }


    std::int32_t stackTmpsBeg() const {
        return _nofArguments + _nofRegisters;
    }


    std::int32_t stackTmpsEnd() const {
        return _freeList->length();
    }


    std::int32_t locationsBeg() const {
        return 0;
    }


    std::int32_t locationsEnd() const {
        return _freeList->length();
    }


public:
    enum {
        noLocation            = -1,         // isLocation(noLocation) return false
        maxNofUsableRegisters = 6,          // the maximum number of usable registers (<= nofRegisters)
        sentinel              = 999999999   // simply much bigger than _freeList can ever get
    };

    Locations( std::int32_t nofArgs, std::int32_t nofRegs, std::int32_t nofInitialStackTmps );    // nofRegisters <= maxNofUsableRegisters
    Locations( Locations *l );                                          // to copy locations

    void extendTo( std::int32_t newValue );

    // Location management
    std::int32_t allocateRegister();                 // allocates a new register (fatal if not freeRegisters())
    std::int32_t allocateStackTmp();                 // allocates a new stack location
    void allocate( std::int32_t i );                 // allocates location i, i must have been unallocated
    void use( std::int32_t i );                      // uses location i once again, i must be allocated already
    void release( std::int32_t i );                  // releases a register or stack location

    // Testers
    std::int32_t nofUses( std::int32_t i ) const;             // the number of times the location has been use'd (including allocation)
    std::int32_t nofTotalUses() const;               // the number of total uses of all locations (for verification purposes)
    std::int32_t nofArguments() const {
        return _nofArguments;
    }


    std::int32_t nofRegisters() const {
        return _nofRegisters;
    }


    std::int32_t nofStackTmps() const {
        return stackTmpsEnd() - stackTmpsBeg();
    }


    std::int32_t nofFreeRegisters() const;        // the number of available registers

    bool freeRegisters() const {
        return _firstFreeRegister not_eq sentinel;
    }


    bool isLocation( std::int32_t i ) const {
        return locationsBeg() <= i and i < locationsEnd();
    }


    bool isArgument( std::int32_t i ) const {
        return argumentsBeg() <= i and i < argumentsEnd();
    }


    bool isRegister( std::int32_t i ) const {
        return registersBeg() <= i and i < registersEnd();
    }


    bool isStackTmp( std::int32_t i ) const {
        return stackTmpsBeg() <= i and i < stackTmpsEnd();
    }


    bool isStackLoc( std::int32_t i ) const {
        return isArgument( i ) or isStackTmp( i );
    }


    // Machine-dependent mapping of Registers/locations
    std::int32_t freeRegisterMask() const;        // bit i corresponds to register i; bit set <==> register is free
    std::int32_t usedRegisterMask() const;        // bit i corresponds to register i; bit set <==> register is used

    std::int32_t argumentAsLocation( std::int32_t argNo ) const;    // the location encoding for argument argNo
    std::int32_t registerAsLocation( Register reg ) const;    // the location encoding for register reg
    std::int32_t temporaryAsLocation( std::int32_t tempNo ) const;    // the location encoding for temporary tempNo

    Register locationAsRegister( std::int32_t loc ) const;    // the register corresponding to loc
    std::int32_t locationAsRegisterNo( std::int32_t loc ) const {
        return locationAsRegister( loc ).number();
    }


    std::int32_t locationAsWordOffset( std::int32_t loc ) const;    // the (ebp) word offset corresponding to loc
    std::int32_t locationAsByteOffset( std::int32_t loc ) const {
        return locationAsWordOffset( loc ) * OOP_SIZE;
    }


    Address locationAsAddress( std::int32_t loc ) const {
        return Address( ebp, locationAsByteOffset( loc ) );
    }


    // Debugging
    void print();

    void verify();
};
