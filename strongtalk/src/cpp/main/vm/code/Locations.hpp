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
#include "vm/system/sizes.hpp"


// Locations manages register and stack locations. All locations are numbered. A fixed range
// of indices >= 0 is assigned to arguments & registers and a growable range of indices >= 0
// is assigned to stack locations.
//
// Note: Do not confuse Locations with the (old) class Location, which should disappear
//       as soon as the new backend is up and running.

class Locations : public PrintableResourceObject {

    private:
        int _nofArguments;          // the number of arguments
        int _nofRegisters;          // the maximum number of available registers
        GrowableArray <int> * _freeList;             // the list of free locations
        int _firstFreeRegister;     // the index of the first free register in _freeList
        int _firstFreeStackTmp;     // the index of the first free stack temporary in _freeList

        int argumentsBeg() const {
            return 0;
        }


        int argumentsEnd() const {
            return _nofArguments;
        }


        int registersBeg() const {
            return _nofArguments;
        }


        int registersEnd() const {
            return _nofArguments + _nofRegisters;
        }


        int stackTmpsBeg() const {
            return _nofArguments + _nofRegisters;
        }


        int stackTmpsEnd() const {
            return _freeList->length();
        }


        int locationsBeg() const {
            return 0;
        }


        int locationsEnd() const {
            return _freeList->length();
        }


    public:
        enum {
            noLocation            = -1,         // isLocation(noLocation) return false
            maxNofUsableRegisters = 6,          // the maximum number of usable registers (<= nofRegisters)
            sentinel              = 999999999   // simply much bigger than _freeList can ever get
        };

        Locations( int nofArgs, int nofRegs, int nofInitialStackTmps );    // nofRegisters <= maxNofUsableRegisters
        Locations( Locations * l );                                          // to copy locations

        void extendTo( int newValue );

        // Location management
        int allocateRegister();                 // allocates a new register (fatal if not freeRegisters())
        int allocateStackTmp();                 // allocates a new stack location
        void allocate( int i );                 // allocates location i, i must have been unallocated
        void use( int i );                      // uses location i once again, i must be allocated already
        void release( int i );                  // releases a register or stack location

        // Testers
        int nofUses( int i ) const;             // the number of times the location has been use'd (including allocation)
        int nofTotalUses() const;               // the number of total uses of all locations (for verification purposes)
        int nofArguments() const {
            return _nofArguments;
        }


        int nofRegisters() const {
            return _nofRegisters;
        }


        int nofStackTmps() const {
            return stackTmpsEnd() - stackTmpsBeg();
        }


        int nofFreeRegisters() const;        // the number of available registers

        bool_t freeRegisters() const {
            return _firstFreeRegister not_eq sentinel;
        }


        bool_t isLocation( int i ) const {
            return locationsBeg() <= i and i < locationsEnd();
        }


        bool_t isArgument( int i ) const {
            return argumentsBeg() <= i and i < argumentsEnd();
        }


        bool_t isRegister( int i ) const {
            return registersBeg() <= i and i < registersEnd();
        }


        bool_t isStackTmp( int i ) const {
            return stackTmpsBeg() <= i and i < stackTmpsEnd();
        }


        bool_t isStackLoc( int i ) const {
            return isArgument( i ) or isStackTmp( i );
        }


        // Machine-dependent mapping of Registers/locations
        int freeRegisterMask() const;        // bit i corresponds to register i; bit set <==> register is free
        int usedRegisterMask() const;        // bit i corresponds to register i; bit set <==> register is used

        int argumentAsLocation( int argNo ) const;    // the location encoding for argument argNo
        int registerAsLocation( Register reg ) const;    // the location encoding for register reg
        int temporaryAsLocation( int tempNo ) const;    // the location encoding for temporary tempNo

        Register locationAsRegister( int loc ) const;    // the register corresponding to loc
        int locationAsRegisterNo( int loc ) const {
            return locationAsRegister( loc ).number();
        }


        int locationAsWordOffset( int loc ) const;    // the (ebp) word offset corresponding to loc
        int locationAsByteOffset( int loc ) const {
            return locationAsWordOffset( loc ) * oopSize;
        }


        Address locationAsAddress( int loc ) const {
            return Address( ebp, locationAsByteOffset( loc ) );
        }


        // Debugging
        void print();

        void verify();
};
