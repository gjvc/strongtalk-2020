
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/system/asserts.hpp"
#include "vm/system/platform.hpp"
#include "vm/code/NodeVisitor.hpp"
#include "vm/assembler/MacroAssembler.hpp"
#include "vm/code/PseudoRegisterMapping.hpp"
#include "vm/utilities/GrowableArray.hpp"
#include "vm/assembler/CodeBuffer.hpp"
#include "vm/assembler/Register.hpp"
#include "vm/compiler/Node.hpp"


class Stub;

class DebugInfoWriter;


class CodeGenerator : public NodeVisitor {

private:
    MacroAssembler        *_masm;               // the low-level assembler
    PseudoRegisterMapping *_currentMapping;     // currently used mapping of PseudoRegisters
    GrowableArray<Stub *> _mergeStubs;          // a stack of yet to generate merge stubs
    DebugInfoWriter *_debugInfoWriter;    // keeps track of PseudoRegister location changes and updates debug info
    int _maxNofStackTmps;     // the maximum number of stack allocated variables so far
    Node *_previousNode;       // the previous node in the same basic block or nullptr info used to patch temporary initialization
    Register _nilReg;              // the register holding nilObj used to initialize the stack frame
    CodeBuffer *_pushCode;           // the code area that can be patched with push instructions
    int _nofCompilations;

private:
    // Helper routines for mapping
    Register def( PseudoRegister *preg ) const;


    Register use( PseudoRegister *preg ) const {
        return _currentMapping->use( preg );
    }


    void setMapping( PseudoRegisterMapping *mapping );

    int maxNofStackTmps();

    bool_t isLiveRangeBoundary( Node *a, Node *b ) const;

    void jmp( Node *from, Node *to, bool_t to_maybe_nontrivial = false );

    void jcc( Assembler::Condition cc, Node *from, Node *to, bool_t to_maybe_nontrivial = false );

    void bindLabel( Node *node );

    void inlineCache( Node *call, MergeNode *nlrTestPoint, int flags = 0 );

    void updateDebuggingInfo( Node *node );

    // Helper routines for code generation
    const char *nativeMethodAddress() const;

    void incrementInvocationCounter();

    int byteOffset( int offset );

    void zapContext( PseudoRegister *context );

    void storeCheck( Register obj );

    void assign( PseudoRegister *dst, PseudoRegister *src, bool_t needsStoreCheck = true );

    void uplevelBase( PseudoRegister *startContext, int nofLevels, Register base );

    void moveConstant( ArithOpCode op, PseudoRegister *&x, PseudoRegister *&y, bool_t &x_attr, bool_t &y_attr );

    void arithRROp( ArithOpCode op, Register x, Register y );

    void arithRCOp( ArithOpCode op, Register x, int y );

    void arithROOp( ArithOpCode op, Register x, Oop y );

    void arithRXOp( ArithOpCode op, Register x, Oop y );

    bool_t producesResult( ArithOpCode op );

    Register targetRegister( ArithOpCode op, PseudoRegister *z, PseudoRegister *x );

    Assembler::Condition mapToCC( BranchOpCode op );

    void generateMergeStubs();

    void copyIntoContexts( BlockCreateNode *node );

    void materializeBlock( BlockCreateNode *node );

    void jcc_error( Assembler::Condition cc, AbstractBranchNode *node, Label &label );

    void testForSingleKlass( Register obj, KlassOop klass, Register klassReg, Label &success, Label &failure );

    void generateTypeTests( LoopHeaderNode *node, Label &failure );

    void generateIntegerLoopTest( PseudoRegister *preg, LoopHeaderNode *node, Label &failure );

    void generateIntegerLoopTests( LoopHeaderNode *node, Label &failure );

    void generateArrayLoopTests( LoopHeaderNode *node, Label &failure );

    // Debugging
    static void indent();

    static const char *nativeMethodName();

    static void verifyObj( Oop obj );

    static void verifyContext( Oop obj );

    static void verifyArguments( Oop recv, Oop *ebp, int nofArgs );

    static void verifyReturn( Oop result );

    static void verifyNonLocalReturn( const char *fp, const char *nlrFrame, int nlrScopeID, Oop result );

    void callVerifyObj( Register obj );

    void callVerifyContext( Register context );

    void callVerifyArguments( Register recv, int nofArgs );

    void callVerifyReturn();

    void callVerifyNonLocalReturn();

public:
    CodeGenerator( MacroAssembler *masm, PseudoRegisterMapping *mapping );


    Assembler *assembler() const {
        return _masm;
    }


    void initialize( InlinedScope *scope );        // call this before code generation
    void finalize( InlinedScope *scope );        // call this at end of code generation
    void finalize2( InlinedScope *scope );        // call this at end of code generation

public:
    void beginOfBasicBlock( Node *node );

    void endOfBasicBlock( Node *node );

public:
    void beginOfNode( Node *node );

    void endOfNode( Node *node );

public:
    void aPrologueNode( PrologueNode *node );

    void aLoadIntNode( LoadIntNode *node );

    void aLoadOffsetNode( LoadOffsetNode *node );

    void aLoadUplevelNode( LoadUplevelNode *node );

    void anAssignNode( AssignNode *node );

    void aStoreOffsetNode( StoreOffsetNode *node );

    void aStoreUplevelNode( StoreUplevelNode *node );

    void anArithRRNode( ArithRRNode *node );

    void anArithRCNode( ArithRCNode *node );

    void aTArithRRNode( TArithRRNode *node );

    void aFloatArithRRNode( FloatArithRRNode *node );

    void aFloatUnaryArithNode( FloatUnaryArithNode *node );

    void aContextCreateNode( ContextCreateNode *node );

    void aContextInitNode( ContextInitNode *node );

    void aContextZapNode( ContextZapNode *node );

    void aBlockCreateNode( BlockCreateNode *node );

    void aBlockMaterializeNode( BlockMaterializeNode *node );

    void aSendNode( SendNode *node );

    void aPrimitiveNode( PrimitiveNode *node );

    void aDLLNode( DLLNode *node );

    void aLoopHeaderNode( LoopHeaderNode *node );

    void aReturnNode( ReturnNode *node );

    void aNonLocalReturnSetupNode( NonLocalReturnSetupNode *node );

    void anInlinedReturnNode( InlinedReturnNode *node );

    void aNonLocalReturnContinuationNode( NonLocalReturnContinuationNode *node );

    void aBranchNode( BranchNode *node );

    void aTypeTestNode( TypeTestNode *node );

    void aNonLocalReturnTestNode( NonLocalReturnTestNode *node );

    void aMergeNode( MergeNode *node );

    void anArrayAtNode( ArrayAtNode *node );

    void anArrayAtPutNode( ArrayAtPutNode *node );

    void anInlinedPrimitiveNode( InlinedPrimitiveNode *node );

    void anUncommonNode( UncommonNode *node );

    void aFixedCodeNode( FixedCodeNode *node );


    void aNopNode( NopNode *node ) {
    }


    void aCommentNode( CommentNode *node ) {
    }
};
