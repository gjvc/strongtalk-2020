//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once


// The Inliner controls and performs method inlining in the compiler.
// It contains all the code to set up the new scopes once a go-ahead decision has been made.
// (Most of) the actual decisions are made by InliningPolicy (further down).

#include "vm/compiler/InliningPolicy.hpp"
#include "vm/compiler/NodeBuilder.hpp"
#include "vm/runtime/ResourceObject.hpp"

enum class SendKind {
    NormalSend, //
    SelfSend,   //
    SuperSend   //
};

class Scope;

class ScopeDescriptor;

class RecompilationScope;

class UntakenRecompilationScope;


class Inliner : public PrintableResourceObject {

private:
    void reportInline( const char *prefix ); // Add a comment node delimiting an inlined send

protected:
    InlinedScope                 *_sender;         // scope containing the send
    InlinedScope                 *_callee;         // scope being inlined (or nullptr)
    SendInfo                     *_info;           // send being inlined
    Expression                   *_result;         // result expression
    SinglyAssignedPseudoRegister *_resultPR;       // result PseudoRegister
    NodeBuilder                  *_generator;      // current generator (sender's or callee's)
    MergeNode                    *_merge;          // where multiple versions merge (nullptr if only one)
    const char                   *_msg;            // reason for not inlining the send
    SendKind _sendKind;         //
    bool   _lastLookupFailed; // last tryLookup failed because no method found


public:
    std::int32_t depth;            // nesting depth (for debug output)

    Inliner( InlinedScope *s ) {
        this->_sender = s;
        initialize();
    }


    // The inlineXXX generate a non-inlined send if necessary, with the exception
    // of inlineBlockInvocation which returns nullptr (and does nothing) if the block
    // shouldn't be inlined
    Expression *inlineNormalSend( SendInfo *info );

    Expression *inlineSuperSend( SendInfo *info );

    Expression *inlineSelfSend( SendInfo *info );

    Expression *inlineBlockInvocation( SendInfo *info );


    SendInfo *info() const {
        return _info;
    }


    const char *msg() const {
        return _msg;
    }


    void print();

protected:
    void initialize();

    void initialize( SendInfo *info, SendKind kind );

    Expression *inlineSend();

    void tryInlineSend();

    Expression *inlineMerge( SendInfo *info );

    Expression *picPredict();

    Expression *picPredictUnlikely( SendInfo *info, UntakenRecompilationScope *uscope );

    Expression *typePredict();

    Expression *genRealSend();

    InlinedScope *tryLookup( Expression *receiver );      // try lookup and determine if should inline send
    Expression *doInline( Node *start );

    const char *checkSendInPrimFailure();

    InlinedScope *notify( const char *msg );

    RecompilationScope *makeBlockRScope( const Expression *receiver, LookupKey *key, const MethodOop method );

    InlinedScope *makeScope( const Expression *receiver, const KlassOop klass, const LookupKey *key, const MethodOop method );

    Expression *makeResult( Expression *res );

    bool checkSenderPath( Scope *here, ScopeDescriptor *there ) const;

    friend class InliningPolicy;
};
