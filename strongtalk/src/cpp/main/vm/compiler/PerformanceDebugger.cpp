//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/compiler/PerformanceDebugger.hpp"
#include "vm/compiler/Compiler.hpp"
#include "vm/utilities/GrowableArray.hpp"
#include "vm/compiler/PseudoRegister.hpp"
#include "vm/utilities/OutputStream.hpp"
#include "vm/primitives/primitives.hpp"
#include "vm/interpreter/InterpretedInlineCache.hpp"
#include "vm/utilities/StringOutputStream.hpp"


PerformanceDebugger::PerformanceDebugger( Compiler *c ) {
    _compiler                            = c;
    _compileAlreadyReported              = false;
    _blockPseudoRegisters                = new GrowableArray<BlockPseudoRegister *>( 5 );
    _reports                             = new GrowableArray<char *>( 5 );
    _stringStream                        = nullptr;
    _notInlinedBecauseNativeMethodTooBig = nullptr;
}


void PerformanceDebugger::start_report() {
    _stringStream = new StringOutputStream( 256 );
    report_compile();
}


void PerformanceDebugger::stop_report() {
    char *report = _stringStream->as_string();

    for ( std::int32_t i = _reports->length() - 1; i >= 0; i-- ) {
        if ( strcmp( _reports->at( i ), report ) == 0 )
            return;  // already printed identical msg
    }

    _console->print( report );
    _reports->append( report );
    _stringStream = nullptr;
}


void PerformanceDebugger::report_compile() {
    if ( not _compileAlreadyReported ) {
        _compileAlreadyReported = true;
        spdlog::info( "\n*while compiling NativeMethod for %s:", _compiler->key->toString() );
    }
}


// always put one of these in your top_level reporting method
// see PerformanceDebugger::report_context for an example

class Reporter {

private:
    PerformanceDebugger *_performanceDebugger;

public:
    Reporter( PerformanceDebugger *d ) {
        _performanceDebugger = d;
        d->start_report();
    }


    ~Reporter() {
        _performanceDebugger->stop_report();
    }

};


void PerformanceDebugger::finish_reporting() {
    // output messages about non-inlined sends
    if ( DebugPerformance and _notInlinedBecauseNativeMethodTooBig ) {
        Reporter r( this );
        spdlog::info( "  did not inline the following sends because the NativeMethod was getting too big:" );
        std::int32_t len = _notInlinedBecauseNativeMethodTooBig->length();
        std::int32_t i   = 0;
        for ( ; i < min( 9, len ); i++ ) {
            if ( i % 3 == 0 )
                spdlog::info( "" );
            InlinedScope *s = _notInlinedBecauseNativeMethodTooBig->at( i );
            spdlog::info( "%s  ", s->key()->toString() );
        }
        if ( i < len )
            spdlog::info( "\n    (%d more sends omitted)\n", len );
//        _stringStream->put( '\n' );
    }
}


void PerformanceDebugger::report_context( InlinedScope *s ) {
    if ( not DebugPerformance )
        return;
    Reporter                    r( this );
    GrowableArray<Expression *> *temps = s->contextTemporaries();
    const std::int32_t          len    = temps->length();
    std::int32_t                nused  = 0;
    for ( std::int32_t          i      = 0; i < len; i++ ) {
        PseudoRegister *r = temps->at( i )->pseudoRegister();
        if ( r->uplevelR() or r->uplevelW() or ( r->isBlockPseudoRegister() and not r->isUnused() ) )
            nused++;
    }
    if ( nused == 0 ) {
        spdlog::info( "  could not eliminate context of scope %s (fixable compiler restriction; should be eliminated)\n", s->key()->toString() );
    } else {
        spdlog::info( "  could not eliminate context of scope %s; temp(s) still used: ", s->key()->toString() );
        for ( std::int32_t j = 0; j < len; j++ ) {
            PseudoRegister *r = temps->at( j )->pseudoRegister();
            if ( r->uplevelR() or r->uplevelW() ) {
                spdlog::info( "%d ", j );
            } else if ( r->isBlockPseudoRegister() and not r->isUnused() ) {
                spdlog::info( "%d (non-inlined block)", j );
            }
        }
        spdlog::info( "" );
    }
}


void PerformanceDebugger::report_toobig( InlinedScope *s ) {
    if ( not DebugPerformance )
        return;
    report_compile();
    if ( not _notInlinedBecauseNativeMethodTooBig )
        _notInlinedBecauseNativeMethodTooBig = new GrowableArray<InlinedScope *>( 20 );
    _notInlinedBecauseNativeMethodTooBig->append( s );
}


void PerformanceDebugger::report_uncommon( bool reoptimizing ) {
    if ( not DebugPerformance )
        return;
    Reporter r( this );
    if ( reoptimizing ) {
        spdlog::info( " -- reoptimizing previously compiled 'uncommon' version of NativeMethod" );
    } else {
        spdlog::info( " -- creating 'uncommon' version of NativeMethod" );
    }
}


void PerformanceDebugger::report_primitive_failure( PrimitiveDescriptor *pd ) {
    // suppress methods for uncommon compiles -- too many (and not interesting)
    if ( not DebugPerformance or theCompiler->is_uncommon_compile() )
        return;
    Reporter r( this );
    spdlog::info( " primitive failure of %s not uncommon\n", pd->name() );
}


void PerformanceDebugger::report_block( Node *n, BlockPseudoRegister *blk, const char *what ) {
    if ( not DebugPerformance )
        return;
    if ( _blockPseudoRegisters->contains( blk ) )
        return;
    if ( blk->method()->is_clean_block() )
        return;
    Reporter r( this );
    spdlog::info( " could not eliminate block in [{}]", blk->method()->home()->selector()->print_value_string() );
//    blk->method()->home()->selector()->print_symbol_on( _stringStream );
    spdlog::info( " because it is [{}] in scope [{}] at bytecode [{}]", what, n->scope()->key()->toString(), n->byteCodeIndex() );
    InterpretedInlineCache *ic = n->scope()->method()->ic_at( n->byteCodeIndex() );
    if ( ic ) {
        spdlog::info( " (send of [{}])", ic->selector()->copy_null_terminated() );
    }
    spdlog::info( "" );
    _blockPseudoRegisters->append( blk );
}
