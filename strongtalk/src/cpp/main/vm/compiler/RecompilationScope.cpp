//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/system/platform.hpp"
#include "vm/system/asserts.hpp"
#include "vm/compiler/RecompilationScope.hpp"
#include "vm/runtime/Process.hpp"
#include "vm/interpreter/InterpretedInlineCache.hpp"
#include "vm/oops/ObjectArrayOopDescriptor.hpp"
#include "vm/interpreter/Floats.hpp"
#include "vm/memory/oopFactory.hpp"
#include "vm/code/ScopeDescriptor.hpp"
#include "vm/code/NativeMethod.hpp"
#include "vm/compiler/InliningPolicy.hpp"
#include "vm/compiler/Scope.hpp"
#include "vm/compiler/Compiler.hpp"
#include "vm/code/PolymorphicInlineCache.hpp"
#include "vm/interpreter/CodeIterator.hpp"
#include "vm/memory/vmSymbols.hpp"
#include "vm/interpreter/InlineCacheIterator.hpp"
#include "vm/system/sizes.hpp"


RecompilationScope::RecompilationScope( NonDummyRecompilationScope * s, int byteCodeIndex ) :
    _senderByteCodeIndex( byteCodeIndex ) {
    _sender = s;
    if ( s ) {
        s->addScope( byteCodeIndex, this );
        _invocationCount = s->_invocationCount;
    } else {
        _invocationCount = -1;
    }
}


GrowableArray <RecompilationScope *> * NullRecompilationScope::subScopes( int byteCodeIndex ) const {
    return new GrowableArray <RecompilationScope *>( 1 );
}


static int compare_pcDescs( ProgramCounterDescriptor ** a, ProgramCounterDescriptor ** b ) {
    // to sort by descending scope and ascending byteCodeIndex
    int diff = ( *b )->_scope - ( *a )->_scope;
    return diff ? diff : ( *a )->_byteCodeIndex - ( *b )->_byteCodeIndex;
}


int NonDummyRecompilationScope::compare( NonDummyRecompilationScope ** a, NonDummyRecompilationScope ** b ) {
    return ( *b )->scopeID() - ( *a )->scopeID();
}


NonDummyRecompilationScope::NonDummyRecompilationScope( NonDummyRecompilationScope * s, int byteCodeIndex, MethodOop m, int level ) :
    RecompilationScope( s, byteCodeIndex ), _level( level ), uncommon( 1 ), ncodes( m == nullptr ? 1 : m->size_of_codes() * oopSize ) {
    _subScopes = new_resource_array <GrowableArray <RecompilationScope *> *>( ncodes + 1 );
    for ( int i = 0; i <= ncodes; i++ )
        _subScopes[ i ] = nullptr;
}


InlinedRecompilationScope::InlinedRecompilationScope( NonDummyRecompilationScope * s, int byteCodeIndex, const NativeMethod * n, ScopeDescriptor * d, int level ) :
    NonDummyRecompilationScope( s, byteCodeIndex, d->method(), level ), desc( d ), nm( n ) {
}


PICRecompilationScope::PICRecompilationScope( const NativeMethod * c, ProgramCounterDescriptor * pc, CompiledInlineCache * s, KlassOop k, ScopeDescriptor * dsc, NativeMethod * n, MethodOop m, int ns, int lev, bool_t tr ) :
    NonDummyRecompilationScope( nullptr, pc->_byteCodeIndex, m, lev ), caller( c ), _sd( s ), programCounterDescriptor( pc ), klass( k ), nm( n ), _method( m ), trusted( tr ), _desc( dsc ) {
    _invocationCount = ns;
    _extended        = false;
}


InliningDatabaseRecompilationScope::InliningDatabaseRecompilationScope( NonDummyRecompilationScope * sender, int byteCodeIndex, KlassOop receiver_klass, MethodOop method, int level ) :
    NonDummyRecompilationScope( sender, byteCodeIndex, method, level ) {
    _receiver_klass = receiver_klass;
    _method         = method;
    _key            = LookupKey::allocate( receiver_klass, method->is_blockMethod() ? Oop( method ) : Oop( _method->selector() ) );
    _uncommon       = new GrowableArray <bool_t>( ncodes );
    for ( int i = 0; i <= ncodes; i++ )
        _uncommon->append( false );
}


UntakenRecompilationScope::UntakenRecompilationScope( NonDummyRecompilationScope * sender, ProgramCounterDescriptor * p, bool_t u ) :
    NonDummyRecompilationScope( sender, p->_byteCodeIndex, nullptr, 0 ), isUncommon( u ), pc( p ) {
    int i = 0;    // to allow setting breakpoints
}


UninlinableRecompilationScope::UninlinableRecompilationScope( NonDummyRecompilationScope * sender, int byteCodeIndex ) :
    NullRecompilationScope( sender, byteCodeIndex ) {
    int i = 0;    // to allow setting breakpoints
}


InterpretedRecompilationScope::InterpretedRecompilationScope( NonDummyRecompilationScope * sender, int byteCodeIndex, LookupKey * key, MethodOop m, int level, bool_t trusted ) :
    NonDummyRecompilationScope( sender, byteCodeIndex, m, level ) {
    _key             = key;
    _method          = m;
    _invocationCount = m->invocation_count();
    _is_trusted      = trusted;
    extended         = false;
}


LookupKey * InlinedRecompilationScope::key() const {
    return desc->key();
}


LookupKey * PICRecompilationScope::key() const {
    // If we have a NativeMethod, return the key of the NativeMethod
    if ( nm )
        return ( LookupKey * ) &nm->_lookupKey;

    // If we have a scope desc, return the key of the scope desc
    if ( _desc )
        return _desc->key();

    // If we have a send desc, return an allocated lookup key
    if ( sd() ) {
        return sd()->isSuperSend() ? LookupKey::allocate( klass, _method ) : LookupKey::allocate( klass, _method->selector() );
    }
    ShouldNotReachHere();
    // return nm ? (LookupKey*)&nm->key : LookupKey::allocate(klass, _method->selector());
    //			// potential bug -- is key correct?  (super sends) -- fix this
    return nullptr;
}


MethodOop InlinedRecompilationScope::method() const {
    return desc->method();
}


bool_t RecompilationScope::wasNeverExecuted() const {
    MethodOop m   = method();
    SymbolOop sel = m->selector();
    if ( InliningPolicy::isInterpreterPredictedSmiSelector( sel ) or InliningPolicy::isInterpreterPredictedBoolSelector( sel ) ) {
        // predicted methods aren't called by interpreter if prediction succeeds
        return false;
    } else {
        return m->was_never_executed();
    }
}


// equivalent: test whether receiver scope and argument (a InlinedScope or a LookupKey) denote the same source-level scope

bool_t InterpretedRecompilationScope::equivalent( LookupKey * l ) const {
    return _key->equal( l );
}


bool_t InterpretedRecompilationScope::equivalent( InlinedScope * s ) const {
    return _key->equal( s->key() );
}


bool_t InlinedRecompilationScope::equivalent( LookupKey * l ) const {
    return desc->l_equivalent( l );
}


bool_t InlinedRecompilationScope::equivalent( InlinedScope * s ) const {
    if ( not s->isInlinedScope() )
        return false;
    InlinedScope * ss = ( InlinedScope * ) s;
    // don't use ss->rscope because it may not be set yet; but ss's sender
    // must have an rscope if ss is equivalent to this.
    return ss->senderByteCodeIndex() == desc->senderByteCodeIndex() and ss->sender()->rscope == sender();
}


bool_t PICRecompilationScope::equivalent( InlinedScope * s ) const {
// an PICRecompilationScope represents a non-inlined scope, so it can't be equivalent
    // to any InlinedScope
    return false;
}


bool_t PICRecompilationScope::equivalent( LookupKey * l ) const {
    if ( _desc not_eq nullptr )
        return _desc->l_equivalent( l );   // compiled case
    st_assert( not _sd->isSuperSend(), "this code probably doesn't work for super sends" );
    return klass == l->klass() and _method->selector() == l->selector();
}


RecompilationScope * NonDummyRecompilationScope::subScope( int byteCodeIndex, LookupKey * k ) const {
    // return the subscope matching the lookup
    st_assert( byteCodeIndex >= 0 and byteCodeIndex < ncodes, "byteCodeIndex out of range" );
    GrowableArray <RecompilationScope *> * list = _subScopes[ byteCodeIndex ];
    if ( list == nullptr )
        return new NullRecompilationScope;
    for ( int i = 0; i < list->length(); i++ ) {
        RecompilationScope * rs = list->at( i );
        if ( rs->equivalent( k ) )
            return rs;
    }
    return new NullRecompilationScope;
}


GrowableArray <RecompilationScope *> * NonDummyRecompilationScope::subScopes( int byteCodeIndex ) const {
    // return all subscopes at byteCodeIndex
    st_assert( byteCodeIndex >= 0 and byteCodeIndex < ncodes, "byteCodeIndex out of range" );
    GrowableArray <RecompilationScope *> * list = _subScopes[ byteCodeIndex ];
    if ( list == nullptr )
        return new GrowableArray <RecompilationScope *>( 1 );
    return list;
}


bool_t NonDummyRecompilationScope::hasSubScopes( int byteCodeIndex ) const {
    st_assert( byteCodeIndex >= 0 and byteCodeIndex < ncodes, "byteCodeIndex out of range" );
    return _subScopes[ byteCodeIndex ] not_eq nullptr;
}


void NonDummyRecompilationScope::addScope( int byteCodeIndex, RecompilationScope * s ) {
    st_assert( byteCodeIndex >= 0 and byteCodeIndex < ncodes, "byteCodeIndex out of range" );
    if ( _subScopes[ byteCodeIndex ] == nullptr )
        _subScopes[ byteCodeIndex ] = new GrowableArray <RecompilationScope *>( 5 );

    st_assert( not _subScopes[ byteCodeIndex ]->contains( s ), "already there" );
    // remove uninlineble markers if real scopes are added
    if ( _subScopes[ byteCodeIndex ]->length() == 1 and _subScopes[ byteCodeIndex ]->first()->isUninlinableScope() ) {
        _subScopes[ byteCodeIndex ]->pop();
    }
    _subScopes[ byteCodeIndex ]->append( s );
}


bool_t InterpretedRecompilationScope::isUncommonAt( int byteCodeIndex ) const {
    return DeferUncommonBranches;
}


bool_t NonDummyRecompilationScope::isUncommonAt( int byteCodeIndex ) const {
    if ( _subScopes[ byteCodeIndex ] ) {
        RecompilationScope * s = _subScopes[ byteCodeIndex ]->first();
        if ( s and s->isUntakenScope() ) {
            // send was never executed - make it uncommon
            return true;
        }
    }
    return false;
}


bool_t NonDummyRecompilationScope::isNotUncommonAt( int byteCodeIndex ) const {
    st_assert( byteCodeIndex >= 0 and byteCodeIndex < ncodes, "byteCodeIndex out of range" );

    // check if program got uncommon trap in the past
    for ( int i = 0; i < uncommon.length(); i++ ) {
        if ( uncommon.at( i )->byteCodeIndex() == byteCodeIndex )
            return true;
    }

    if ( _subScopes[ byteCodeIndex ] ) {
        RecompilationScope * s = _subScopes[ byteCodeIndex ]->first();
        if ( s and not s->isUntakenScope() ) {
            // send was executed at least once - don't make it uncommon
            return true;
        }
    }
    return false;
}


Expression * RecompilationScope::receiverExpression( PseudoRegister * p ) const {
    // guess that true/false map really means true/false object
    // (gives more efficient testing code)
    KlassOop k = receiverKlass();
    if ( k == trueObj->klass() ) {
        return new ConstantExpression( trueObj, p, nullptr );

    } else if ( k == falseObj->klass() ) {
        return new ConstantExpression( falseObj, p, nullptr );

    } else {
        return new KlassExpression( k, p, nullptr );
    }
}


Expression * UntakenRecompilationScope::receiverExpression( PseudoRegister * p ) const {
    return new UnknownExpression( p, nullptr, true );
}


bool_t InlinedRecompilationScope::isLite() const {
    return desc->is_lite();
}


bool_t PICRecompilationScope::isLite() const {
    return _desc and _desc->is_lite();
}


KlassOop InterpretedRecompilationScope::receiverKlass() const {
    return _key->klass();
}


KlassOop InlinedRecompilationScope::receiverKlass() const {
    return desc->selfKlass();
}


void NonDummyRecompilationScope::unify( NonDummyRecompilationScope * s ) {
    st_assert( ncodes == s->ncodes, "should be the same" );
    for ( int i = 0; i < ncodes; i++ ) {
        _subScopes[ i ] = s->_subScopes[ i ];
        if ( _subScopes[ i ] ) {
            for ( int j = _subScopes[ i ]->length() - 1; j >= 0; j-- ) {
                _subScopes[ i ]->at( j )->_sender = this;
            }
        }
    }
}


void PICRecompilationScope::unify( NonDummyRecompilationScope * s ) {
    NonDummyRecompilationScope::unify( s );
    if ( s->isPICScope() ) {
        uncommon.appendAll( &( ( PICRecompilationScope * ) s )->uncommon );
    }

}


constexpr int UntrustedPICLimit = 2;
constexpr int PICTrustLimit     = 2;


static void getCallees( const NativeMethod * nm, GrowableArray <ProgramCounterDescriptor *> *& taken_uncommon, GrowableArray <ProgramCounterDescriptor *> *& untaken_uncommon, GrowableArray <ProgramCounterDescriptor *> *& uninlinable, GrowableArray <NonDummyRecompilationScope *> *& sends, bool_t trusted, int level ) {
    // return a list of all uncommon branches of nm, plus a list
    // of all nativeMethods called by nm (in the form of PICScopes)
    // all lists are sorted by scope (biggest offset first)
    if ( theCompiler and CompilerDebug ) {
        cout( PrintRScopes )->print( "%*s*searching nm %#lx \"%s\" (%strusted; %ld callers)\n", 2 * level, "", nm, nm->_lookupKey.selector()->as_string(), trusted ? "" : "not ", nm->ncallers() );
    }
    taken_uncommon   = new GrowableArray <ProgramCounterDescriptor *>( 1 );
    untaken_uncommon = new GrowableArray <ProgramCounterDescriptor *>( 16 );
    uninlinable      = new GrowableArray <ProgramCounterDescriptor *>( 16 );
    sends            = new GrowableArray <NonDummyRecompilationScope *>( 10 );
    RelocationInformationIterator iter( nm );
    while ( iter.next() ) {
        if ( iter.type() == RelocationInformation::RelocationType::uncommon_type ) {
            GrowableArray <ProgramCounterDescriptor *> * l = iter.wasUncommonTrapExecuted() ? taken_uncommon : untaken_uncommon;
            l->append( nm->containingProgramCounterDescriptor( ( const char * ) iter.word_addr() ) );
        }
    }

    taken_uncommon->sort( &compare_pcDescs );
    untaken_uncommon->sort( &compare_pcDescs );

    if ( TypeFeedback ) {
        RelocationInformationIterator iter( nm );
        while ( iter.next() ) {
            if ( iter.type() not_eq RelocationInformation::RelocationType::ic_type )
                continue;
            CompiledInlineCache      * sd = iter.ic();
            ProgramCounterDescriptor * p  = nm->containingProgramCounterDescriptor( ( const char * ) sd );
            if ( sd->wasNeverExecuted() ) {
                // this send was never taken
                sends->append( new UntakenRecompilationScope( nullptr, p, false ) );
            } else if ( sd->isUninlinable() or sd->isMegamorphic() ) {
                // don't inline this send
                uninlinable->append( p );
            } else {
                bool_t useInfo = trusted or sd->ntargets() <= UntrustedPICLimit;
                if ( useInfo ) {
                    CompiledInlineCacheIterator it( sd );
                    while ( not it.at_end() ) {
                        NativeMethod * callee = it.compiled_method();
                        MethodOop m = it.interpreted_method();
                        ScopeDescriptor * desc;
                        int count;
                        if ( callee not_eq nullptr ) {
                            // compiled target
                            desc  = callee->scopes()->root();
                            count = callee->invocation_count() / max( 1, callee->ncallers() );
                        } else {
                            // interpreted target
                            desc  = nullptr;
                            count = m->invocation_count();
                        }
                        sends->append( new PICRecompilationScope( nm, p, sd, it.klass(), desc, callee, m, count, level, trusted ) );
                        it.advance();
                    }
                } else if ( theCompiler and CompilerDebug ) {
                    cout( PrintRScopes )->print( "%*s*not trusting PICs in sd %#lx \"%s\" (%ld cases)\n", 2 * level, "", sd, sd->selector()->as_string(), sd->ntargets() );
                }
            }
        }
        sends->sort( &PICRecompilationScope::compare );
        uninlinable->sort( &compare_pcDescs );
    }
}


NonDummyRecompilationScope * NonDummyRecompilationScope::constructRScopes( const NativeMethod * nm, bool_t trusted, int level ) {
    // construct nm's RecompilationScope tree and return the root
    // level > 0 means recursive invocation through a PICRecompilationScope (level
    // is the recursion depth); trusted means PICs info is considered accurate
    NonDummyRecompilationScope * current = nullptr;
    NonDummyRecompilationScope * root    = nullptr;
    GrowableArray <ProgramCounterDescriptor *>   * taken_uncommon;
    GrowableArray <ProgramCounterDescriptor *>   * untaken_uncommon;
    GrowableArray <ProgramCounterDescriptor *>   * uninlinable;
    GrowableArray <NonDummyRecompilationScope *> * sends;
    getCallees( nm, taken_uncommon, untaken_uncommon, uninlinable, sends, trusted, level );

    // visit each scope in the debug info and enter it into the tree
    FOR_EACH_SCOPE( nm->scopes(), s ) {
        // search s' sender RecompilationScope
        ScopeDescriptor            * sender  = s->sender();
        NonDummyRecompilationScope * rsender = current;
        for ( ; rsender; rsender = rsender->sender() ) {
            if ( rsender->isInlinedScope() and ( ( InlinedRecompilationScope * ) rsender )->desc->is_equal( sender ) )
                break;
        }
        int                      byteCodeIndex = sender ? s->senderByteCodeIndex() : IllegalByteCodeIndex;
        current = new InlinedRecompilationScope( ( InlinedRecompilationScope * ) rsender, byteCodeIndex, nm, s, level );
        if ( not root ) {
            root = current;
            root->_invocationCount = nm->invocation_count();
        }

        // enter byteCodeIndexs with taken uncommon branches
        while ( taken_uncommon->nonEmpty() and taken_uncommon->top()->_scope == s->offset() ) {
            current->uncommon.push( new RUncommonBranch( current, taken_uncommon->pop() ) );
        }
        // enter info from PICs
        while ( sends->nonEmpty() and sends->top()->scopeID() == s->offset() ) {
            NonDummyRecompilationScope * s = sends->pop();
            s->_sender = current;
            current->addScope( s->senderByteCodeIndex(), s );
        }
        // enter untaken uncommon branches
        ProgramCounterDescriptor * u;
        while ( untaken_uncommon->nonEmpty() and ( u = untaken_uncommon->top() )->_scope == s->offset() ) {
            new UntakenRecompilationScope( current, u, true );    // will add it as subscope of current
            untaken_uncommon->pop();
        }
        // enter uninlinable sends
        while ( uninlinable->nonEmpty() and ( u      = uninlinable->top() )->_scope == s->offset() ) {
            // only add uninlinable markers for sends that have no inlined cases
            int byteCodeIndex = u->_byteCodeIndex;
            if ( not current->hasSubScopes( byteCodeIndex ) ) {
                new UninlinableRecompilationScope( current, byteCodeIndex );    // will add it as subscope of current
            }
            uninlinable->pop();
        }
    }
    st_assert( sends->isEmpty(), "sends should have been connected to rscopes" );
    st_assert( taken_uncommon->isEmpty(), "taken uncommon branches should have been connected to rscopes" );
    st_assert( untaken_uncommon->isEmpty(), "untaken uncommon branches should have been connected to rscopes" );
    st_assert( uninlinable->isEmpty(), "uninlinable sends should have been connected to rscopes" );
    return root;
}


void NonDummyRecompilationScope::constructSubScopes( bool_t trusted ) {
    // construct all our (immediate) subscopes
    MethodOop m = method();
    if ( m->is_accessMethod() )
        return;
    CodeIterator iter( m );
    do {
        switch ( iter.send() ) {
            case ByteCodes::SendType::interpreted_send:
            case ByteCodes::SendType::compiled_send:
            case ByteCodes::SendType::predicted_send:
            case ByteCodes::SendType::accessor_send:
            case ByteCodes::SendType::polymorphic_send:
            case ByteCodes::SendType::primitive_send  : {
                NonDummyRecompilationScope * s  = nullptr;
                InterpretedInlineCache     * ic = iter.ic();
                for ( InterpretedInlineCacheIterator it( ic ); not it.at_end(); it.advance() ) {
                    if ( it.is_compiled() ) {
                        NativeMethod               * nm = it.compiled_method();
                        NonDummyRecompilationScope * s  = constructRScopes( nm, trusted and trustPICs( m ), _level + 1 );
                        addScope( iter.byteCodeIndex(), s );
                    } else {
                        MethodOop m = it.interpreted_method();
                        LookupKey * k = LookupKey::allocate( it.klass(), it.selector() );
                        new InterpretedRecompilationScope( this, iter.byteCodeIndex(), k, m, _level + 1, trusted and trustPICs( m ) );
                        // NB: constructor adds callee to our subScope list
                    }
                }
            }
                break;
            case ByteCodes::SendType::megamorphic_send:
                new UninlinableRecompilationScope( this, iter.byteCodeIndex() );  // constructor adds callee to our subScope list
                break;
            case ByteCodes::SendType::no_send:
                break;
            default: st_fatal1( "unexpected send type %d", iter.send() );
        }
    } while ( iter.advance() );
}


bool_t NonDummyRecompilationScope::trustPICs( MethodOop m ) {
    // should the PICs in m be trusted?
    SymbolOop sel = m->selector();
    if ( sel == vmSymbols::plus() or sel == vmSymbols::minus() or sel == vmSymbols::multiply() or sel == vmSymbols::divide() ) {
        // code Space optimization: try to avoid unnecessary mixed-type arithmetic
        return false;
    } else {
        return true;    // can't easily determine number of callers
    }
}


bool_t PICRecompilationScope::trustPICs( const NativeMethod * nm ) {
    // should the PICs in nm be trusted?
    int invoc = nm->invocation_count();
    if ( invoc < MinInvocationsBeforeTrust )
        return false;
    int       ncallers = nm->ncallers();
    SymbolOop sel      = nm->_lookupKey.selector();
    if ( sel == vmSymbols::plus() or sel == vmSymbols::minus() or sel == vmSymbols::multiply() or sel == vmSymbols::divide() ) {
        // code Space optimization: try to avoid unnecessary mixed-type arithmetic
        return ncallers <= 1;
    } else {
        return ncallers <= PICTrustLimit;
    }
}


void PICRecompilationScope::extend() {
    // try to follow PolymorphicInlineCache info one level deeper (i.e. extend rscope tree)
    if ( _extended )
        return;
    if ( nm and not nm->isZombie() ) {
        // search the callee for type info
        NonDummyRecompilationScope * s = constructRScopes( nm, trusted and trustPICs( nm ), _level + 1 );
        // s and receiver represent the same scope - unify them
        unify( s );
    } else {
        constructSubScopes( false );    // search interpreted inline caches but don't trust their info
    }
    _extended = true;
}


void InterpretedRecompilationScope::extend() {
    // try to follow PolymorphicInlineCache info one level deeper (i.e. extend rscope tree)
    if ( not extended ) {
        // search the inline caches for type info
        constructSubScopes( _is_trusted );
        if ( PrintRScopes )
            printTree( _senderByteCodeIndex, _level );
    }
    extended = true;
}


void RecompilationScope::print() {
    _console->print_cr( "; sender: %#lx@%ld; count %ld", PrintHexAddresses ? _sender : 0, _senderByteCodeIndex, _invocationCount );
}


void NonDummyRecompilationScope::printSubScopes() const {
    int i = 0;
    for ( ; i < ncodes and _subScopes[ i ] == nullptr; i++ );
    if ( i < ncodes ) {
        _console->print( "{ " );
        for ( int i = 0; i < ncodes; i++ ) {
            _console->print( "%#lx ", PrintHexAddresses ? _subScopes[ i ] : 0 );
        }
        _console->print( "}" );
    } else {
        _console->print( "none" );
    }
}


void InterpretedRecompilationScope::print_short() {
    _console->print( "((InterpretedRecompilationScope*)%#lx) \"%s\" #%ld", PrintHexAddresses ? this : 0, _key->print_string(), _invocationCount );
}


void InlinedRecompilationScope::print_short() {
    _console->print( "((InlinedRecompilationScope*)%#lx) \"%s\" #%ld", PrintHexAddresses ? this : 0, desc->selector()->as_string(), _invocationCount );
}


void InlinedRecompilationScope::print() {
    print_short();
    _console->print( ": scope %#lx; subScopes: ", PrintHexAddresses ? desc : 0 );
    printSubScopes();
    if ( uncommon.nonEmpty() ) {
        _console->print( "; uncommon " );
        uncommon.print();
    }
    RecompilationScope::print();
}


void PICRecompilationScope::print_short() {
    _console->print( "((PICRecompilationScope*)%#lx) \"%s\" #%ld", PrintHexAddresses ? this : 0, method()->selector()->as_string(), _invocationCount );
}


void PICRecompilationScope::print() {
    print_short();
    _console->print( ": InlineCache %#lx; subScopes: ", PrintHexAddresses ? _sd : 0 );
    printSubScopes();
    if ( uncommon.nonEmpty() ) {
        _console->print( "; uncommon " );
        uncommon.print();
    }
}


void UntakenRecompilationScope::print_short() {
    _console->print( "((UntakenRecompilationScope*)%#lx) \"%s\"", PrintHexAddresses ? this : 0 );
}


void UntakenRecompilationScope::print() {
    print_short();
    st_assert( !*_subScopes, "should have no subscopes" );
    st_assert( uncommon.isEmpty(), "should have no uncommon branches" );
}


void RUncommonBranch::print() {
    _console->print_cr( "((RUncommonScope*)%#lx) : %#lx@%ld", PrintHexAddresses ? this : 0, PrintHexAddresses ? scope : 0, byteCodeIndex() );
}


void UninlinableRecompilationScope::print_short() {
    _console->print( "((UninlinableRecompilationScope*)%#lx)", PrintHexAddresses ? this : 0 );
}


void NullRecompilationScope::print_short() {
    _console->print( "((NullRecompilationScope*)%#lx)", PrintHexAddresses ? this : 0 );
}


void NullRecompilationScope::printTree( int byteCodeIndex, int level ) const {
}


void RecompilationScope::printTree( int byteCodeIndex, int level ) const {
    _console->print( "%*s%3ld: ", level * 2, "", byteCodeIndex );
    ( ( RecompilationScope * ) this )->print_short();
    _console->print_cr( "" );
}


void NonDummyRecompilationScope::printTree( int senderByteCodeIndex, int level ) const {
    RecompilationScope::printTree( senderByteCodeIndex, level );

    int u = 0;          // current position in uncommon

    for ( int byteCodeIndex = 0; byteCodeIndex < ncodes; byteCodeIndex++ ) {
        if ( _subScopes[ byteCodeIndex ] ) {
            for ( int j = 0; j < _subScopes[ byteCodeIndex ]->length(); j++ ) {
                _subScopes[ byteCodeIndex ]->at( j )->printTree( byteCodeIndex, level + 1 );
            }
        }
        int j = u;
        for ( ; j < uncommon.length() and uncommon.at( j )->byteCodeIndex() < byteCodeIndex; u++, j++ );
        if ( j < uncommon.length() and uncommon.at( j )->byteCodeIndex() == byteCodeIndex ) {
            _console->print_cr( "  %*s%3ld: uncommson", level * 2, "", byteCodeIndex );
        }
    }
}


void InliningDatabaseRecompilationScope::print() {
    print_short();
    printSubScopes();
}


void InliningDatabaseRecompilationScope::print_short() {
    _console->print( "((InliningDatabaseRecompilationScope*)%#lx)  \"%s\"", PrintHexAddresses ? this : 0, _key->print_string() );
}


bool_t InliningDatabaseRecompilationScope::equivalent( InlinedScope * s ) const {
    Unimplemented();
    return false;
}


bool_t InliningDatabaseRecompilationScope::equivalent( LookupKey * l ) const {
    return _key->equal( l );
}


bool_t InliningDatabaseRecompilationScope::isUncommonAt( int byteCodeIndex ) const {
    // if the DB has an uncommon branch at byteCodeIndex, treat it as untaken
    return _uncommon->at( byteCodeIndex );
}


bool_t InliningDatabaseRecompilationScope::isNotUncommonAt( int byteCodeIndex ) const {
    // if there's no uncommon branch in the DB, don't use it here either
    return not _uncommon->at( byteCodeIndex );
}


int InlinedRecompilationScope::inlining_database_size() {
    int result = 1; // Count this node

    for ( int i = 0; i < ncodes; i++ ) {
        if ( _subScopes[ i ] ) {
            for ( int j = 0; j < _subScopes[ i ]->length(); j++ ) {
                result += _subScopes[ i ]->at( j )->inlining_database_size();
            }
        }
    }
    return result;
}



// don't file out PolymorphicInlineCache scopes in the output since they're not inlined into the current NativeMethod;
// same for interpreted scopes

ProgramCounterDescriptor * next_uncommon( int scope, int u, GrowableArray <ProgramCounterDescriptor *> * uncommon ) {
    if ( uncommon == nullptr or u >= uncommon->length() )
        return nullptr;   // none left
    ProgramCounterDescriptor * pc = uncommon->at( u );
    return ( pc->_scope == scope ) ? pc : nullptr;
}


void UninlinableRecompilationScope::print_inlining_database_on( ConsoleOutputStream * stream, GrowableArray <ProgramCounterDescriptor *> * uncommon, int byteCodeIndex, int level ) {
    // not necessary to actually write out this info since DB-driven compilation won't inline anything not inlined in DB
    // stream->print_cr("%*s%d uninlinable", level * 2, "", byteCodeIndex);
}


void InlinedRecompilationScope::print_inlining_database_on( ConsoleOutputStream * stream, GrowableArray <ProgramCounterDescriptor *> * uncommon, int byteCodeIndex, int level ) {
    // File out level and byteCodeIndex
    if ( byteCodeIndex not_eq -1 ) {
        stream->print( "%*s%d ", level * 2, "", byteCodeIndex );
    }

    LookupKey * k = key();
    k->print_inlining_database_on( stream );
    stream->cr();

    // find scope in uncommon list
    int scope = desc->offset();
    int u     = 0;
    for ( ; uncommon and u < uncommon->length() - 1 and uncommon->at( u )->_scope < scope; u++ );
    ProgramCounterDescriptor * current_uncommon = next_uncommon( scope, u, uncommon );

    // File out subscopes
    for ( int i = 0; i < ncodes; i++ ) {
        if ( _subScopes[ i ] ) {
            for ( int j = 0; j < _subScopes[ i ]->length(); j++ ) {
                _subScopes[ i ]->at( j )->print_inlining_database_on( stream, uncommon, i, level + 1 );
            }
        }
        if ( current_uncommon and current_uncommon->_byteCodeIndex == i ) {
            // NativeMethod has an uncommon branch at this byteCodeIndex
            stream->print_cr( "%*s%d uncommon", ( level + 1 ) * 2, "", i );
            // advance to next uncommon branch
            u++;
            current_uncommon = next_uncommon( scope, u, uncommon );
        }
    }
}
