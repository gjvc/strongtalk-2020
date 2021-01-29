//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/memory/allocation.hpp"
#include "vm/assembler/Location.hpp"
#include "vm/runtime/Frame.hpp"
#include "ResourceObject.hpp"


//
// A VirtualFrame is a virtual stack frame representing a source-level activation.
// A DeltaVirtualFrame represents an activation of a Delta level method.
//
// A single frame may hold several source level activations in the case of
// optimized code. The debugging stored with the optimized code enables
// us to unfold a frame as a stack of vframes.
//
// A cVFrame represents an activation of a non-Delta method.
//

// The VirtualFrame inheritance hierarchy:
// - VirtualFrame
//   - DeltaVirtualFrame
//     - InterpretedVirtualFrame
//     - CompiledVirtualFrame
//     - DeoptimizedVirtualFrame  ; special DeltaVirtualFrame for deoptimized off stack VFrames
//
//   - cVFrame
//     - cChunk             ; special frame created when entering Delta from C

class VirtualFrame : public PrintableResourceObject {

private:
    // Interface for the accessing the callees argument.
    // Must be provided for all vframes calling deltaVFrames.
    virtual Oop callee_argument_at( std::int32_t index ) const;

protected:
    Frame _frame;


    VirtualFrame( const Frame *frame ) {
        _frame = *frame;
    }    // for subclass use only

public:
    // constructor: creates bottom (most recent) VirtualFrame from a frame
    static VirtualFrame *new_vframe( Frame *f );


    // returns the frame
    Frame fr() const {
        return _frame;
    }


    // returns the sender VirtualFrame
    virtual VirtualFrame *sender() const;


    // answers if the receiver is the top VirtualFrame in the frame, i.e., if the sender VirtualFrame
    // is in the caller frame
    virtual bool is_top() const {
        return true;
    }


    // returns top VirtualFrame within same frame (see is_top())
    virtual VirtualFrame *top() const;

    // comparison operation
    virtual bool equal( const VirtualFrame *virtualFrame ) const;


    // type testing operations
    virtual bool is_c_frame() const {
        return false;
    }


    virtual bool is_c_chunk() const {
        return false;
    }


    virtual bool is_delta_frame() const {
        return false;
    }


    virtual bool is_interpreted_frame() const {
        return false;
    }


    virtual bool is_compiled_frame() const {
        return false;
    }


    virtual bool is_deoptimized_frame() const {
        return false;
    }


    // printing operations
    virtual void print_value() const;

    virtual void print();


    // verify operations
    virtual void verify() const {
    };

    // friends
    friend class InterpretedVirtualFrame;

    friend class CompiledVirtualFrame;

    friend class cVFrame;

    friend class DeltaVirtualFrame;

    friend class DeoptimizedVirtualFrame;
};

class DeltaVirtualFrame : public VirtualFrame {
private:
    Oop callee_argument_at( std::int32_t index ) const; // see VirtualFrame

public:
    DeltaVirtualFrame( const Frame *fr ) :
            VirtualFrame( fr ) {
    }


    bool is_delta_frame() const {
        return true;
    }


    // returns the receiver (block for block invoc.)
    virtual Oop receiver() const = 0;

    // returns the active method
    virtual MethodOop method() const = 0;

    // returns the current byte code index
    virtual std::int32_t byteCodeIndex() const = 0;

    virtual Oop temp_at( std::int32_t offset ) const = 0;

    virtual Oop expression_at( std::int32_t index ) const = 0;

    virtual Oop context_temp_at( std::int32_t offset ) const = 0;

    // Returns the interpreter contextOop for this VirtualFrame.
    // If the frame is optimized a converted context will be returned.
    // Returns nullptr is canonical form has no context.
    // (Only used during deoptimization)
    virtual ContextOop canonical_context() const = 0;

    // returns the expression stack
    virtual GrowableArray<Oop> *expression_stack() const = 0;

    // returns the lexical scope of an activation
    // - method activations yield nullptr.
    // - pure block activations yield nullptr.
    // - non lifo block activations yield nullptr.
    // - other block activations yield the parent activation
    //   if the parent frame resides on the same stack, nullptr otherwise!
    virtual DeltaVirtualFrame *parent() const = 0;

    // returns the nearest delta VirtualFrame in the sender chain
    DeltaVirtualFrame *sender_delta_frame() const;

    // returns the arguments
    GrowableArray<Oop> *arguments() const;

    // arguments are numbered from 1 to n
    Oop argument_at( std::int32_t index ) const;

    // printing operations
    void print();

    void print_activation( std::int32_t index ) const;

    // verify operations
    void verify() const;


    virtual void verify_debug_info() const {
    };
};

// Layout of an interpreter frame:
// sp-> [expressions ] *   ^
//      [temps       ] *   |
//      [hp          ]    -2
//      [receiver    ]    -1
// fp-> [link        ]     0
//      [return pc   ]    +1
//      [arguments   ]    +2

class InterpretedVirtualFrame : public DeltaVirtualFrame {
public:
    InterpretedVirtualFrame( Frame *fr ) :
            DeltaVirtualFrame( fr ) {
    };

    // Sets the receiver object
    void set_receiver( Oop obj );

    // Accessors for HP
    std::uint8_t *hp() const;

    void set_hp( std::uint8_t *p );

    // Sets temporaries
    void temp_at_put( std::int32_t offset, Oop obj );

    // Sets element on expression stack
    void expression_at_put( std::int32_t index, Oop obj );

    // Returns the contextOop for this interpreter frame
    // nullptr is returned is no context exists.
    ContextOop interpreter_context() const;

private:
    static const std::int32_t temp_offset;
    static const std::int32_t hp_offset;
    static const std::int32_t receiver_offset;
    static const std::int32_t argument_offset;

    Oop *expression_addr( std::int32_t offset ) const;

    bool has_interpreter_context() const;

public:
    // Virtuals from VirtualFrame
    bool is_interpreted_frame() const {
        return true;
    }


    bool equal( const VirtualFrame *f ) const;

public:
    // Virtuals from DeltaVirtualFrame
    Oop receiver() const;

    MethodOop method() const;

    std::int32_t byteCodeIndex() const;

    Oop temp_at( std::int32_t offset ) const;

    Oop expression_at( std::int32_t index ) const;

    Oop context_temp_at( std::int32_t offset ) const;

    ContextOop canonical_context() const;

    GrowableArray<Oop> *expression_stack() const;

    DeltaVirtualFrame *parent() const;

    void verify() const;
};

class DeferredExpression;

class ScopeDescriptor;

class NameDescriptor;

class CompiledVirtualFrame : public DeltaVirtualFrame {
public:
    // Constructors
    static CompiledVirtualFrame *new_vframe( const Frame *fr, ScopeDescriptor *sd, std::int32_t byteCodeIndex );

    CompiledVirtualFrame( const Frame *fr, ScopeDescriptor *sd, std::int32_t byteCodeIndex );

    // Returns the active NativeMethod
    NativeMethod *code() const;


    // Returns the scopeDesc
    ScopeDescriptor *scope() const {
        return _scopeDescriptor;
    }


    // Returns the contextOop for this interpreter frame
    // nullptr is returned is no context exists.
    ContextOop compiled_context() const;

    // Rewind the byteCodeIndex one step
    void rewind_byteCodeIndex();

    // Returns the scope for the parent.
    virtual ScopeDescriptor *parent_scope() const = 0;

protected:
    ScopeDescriptor *_scopeDescriptor;
    std::int32_t    _byteCodeIndex;

    static ContextOop compute_canonical_context( ScopeDescriptor *sd, const CompiledVirtualFrame *vf, ContextOop con = nullptr );

    static ContextOop compute_canonical_parent_context( ScopeDescriptor *scope, const CompiledVirtualFrame *vf, ContextOop con );

    static Oop resolve_name( NameDescriptor *nd, const CompiledVirtualFrame *vf, ContextOop con = nullptr );

    static Oop resolve_location( Location loc, const CompiledVirtualFrame *vf, ContextOop con = nullptr );

    // The filler_obj is used during deoptimization for values that couldn't be retrieved.
    // - stack temps if the frame is absent
    // - context variables if the frame or optimized context is absent.
    // In the ideal situation this should never be used but is works great as a defensive mechanism.
    static Oop filler_oop();

    // Returns the byteCodeIndex for a scope desc.
    // d must belong to the same NativeMethod and be in the sender chain.
    std::int32_t byteCodeIndex_for( ScopeDescriptor *d ) const;

    friend struct MemoizedBlockNameDescriptor;

    friend class BlockClosureOopDescriptor;

public:
    // Virtuals defined in VirtualFrame
    bool is_compiled_frame() const {
        return true;
    }


    VirtualFrame *sender() const;

    bool equal( const VirtualFrame *f ) const;

public:
    // Virtuals defined in DeltaVirtualFrame
    MethodOop method() const;

    std::int32_t byteCodeIndex() const;

    Oop temp_at( std::int32_t offset ) const;

    Oop expression_at( std::int32_t index ) const;

    Oop context_temp_at( std::int32_t offset ) const;

    GrowableArray<Oop> *expression_stack() const;

    GrowableArray<DeferredExpression *> *deferred_expression_stack() const;

    void verify() const;

    void verify_debug_info() const;

    friend class DeferredExpression;
};

class DeferredExpression : public ResourceObject {
private:
    CompiledVirtualFrame const *const _frame;
    NameDescriptor *expression;
public:
    DeferredExpression( CompiledVirtualFrame const *const aframe, NameDescriptor *expression ) :
            _frame( aframe ), expression( expression ) {
    }


    Oop value() {
        return CompiledVirtualFrame::resolve_name( expression, _frame );
    }
};

class CompiledMethodVirtualFrame : public CompiledVirtualFrame {
public:
    CompiledMethodVirtualFrame( const Frame *fr, ScopeDescriptor *sd, std::int32_t byteCodeIndex );

public:
    // Virtuals defined in VirtualFrame
    bool is_top() const;

public:
    // Virtuals defined in DeltaVirtualFrame
    DeltaVirtualFrame *parent() const {
        return nullptr;
    }


    ContextOop canonical_context() const;

    Oop receiver() const;


    // Virtuals defined in CompiledVirtualFrame
    ScopeDescriptor *parent_scope() const {
        return nullptr;
    }
};


class CompiledBlockVirtualFrame : public CompiledVirtualFrame {
public:
    CompiledBlockVirtualFrame( const Frame *fr, ScopeDescriptor *sd, std::int32_t byteCodeIndex );

public:
    // Virtuals defined in VirtualFrame
    bool is_top() const;

public:
    // Virtuals defined in DeltaVirtualFrame
    DeltaVirtualFrame *parent() const;

    ContextOop canonical_context() const;

    Oop receiver() const;

    // Virtuals defined in CompiledVirtualFrame
    ScopeDescriptor *parent_scope() const;
};


class CompiledTopLevelBlockVirtualFrame : public CompiledVirtualFrame {
public:
    CompiledTopLevelBlockVirtualFrame( const Frame *fr, ScopeDescriptor *sd, std::int32_t byteCodeIndex );

public:
    // Virtuals defined in VirtualFrame
    bool is_top() const {
        return true;
    }


public:
    // Virtuals defined in DeltaVirtualFrame
    Oop receiver() const;

    ContextOop canonical_context() const;

    DeltaVirtualFrame *parent() const;

    // Virtuals defined in CompiledVirtualFrame
    ScopeDescriptor *parent_scope() const;
};


// A DeoptimizedVirtualFrame is represented by a frame and an offset into the packed array of frames.
class DeoptimizedVirtualFrame : public DeltaVirtualFrame {

public:
    DeoptimizedVirtualFrame( const Frame *fr );

    DeoptimizedVirtualFrame( const Frame *fr, std::int32_t offset );

    // Returns the contextOop for this unoptimized frame. nullptr is returned is no context exists.
    ContextOop deoptimized_context() const;

private:
    std::int32_t   _offset;
    ObjectArrayOop _frameArray;

    // Retrieves the frame array from the frame
    ObjectArrayOop retrieve_frame_array() const;

    // Returns the Oop at offset+index
    Oop obj_at( std::int32_t index ) const;

    std::int32_t end_of_expressions() const;

    enum {
        receiver_offset      = 0, //
        method_offset        = 1, //
        byteCodeIndex_offset = 2, //
        locals_size_offset   = 3, //
        first_temp_offset    = 4, //
    };

    friend class StackChunkBuilder;

public:
    // Virtuals defined in VirtualFrame
    bool equal( const VirtualFrame *f ) const;


    bool is_deoptimized_frame() const {
        return true;
    }


    VirtualFrame *sender() const;

    bool is_top() const;

public:
    // Virtuals defined in DeltaVirtualFrame
    Oop receiver() const;

    MethodOop method() const;

    std::int32_t byteCodeIndex() const;


    DeltaVirtualFrame *parent() const {
        return nullptr;
    }


    Oop temp_at( std::int32_t offset ) const;

    Oop expression_at( std::int32_t index ) const;

    Oop context_temp_at( std::int32_t offset ) const;

    GrowableArray<Oop> *expression_stack() const;

    ContextOop canonical_context() const;
};

class cVFrame : public VirtualFrame {

public:
    cVFrame( const Frame *fr ) :
            VirtualFrame( fr ) {
    }


public:
    // Virtuals defined in VirtualFrame
    bool is_c_frame() const {
        return true;
    }


    void print_value() const;

    void print();
};

class cChunk : public cVFrame {
public:
    cChunk( const Frame *fr ) :
            cVFrame( fr ) {
    }


public:
    // Virtuals defined in VirtualFrame
    bool is_c_chunk() const {
        return true;
    }


    VirtualFrame *sender() const;

    void print_value() const;

    void print();

private:
    // Virtual defined in VirtualFrame
    Oop callee_argument_at( std::int32_t index ) const;
};
