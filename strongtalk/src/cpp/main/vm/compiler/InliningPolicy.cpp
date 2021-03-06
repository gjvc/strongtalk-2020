//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/oop/MethodOopDescriptor.hpp"
#include "vm/runtime/VMSymbol.hpp"
#include "vm/compiler/InliningPolicy.hpp"
#include "vm/compiler/Compiler.hpp"
#include "vm/compiler/RecompilationScope.hpp"
#include "vm/compiler/Scope.hpp"
#include "vm/recompiler/Recompilation.hpp"
#include "vm/recompiler/RecompilerFrame.hpp"
#include "vm/compiler/NodeFactory.hpp"


bool InliningPolicy::shouldNotInline() const {
    if ( _methodOop->method_inlining_info() == MethodOopDescriptor::never_inline ) {
        return true;
    }
    const SymbolOop sel = _methodOop->selector();
    return ( sel == vmSymbols::error() or sel == vmSymbols::error_() or sel == vmSymbols::subclassResponsibility() );
}


bool InliningPolicy::isCriticalSmiSelector( const SymbolOop sel ) {
    // true if performance-critical small_int_t method in standard library
    // could also handle these by putting a bit in the methodOops
    return sel == vmSymbols::plus() or sel == vmSymbols::minus() or sel == vmSymbols::multiply() or sel == vmSymbols::divide() or sel == vmSymbols::mod() or sel == vmSymbols::equal() or sel == vmSymbols::not_equal() or sel == vmSymbols::less_than() or sel == vmSymbols::less_than() or sel == vmSymbols::less_than_or_equal() or sel == vmSymbols::greater_than() or sel == vmSymbols::greater_than_or_equal() or sel == vmSymbols::double_equal() or sel == vmSymbols::bitAnd_() or sel == vmSymbols::bitOr_() or sel == vmSymbols::bitXor_() or sel == vmSymbols::bitShift_() or sel == vmSymbols::bitInvert();
}


const char *InliningPolicy::basic_shouldInline( MethodOop method ) {

    // should the interpreted method be inlined?
    if ( method->method_inlining_info() == MethodOopDescriptor::always_inline ) {
        return nullptr;
    }
    calleeCost = method->estimated_inline_cost( receiverKlass() );

    if ( method->is_blockMethod() ) {
        // even large blocks should be inlined if they make up most of their home's code
        std::int32_t parentCost = method->parent()->estimated_inline_cost( receiverKlass() );
        st_assert( parentCost > calleeCost, "must be higher" );
        if ( float( parentCost - calleeCost ) / parentCost * 100.0 < MinBlockCostFraction )
            return nullptr;
    }

    // compute the cost limit based on the provided arguments
    std::int32_t       cost_limit = method->is_blockMethod() ? MaxBlockInlineCost : MaxFnInlineCost;
    for ( std::int32_t i          = method->number_of_arguments() - 1; i >= 0; i-- ) {
        KlassOop k = nthArgKlass( i );
        if ( k and k->klass_part()->oopIsBlock() ) {
            cost_limit += BlockArgAdditionalAllowedInlineCost;
        }
    }
    if ( calleeCost < cost_limit ) {
        return nullptr;
    }
    if ( isBuiltinMethod() ) {
        return nullptr;
    }
    return "too big";
}


bool InliningPolicy::isCriticalArraySelector( const SymbolOop sel ) {
    return sel == vmSymbols::at() or sel == vmSymbols::at_put() or sel == vmSymbols::size();
}


bool InliningPolicy::isCriticalBoolSelector( const SymbolOop sel ) {
    return sel == vmSymbols::and_() or sel == vmSymbols::or_() or sel == vmSymbols::_and() or sel == vmSymbols::_or() or sel == vmSymbols::and1() or sel == vmSymbols::or1() or sel == vmSymbols::_and() or sel == vmSymbols::_not() or sel == vmSymbols::xor_() or sel == vmSymbols::eqv_();
}


bool InliningPolicy::isPredictedSmiSelector( const SymbolOop sel ) {
    return sel not_eq vmSymbols::equal() and sel not_eq vmSymbols::not_equal() and isCriticalSmiSelector( sel );
}


bool InliningPolicy::isPredictedArraySelector( const SymbolOop sel ) {
    return isCriticalArraySelector( sel );
}


bool InliningPolicy::isPredictedBoolSelector( const SymbolOop sel ) {
    return isCriticalBoolSelector( sel );
}


bool InliningPolicy::isInterpreterPredictedSmiSelector( const SymbolOop sel ) {
    // true if performance-critical small_int_t method in standard library
    // could also handle these by putting a bit in the methodOops
    return sel == vmSymbols::plus() or sel == vmSymbols::minus() or sel == vmSymbols::multiply() or sel == vmSymbols::divide() or sel == vmSymbols::mod() or sel == vmSymbols::equal() or sel == vmSymbols::not_equal() or sel == vmSymbols::less_than() or sel == vmSymbols::less_than() or sel == vmSymbols::less_than_or_equal() or sel == vmSymbols::greater_than() or sel == vmSymbols::greater_than_or_equal();
}


bool InliningPolicy::isInterpreterPredictedArraySelector( const SymbolOop sel ) {
    st_unused( sel ); // unused
    return false;
}


bool InliningPolicy::isInterpreterPredictedBoolSelector( const SymbolOop sel ) {
    st_unused( sel ); // unused
    return false;
}


bool InliningPolicy::isBuiltinMethod() const {
    // true if performance-critical method in standard library
    // could also handle these by putting a bit in the methodOops
    if ( _methodOop->method_inlining_info() == MethodOopDescriptor::always_inline ) {
        return true;
    }
    const SymbolOop sel   = _methodOop->selector();
    const KlassOop  klass = receiverKlass();
    const bool      isNum = klass == Universe::smiKlassObject() or klass == Universe::doubleKlassObject();
    if ( isNum and isCriticalSmiSelector( sel ) ) {
        return true;
    }
    const bool isArr = klass == Universe::objectArrayKlassObject() or klass == Universe::byteArrayKlassObject() or klass == Universe::symbolKlassObject() or false;    // probably should add doubleByteArray et al
    if ( isArr and isCriticalArraySelector( sel ) ) {
        return true;
    }
    const bool isBool = klass == Universe::trueObject()->klass() or klass == Universe::falseObject()->klass();
    if ( isBool and isCriticalBoolSelector( sel ) ) {
        return true;
    }
    return false;
}


KlassOop CompilerInliningPolicy::nthArgKlass( std::int32_t i ) const {
    std::int32_t first = _sender->exprStack()->length() - _methodOop->number_of_arguments();
    Expression   *e    = _sender->exprStack()->at( first + i );
    return e->hasKlass() ? e->klass() : nullptr;
}


KlassOop CompilerInliningPolicy::receiverKlass() const {
    return _receiver->klass();
}


const char *CompilerInliningPolicy::shouldInline( InlinedScope *s, InlinedScope *callee ) {
    if ( callee == nullptr ) {
        return "cannot handle super sends";
    }

    if ( callee->rscope->isDatabaseScope() ) {
        // This scope is provided by the inlining database and should always be inlined.
        return nullptr;
    }

    if ( s->rscope->isDatabaseScope() ) {
        // The caller scope is provided by the inlining database but the callee scope is not.
        // ignore the callee when inlining.
        return "do not inline (Inlining Database)";
    }

    // should check for existing compiled version here -- fix this
    _sender    = s;
    _methodOop = callee->method();
    _receiver  = callee->self();
    if ( NodeFactory::_cumulativeCost > MaxNmInstrSize ) {
        theCompiler->reporter->report_toobig( callee );
        return "method getting too big";
    }

    if ( shouldNotInline() ) {
        return "should not inline (special)";
    }

    // performance bug: should check how many recursive calls the method has -- unrolling factorial
    // to depth N gives N copies, but unrolling fibonacci gives 2**N
    // also, should look at call chain to estimate how big inlined recursion will get
    if ( _sender->isRecursiveCall( _methodOop, callee->selfKlass(), MaxRecursionUnroll ) ) {
        return "recursive";
    }

    return basic_shouldInline( _methodOop );
}


KlassOop RecompilerInliningPolicy::nthArgKlass( std::int32_t i ) const {
    return _deltaVirtualFrame ? _deltaVirtualFrame->argument_at( i )->klass() : nullptr;
}


KlassOop RecompilerInliningPolicy::receiverKlass() const {
    return _deltaVirtualFrame ? theRecompilation->receiverOf( _deltaVirtualFrame )->klass() : nullptr;
}


const char *RecompilerInliningPolicy::shouldInline( RecompilerFrame *recompilerFrame ) {

    // determine if _recompilerFrame's method or NativeMethod should be inlined into its caller
    // use compiled-code size if available, even for interpreted methods
    // (gives better info on how big method will become since it includes inlined methods)
    // return nullptr if ok, reason for not inlining otherwise (for performance debugging)

    // for now, always inline super rfrfsends
    extern bool SuperSendsAreAlwaysInlined;
    st_assert( SuperSendsAreAlwaysInlined, "SuperSendsAreAlwaysInlined is set to true; fix this" );
    if ( recompilerFrame->is_super() )
        return nullptr;

    _deltaVirtualFrame = recompilerFrame->top_vframe();
    _methodOop         = recompilerFrame->top_method();
    const NativeMethod *nm = nullptr;
    if ( recompilerFrame->is_interpreted() ) {
        // check to see if we have a compiled version of the method
        const LookupKey *key = recompilerFrame->key();
        if ( key ) {
            nm = Universe::code->lookup( key );
        } else {
            // interpreted block; should check for compiled block
            // fix this later
        }
    } else {
        st_assert( recompilerFrame->is_compiled(), "oops" );
        nm = ( (CompiledRecompilerFrame *) recompilerFrame )->nm();
    }
    if ( nm ) {
        return shouldInline( nm );
    } else {
        return basic_shouldInline( _methodOop );
    }
}


const char *RecompilerInliningPolicy::shouldInline( const NativeMethod *nm ) {

    if ( not CodeSizeImpactsInlining )
        return nullptr;

    if ( _methodOop->method_inlining_info() == MethodOopDescriptor::always_inline )
        return nullptr;

    // compute the allowable cost based on the method type and the provided arguments
    std::int32_t cost_limit = _methodOop->is_blockMethod() ? MaxBlockInstrSize : MaxFnInstrSize;
    std::int32_t i          = _methodOop->number_of_arguments();
    while ( i-- > 0 ) {
        KlassOop k = nthArgKlass( i );
        if ( k and k->klass_part()->oopIsBlock() )
            cost_limit += BlockArgAdditionalInstrSize;
    }

    if ( nm->size() < cost_limit )
        return nullptr;    // ok

    if ( isBuiltinMethod() )
        return nullptr;        // ok, special case (?)

    return "too big (compiled)";
}
