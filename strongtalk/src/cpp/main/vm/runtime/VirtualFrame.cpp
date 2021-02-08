//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/runtime/VirtualFrame.hpp"
#include "vm/system/asserts.hpp"
#include "vm/utilities/OutputStream.hpp"
#include "vm/code/NativeMethod.hpp"
#include "vm/code/ProgramCounterDescriptor.hpp"
#include "vm/interpreter/PrettyPrinter.hpp"
#include "vm/oops/BlockClosureOopDescriptor.hpp"
#include "vm/memory/oopFactory.hpp"
#include "vm/runtime/SavedRegisters.hpp"
#include "vm/runtime/StackChunkBuilder.hpp"
#include "vm/oops/BlockClosureKlass.hpp"
#include "vm/oops/ObjectArrayOopDescriptor.hpp"
#include "vm/runtime/ResourceMark.hpp"
#include "vm/oops/ContextKlass.hpp"
#include "vm/utilities/StringOutputStream.hpp"

// Ideas:
//   Maybe cache methodOop in DeltaVirtualFrame for faster argument access. (Lars 8/10/95)

// ------------- VirtualFrame --------------

bool VirtualFrame::equal( const VirtualFrame *virtualFrame ) const {
    return _frame.fp() == virtualFrame->_frame.fp();
}


Oop VirtualFrame::callee_argument_at( std::int32_t index ) const {
    static_cast<void>(index); // unused
    SPDLOG_INFO( "VirtualFrame::callee_argument_at should be specialized for all vframes calling deltaVFrames" );
    st_fatal( "aborting" );
    return nullptr;
}


VirtualFrame *VirtualFrame::new_vframe( Frame *f ) {

    if ( f->is_interpreted_frame() )
        return new InterpretedVirtualFrame( f );

    if ( f->is_entry_frame() )
        return new cChunk( f );

    if ( f->is_deoptimized_frame() )
        return new DeoptimizedVirtualFrame( f );

    if ( f->is_compiled_frame() ) {
        NativeMethod *nm = f->code();
        st_assert( nm, "NativeMethod not found in compiled frame" );

        // NB: pc points *after* the current instruction (e.g., call), so must adjust it
        // to get the right byteCodeIndex; -1 will do portably    -Urs 2/96
        const char               *pc = f->pc() - 1;
        ProgramCounterDescriptor *pd = nm->containingProgramCounterDescriptor( pc );
        st_assert( pd, "ProgramCounterDescriptor not found" );

        ScopeDescriptor *sd = nm->scopes()->at( pd->_scope, pc );
        st_assert( sd, "ScopeDescriptor not found" );
        return CompiledVirtualFrame::new_vframe( f, sd, pd->_byteCodeIndex );
    }

    return new cVFrame( f );
}


VirtualFrame *VirtualFrame::sender() const {
    st_assert( is_top(), "just checking" );
    Frame s = _frame.sender();
    if ( s.is_first_frame() )
        return nullptr;
    return VirtualFrame::new_vframe( &s );
}


VirtualFrame *VirtualFrame::top() const {
    VirtualFrame *vf = (VirtualFrame *) this;
    while ( not vf->is_top() )
        vf = vf->sender();
    return vf;
}


void VirtualFrame::print() {
    if ( WizardMode or ActivationShowFrame ) {
        _frame.print();
    }
}


void VirtualFrame::print_value() const {
    ( (VirtualFrame *) this )->print();
}

// ------------- DeltaVirtualFrame --------------

GrowableArray<Oop> *DeltaVirtualFrame::arguments() const {
    std::int32_t       nargs   = method()->number_of_arguments();
    GrowableArray<Oop> *result = new GrowableArray<Oop>( nargs );
    //VirtualFrame       *s      = sender();
    for ( std::int32_t index   = 0; index < nargs; index++ ) {
        result->push( argument_at( index ) );
    }
    return result;
}


Oop DeltaVirtualFrame::argument_at( std::int32_t index ) const {
    return sender()->callee_argument_at( method()->number_of_arguments() - ( index + 1 ) );
}


Oop DeltaVirtualFrame::callee_argument_at( std::int32_t index ) const {
    return expression_at( index );
}


void DeltaVirtualFrame::print() {
    SPDLOG_INFO( "Delta frame: " );
    VirtualFrame::print();
}


void DeltaVirtualFrame::print_activation( std::int32_t index ) const {
    ( (VirtualFrame *) this )->VirtualFrame::print();
    PrettyPrinter::print( index, (DeltaVirtualFrame *) this );
}


DeltaVirtualFrame *DeltaVirtualFrame::sender_delta_frame() const {
    VirtualFrame *f = sender();
    while ( f not_eq nullptr ) {
        if ( f->is_delta_frame() )
            return (DeltaVirtualFrame *) f;
        f = f->sender();
    }
    return nullptr;
}


void DeltaVirtualFrame::verify() const {

    // Verify the frame
    fr().verify();

    // Make sure we do not have any context objects in the argument list
    std::int32_t number_of_arguments = method()->number_of_arguments();

    for ( std::int32_t i = 0; i < number_of_arguments; i++ ) {
        Oop argument = argument_at( i );
        if ( argument->is_context() ) {
            SPDLOG_INFO( "Argument is a context" );
            print_activation( 0 );
            spdlog::warn( "verify failed" );
        }
    }
}


// ------------- InterpretedVirtualFrame --------------

bool InterpretedVirtualFrame::has_interpreter_context() const {
    return method()->activation_has_context();
}


Oop InterpretedVirtualFrame::temp_at( std::int32_t offset ) const {
    st_assert( offset > 0 or not has_interpreter_context(), "you cannot use temp(0) when a context is present" );
    st_assert( offset < method()->number_of_stack_temporaries(), "checking bounds" );
    return _frame.temp( offset );
}


Oop InterpretedVirtualFrame::context_temp_at( std::int32_t offset ) const {
    st_assert( has_interpreter_context(), "must have context when using context_temp_at" );
    return interpreter_context()->obj_at( offset );
}


ContextOop InterpretedVirtualFrame::interpreter_context() const {
    if ( not has_interpreter_context() )
        return nullptr;
    ContextOop result = ContextOop( _frame.temp( 0 ) );
    st_assert( method()->in_context_allocation( byteCodeIndex() ) or result->is_context(), "context type check" );
    return result;
}


ContextOop InterpretedVirtualFrame::canonical_context() const {
    return interpreter_context();
}


Oop InterpretedVirtualFrame::expression_at( std::int32_t index ) const {
    return *expression_addr( index );
}


Oop *InterpretedVirtualFrame::expression_addr( std::int32_t offset ) const {
    return (Oop *) &( (Oop *) _frame.sp() )[ offset ];
}


GrowableArray<Oop> *InterpretedVirtualFrame::expression_stack() const {

    std::int32_t last_temp_number = method()->number_of_stack_temporaries() - 1;
    std::int32_t size             = _frame.temp_addr( last_temp_number ) - expression_addr( 0 );
    st_assert( size >= 0, "expr stack size must be non-negative" );
    GrowableArray<Oop> *result = new GrowableArray<Oop>( size );

    for ( std::int32_t i = 0; i < size; i++ ) {
        Oop value = expression_at( i );
        st_assert( not value->is_context(), "checking for contextOop on expression stack" );
        result->push( expression_at( i ) );
    }

    std::int32_t computed_size = method()->expression_stack_mapping( byteCodeIndex() )->length();
    if ( size not_eq computed_size ) {
        spdlog::warn( "Expression stack size  @%d is %d but computed to %d", byteCodeIndex(), size, computed_size );
        SPDLOG_INFO( "[expression stack:" );
        for ( std::int32_t i = 0; i < result->length(); i++ ) {
            _console->print( " - " );
            result->at( i )->print_value_on( _console );
            _console->cr();
        }
        SPDLOG_INFO( "]" );
        method()->pretty_print();
        method()->print_codes();
    }

    return result;
}


std::uint8_t *InterpretedVirtualFrame::hp() const {
    return _frame.hp();
}


void InterpretedVirtualFrame::set_hp( std::uint8_t *p ) {
    _frame.set_hp( p );
}


Oop InterpretedVirtualFrame::receiver() const {
    // In case of a block invocation the receiver might be a contextOop
    // due to an interpreter optimization (ask Robert for details).
    // To provide clean semantics in this case nilObject is returned.
    Oop r = _frame.receiver();
    st_assert( not r->is_context() or method()->is_blockMethod(), "check: context implies block method" );
    return r->is_context() ? nilObject : r;
}


void InterpretedVirtualFrame::set_receiver( Oop obj ) {
    _frame.set_receiver( obj );
}


void InterpretedVirtualFrame::temp_at_put( std::int32_t offset, Oop obj ) {
    _frame.set_temp( offset, obj );
}


void InterpretedVirtualFrame::expression_at_put( std::int32_t offset, Oop obj ) {
    static_cast<void>(offset); // unused
    static_cast<void>(obj); // unused
    // FIX LATER p.set_expr(offset) = obj;
    Unimplemented();
}


bool InterpretedVirtualFrame::equal( const VirtualFrame *f ) const {
    if ( not f->is_interpreted_frame() )
        return false;
    return VirtualFrame::equal( f );
}


std::int32_t InterpretedVirtualFrame::byteCodeIndex() const {
    return method()->byteCodeIndex_from( hp() );
}


MethodOop InterpretedVirtualFrame::method() const {
    MemOop m = as_memOop( Universe::object_start( (Oop *) ( hp() - 1 ) ) );
    st_assert( m->is_method(), "must be method" );
    return MethodOop( m );
}


DeltaVirtualFrame *InterpretedVirtualFrame::parent() const {
    MethodOop m = method();

    // Return nullptr if method is outer.
    if ( not m->is_blockMethod() )
        return nullptr;

    ContextOop target = interpreter_context();
    if ( not target )
        return nullptr; // Return nullptr if no context is present.

    // Walk the stack and find the VirtualFrame with outer as context.
    // NB: the parent may still be alive even though it cannot be
    //     found on this stack. It might reside on another stack.

    for ( VirtualFrame *p = sender(); p; p = p->sender() ) {
        if ( p->is_interpreted_frame() )
            if ( ( (InterpretedVirtualFrame *) p )->interpreter_context() == target )
                return (DeltaVirtualFrame *) p;
    }

    spdlog::warn( "parent frame is not found on same stack" );

    return nullptr;
}


void InterpretedVirtualFrame::verify() const {
    DeltaVirtualFrame::verify();
    MethodOop m = method();
    if ( m->activation_has_context() ) {
        if ( not m->in_context_allocation( byteCodeIndex() ) ) {
            ContextOop con = interpreter_context();
            if ( not con->is_context() )
                spdlog::warn( "expecting context" );
            if ( not m->is_blockMethod() ) {
                if ( con->parent_fp() == nullptr )
                    spdlog::warn( "expecting frame in context" );
            }
            m->verify_context( con );
        }
    }
}


// ------------- CompiledVirtualFrame --------------

CompiledVirtualFrame *CompiledVirtualFrame::new_vframe( const Frame *fr, ScopeDescriptor *sd, std::int32_t byteCodeIndex ) {
    if ( sd->isMethodScope() )
        return new CompiledMethodVirtualFrame( fr, sd, byteCodeIndex );
    if ( sd->isTopLevelBlockScope() )
        return new CompiledTopLevelBlockVirtualFrame( fr, sd, byteCodeIndex );
    if ( sd->isBlockScope() )
        return new CompiledBlockVirtualFrame( fr, sd, byteCodeIndex );

    st_fatal( "unknown scope desc" );
    return nullptr;
}


void CompiledVirtualFrame::rewind_byteCodeIndex() {
    std::int32_t new_byteCodeIndex = method()->find_byteCodeIndex_from( _byteCodeIndex );
    st_assert( new_byteCodeIndex >= 0, "must be real byteCodeIndex" );
    SPDLOG_INFO( "{} -> {}", _byteCodeIndex, new_byteCodeIndex );
    _byteCodeIndex = new_byteCodeIndex;
}


CompiledVirtualFrame::CompiledVirtualFrame( const Frame *fr, ScopeDescriptor *sd, std::int32_t byteCodeIndex ) :
    DeltaVirtualFrame( fr ),
    _scopeDescriptor{ sd },
    _byteCodeIndex{ byteCodeIndex } {
}


VirtualFrame *CompiledVirtualFrame::sender() const {
    if ( _scopeDescriptor->isTop() ) {
        return VirtualFrame::sender();
    }

    return CompiledVirtualFrame::new_vframe( &_frame, _scopeDescriptor->sender(), _scopeDescriptor->senderByteCodeIndex() );
}


Oop CompiledVirtualFrame::temp_at( std::int32_t offset ) const {
    st_assert( offset > 0 or not method()->activation_has_context(), "you cannot use temp(0) when a context is present" );
    st_assert( offset < method()->number_of_stack_temporaries(), "checking bounds" );
    return resolve_name( scope()->temporary( offset ), this );
}


class ContextTempFindClosure : public NameDescriptorClosure {
public:
    NameDescriptor *result;
    std::int32_t   i;


    ContextTempFindClosure( std::int32_t index ) :
        i{ index },
        result{ nullptr } {
    };


    ContextTempFindClosure() = default;
    virtual ~ContextTempFindClosure() = default;
    ContextTempFindClosure( const ContextTempFindClosure & ) = default;
    ContextTempFindClosure &operator=( const ContextTempFindClosure & ) = default;
    void operator delete( void *ptr ) { (void)(ptr); }


    void context_temp( std::int32_t no, NameDescriptor *a, char *pc ) {
        static_cast<void>(pc); // unused
        if ( no == i ) {
            result = a;
        }
    }

};


Oop CompiledVirtualFrame::context_temp_at( std::int32_t offset ) const {
    st_assert( method()->activation_has_context(), "you cannot use context_temp_at() when methodOop does not allocate context" );
    ContextTempFindClosure blk( offset );
    _scopeDescriptor->iterate( &blk );
    st_assert( blk.result, "must find result" );
    return resolve_name( blk.result, this );
}


Oop CompiledVirtualFrame::expression_at( std::int32_t index ) const {
    GrowableArray<DeferredExpression *> *stack = deferred_expression_stack();
    if ( stack->length() <= index ) {
        // Hack for Robert 1/15/96, probably wrong expression stack
        return oopFactory::new_symbol( "invalid stack element" );
    }
    return stack->at( index )->value();
}


class CollectContextInfoClosure : public NameDescriptorClosure {
public:
    GrowableArray<NameDescriptor *> *result;


    CollectContextInfoClosure() :
        result{ new GrowableArray<NameDescriptor *>( 10 ) } {
    }

    virtual ~CollectContextInfoClosure() = default;
    CollectContextInfoClosure( const CollectContextInfoClosure & ) = default;
    CollectContextInfoClosure &operator=( const CollectContextInfoClosure & ) = default;
    void operator delete( void *ptr ) { (void)(ptr); }


    void context_temp( std::int32_t no, NameDescriptor *a, char *pc ) {
        static_cast<void>(no); // unused
        static_cast<void>(pc); // unused
        result->append( a );
    }
};

extern "C" ContextOop allocateContext( SMIOop nofVars );


ContextOop CompiledVirtualFrame::compiled_context() const {
    if ( not method()->activation_has_context() )
        return nullptr;

    // Hack suggested by Urs
    // if temp 0 contains nil the compiler has optimized away the
    // contextOop.
    // A better solution would be adding has_compiled_context to scopeDesc.
    Oop con = resolve_name( scope()->temporary( 0 ), this );
    if ( con == nilObject )
        return nullptr;

    st_assert( con->is_context(), "context type check" );
    return ContextOop( con );
}


GrowableArray<DeferredExpression *> *CompiledVirtualFrame::deferred_expression_stack() const {
    GrowableArray<std::int32_t>         *mapping = method()->expression_stack_mapping( byteCodeIndex() );
    GrowableArray<DeferredExpression *> *result  = new GrowableArray<DeferredExpression *>( mapping->length() );

    for ( std::int32_t index = 0; index < mapping->length(); index++ ) {
        NameDescriptor *nd = _scopeDescriptor->exprStackElem( mapping->at( index ) );
        result->push( new DeferredExpression( this, nd ) );
    }
    return result;
}


GrowableArray<Oop> *CompiledVirtualFrame::expression_stack() const {
    GrowableArray<std::int32_t> *mapping = method()->expression_stack_mapping( byteCodeIndex() );
    GrowableArray<Oop>          *result  = new GrowableArray<Oop>( mapping->length() );

    for ( std::int32_t i = 0; i < mapping->length(); i++ ) {
        NameDescriptor *nd   = _scopeDescriptor->exprStackElem( mapping->at( i ) );
        Oop            value = resolve_name( nd, this );
        result->push( value );
    }

    std::int32_t computed_size = method()->expression_stack_mapping( byteCodeIndex() )->length();

    if ( result->length() not_eq computed_size ) {
        spdlog::warn( "Expression stack size  @%d is %d but computed to %d", byteCodeIndex(), result->length(), computed_size );
        SPDLOG_INFO( "[expression stack:" );
        for ( std::int32_t i = 0; i < result->length(); i++ ) {
            _console->print( " - " );
            result->at( i )->print_value_on( _console );
            _console->cr();
        }
        SPDLOG_INFO( "]" );
        method()->pretty_print();
        method()->print_codes();
    }
    return result;
}


bool CompiledVirtualFrame::equal( const VirtualFrame *f ) const {
    if ( not f->is_compiled_frame() )
        return false;
    return VirtualFrame::equal( f ) and scope()->is_equal( ( (CompiledVirtualFrame *) f )->scope() );
}


std::int32_t CompiledVirtualFrame::byteCodeIndex() const {
    return _byteCodeIndex;
}


MethodOop CompiledVirtualFrame::method() const {
    return _scopeDescriptor->method();
}


NativeMethod *CompiledVirtualFrame::code() const {
    return _frame.code();
}


Oop CompiledVirtualFrame::resolve_location( Location loc, const CompiledVirtualFrame *vf, ContextOop con ) {

    // Context location
    if ( loc.isStackLocation() ) {
        return vf ? Oop( vf->fr().at( loc.offset() ) ) : filler_oop();
    }

    // Context location
    if ( loc.isContextLocation() ) {
        if ( vf ) {
            ScopeDescriptor *scope = vf->code()->scopes()->at( loc.scopeOffs(), vf->fr().pc() );
            st_assert( scope->allocates_compiled_context(), "must have context" );
            st_assert( scope->compiled_context()->isLocation(), "context must be a location" );
            ContextOop context = ContextOop( resolve_location( scope->compiled_context()->location(), vf ) );
            return context->obj_at( loc.tempNo() );
        }

        if ( con ) {
            return con->obj_at( loc.tempNo() );
        }

        return filler_oop();
    }

    // Register location
    if ( loc.isRegisterLocation() ) {
        return SavedRegisters::fetch( loc.number(), vf->fr().fp() );
    }

    ShouldNotReachHere();
    return nullptr;
}


Oop CompiledVirtualFrame::resolve_name( NameDescriptor *nd, const CompiledVirtualFrame *vf, ContextOop con ) {
    // takes a NameDescriptor & looks up the Oop it describes (on the stack, in contexts, etc.)

    if ( nd->isLocation() ) {
        // describes a location where we can find the Oop.
        Location loc = nd->location();
        return resolve_location( loc, vf, con );
    }

    if ( nd->isValue() ) {
        // an Oop constant.
        return nd->value();
    }

    if ( nd->isBlockValue() ) {
        // a blockClosureOop that has been completely optimized away.
        Frame f      = vf->fr();
        Oop   result = nd->value( &f );
        return result;
    }

    if ( nd->isMemoizedBlock() ) {
        // a blockClosureOop that has been partially optimized away.
        // if the block doesn't exist yet, we have to create one and store it in the location.
        Frame f      = vf->fr();
        Oop   result = nd->value( &f );
        return result;
    }

    if ( nd->isIllegal() ) {
        // This should never happen.
        // For now, a fake value is returned to avoid crashes when tracing the stack.
        // Lars 7/3/95
        if ( UseNewBackend ) {
            // This is a hack - we should introduce a special
            // nameDesc instead - gri 8-5-96
            return nilObject;
        }
        spdlog::warn( "Compiler Bug: Illegal name desc found in NativeMethod 0x{0:x} @ {:d}", static_cast<const void *>(vf->fr().code()), vf->scope()->offset() );
        return oopFactory::new_symbol( "illegal nameDesc" );
    }

    ShouldNotReachHere();
    return nullptr;
}


Oop CompiledVirtualFrame::filler_oop() {
    return nilObject;
    // This is useful for debugging
    // return oopFactory::new_symbol("CompiledVirtualFrame::filler_oop");
}


std::int32_t CompiledVirtualFrame::byteCodeIndex_for( ScopeDescriptor *d ) const {
    ScopeDescriptor *s = _scopeDescriptor;
    std::int32_t    b  = byteCodeIndex();
    while ( not s->is_equal( d ) ) {
        b = s->senderByteCodeIndex();
        st_assert( s->sender(), "make sure we have a sender" );
        s = s->sender();
    }
    return b;
}


#define CHECK( n )  if (n->isIllegal()) ok = false

class VerifyNDClosure : public NameDescriptorClosure {
public:
    bool ok;


    VerifyNDClosure() :
        ok{ true } {
    }


    void arg( std::int32_t no, NameDescriptor *a, char *pc ) {
        static_cast<void>(no); // unused
        static_cast<void>(a); // unused
        static_cast<void>(pc); // unused
        CHECK( a );
    }


    void temp( std::int32_t no, NameDescriptor *a, char *pc ) {
        static_cast<void>(no); // unused
        static_cast<void>(a); // unused
        static_cast<void>(pc); // unused
        CHECK( a );
    }


    void context_temp( std::int32_t no, NameDescriptor *a, char *pc ) {
        static_cast<void>(no); // unused
        static_cast<void>(a); // unused
        static_cast<void>(pc); // unused
        CHECK( a );
    }
};


void CompiledVirtualFrame::verify_debug_info() const {
    // verify that all source-visible names / expr stack entries have valid nameDescs
    ResourceMark    rm;
    bool            ok = true;
    VerifyNDClosure blk;
    _scopeDescriptor->iterate( &blk );
    CHECK( _scopeDescriptor->self() );

    if ( not ok or not blk.ok ) {
        print_activation( 0 );
        error( "illegal nameDescs in VirtualFrame" );
    }
#undef CHECK
}


class Indenting : public ValueObject {
private:
public:
    Indenting() {
        _console->inc();
    }


    ~Indenting() {
        _console->dec();
    }
};


void traceFrame( const CompiledVirtualFrame *vf, ContextOop con ) {
    if ( TraceCanonicalContext ) {
        FlagSetting flag( TraceCanonicalContext, false );
        _console->cr();
        _console->indent();
        SPDLOG_INFO( "context(0x{0:x}), VirtualFrame(0x{0:x}), block? {}", static_cast<const void *>(con), static_cast<const void *>(vf), vf ? vf->method()->is_blockMethod() : false );
        if ( vf ) {
            vf->print_activation( 0 );
            vf->method()->print_codes();
        }
    }
}


ContextOop CompiledVirtualFrame::compute_canonical_parent_context( ScopeDescriptor *scope, const CompiledVirtualFrame *vf, ContextOop con ) {
    CompiledVirtualFrame *parent_vf = ( not vf or not vf->parent() or not vf->parent()->is_compiled_frame() ) ? nullptr : (CompiledVirtualFrame *) vf->parent();
    return compute_canonical_context( scope->parent( true ), parent_vf, con );
}


ContextOop CompiledVirtualFrame::compute_canonical_context( ScopeDescriptor *scope, const CompiledVirtualFrame *vf, ContextOop con ) {
    // Computes the canonical contextOop for a scope desc.
    //
    // Recipe:
    // 1. Search the stack builder context cache (in case we're during deoptimizing)
    // 2. Check the forward reference in contextOop.
    // 3. Allocate a fresh contextOop.

    // Playing rules for contextOops
    // - If there exists a compiled context there is an 1-to-1 mapping to an interpreted context
    Indenting in;
    traceFrame( vf, con );

    if ( not scope->allocates_interpreted_context() ) {
        // This scope does not allocate an interpreter contextOop
        if ( scope->isMethodScope() )
            return nullptr;

        if ( not scope->method()->expectsContext() and scope->method()->parent()->allocatesInterpretedContext() ) {
            spdlog::warn( "May be allocating context when unneeded" );
        }
        return compute_canonical_parent_context( scope, vf, con );
    }

    ContextOop result;

    // step 1.
    if ( vf ) {
        if ( ( result = StackChunkBuilder::context_at( vf ) ) not_eq nullptr ) {
            st_assert( result->unoptimized_context() == nullptr, "cannot have deoptimized context" );
            return result;
        }
    }

    // step 2
    if ( con and con->unoptimized_context() ) {
        result = con->unoptimized_context(); // shouldn't this go in the SCB context cache?
        st_assert( result->unoptimized_context() == nullptr, "cannot have deoptimized context" );
        return result;
    }

    ContextOop comp_context = con;
    st_assert( comp_context == nullptr or comp_context->is_context(), "must be context" );

    // step 3
    if ( not MaterializeEliminatedBlocks and not StackChunkBuilder::is_deoptimizing() ) {
        // don't create a context (for better compiler debugging)
        // not quite kosher since the return type is expected to be a contextOop,
        // but for stack printing code it shouldn't matter
        ResourceMark       resourceMark;
        StringOutputStream stream( 50 );
        stream.print( "eliminated context in " );
        scope->selector()->print_symbol_on( &stream );
        return (ContextOop) oopFactory::new_symbol( stream.as_string() );      // unsafe cast
    }

    // collect all NameDescs
    CollectContextInfoClosure blk;
    scope->iterate( &blk );

    // allocate the new context
    result = ContextKlass::allocate_context( blk.result->length() );

    // fill in the meat
    for ( std::int32_t i = 0; i < blk.result->length(); i++ ) {
        NameDescriptor *nd = blk.result->at( i );
        result->obj_at_put( i, resolve_name( nd, vf, comp_context ) );
    }

    if ( vf ) {
        StackChunkBuilder::context_at_put( vf, result );
    } else {
        // kill the home field
        result->kill();
    }

    st_assert( result->unoptimized_context() == nullptr, "cannot have deoptimized context" );

    // Set parent scope if needed
    if ( not scope->isMethodScope() ) {
        ContextOop parent = compute_canonical_parent_context( scope, vf, con ? con->outer_context() : nullptr );
        result->set_parent( parent );
    }

    if ( StackChunkBuilder::is_deoptimizing() and comp_context ) {
        // Save the unoptimized context as a forward reference in the
        // compiled context.
        comp_context->set_unoptimized_context( result );
    }

    if ( TraceDeoptimization and comp_context ) {
        _console->print( " - " );
        comp_context->print_value();
        SPDLOG_INFO( " -> " );
        result->print_value();
        _console->cr();
    }

    st_assert( result->unoptimized_context() == nullptr, "cannot have deoptimized context" );
    return result;
}


void CompiledVirtualFrame::verify() const {
    DeltaVirtualFrame::verify();
    ContextOop con = compiled_context();
    if ( con ) {
        if ( con->mark()->has_context_forward() )
            spdlog::warn( "context has forwarder" );
    }
}
// ------------- compiledMethodVFrame --------------

CompiledMethodVirtualFrame::CompiledMethodVirtualFrame( const Frame *fr, ScopeDescriptor *sd, std::int32_t byteCodeIndex ) :
    CompiledVirtualFrame( fr, sd, byteCodeIndex ) {
}


bool CompiledMethodVirtualFrame::is_top() const {
    return _scopeDescriptor->isTop();
}


Oop CompiledMethodVirtualFrame::receiver() const {
    return resolve_name( _scopeDescriptor->self(), this );
}


ContextOop CompiledMethodVirtualFrame::canonical_context() const {
    st_assert( not method()->is_blockMethod(), "check methodOop type" );
    ContextOop conIn  = compiled_context();
    ContextOop conOut = compute_canonical_context( scope(), this, conIn );
    if ( TraceCanonicalContext ) {
        _console->print( "context in(0x{0:x}), vf(0x{0:x}), context out(0x{0:x}), block? %d", conIn, this, conOut, method()->is_blockMethod() );
    }
    return conOut;
}

// ------------- CompiledBlockVirtualFrame --------------

CompiledBlockVirtualFrame::CompiledBlockVirtualFrame( const Frame *fr, ScopeDescriptor *sd, std::int32_t byteCodeIndex ) :
    CompiledVirtualFrame( fr, sd, byteCodeIndex ) {
}


bool CompiledBlockVirtualFrame::is_top() const {
    return _scopeDescriptor->isTop();
}


Oop CompiledBlockVirtualFrame::receiver() const {
    NameDescriptor *nd = _scopeDescriptor->self();
    if ( nd ) {
        return resolve_name( nd, this );
    } else {
        st_fatal( "self is unknown" );    // can't handle this yet -- fix this
        return nullptr;
    }
}


DeltaVirtualFrame *CompiledBlockVirtualFrame::parent() const {
    ScopeDescriptor *ps                  = parent_scope();
    std::int32_t    parent_byteCodeIndex = byteCodeIndex_for( ps );
    return CompiledVirtualFrame::new_vframe( &_frame, ps, parent_byteCodeIndex );
}


ContextOop CompiledBlockVirtualFrame::canonical_context() const {
    st_assert( method()->is_blockMethod(), "must be block method" );
    ContextOop conIn  = compiled_context();
    ContextOop conOut = compute_canonical_context( scope(), this, conIn );
    if ( TraceCanonicalContext ) {
        _console->print( "context in(0x{0:x}), vf(0x{0:x}), context out(0x{0:x}), block? %d", conIn, this, conOut, method()->is_blockMethod() );
    }
    return conOut;
}


ScopeDescriptor *CompiledBlockVirtualFrame::parent_scope() const {
    ScopeDescriptor *result = scope()->parent();
    st_assert( result, "parent should be within same NativeMethod" );
    return result;
}

// ------------- CompiledTopLevelBlockVirtualFrame --------------

CompiledTopLevelBlockVirtualFrame::CompiledTopLevelBlockVirtualFrame( const Frame *fr, ScopeDescriptor *sd, std::int32_t byteCodeIndex ) :
    CompiledVirtualFrame( fr, sd, byteCodeIndex ) {
}


Oop CompiledTopLevelBlockVirtualFrame::receiver() const {
    return resolve_name( _scopeDescriptor->self(), this );
}


DeltaVirtualFrame *CompiledTopLevelBlockVirtualFrame::parent() const {
    MethodOop m = method();
    if ( not m->expectsContext() )
        return nullptr;

    ContextOop parent_context = m->allocatesInterpretedContext() ? compiled_context()->outer_context() : compiled_context();

    // If the context is killed return nullptr
    if ( not parent_context or parent_context->is_dead() )
        return nullptr;

    // Now we have to search for the parent on the stack.
    ScopeDescriptor *ps                  = parent_scope();
    NativeMethod    *parent_nativeMethod = ps->scopes()->my_nativeMethod();

    Frame v = fr().sender();
    do {
        if ( v.is_compiled_frame() ) {
            if ( v.code() == parent_nativeMethod ) {
                CompiledVirtualFrame *result = (CompiledVirtualFrame *) VirtualFrame::new_vframe( &v );
                // Run throuch the scopes and find a matching one
                while ( result ) {
                    st_assert( result->is_compiled_frame(), "must be compiled frame" );
                    if ( result->scope()->is_equal( ps ) ) {
                        if ( result->compiled_context() == parent_context )
                            return result;
                    }
                    result = result->is_top() ? nullptr : (CompiledVirtualFrame *) result->sender();
                }
            }
        }
        v = v.sender();
    } while ( not v.is_first_frame() );
    return nullptr;
}


ContextOop CompiledTopLevelBlockVirtualFrame::canonical_context() const {
    st_assert( method()->is_blockMethod(), "must be block method" );
    ContextOop conIn  = compiled_context();
    ContextOop conOut = compute_canonical_context( scope(), this, conIn );
    if ( TraceCanonicalContext ) {
        _console->print( "context in(0x{0:x}), vf(0x{0:x}), context out(0x{0:x}), block? %d", conIn, this, conOut, method()->is_blockMethod() );
    }
    return conOut;
}


ScopeDescriptor *CompiledTopLevelBlockVirtualFrame::parent_scope() const {
    ScopeDescriptor *result = scope()->parent( true );
    st_assert( result, "parent scope must be present" );
    return result;
}

// ------------- DeoptimizedVirtualFrame --------------

ObjectArrayOop DeoptimizedVirtualFrame::retrieve_frame_array() const {
    ObjectArrayOop array = _frame.frame_array();
    st_assert( array->is_objArray(), "expecting objArray" );
    return array;
}


Oop DeoptimizedVirtualFrame::obj_at( std::int32_t index ) const {
    return _frameArray->obj_at( _offset + index );
}


bool DeoptimizedVirtualFrame::is_top() const {
    return _offset + end_of_expressions() > _frameArray->length();
}


std::int32_t DeoptimizedVirtualFrame::end_of_expressions() const {
    SMIOop n = SMIOop( obj_at( locals_size_offset ) );
    st_assert( n->is_smi(), "expecting smi_t" );
    return first_temp_offset + n->value();
}


DeoptimizedVirtualFrame::DeoptimizedVirtualFrame( const Frame *fr ) :
    DeltaVirtualFrame( fr ),
    _offset{ StackChunkBuilder::first_frame_index },
    _frameArray{ retrieve_frame_array() } {
    // the first frame in the array is located at position 3 (after #frames, #locals)
}


DeoptimizedVirtualFrame::DeoptimizedVirtualFrame( const Frame *fr, std::int32_t offset ) :
    DeltaVirtualFrame( fr ),
    _offset{ offset },
    _frameArray{} {
    _frameArray = retrieve_frame_array();
}


Oop DeoptimizedVirtualFrame::receiver() const {
    return obj_at( receiver_offset );
}


MethodOop DeoptimizedVirtualFrame::method() const {
    MethodOop m = MethodOop( obj_at( method_offset ) );
    st_assert( m->is_method(), "expecting method" );
    return m;
}


std::int32_t DeoptimizedVirtualFrame::byteCodeIndex() const {
    SMIOop b = SMIOop( obj_at( byteCodeIndex_offset ) );
    st_assert( b->is_smi(), "expecting smi_t" );
    return b->value();
}


VirtualFrame *DeoptimizedVirtualFrame::sender() const {
    return is_top() ? VirtualFrame::sender() : new DeoptimizedVirtualFrame( &_frame, _offset + end_of_expressions() );
}


bool DeoptimizedVirtualFrame::equal( const VirtualFrame *f ) const {
    if ( not f->is_deoptimized_frame() )
        return false;
    return VirtualFrame::equal( f ) and _offset == ( (DeoptimizedVirtualFrame *) f )->_offset;
}


Oop DeoptimizedVirtualFrame::temp_at( std::int32_t offset ) const {
    return obj_at( first_temp_offset + offset );
}


Oop DeoptimizedVirtualFrame::expression_at( std::int32_t index ) const {
    return obj_at( end_of_expressions() - 1 - index );
}


Oop DeoptimizedVirtualFrame::context_temp_at( std::int32_t offset ) const {
    st_assert( deoptimized_context(), "must have context when using context_temp_at" );
    return deoptimized_context()->obj_at( offset );
}


ContextOop DeoptimizedVirtualFrame::deoptimized_context() const {
    if ( not method()->activation_has_context() )
        return nullptr;
    ContextOop result = ContextOop( temp_at( 0 ) );
    st_assert( result->is_context(), "context type check" );
    return result;
}


ContextOop DeoptimizedVirtualFrame::canonical_context() const {
    return deoptimized_context();
}


GrowableArray<Oop> *DeoptimizedVirtualFrame::expression_stack() const {
    std::int32_t locals   = end_of_expressions() - first_temp_offset;
    std::int32_t temps    = method()->number_of_stack_temporaries();
    std::int32_t exp_size = locals - temps;

    GrowableArray<Oop> *array = new GrowableArray<Oop>( exp_size );
    for ( std::int32_t index  = 0; index < exp_size; index++ ) {
        array->push( expression_at( index ) );
    }

    std::int32_t computed_size = method()->expression_stack_mapping( byteCodeIndex() )->length();
    if ( exp_size not_eq computed_size )
        spdlog::warn( "Expression stack size is %d but computed to %d", exp_size, computed_size );

    return array;
}


// ------------- cVFrame --------------

void cVFrame::print() {
    VirtualFrame::print();
    SPDLOG_INFO( "C frame" );
}


void cVFrame::print_value() const {
    ( (VirtualFrame *) this )->print();
}

// ------------- cChunk --------------

VirtualFrame *cChunk::sender() const {
    return cVFrame::sender();
}


void cChunk::print_value() const {
    ( (cChunk *) this )->print();
}


void cChunk::print() {
    VirtualFrame::print();
    SPDLOG_INFO( "C Chunk inbetween Delta" );
    SPDLOG_INFO( "C     link 0x%lx", static_cast<const void *>(_frame.link()) );
}


Oop cChunk::callee_argument_at( std::int32_t index ) const {
    return Oop( _frame.sp()[ index ] );
}
