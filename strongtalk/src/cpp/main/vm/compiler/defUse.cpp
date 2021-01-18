//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/compiler/defUse.hpp"
#include "vm/compiler/DefinitionUsageInfo.hpp"
#include "vm/compiler/Compiler.hpp"
#include "vm/compiler/CopyPropagationInfo.hpp"
#include "vm/compiler/BasicBlock.hpp"


static Closure <Definition *> * theDefIterator;


void Usage::print() {
    lprintf( "Use %#lx (N%ld)", PrintHexAddresses ? this : 0, _node->id() );
}


void PSoftUsage::print() {
    lprintf( "PSoftUsage %#lx (N%ld)", PrintHexAddresses ? this : 0, _node->id() );
}


void Definition::print() {
    lprintf( "Def %#lx (N%ld)", PrintHexAddresses ? this : 0, _node->id() );
}


void BasicBlockDefinitionAndUsageTable::print() {
    if ( not info )
        return;        // not built yet
    print_short();
    for ( int i = 0; i < info->length(); i++ ) {
        lprintf( "%3ld: ", i );
        info->at( i )->print();
    }
}


void PseudoRegisterBasicBlockIndex::print_short() {
    lprintf( "PseudoRegisterBasicBlockIndex [%ld] for", _index );
    _basicBlock->print_short();
}


void PseudoRegisterBasicBlockIndex::print() {
    print_short();
    lprintf( ": " );
    _basicBlock->duInfo.info->at( _index )->print();
}


static bool_t cpCreateFailed = false;


CopyPropagationInfo * new_CPInfo( NonTrivialNode * n ) {
    CopyPropagationInfo * cpi = new CopyPropagationInfo( n );
    if ( cpCreateFailed ) {
        cpCreateFailed = false;
        return nullptr;
    }
    return cpi;
}


CopyPropagationInfo::CopyPropagationInfo( NonTrivialNode * n ) {
    _definition = n;
    if ( not n->hasSrc() ) {
        cpCreateFailed = true;        // can't eliminate register
        return;
    }
    _register = n->src();
    if ( _register->isConstPseudoRegister() )
        return;   // can always eliminate if defined by constant  (was bug; fixed 7/26/96 -Urs)
    // (as std::int32_t as constants aren't register-allocated)
    PseudoRegister * eliminatee = n->dest();
    if ( eliminatee->_debug ) {
        if ( _register->extendLiveRange( eliminatee->scope(), eliminatee->endByteCodeIndex() ) ) {
            // ok, the replacement PseudoRegister covers the eliminated PseudoRegister's debug-visible live range
            // (begByteCodeIndex must be covered if endByteCodeIndex is covered)
        } else {
            cpCreateFailed = true;      // need register for debug info (was bug; fixed 5/14/96 -Urs)
        }
    }
}


bool_t CopyPropagationInfo::isConstant() const {
    return _register->isConstPseudoRegister();
}


Oop CopyPropagationInfo::constant() const {
    st_assert( isConstant(), "not constant" );
    return ( ( ConstPseudoRegister * ) _register )->constant;
}


void CopyPropagationInfo::print() {
    lprintf( "*(CopyPropagationInfo*)%#x : def %#x, %s\n", this, _definition, _register->name() );
}


static void printNodeFn( DefinitionUsage * du ) {
    lprintf( "N%d ", du->_node->id() );
}


struct PrintNodeClosureD : public Closure <Definition *> {
    void do_it( Definition * d ) {
        printNodeFn( d );
    }
};


struct PrintNodeClosureU : public Closure <Usage *> {
    void do_it( Usage * u ) {
        printNodeFn( u );
    }
};


void printDefsAndUses( const GrowableArray <PseudoRegisterBasicBlockIndex *> * l ) {
    lprintf( "definitions: " );
    PrintNodeClosureD printNodeD;
    forAllDefsDo( l, &printNodeD );
    lprintf( "; uses: " );
    PrintNodeClosureU printNodeU;
    forAllUsesDo( l, &printNodeU );
}


static void doDefsFn( PseudoRegisterBasicBlockIndex * p ) {
    DefinitionUsageInfo * info = p->_basicBlock->duInfo.info->at( p->_index );
    info->_definitions.apply( theDefIterator );
}


void forAllDefsDo( const GrowableArray <PseudoRegisterBasicBlockIndex *> * l, Closure <Definition *> * f ) {
    theDefIterator = f;
    l->apply( doDefsFn );
}


static Closure <Usage *> * theUseIterator;


static void doUsesFn( PseudoRegisterBasicBlockIndex * p ) {
    DefinitionUsageInfo * info = p->_basicBlock->duInfo.info->at( p->_index );
    info->_usages.apply( theUseIterator );
}


void forAllUsesDo( const GrowableArray <PseudoRegisterBasicBlockIndex *> * l, Closure <Usage *> * f ) {
    theUseIterator = f;
    l->apply( doUsesFn );
}
