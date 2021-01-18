//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/system/asserts.hpp"
#include "vm/memory/allocation.hpp"
#include "vm/interpreter/MethodIterator.hpp"



// MethodPrinter dumps, on the _console, all byte-codes of a method.
// In addition blocks contained in the method are dumped at the byte code pushing the block context.
// Usage:
//   MethodPrinterClosure blk(output);
//   MethodIterator mi(method, &blk);

class MethodPrinterClosure : public MethodClosure {

private:
    ConsoleOutputStream *_outputStream;

    void print_sendtype( ByteCodes::SendType type );

    void show( const char *str );

    void indent();

public:
    MethodPrinterClosure( ConsoleOutputStream *stream = nullptr );

    void if_node( IfNode *node );

    void cond_node( CondNode *node );

    void while_node( WhileNode *node );

    void primitive_call_node( PrimitiveCallNode *node );

    void dll_call_node( DLLCallNode *node );

    void allocate_temporaries( int nofTemps );

    void push_self();

    void push_tos();

    void push_literal( Oop obj );

    void push_argument( int no );

    void push_temporary( int no );

    void push_temporary( int no, int context );

    void push_instVar( int offset );

    void push_instVar_name( SymbolOop name );

    void push_classVar( AssociationOop assoc );

    void push_classVar_name( SymbolOop name );

    void push_global( AssociationOop obj );

    void store_temporary( int no );

    void store_temporary( int no, int context );

    void store_instVar( int offset );

    void store_instVar_name( SymbolOop name );

    void store_classVar( AssociationOop assoc );

    void store_classVar_name( SymbolOop name );

    void store_global( AssociationOop obj );

    void pop();

    void normal_send( InterpretedInlineCache *ic );

    void self_send( InterpretedInlineCache *ic );

    void super_send( InterpretedInlineCache *ic );

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

    void predict_primitive_call( PrimitiveDescriptor *pdesc, int failure_start );

    void float_allocate( int nofFloatTemps, int nofFloatExprs );

    void float_floatify( Floats::Function f, int tof );

    void float_move( int tof, int from );

    void float_set( int tof, DoubleOop value );

    void float_nullary( Floats::Function f, int tof );

    void float_unary( Floats::Function f, int tof );

    void float_binary( Floats::Function f, int tof );

    void float_unaryToOop( Floats::Function f, int tof );

    void float_binaryToOop( Floats::Function f, int tof );
};
