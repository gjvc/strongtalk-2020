//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/system/asserts.hpp"
#include "vm/system/platform.hpp"
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

    void allocate_temporaries( std::int32_t nofTemps );

    void push_self();

    void push_tos();

    void push_literal( Oop obj );

    void push_argument( std::int32_t no );

    void push_temporary( std::int32_t no );

    void push_temporary( std::int32_t no, std::int32_t context );

    void push_instVar( std::int32_t offset );

    void push_instVar_name( SymbolOop name );

    void push_classVar( AssociationOop assoc );

    void push_classVar_name( SymbolOop name );

    void push_global( AssociationOop obj );

    void store_temporary( std::int32_t no );

    void store_temporary( std::int32_t no, std::int32_t context );

    void store_instVar( std::int32_t offset );

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

    void method_return( std::int32_t nofArgs );

    void nonlocal_return( std::int32_t nofArgs );

    void allocate_closure( AllocationType type, std::int32_t nofArgs, MethodOop meth );

    void allocate_context( std::int32_t nofTemps, bool_t forMethod );

    void set_self_via_context();

    void copy_self_into_context();

    void copy_argument_into_context( std::int32_t argNo, std::int32_t no );

    void zap_scope();

    void predict_primitive_call( PrimitiveDescriptor *pdesc, std::int32_t failure_start );

    void float_allocate( std::int32_t nofFloatTemps, std::int32_t nofFloatExprs );

    void float_floatify( Floats::Function f, std::int32_t tof );

    void float_move( std::int32_t tof, std::int32_t from );

    void float_set( std::int32_t tof, DoubleOop value );

    void float_nullary( Floats::Function f, std::int32_t tof );

    void float_unary( Floats::Function f, std::int32_t tof );

    void float_binary( Floats::Function f, std::int32_t tof );

    void float_unaryToOop( Floats::Function f, std::int32_t tof );

    void float_binaryToOop( Floats::Function f, std::int32_t tof );
};
