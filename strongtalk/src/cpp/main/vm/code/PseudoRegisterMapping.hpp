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
    virtual void preg_do( PseudoRegister *preg ) {
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
    bool_t                          _nonLocalReturnInProgress;     // indicates that a NonLocalReturn is in progress (see also Note above)
    Locations                       *_locations;                   // the locations freelist
    GrowableArray<PseudoRegister *> *_pseudoRegisters;             // the PseudoRegisters; a nullptr entry means the slot is not used
    GrowableArray<std::int32_t>     *_registerLocations;           // the register to which a PseudoRegister is mapped or -1
    GrowableArray<std::int32_t>     *_stackLocations;              // the stack location to which a PseudoRegister is mapped or -1
    GrowableArray<std::int32_t>     *_temporaryLocations;          // a list of temporary locations used by instances of Temporary (these locations will be freed when the mapping is copied)

    // Helper routines
    std::int32_t size() const {
        return _pseudoRegisters->length();
    }


    bool_t used( std::int32_t i ) const {
        return _pseudoRegisters->at( i ) not_eq nullptr;
    }


    std::int32_t regLoc( std::int32_t i ) const {
        return _registerLocations->at( i );
    }


    std::int32_t stkLoc( std::int32_t i ) const {
        return _stackLocations->at( i );
    }


    bool_t hasRegLoc( std::int32_t i ) const {
        return _locations->isLocation( regLoc( i ) );
    }


    bool_t hasStkLoc( std::int32_t i ) const {
        return _locations->isLocation( stkLoc( i ) );
    }


    std::int32_t location( std::int32_t i ) const {
        std::int32_t rloc = regLoc( i );
        return rloc >= 0 ? rloc : stkLoc( i );
    }


    void set_entry( std::int32_t i, PseudoRegister *preg, std::int32_t rloc, std::int32_t sloc );

    std::int32_t index( PseudoRegister *preg );

    std::int32_t freeSlot();

    void print( std::int32_t i );

    void destroy();                // destroys mapping to make sure it is not accidentally used afterwards

    // Register allocation/spilling
    std::int32_t spillablePRegIndex();            // returns the _pregs/_mappings index of a PseudoRegister mapped to a non-locked register
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
    bool_t isInjective();

    bool_t isConformant( PseudoRegisterMapping *with );


    bool_t isDefined( PseudoRegister *preg ) {
        return index( preg ) >= 0;
    }


    bool_t inRegister( PseudoRegister *preg ) {
        std::int32_t i = index( preg );
        return used( i ) and hasRegLoc( i );
    }


    bool_t onStack( PseudoRegister *preg ) {
        std::int32_t i = index( preg );
        return used( i ) and hasStkLoc( i );
    }


    // Definition
    void mapToArgument( PseudoRegister *preg, std::int32_t argNo );

    void mapToRegister( PseudoRegister *preg, Register reg );

    void mapToTemporary( PseudoRegister *preg, std::int32_t tempNo );

    void kill( PseudoRegister *preg );

    void killDeadsAt( Node *node, PseudoRegister *exception = nullptr );

    void killAll( PseudoRegister *exception = nullptr );

    void cleanupContextReferences();

    // Expressions
    Register def( PseudoRegister *preg, Register hint = noreg );    // defines a new value for preg (uses hint if given)
    Register use( PseudoRegister *preg, Register hint );        // uses the value of preg (uses hint if given)
    Register use( PseudoRegister *preg );                // deals also with constants (code originally in CodeGenerator)

    // Assignments
    void move( PseudoRegister *dst, PseudoRegister *src );

    // Calls
    void saveRegisters( PseudoRegister *exception = nullptr );

    void killRegisters( PseudoRegister *exception = nullptr );

    void killRegister( PseudoRegister *preg );


    // Non-local returns
    bool_t NonLocalReturninProgress() const {
        return _nonLocalReturnInProgress;
    }


    void acquireNonLocalReturnRegisters();

    void releaseNonLocalReturnRegisters();

    // Labels
    void makeInjective();

    void makeConformant( PseudoRegisterMapping *with );

    // Iteration/Debug info
    void iterate( PseudoRegisterClosure *closure );

    Location locationFor( PseudoRegister *preg );

    // Space usage
    std::int32_t nofPRegs();

    std::int32_t maxNofStackTmps();

    // Debugging
    void my_print();

    void print();

    void verify();

    friend class Temporary;
};


// A PseudoRegisterLocker is used to lock certain PseudoRegisters for the existence of the scope of
// a C++ function activation. A PseudoRegister that is locked in a PseudoRegisterLocker is kept in
// the same register once it has been mapped to a register location by a PRegMapping.
// NOTE: PRegLockers MUST only be created/destructed in a stack-fashioned manner.

class PseudoRegisterLocker : StackAllocatedObject {
private:
    static PseudoRegisterLocker *_top;            // the topmost PseudoRegisterLocker
    PseudoRegisterLocker        *_prev;            // the previous PseudoRegisterLocker
    PseudoRegister              *_pregs[3];        // the locked PRregs

    void lock( PseudoRegister *r0, PseudoRegister *r1, PseudoRegister *r2 ) {
        _prev = _top;
        _top  = this;
        _pregs[ 0 ] = r0;
        _pregs[ 1 ] = r1;
        _pregs[ 2 ] = r2;
    }


    bool_t holds( PseudoRegister *preg ) const;            // returns true if preg belongs to the locked PseudoRegisters

public:
    PseudoRegisterLocker( PseudoRegister *r0 );

    PseudoRegisterLocker( PseudoRegister *r0, PseudoRegister *r1 );

    PseudoRegisterLocker( PseudoRegister *r0, PseudoRegister *r1, PseudoRegister *r2 );


    ~PseudoRegisterLocker() {
        _top = _prev;
    }


    static bool_t locks( PseudoRegister *preg );        // returns true if preg is locked in any PseudoRegisterLocker instance
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

    Temporary( PseudoRegisterMapping *mapping, PseudoRegister *preg );    // keep a (modifiable) copy of the preg value in temporary register
    ~Temporary();


    Register reg() const {
        return _mapping->_locations->locationAsRegister( _regLoc );
    }
};
