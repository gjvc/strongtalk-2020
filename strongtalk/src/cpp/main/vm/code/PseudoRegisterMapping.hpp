//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/system/asserts.hpp"
#include "vm/system/platform.hpp"
#include "vm/code/Locations.hpp"
#include "vm/utilities/GrowableArray.hpp"
#include "vm/assembler/MacroAssembler.hpp"
#include "vm/assembler/Location.hpp"
#include "vm/runtime/ResourceObject.hpp"


// A PseudoRegisterClosure is used when iterating over a PseudoRegisterMapping.

class PseudoRegister;

class PseudoRegisterClosure : public PrintableResourceObject {
public:
    virtual void pseudoRegister_do( PseudoRegister *pseudoRegister ) {
        static_cast<void>(pseudoRegister); // unused
    }        // called for each PseudoRegister in the mapping
};


// A PseudoRegisterMapping holds the mapping of life PseudoRegisters to (register and/or stack) locations.
// Within one PseudoRegisterMapping, at any time a particular PseudoRegister is mapped to at most one register
// and one stack location, but different PseudoRegisters may be mapped to the same location. Because
// of register moving/spilling, PseudoRegister locations may change automatically.
//
// Note: The _NonLocalReturnInProgress flag indicates that the current mapping also reserves the
//       3 special registers used during NonLocalReturns. This is needed because these registers
//       are not explicitly visible in the intermediate data structure (no PseudoRegisters) but
//       have to be preserved anyway (e.g. code generation for conformance mappings).

class PseudoRegisterMapping : public PrintableResourceObject {

private:
    MacroAssembler                  *_macroAssembler;              // the low_level assembler (for spill code generation, etc.)
    bool                            _nonLocalReturnInProgress;     // indicates that a NonLocalReturn is in progress (see also Note above)
    Locations                       *_locations;                   // the locations freelist
    GrowableArray<PseudoRegister *> *_pseudoRegisters;             // the PseudoRegisters; a nullptr entry means the slot is not used
    GrowableArray<std::int32_t>     *_registerLocations;           // the register to which a PseudoRegister is mapped or -1
    GrowableArray<std::int32_t>     *_stackLocations;              // the stack location to which a PseudoRegister is mapped or -1
    GrowableArray<std::int32_t>     *_temporaryLocations;          // a list of temporary locations used by instances of Temporary (these locations will be freed when the mapping is copied)

    // Helper routines
    std::int32_t size() const {
        return _pseudoRegisters->length();
    }


    bool used( std::int32_t i ) const {
        return _pseudoRegisters->at( i ) not_eq nullptr;
    }


    std::int32_t regLoc( std::int32_t i ) const {
        return _registerLocations->at( i );
    }


    std::int32_t stkLoc( std::int32_t i ) const {
        return _stackLocations->at( i );
    }


    bool hasRegLoc( std::int32_t i ) const {
        return _locations->isLocation( regLoc( i ) );
    }


    bool hasStkLoc( std::int32_t i ) const {
        return _locations->isLocation( stkLoc( i ) );
    }


    std::int32_t location( std::int32_t i ) const {
        std::int32_t rloc = regLoc( i );
        return rloc >= 0 ? rloc : stkLoc( i );
    }


    void set_entry( std::int32_t i, PseudoRegister *pseudoRegister, std::int32_t rloc, std::int32_t sloc );

    std::int32_t index( PseudoRegister *pseudoRegister );

    std::int32_t freeSlot();

    void print( std::int32_t i );

    void destroy();                // destroys mapping to make sure it is not accidentally used afterwards

    // Register allocation/spilling
    std::int32_t spillablePseudoRegisterIndex();            // returns the _pseudoRegisters/_mappings index of a PseudoRegister mapped to a non-locked register
    void ensureOneFreeRegister();            // ensures at least one free register in locations - spill a register if necessary
    void spillRegister( std::int32_t loc );            // spills register loc to a free stack location
    void saveRegister( std::int32_t loc );

    // Helpers for class Temporary
    std::int32_t allocateTemporary( Register hint = noreg );

    void releaseTemporary( std::int32_t regLoc );

    void releaseAllTemporaries();

    // make conformant implementations
    void old_makeConformant( PseudoRegisterMapping *with );

    void new_makeConformant( PseudoRegisterMapping *with );

public:
    // Creation
    PseudoRegisterMapping( MacroAssembler *assm, std::int32_t nofArgs, std::int32_t nofRegs, std::int32_t nofTemps );

    PseudoRegisterMapping( PseudoRegisterMapping *m );


    MacroAssembler *assembler() const {
        return _macroAssembler;
    }


    // Testers
    bool isInjective();

    bool isConformant( PseudoRegisterMapping *with );


    bool isDefined( PseudoRegister *pseudoRegister ) {
        return index( pseudoRegister ) >= 0;
    }


    bool inRegister( PseudoRegister *pseudoRegister ) {
        std::int32_t i = index( pseudoRegister );
        return used( i ) and hasRegLoc( i );
    }


    bool onStack( PseudoRegister *pseudoRegister ) {
        std::int32_t i = index( pseudoRegister );
        return used( i ) and hasStkLoc( i );
    }


    // Definition
    void mapToArgument( PseudoRegister *pseudoRegister, std::int32_t argNo );

    void mapToRegister( PseudoRegister *pseudoRegister, Register reg );

    void mapToTemporary( PseudoRegister *pseudoRegister, std::int32_t tempNo );

    void kill( PseudoRegister *pseudoRegister );

    void killDeadsAt( Node *node, PseudoRegister *exception = nullptr );

    void killAll( PseudoRegister *exception = nullptr );

    void cleanupContextReferences();

    // Expressions
    Register def( PseudoRegister *pseudoRegister, Register hint = noreg );    // defines a new value for pseudoRegister (uses hint if given)
    Register use( PseudoRegister *pseudoRegister, Register hint );        // uses the value of pseudoRegister (uses hint if given)
    Register use( PseudoRegister *pseudoRegister );                // deals also with constants (code originally in CodeGenerator)

    // Assignments
    void move( PseudoRegister *dst, PseudoRegister *src );

    // Calls
    void saveRegisters( PseudoRegister *exception = nullptr );

    void killRegisters( PseudoRegister *exception = nullptr );

    void killRegister( PseudoRegister *pseudoRegister );


    // Non-local returns
    bool NonLocalReturninProgress() const {
        return _nonLocalReturnInProgress;
    }


    void acquireNonLocalReturnRegisters();

    void releaseNonLocalReturnRegisters();

    // Labels
    void makeInjective();

    void makeConformant( PseudoRegisterMapping *with );

    // Iteration/Debug info
    void iterate( PseudoRegisterClosure *closure );

    Location locationFor( PseudoRegister *pseudoRegister );

    // Space usage
    std::int32_t nofPseudoRegisters();

    std::int32_t maxNofStackTmps();

    // Debugging
    void my_print();

    void print();

    void verify();

    friend class Temporary;
};


// A PseudoRegisterLocker is used to lock certain PseudoRegisters for the existence of the scope of
// a C++ function activation. A PseudoRegister that is locked in a PseudoRegisterLocker is kept in
// the same register once it has been mapped to a register location by a PseudoRegisterMapping.
// NOTE: PseudoRegisterLockers MUST only be created/destructed in a stack-fashioned manner.

class PseudoRegisterLocker : StackAllocatedObject {
private:
    static PseudoRegisterLocker *_top;                  // the topmost PseudoRegisterLocker
    PseudoRegisterLocker        *_prev;                 // the previous PseudoRegisterLocker
    PseudoRegister              *_pseudoRegisters[3];   // the locked PRregs

    void lock( PseudoRegister *r0, PseudoRegister *r1, PseudoRegister *r2 ) {
        _prev = _top;
        _top  = this;
        _pseudoRegisters[ 0 ] = r0;
        _pseudoRegisters[ 1 ] = r1;
        _pseudoRegisters[ 2 ] = r2;
    }


    bool holds( PseudoRegister *pseudoRegister ) const;            // returns true if pseudoRegister belongs to the locked PseudoRegisters

public:
    PseudoRegisterLocker( PseudoRegister *r0 );

    PseudoRegisterLocker( PseudoRegister *r0, PseudoRegister *r1 );

    PseudoRegisterLocker( PseudoRegister *r0, PseudoRegister *r1, PseudoRegister *r2 );


    ~PseudoRegisterLocker() {
        _top = _prev;
    }


    static bool locks( PseudoRegister *pseudoRegister );        // returns true if pseudoRegister is locked in any PseudoRegisterLocker instance
    static void initialize() {
        _top = nullptr;
    }


    friend class PseudoRegisterMapping;
};


// A Temporary is a freely usable register allocated for the time the Temporary
// is alive. Temporaries must be created/destructed in a stack-fashioned manner.

class Temporary : StackAllocatedObject {
private:
    PseudoRegisterMapping *_mapping;
    std::int32_t          _regLoc;

public:
    Temporary( PseudoRegisterMapping *mapping, Register hint = noreg );

    Temporary( PseudoRegisterMapping *mapping, PseudoRegister *pseudoRegister );    // keep a (modifiable) copy of the pseudoRegister value in temporary register
    ~Temporary();


    Register reg() const {
        return _mapping->_locations->locationAsRegister( _regLoc );
    }
};
