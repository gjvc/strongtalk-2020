//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/system/asserts.hpp"
#include "vm/system/platform.hpp"
#include "vm/compiler/NodeBuilder.hpp"
#include "vm/utilities/GrowableArray.hpp"
#include "vm/primitives/primitives.hpp"
#include "vm/compiler/Expression.hpp"
#include "vm/compiler/OpCode.hpp"
#include "vm/compiler/Node.hpp"
#include "vm/memory/vmSymbols.hpp"
#include "vm/memory/oopFactory.hpp"
#include "vm/runtime/ResourceObject.hpp"


// the PrimitiveInliner inlines primitives (if possible) or generates a non-inlined call
// there's one PrimitiveInliner for each primitive call encountered


class PrimitiveInliner : public PrintableResourceObject {

private:
    NodeBuilder         *_gen;                // the active node generator
    std::int32_t        _byteCodeIndex;                // byteCodeIndex of primitive call
    PrimitiveDescriptor *_primitiveDescriptor;                // the primitive
    MethodInterval      *_failure_block;            // code in primitive failure block

    InlinedScope                *_scope;                // the current scope
    ExpressionStack             *_expressionStack;            // the current expression stack
    GrowableArray<Expression *> *_params;            // the copy of the top number_of_parameters() elements of _exprStack NB: don't use _params->at(...) -- use parameter() below
    bool                        _usingUncommonTrap;                // using uncommon trap for prim. failure?
    bool                        _cannotFail;                        // true if primitive can't fail

    std::int32_t number_of_parameters() const {
        return _primitiveDescriptor->number_of_parameters();
    }


    Expression *parameter( std::int32_t index ) const {
        return _params->at( index );
    }        // parameter of primitive call
    bool is_power_of_2( std::int32_t x ) const {
        return x > 0 and ( x & ( x - 1 ) ) == 0;
    }    // true if there's an n with 2^n = x
    std::int32_t log2( std::int32_t x ) const;                // if is_power_of_2(x) then 2^(log2(x)) = x

    void assert_failure_block();            // debugging: asserts that there's a failure block
    void assert_no_failure_block();        // debugging: asserts that there's no failure block
    void assert_receiver();            // debugging: asserts that the first parameter is self

    Expression *tryConstantFold();                // try constant-folding the primitive
    Expression *tryTypeCheck();                    // try constant-folding primitive failures
    Expression *tryInline();                    // try inlining or special-casing the primitive
    Expression *genCall( bool canFail );                // generate non-inlined primitive call
    Expression *primitiveFailure( SymbolOop failureCode );    // handle primitive that always fail
    Expression *merge_failure_block( Node *ok_exit, Expression *ok_result, Node *failure_exit, Expression *failure_code, bool ok_result_is_read_only = true );

    SymbolOop failureSymbolForArg( std::int32_t i );            // error string for "n.th arg has wrong type"
    bool shouldUseUncommonTrap();            // use uncommon trap for primitive failure?
    bool basic_shouldUseUncommonTrap() const;

    Expression *smi_ArithmeticOp( ArithOpCode op, Expression *x, Expression *y );

    Expression *smi_Comparison( BranchOpCode cond, Expression *x, Expression *y );

    Expression *smi_BitOp( ArithOpCode op, Expression *x, Expression *y );

    Expression *smi_Div( Expression *x, Expression *y );

    Expression *smi_Mod( Expression *x, Expression *y );

    Expression *smi_Shift( Expression *x, Expression *y );

    Expression *array_size();

    Expression *array_at_ifFail( ArrayAtNode::AccessType access_type );

    Expression *array_at_put_ifFail( ArrayAtPutNode::AccessType access_type );

    Expression *obj_new();

    Expression *obj_shallowCopy();

    Expression *obj_equal();

    Expression *obj_class( bool has_receiver );

    Expression *obj_hash( bool has_receiver );

    Expression *proxy_byte_at();

    Expression *proxy_byte_at_put();

    Expression *block_primitiveValue();

public:
    PrimitiveInliner( NodeBuilder *gen, PrimitiveDescriptor *pdesc, MethodInterval *failure_block );

    void generate();

    static Expression *generate_cond( BranchOpCode cond, NodeBuilder *gen, PseudoRegister *resPReg );
    // generates cond. branch and code to assign true/false to resPReg

    void print();
};
