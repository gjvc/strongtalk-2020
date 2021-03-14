
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/memory/allocation.hpp"
#include "vm/runtime/ResourceArea.hpp"
#include "vm/utility/GrowableArray.hpp"
#include "vm/memory/Closure.hpp"
#include "vm/runtime/ResourceObject.hpp"


// Definition represents the definition of a PseudoRegister.
// Usage represents the usage of a PseudoRegister.
//
// Each PseudoRegister has a list of the BasicBlocks in which is it used or defined.
// Each BasicBlock has a list of the Definitions and Usages it contains.

class NonTrivialNode;


// "abstract"
class DefinitionUsage : public PrintableResourceObject {

public:
    NonTrivialNode *_node;


    DefinitionUsage( NonTrivialNode *n ) :
        _node{ n } {
    }


    DefinitionUsage() = default;
    virtual ~DefinitionUsage() = default;
    DefinitionUsage( const DefinitionUsage & ) = default;
    DefinitionUsage &operator=( const DefinitionUsage & ) = default;


    void operator delete( void *ptr ) { (void)(ptr); }


};


class Definition : public DefinitionUsage {

public:
    Definition( NonTrivialNode *n ) :
        DefinitionUsage( n ) {
    }


    Definition() = default;
    virtual ~Definition() = default;
    Definition( const Definition & ) = default;
    Definition &operator=( const Definition & ) = default;


    void operator delete( void *ptr ) { (void)(ptr); }


    void print();
};


class Usage : public DefinitionUsage {

public:
    Usage( NonTrivialNode *n ) :
        DefinitionUsage( n ) {
    }


    Usage() = default;
    virtual ~Usage() = default;
    Usage( const Usage & ) = default;
    Usage &operator=( const Usage & ) = default;


    void operator delete( void *ptr ) { (void)(ptr); }


    virtual bool isSoft() const {
        return false;
    }


    void print();
};


// a debugger-related use; doesn't prevent block elimination
class PSoftUsage : public Usage {

public:
    PSoftUsage( NonTrivialNode *n ) :
        Usage( n ) {
    }


    PSoftUsage() = default;
    virtual ~PSoftUsage() = default;
    PSoftUsage( const PSoftUsage & ) = default;
    PSoftUsage &operator=( const PSoftUsage & ) = default;


    void operator delete( void *ptr ) { (void)(ptr); }


    bool isSoft() const {
        return true;
    }


    void print();
};


class PseudoRegister;

class BasicBlock;

// a PseudoRegisterBasicBlockIndex is an index into a particular element of a BasicBlockDefinitionAndUsageTable
class PseudoRegisterBasicBlockIndex : public PrintableResourceObject {

public:
    BasicBlock   *_basicBlock;    // BasicBlock containing some of PseudoRegister's definitions/uses
    std::int32_t _index;          // index into BasicBlock's BasicBlockDefinitionAndUsageTable

    PseudoRegisterBasicBlockIndex( BasicBlock *b, std::int32_t i, PseudoRegister *pr ) :
        _basicBlock{ b },
        _index{ i } {
            st_unused( pr );
    }


    PseudoRegisterBasicBlockIndex() = default;
    virtual ~PseudoRegisterBasicBlockIndex() = default;
    PseudoRegisterBasicBlockIndex( const PseudoRegisterBasicBlockIndex & ) = default;
    PseudoRegisterBasicBlockIndex &operator=( const PseudoRegisterBasicBlockIndex & ) = default;


    void operator delete( void *ptr ) { (void)(ptr); }


    void print_short();

    void print();
};

void forAllDefsDo( const GrowableArray<PseudoRegisterBasicBlockIndex *> *l, Closure<Definition *> *f );

void forAllUsesDo( const GrowableArray<PseudoRegisterBasicBlockIndex *> *l, Closure<Usage *> *f );

void printDefsAndUses( const GrowableArray<PseudoRegisterBasicBlockIndex *> *l );    // for debugging
