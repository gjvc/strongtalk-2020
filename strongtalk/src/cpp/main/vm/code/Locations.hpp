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
    std::size_t _nofArguments;          // the number of arguments
    std::size_t _nofRegisters;          // the maximum number of available registers
    GrowableArray<std::size_t> *_freeList;             // the list of free locations
    std::size_t _firstFreeRegister;     // the index of the first free register in _freeList
    std::size_t _firstFreeStackTmp;     // the index of the first free stack temporary in _freeList

    std::size_t argumentsBeg() const {
        return 0;
    }


    std::size_t argumentsEnd() const {
        return _nofArguments;
    }


    std::size_t registersBeg() const {
        return _nofArguments;
    }


    std::size_t registersEnd() const {
        return _nofArguments + _nofRegisters;
    }


    std::size_t stackTmpsBeg() const {
        return _nofArguments + _nofRegisters;
    }


    std::size_t stackTmpsEnd() const {
        return _freeList->length();
    }


    std::size_t locationsBeg() const {
        return 0;
    }


    std::size_t locationsEnd() const {
        return _freeList->length();
    }


public:
    enum {
        noLocation            = -1,         // isLocation(noLocation) return false
        maxNofUsableRegisters = 6,          // the maximum number of usable registers (<= nofRegisters)
        sentinel              = 999999999   // simply much bigger than _freeList can ever get
    };

    Locations( std::size_t nofArgs, std::size_t nofRegs, std::size_t nofInitialStackTmps );    // nofRegisters <= maxNofUsableRegisters
    Locations( Locations *l );                                          // to copy locations

    void extendTo( std::size_t newValue );

    // Location management
    std::size_t allocateRegister();                 // allocates a new register (fatal if not freeRegisters())
    std::size_t allocateStackTmp();                 // allocates a new stack location
    void allocate( std::size_t i );                 // allocates location i, i must have been unallocated
    void use( std::size_t i );                      // uses location i once again, i must be allocated already
    void release( std::size_t i );                  // releases a register or stack location

    // Testers
    std::size_t nofUses( std::size_t i ) const;             // the number of times the location has been use'd (including allocation)
    std::size_t nofTotalUses() const;               // the number of total uses of all locations (for verification purposes)
    std::size_t nofArguments() const {
        return _nofArguments;
    }


    std::size_t nofRegisters() const {
        return _nofRegisters;
    }


    std::size_t nofStackTmps() const {
        return stackTmpsEnd() - stackTmpsBeg();
    }


    std::size_t nofFreeRegisters() const;        // the number of available registers

    bool_t freeRegisters() const {
        return _firstFreeRegister not_eq sentinel;
    }


    bool_t isLocation( std::size_t i ) const {
        return locationsBeg() <= i and i < locationsEnd();
    }


    bool_t isArgument( std::size_t i ) const {
        return argumentsBeg() <= i and i < argumentsEnd();
    }


    bool_t isRegister( std::size_t i ) const {
        return registersBeg() <= i and i < registersEnd();
    }


    bool_t isStackTmp( std::size_t i ) const {
        return stackTmpsBeg() <= i and i < stackTmpsEnd();
    }


    bool_t isStackLoc( std::size_t i ) const {
        return isArgument( i ) or isStackTmp( i );
    }


    // Machine-dependent mapping of Registers/locations
    std::size_t freeRegisterMask() const;        // bit i corresponds to register i; bit set <==> register is free
    std::size_t usedRegisterMask() const;        // bit i corresponds to register i; bit set <==> register is used

    std::size_t argumentAsLocation( std::size_t argNo ) const;    // the location encoding for argument argNo
    std::size_t registerAsLocation( Register reg ) const;    // the location encoding for register reg
    std::size_t temporaryAsLocation( std::size_t tempNo ) const;    // the location encoding for temporary tempNo

    Register locationAsRegister( std::size_t loc ) const;    // the register corresponding to loc
    std::size_t locationAsRegisterNo( std::size_t loc ) const {
        return locationAsRegister( loc ).number();
    }


    std::size_t locationAsWordOffset( std::size_t loc ) const;    // the (ebp) word offset corresponding to loc
    std::size_t locationAsByteOffset( std::size_t loc ) const {
        return locationAsWordOffset( loc ) * oopSize;
    }


    Address locationAsAddress( std::size_t loc ) const {
        return Address( ebp, locationAsByteOffset( loc ) );
    }


    // Debugging
    void print();

    void verify();
};
