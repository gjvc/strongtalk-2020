//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/compiler/PseudoRegister.hpp"
#include "vm/compiler/Node.hpp"
#include "vm/compiler/Scope.hpp"
#include "vm/runtime/ResourceObject.hpp"
#include "vm/compiler/OpCode.hpp"
#include "vm/utilities/ConsoleOutputStream.hpp"

// Implementation of loop optimizations: moving type tests out of loops, finding candidates for register
// allocation within a loop, plus integer-specific optimizations (removing tag checks and bound checks).

// a candidate for register allocation within a loop
class LoopPseudoRegisterCandidate : public PrintableResourceObject {

private:
    PseudoRegister *_pseudoRegister;
    std::int32_t   _nuses;
    std::int32_t   _ndefs;

public:
    LoopPseudoRegisterCandidate( PseudoRegister *r ) :
        _pseudoRegister{ r },
        _nuses{ 0 },
        _ndefs{ 0 } {
    }


    PseudoRegister *pseudoRegister() const {
        return _pseudoRegister;
    }


    std::int32_t nuses() const {
        return _nuses;
    }


    std::int32_t ndefs() const {
        return _ndefs;
    }


    std::int32_t weight() const {
        return _ndefs + _nuses;
    }


    void incDUs( std::int32_t u, std::int32_t d ) {
        _nuses += u;
        _ndefs += d;
    }


    void print();
};


// A CompiledLoop contains annotations for the compiler for integer loop optimizations.

class HoistedTypeTest;

class CompiledLoop : public PrintableResourceObject {

private:
    InlinedScope   *_scope;                     //
    Node           *_beforeLoop;                // last node before loop (init. of loop var)
    LoopHeaderNode *_loopHeader;                //
    Node           *_startOfLoop;               //
    Node           *_endOfLoop;                 //
    Node           *_startOfBody;               //
    Node           *_endOfBody;                 //
    Node           *_startOfCond;               //
    Node           *_endOfCond;                 //
    BranchNode     *_loopBranch;                // branch ending loop condition
    std::int32_t   _firstNodeID, _lastNodeID;    // all node IDs in loop are between these two

    // the instance variables below are set as a result of recognize() and are valid only if isIntegerLoop()
    bool           _isIntegerLoop;               // is this loop a recognized integer loop?
    PseudoRegister *_loopVar;                   // suspected loop variable
    PseudoRegister *_lowerBound;                // lower bound of for-like loop (see comment in findLowerBound)
    PseudoRegister *_upperBound;                // upper bound
    PseudoRegister *_increment;                 // increment of loopVar
    bool           _isCountingUp;                // true if loop is counting up, false if counting down
    NonTrivialNode *_incNode;                   // node incrementing loop var
    PseudoRegister *_loopArray;                 // array whose size is upper bound (or nullptr)
    LoadOffsetNode *_loopSizeLoad;              // node loading loopArray's size

    // instance variables for general loop optimizations
    GrowableArray<HoistedTypeTest *>   *_hoistableTests;    // tests that can be hoisted out of the loop
    static GrowableArray<BasicBlock *> *_bbs;      // BBs in code generation order

public:
    CompiledLoop();

    // the routines below should be called with the appropriate nodes
    void set_startOfLoop( LoopHeaderNode *current );

    void set_startOfBody( Node *current );

    void set_endOfBody( Node *current );

    void set_startOfCond( Node *current );

    void set_endOfCond( Node *current );

    void set_endOfLoop( Node *end );

    bool isInLoop( Node *n ) const;

    bool isInLoopCond( Node *n ) const;

    bool isInLoopBody( Node *n ) const;


    LoopHeaderNode *loopHeader() const {
        return _loopHeader;
    }


    const char *recognize(); // does integer loop recognition; called after definitions/uses built (returns nullptr if successful, reason otherwise)

    bool isIntegerLoop() const {
        return _isIntegerLoop;
    }


    void optimize();                // perform general loop optimization
    void optimizeIntegerLoop();     // perform integer loop optimization

    void print();

protected:
    void discoverLoopNesting();

    std::int32_t findStartOfSend( std::int32_t byteCodeIndex );

    const char *findLowerBound();

    const char *findUpperBound();

    const char *checkLoopVar();

    const char *checkUpperBound();

    NonTrivialNode *findDefInLoop( PseudoRegister *r );      // find single definition of r in loop
    bool isIncrement( PseudoRegister *r, ArithOpCode op );      // is r a suitable loop variable increment?
    void removeTagChecks();

    void removeLoopVarOverflow();

    void checkForArraysDefinedInLoop();

    void hoistTypeTests();

    void removeBoundsChecks( PseudoRegister *array, PseudoRegister *var );

    void findRegCandidates();

    bool isEquivalentType( GrowableArray<KlassOop> *klasses1, GrowableArray<KlassOop> *klasses2 );

public:  // for iterators
    std::int32_t defsInLoop( PseudoRegister *r, NonTrivialNode **defNode = nullptr );   // return number of definitions of r in loop and sets defNode if non-nullptr
};

// holds the info associated with a single type test hoisted out of a loop
class HoistedTypeTest : public PrintableResourceObject {

public:
    NonTrivialNode          *_node;         // node (within loop) doing the test
    PseudoRegister          *_testedPR;     // PseudoRegister whose type is tested
    GrowableArray<KlassOop> *_klasses;      // klasses for which the PseudoRegister is tested
    bool                    _invalid;

    HoistedTypeTest( NonTrivialNode *node, PseudoRegister *testedPR, GrowableArray<KlassOop> *klasses );

    void print();

    void print_test_on( ConsoleOutputStream *s );
};
