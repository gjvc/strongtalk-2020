//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/system/asserts.hpp"
#include "vm/memory/allocation.hpp"
#include "vm/interpreter/MethodIterator.hpp"
#include "vm/utilities/GrowableArray.hpp"


class Node;
class AssignNode;
class TypeTestNode;
class ContextInitNode;
class NonTrivialNode;
class MergeNode;
class Inliner;
class InlinedScope;
class ConstantExpression;
class PseudoRegister;
class Expression;
class ExpressionStack;
class BlockPseudoRegister;
class SendInfo;
class SinglyAssignedPseudoRegister;


// The NodeBuilder generates the intermediate representation for a method or block.

class NodeBuilder : public CustomizedMethodClosure {
    private:
        ExpressionStack * _expressionStack;        // current expression stack
        Inliner         * _inliner;        // my inliner
        InlinedScope    * _scope;            // scope for which this NodeBuilder is generating code
        Node            * _current;        // where new nodes are appended

        void append_exit( Node * exitNode );    // append an exit node (UncommonNode, Return, etc.)
        void append1( Node * node );

        void branch( MergeNode * target );

        void comment( const char * s );

        GrowableArray <PseudoRegister *> * copyCurrentExprStack();

        void access_temporary( int no, int context, bool_t push );

        GrowableArray <PseudoRegister *> * pass_arguments( PseudoRegister * self, int nofArgs );

        void splitMergeExpression( Expression * expr, TypeTestNode * test );

        GrowableArray <Expression *> * splittablePaths( const Expression * expr, const TypeTestNode * test ) const;

        GrowableArray <NonTrivialNode *> * nodesBetween( Node * from, Node * to );

        MergeNode * insertMergeBefore( Node * n );

        Expression * copy_into_context( Expression * e, int no );

        void materialize( PseudoRegister * r, GrowableArray <BlockPseudoRegister *> * materialized );    // materialize block (always call before storing/assigning PseudoRegister)

        bool_t abortIfDead( Expression * e );                        // helper function for dead code handling
        void generate_subinterval( MethodInterval * m, bool_t produces_result );    // generate subinterval (e.g., code in then branch)
        void constant_if_node( IfNode * node, ConstantExpression * condition );        // code for if with const condition
        TypeTestNode * makeTestNode( bool_t cond, PseudoRegister * r );            // make boolean type test node

        // for Inliner
        void gen_normal_send( SendInfo * info, int nofArgs, SinglyAssignedPseudoRegister * result );

        void gen_self_send( SendInfo * info, int nofArgs, SinglyAssignedPseudoRegister * result );

        void gen_super_send( SendInfo * info, int nofArgs, SinglyAssignedPseudoRegister * result );

        friend class Inliner;

        friend class CompilerInliningPolicy;

        friend class PrimitiveInliner;

    protected:
        void abort();

    public:
        static Node * EndOfCode;        // "at end of code" marker

        NodeBuilder();

        void initialize( InlinedScope * scope );


        InlinedScope * scope() const {
            return _scope;
        }


        ExpressionStack * exprStack() const {
            return _expressionStack;
        }


        Node * current() const {
            return _current;
        }


        void setCurrent( Node * n ) {
            st_assert( n not_eq EndOfCode, "bad node" );
            _current = n;
        }


        void append( Node * node );        // append a node
        bool_t is_in_dead_code() const {
            return _current == EndOfCode;
        }


        Inliner * inliner() const {
            return _inliner;
        }


        void removeContextCreation();        // remove context creation node
        PseudoRegister * float_at( int fno );

    public:
        void if_node( IfNode * node );

        void cond_node( CondNode * node );

        void while_node( WhileNode * node );

        void primitive_call_node( PrimitiveCallNode * node );

        void dll_call_node( DLLCallNode * node );

    public:
        void allocate_temporaries( int nofTemps );

        void push_self();

        void push_tos();

        void push_literal( Oop obj );

        void push_argument( int no );

        void push_temporary( int no );

        void push_temporary( int no, int context );

        void push_instVar( int offset );

        void push_global( AssociationOop obj );

        void store_temporary( int no );

        void store_temporary( int no, int context );

        void store_instVar( int offset );

        void store_global( AssociationOop obj );

        void pop();

        void normal_send( InterpretedInlineCache * ic );

        void self_send( InterpretedInlineCache * ic );

        void super_send( InterpretedInlineCache * ic );

        void double_equal();

        void double_not_equal();

        void method_return( int nofArgs );

        void nonlocal_return( int nofArgs );

        void allocate_closure( AllocationType type, int nofArgs, MethodOop meth );

        void allocate_context( int nofTemps, bool_t forMethod );

        void set_self_via_context();

        void copy_self_into_context();

        void copy_argument_into_context( int argNo, int no );

        void zap_scope();

        void predict_primitive_call( PrimitiveDescriptor * pdesc, int failure_start );

        void float_allocate( int nofFloatTemps, int nofFloatExprs );

        void float_floatify( Floats::Function f, int fno );

        void float_move( int fno, int from );

        void float_set( int fno, DoubleOop value );

        void float_nullary( Floats::Function f, int fno );

        void float_unary( Floats::Function f, int fno );

        void float_binary( Floats::Function f, int fno );

        void float_unaryToOop( Floats::Function f, int fno );

        void float_binaryToOop( Floats::Function f, int fno );
};

