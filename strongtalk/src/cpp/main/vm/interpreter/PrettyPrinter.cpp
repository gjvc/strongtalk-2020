
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/interpreter/PrettyPrinter.hpp"
#include "vm/runtime/ResourceArea.hpp"
#include "vm/runtime/TempDecoder.hpp"
#include "vm/runtime/VirtualFrame.hpp"
#include "vm/oops/MethodOopDescriptor.hpp"
#include "vm/oops/KlassOopDescriptor.hpp"
#include "vm/oops/ContextOopDescriptor.hpp"
#include "vm/code/ScopeDescriptor.hpp"
#include "vm/oops/ObjectArrayOopDescriptor.hpp"
#include "vm/oops/AssociationOopDescriptor.hpp"
#include "vm/oops/DoubleOopDescriptor.hpp"
#include "vm/interpreter/MethodIterator.hpp"
#include "vm/interpreter/MethodInterval.hpp"
#include "vm/interpreter/MethodIntervalFactory.hpp"
#include "vm/interpreter/InterpretedInlineCache.hpp"
#include "vm/memory/vmSymbols.hpp"
#include "vm/runtime/ResourceMark.hpp"
#include "vm/memory/oopFactory.hpp"
#include "vm/interpreter/MethodClosure.hpp"


// ToDo list for pretty printer
// - convert the stream stuff to ostream.
// - improve the namedesc printout for temps etc.

class statement;

class codeNode;

class scopeNode;

class astNode : public ResourceObject {

protected:
    std::int32_t _byteCodeIndex;
    scopeNode    *_scopeNode;

public:

    //
    astNode( std::int32_t byteCodeIndex, scopeNode *scope ) :
        _byteCodeIndex{ byteCodeIndex },
        _scopeNode{ scope } {
    }


    astNode() = default;
    virtual ~astNode() = default;
    astNode( const astNode & ) = default;
    astNode &operator=( const astNode & ) = default;


    void operator delete( void *ptr ) { (void) ( ptr ); }


    std::int32_t this_byteCodeIndex() {
        return _byteCodeIndex;
    }


    scopeNode *this_scope() {
        return _scopeNode;
    }


    virtual bool is_message() {
        return false;
    }


    virtual bool is_statement() {
        return false;
    }


    virtual bool is_code() {
        return false;
    }


    virtual bool is_cascade() {
        return false;
    }


    virtual void add( astNode *statement ) {
        static_cast<void>(statement); // unused
        st_fatal( "subclass should implement add" );
    }


    void top_level_print( PrettyPrintStream *output );

    virtual bool print( PrettyPrintStream *output );


    virtual std::int32_t width( PrettyPrintStream *output ) {
        return output->infinity();
    }


    virtual SymbolOop selector() {
        return nullptr;
    }


    virtual bool should_wrap_argument( astNode *argument ) {
        static_cast<void>(argument); // unused
        return false;
    };


    virtual astNode *argument_at( std::int32_t i ) {
        static_cast<void>(i); // unused
        return nullptr;
    }
};


class PrintWrapper {
private:
    astNode           *_astNode;
    bool              _hit;
    PrettyPrintStream *_output;
public:
    PrintWrapper( astNode *astNode, PrettyPrintStream *output );

    PrintWrapper() = default;
    ~PrintWrapper();
    PrintWrapper( const PrintWrapper & ) = default;
    PrintWrapper &operator=( const PrintWrapper & ) = default;


    void operator delete( void *ptr ) { (void) ( ptr ); }

};


#define HIGHLIGHT PrintWrapper pw(this, output);

bool should_wrap( std::int32_t type, astNode *arg );


static bool print_selector_with_arguments( PrettyPrintStream *output, SymbolOop selector, GrowableArray<astNode *> *arguments, bool split ) {

    if ( selector->is_binary() ) {
        // binary send
        output->print( selector->as_string() );
        output->space();
        astNode *arg = arguments->at( 0 );
        bool    wrap = should_wrap( 1, arg );
        if ( wrap )
            output->print( "(" );
        bool result = arg->print( output );
        if ( wrap )
            output->print( ")" );
        return result;
    }

    std::int32_t arg = arguments->length();

    if ( arg == 0 ) {
        output->print( selector->as_string() );
        return false;
    }

    for ( std::int32_t i = 1; i <= selector->length(); i++ ) {
        std::int32_t c = selector->byte_at( i );
        output->print_char( c );
        if ( c == ':' ) {
            output->space();
            astNode *a   = arguments->at( --arg );
            bool    wrap = should_wrap( 2, a );
            if ( wrap )
                output->print( "(" );
            a->print( output );
            if ( wrap )
                output->print( ")" );

            if ( i < selector->length() ) {
                if ( split )
                    output->newline();
                else
                    output->space();
            }
        }
    }
    return split;
}


static astNode *get_literal_node( Oop obj, std::int32_t byteCodeIndex, scopeNode *scope );

class PrintTemps : public TempDecoder {

public:
    GrowableArray<astNode *> *_elements;
    scopeNode                *_scope;


    PrintTemps() :
        TempDecoder(),
        _elements{ nullptr },
        _scope{ nullptr } {}


    virtual ~PrintTemps() = default;
    PrintTemps( const PrintTemps & ) = default;
    PrintTemps &operator=( const PrintTemps & ) = default;


    void operator delete( void *ptr ) { (void) ( ptr ); }


    void decode( MethodOop method, scopeNode *scope ) {
        _scope    = scope;
        _elements = new GrowableArray<astNode *>( 10 );
        TempDecoder::decode( method );
    }


    void stack_temp( ByteArrayOop name, std::int32_t offset );

    void heap_temp( ByteArrayOop name, std::int32_t offset );
};


class PrintParams : public TempDecoder {

private:
    scopeNode *_scope;

public:
    GrowableArray<astNode *> *_elements;


    PrintParams() : _scope{nullptr}, _elements{nullptr} {};
    virtual ~PrintParams() = default;
    PrintParams( const PrintParams & ) = default;
    PrintParams &operator=( const PrintParams & ) = default;


    void operator delete( void *ptr ) { (void) ( ptr ); }


    void decode( MethodOop method, scopeNode *scope ) {
        _scope    = scope;
        _elements = new GrowableArray<astNode *>( 10 );
        TempDecoder::decode( method );
    }


    void parameter( ByteArrayOop name, std::int32_t index );
};


class leafNode : public astNode {

public:
    leafNode( std::int32_t byteCodeIndex, scopeNode *scope ) :
        astNode( byteCodeIndex, scope ) {
    }


    leafNode() = default;
    virtual ~leafNode() = default;
    leafNode( const leafNode & ) = default;
    leafNode &operator=( const leafNode & ) = default;


    void operator delete( void *ptr ) { (void) ( ptr ); }


    bool print( PrettyPrintStream *output ) {
        HIGHLIGHT
        astNode::print( output );
        output->print( string() );
        return false;
    }


    std::int32_t width( PrettyPrintStream *output ) {
        return output->width_of_string( string() );
    }


    virtual const char *string() = 0;
};


class paramNode : public leafNode {

private:
    std::int32_t _no;
    const char   *_str;

public:
    paramNode( std::int32_t byteCodeIndex, scopeNode *scope, std::int32_t no );


    paramNode() = default;
    virtual ~paramNode() = default;
    paramNode( const paramNode & ) = default;
    paramNode &operator=( const paramNode & ) = default;


    void operator delete( void *ptr ) { (void) ( ptr ); }


    const char *string() {
        return _str;
    }


    bool is_param() {
        return true;
    }
};


class nameValueNode : public astNode {

private:
    const char *_name;
    const char *_value;

public:
    nameValueNode( const char *name, char *value ) :
        astNode( 0, 0 ),
        _name{ name },
        _value{ value } {
    }


    nameValueNode() = default;
    virtual ~nameValueNode() = default;
    nameValueNode( const nameValueNode & ) = default;
    nameValueNode &operator=( const nameValueNode & ) = default;


    void operator delete( void *ptr ) { (void) ( ptr ); }


    bool print( PrettyPrintStream *output ) {
        output->print( _name );
        output->print( " \"" );
        output->print( _value );
        output->print( "\"" );
        return false;
    }


    std::int32_t width( PrettyPrintStream *output ) {
        return output->width_of_string( _name ) + 3 + output->width_of_string( _value );
    }
};


class nameNode : public leafNode {

private:
    const char *_str;

public:
    nameNode( const char *str ) :
        leafNode( 0, 0 ), _str{ str } {
    }


    nameNode() = default;
    virtual ~nameNode() = default;
    nameNode( const nameNode & ) = default;
    nameNode &operator=( const nameNode & ) = default;


    void operator delete( void *ptr ) { (void) ( ptr ); }


    const char *string() {
        return _str;
    }
};


class listNode : public astNode {

protected:
    GrowableArray<astNode *> *_elements;
    const char               *_beginSym;
    const char               *_endSym;

public:
    listNode( const char *begin_sym, const char *end_sym ) :
        astNode( 0, 0 ),
        _elements{ nullptr },
        _beginSym{ begin_sym },
        _endSym{ end_sym } {
        _elements = new GrowableArray<astNode *>( 10 );
    }


    listNode() = default;
    virtual ~listNode() = default;
    listNode( const listNode & ) = default;
    listNode &operator=( const listNode & ) = default;


    void operator delete( void *ptr ) { (void) ( ptr ); }


    void add( astNode *element ) {
        _elements->push( element );
    }


    bool print( PrettyPrintStream *output ) {
        if ( _beginSym ) {
            output->print( _beginSym );
            output->space();
        }

        for ( std::int32_t i = 0; i < _elements->length(); i++ ) {
            _elements->at( i )->print( output );
            output->space();
        }

        if ( _endSym )
            output->print( _endSym );
        return false;
    }


    std::int32_t width( PrettyPrintStream *output ) {
        std::int32_t w = 0;
        if ( _beginSym )
            w += output->width_of_string( _beginSym ) + output->width_of_space();

        for ( std::int32_t i = 0; i < _elements->length(); i++ ) {
            w += _elements->at( i )->width( output ) + output->width_of_space();
        }

        if ( _endSym )
            w += output->width_of_string( _endSym );

        return w;
    }

};


class scopeNode : public astNode {

protected:
    MethodOop         _methodOop;
    KlassOop          _klassOop;
    std::int32_t      _in;
    std::int32_t      _hotByteCodeIndex;
    std::int32_t      _frameIndex;
    DeltaVirtualFrame *_deltaVirtualFrame;
    ScopeDescriptor   *_scopeDescriptor;
    scopeNode         *_parentScope;
    scopeNode         *_innerScope;

public:
    scopeNode( DeltaVirtualFrame *fr, std::int32_t index, scopeNode *scope = nullptr ) :
        astNode( 0, nullptr ),
        _methodOop{},
        _klassOop{},
        _in{ 0 },
        _hotByteCodeIndex{ 0 },
        _frameIndex{ 0 },
        _deltaVirtualFrame{ nullptr },
        _parentScope{ nullptr },
        _scopeDescriptor{ nullptr },
        _innerScope{ scope } {

        _frameIndex        = index;
        _methodOop         = fr->method();
        _klassOop          = fr->receiver()->klass();
        _hotByteCodeIndex  = scope ? -1 : fr->byteCodeIndex();
        _deltaVirtualFrame = fr;
        _in                = 0;
        _scopeDescriptor   = _deltaVirtualFrame->is_compiled_frame() ? ( (CompiledVirtualFrame *) _deltaVirtualFrame )->scope() : nullptr;
        _innerScope        = scope;

        initParent();
    }


    scopeNode( MethodOop method, KlassOop klass, std::int32_t hot_byteCodeIndex, scopeNode *scope = nullptr ) :

        _methodOop{},
        _klassOop{},
        _in{ 0 },
        _hotByteCodeIndex{ 0 },
        _frameIndex{ 0 },
        _deltaVirtualFrame{ nullptr },
        _parentScope{ nullptr },
        _scopeDescriptor{ nullptr },
        _innerScope{ nullptr },

        astNode( 0, nullptr ) {

        _methodOop         = method;
        _klassOop          = klass;
        _hotByteCodeIndex  = hot_byteCodeIndex;
        _deltaVirtualFrame = nullptr;
        _in                = 0;
        _scopeDescriptor   = nullptr;
        _innerScope        = scope;

        initParent();
    }


    scopeNode() = default;
    virtual ~scopeNode() = default;
    scopeNode( const scopeNode & ) = default;
    scopeNode &operator=( const scopeNode & ) = default;


    void operator delete( void *ptr ) { (void) ( ptr ); }


    void initParent() {
        _parentScope = _methodOop->is_blockMethod() ? new scopeNode( _methodOop->parent(), _klassOop, -1, this ) : nullptr;
    }


    DeltaVirtualFrame *fr() {
        return _deltaVirtualFrame;
    }


    ScopeDescriptor *sd() const {
        return _scopeDescriptor;
    }


    MethodOop method() {
        return _methodOop;
    }


    std::int32_t hot_byteCodeIndex() {
        return _hotByteCodeIndex;
    }


    KlassOop get_klass() {
        return _klassOop;
    }


    bool is_block_method() {
        return method()->is_blockMethod();
    }


    void context_allocated() {
        st_assert( is_context_allocated(), "just checking" );
    }


    bool is_context_allocated() {
        return method()->allocatesInterpretedContext();
    }


    scopeNode *scopeFor( MethodOop method ) {
        return _innerScope and _innerScope->isFor( method ) ? _innerScope : nullptr;
    }


    bool isFor( MethodOop method ) {
        return _methodOop == method;
    }


    scopeNode *homeScope() {
        return _parentScope ? _parentScope->homeScope() : this;
    }


    const char *param_string( std::int32_t index, bool in_block = false ) {
        ByteArrayOop res = find_parameter_name( method(), index );
        if ( in_block ) {
            if ( not res )
                return create_name( ":p", index );
            char *buffer = new_resource_array<char>( res->length() + 2 );
            buffer[ 0 ] = ':';
            res->copy_null_terminated( &buffer[ 1 ], res->length() );
            return buffer;
        } else {
            return res ? res->as_string() : create_name( "p", index );
        }
    }


    const char *stack_temp_string( std::int32_t byteCodeIndex, std::int32_t no ) {
        ByteArrayOop res = find_stack_temp( method(), byteCodeIndex, no );
        return res ? res->as_string() : create_name( "t", no );
    }


    const char *stack_float_temp_string( std::int32_t byteCodeIndex, std::int32_t fno ) {
        ByteArrayOop res = find_stack_float_temp( method(), byteCodeIndex, fno );
        return res ? res->as_string() : create_name( "f", fno );
    }


    const char *create_name( const char *prefix, std::int32_t no ) {
        char *str = new_resource_array<char>( 7 );
        sprintf( str, "%s_%d", prefix, no );
        return str;
    }


    const char *heap_temp_string( std::int32_t byteCodeIndex, std::int32_t no, std::int32_t context_level ) {
        if ( is_block_method() ) {
            std::int32_t level = context_level;
            if ( is_context_allocated() ) {
                level--;
                if ( context_level == 0 ) {
                    ByteArrayOop res = find_heap_temp( method(), byteCodeIndex, no );
                    return res ? res->as_string() : create_name( "ht", no );
                }
            }
            scopeNode    *n    = new scopeNode( method()->parent(), _klassOop, 0 );
            return n->heap_temp_string( byteCodeIndex, no, level );
        } else {
            ByteArrayOop res = find_heap_temp( method(), byteCodeIndex, no );
            return res ? res->as_string() : create_name( "ht", no );
        }
    }


    const char *inst_var_string( std::int32_t offset ) {
        if ( _klassOop ) {
            SymbolOop name = _klassOop->klass_part()->inst_var_name_at( offset );
            if ( name )
                return name->as_string();
        }
        char *str = new_resource_array<char>( 15 );
        sprintf( str, "i_%d", offset );
        return str;
    }


    astNode *temps() {
        PrintTemps p;
        p.decode( method(), this );
        if ( p._elements->length() == 0 )
            return nullptr;
        listNode           *l = new listNode( "|", "|" );
        for ( std::int32_t i  = 0; i < p._elements->length(); i++ )
            l->add( p._elements->at( i ) );
        return l;
    }


    astNode *params() {
        PrintParams p;
        p.decode( method(), this );
        if ( p._elements->length() == 0 )
            return nullptr;

        listNode           *l = new listNode( nullptr, "|" );
        for ( std::int32_t i  = 0; i < p._elements->length(); i++ ) {
            l->add( p._elements->at( i ) );
        }
        return l;
    }


    astNode *heap_temp_at( std::int32_t no ) {
        const char *name = heap_temp_string( 0, no, 0 );
        if ( not fr() )
            return new nameNode( name );
        char *value = fr()->context_temp_at( no )->print_value_string();
        return new nameValueNode( name, value );
    }


    astNode *stack_temp_at( std::int32_t no ) {
        ByteArrayOop res   = find_stack_temp( method(), 0, no );
        const char   *name = res ? res->as_string() : create_name( "t", no );
        if ( not fr() )
            return new nameNode( name );
        char *value = fr()->temp_at( no )->print_value_string();
        return new nameValueNode( name, value );
    }


    astNode *parameter_at( std::int32_t index, bool in_block = false ) {
        const char *name = param_string( index, in_block );
        if ( not fr() )
            return new nameNode( name );
        char *value = fr()->argument_at( index )->print_value_string();
        return new nameValueNode( name, value );
    }


    void print_frame_header( PrettyPrintStream *output ) {
        char num[10];
        output->newline();
        output->print( "#" );
        st_assert( _frameIndex <= 999999999, "frame index too large for buffer - buffer overrun" );
        sprintf( num, "%d", _frameIndex );
        output->print( num );
        output->print( ", receiver \"" );
        output->print( fr()->receiver()->print_value_string() );
        output->print( "\"" );

        if ( not method()->is_blockMethod() ) {
            KlassOop method_holder = fr()->receiver()->klass()->klass_part()->lookup_method_holder_for( method() );
            if ( method_holder and ( method_holder not_eq fr()->receiver()->klass() ) ) {
                output->print( ", method holder \"" );
                output->print( method_holder->print_value_string() );
                output->print( "\"" );
            }
        }
        output->newline();

        print_method_header( output );    // print method header first (in case something crashes later)

        output->newline();

        if ( ActivationShowByteCodeIndex ) {
            // Print current byteCodeIndex
            output->print( " [byteCodeIndex = " );
            sprintf( num, "%d", _hotByteCodeIndex );
            output->print( num );
            output->print( "]" );
            output->newline();
        }

        if ( ActivationShowExpressionStack ) {
            // expression stack:
            GrowableArray<Oop> *stack = fr()->expression_stack();
            if ( stack->length() > 0 ) {
                output->print( " [expression stack:" );
                output->newline();
                GrowableArray<Oop> *stack = fr()->expression_stack();
                for ( std::int32_t index  = 0; index < stack->length(); index++ ) {
                    output->print( " - " );
                    stack->at( index )->print_value();
                    output->newline();
                }
                output->print( " ]" );
                output->newline();
            }
        }

        if ( ActivationShowContext ) {
            // Print the context if present
            ContextOop con = nullptr;
            if ( fr()->is_interpreted_frame() ) {
                con = ( (InterpretedVirtualFrame *) fr() )->interpreter_context();
            } else if ( fr()->is_deoptimized_frame() ) {
                con = ( (DeoptimizedVirtualFrame *) fr() )->deoptimized_context();
            } else if ( fr()->is_compiled_frame() ) {
                con = ( (CompiledVirtualFrame *) fr() )->compiled_context();
            }
            if ( con )
                con->print();
        }
    }


    void print_header( PrettyPrintStream *output ) {
        if ( fr() ) {
            print_frame_header( output );
        } else {
            print_method_header( output );
        }
    }


    void print_method_header( PrettyPrintStream *output ) {
        GrowableArray<astNode *> *arguments = new GrowableArray<astNode *>( 10 );
        for ( std::int32_t       i          = method()->number_of_arguments() - 1; i >= 0; i-- ) {
            arguments->push( parameter_at( i ) );
        }
        print_selector_with_arguments( output, method()->selector(), arguments, false );
    }
};


paramNode::paramNode( std::int32_t byteCodeIndex, scopeNode *scope, std::int32_t no ) :
    _no{ no },
    _str{ nullptr },
    leafNode( byteCodeIndex, scope ) {
    _no  = no;
    _str = scope->param_string( no );
}


PrintWrapper::PrintWrapper( astNode *astNode, PrettyPrintStream *output ) :
    _astNode{ astNode },
    _output{ output },
    _hit{ false } {

    if ( not astNode->this_scope() ) {
        return;
    }

    if ( astNode->this_byteCodeIndex() == astNode->this_scope()->hot_byteCodeIndex() and not output->in_highlight() ) {
        output->begin_highlight();
        _hit = true;
    }
}


PrintWrapper::~PrintWrapper() {
    if ( _hit ) {
        _output->end_highlight();
    }
}


bool astNode::print( PrettyPrintStream *output ) {
    static_cast<void>(output); // unused

    if ( ActivationShowNameDescs ) {
        if ( _scopeNode and _scopeNode->sd() ) {
            NameDescriptor *nd = _scopeNode->sd()->exprStackElem( _byteCodeIndex );
            if ( nd ) {
                nd->print();
            }
        }
    }

    return false;
}


void astNode::top_level_print( PrettyPrintStream *output ) {
    _scopeNode->print_header( output );
    print( output );
}


class codeNode : public astNode {

protected:
    GrowableArray<astNode *> *_statements;

public:
    codeNode( std::int32_t byteCodeIndex, scopeNode *scope ) :
        astNode( byteCodeIndex, scope ),
        _statements{ nullptr } {
        _statements = new GrowableArray<astNode *>( 10 );
    }


    codeNode() = default;
    virtual ~codeNode() = default;
    codeNode( const codeNode & ) = default;
    codeNode &operator=( const codeNode & ) = default;


    void operator delete( void *ptr ) { (void) ( ptr ); }


    void add( astNode *statement ) {
        _statements->push( statement );
    }


    bool print( PrettyPrintStream *output );


    bool is_code() {
        return true;
    }


    statement *last_statement() {
        if ( _statements->length() > 0 ) {
            st_assert( _statements->at( _statements->length() - 1 )->is_statement(), "must be statement" );
            return (statement *) _statements->at( _statements->length() - 1 );
        }
        return nullptr;
    }


    std::int32_t width( PrettyPrintStream *output ) {
        std::int32_t len = _statements->length();
        if ( len == 0 ) {
            return 0;
        }

        if ( len == 1 ) {
            return _statements->at( 0 )->width( output );
        }

        return output->infinity();
    }
};


class inlinedBlockNode : public codeNode {

public:
    inlinedBlockNode( std::int32_t byteCodeIndex, scopeNode *scope ) :
        codeNode( byteCodeIndex, scope ) {
    }


    inlinedBlockNode() = default;
    virtual ~inlinedBlockNode() = default;
    inlinedBlockNode( const inlinedBlockNode & ) = default;
    inlinedBlockNode &operator=( const inlinedBlockNode & ) = default;


    void operator delete( void *ptr ) { (void) ( ptr ); }


    void add( astNode *statement ) {
        _statements->push( statement );
    }


    bool print( PrettyPrintStream *output ) {
        bool split = output->remaining() < width( output );
        output->print( "[" );
        if ( split )
            output->inc_newline();
        codeNode::print( output );
        if ( split )
            output->dec_indent();
        output->print( "]" );
        return split;
    }


    std::int32_t width( PrettyPrintStream *output ) {
        return output->width_of_string( "[" ) + codeNode::width( output ) + output->width_of_string( "]" );
    }
};


class statement : public astNode {

private:
    bool _hasReturn;
    astNode *_stat;


    const char *hat() {
        return "^";
    }


public:


    statement() = default;
    virtual ~statement() = default;
    statement( const statement & ) = default;
    statement &operator=( const statement & ) = default;


    void operator delete( void *ptr ) { (void) ( ptr ); }


    bool is_statement() {
        return true;
    }


    statement( std::int32_t byteCodeIndex, scopeNode *scope, astNode *stat, bool has_return ) :
        astNode( byteCodeIndex, scope ),
        _stat{ stat },
        _hasReturn{ has_return } {
    }


    void set_return() {
        _hasReturn = true;
    }


    bool has_hat() {
        return _hasReturn;
    }


    bool print( PrettyPrintStream *output ) {
        HIGHLIGHT
        if ( _hasReturn )
            output->print( hat() );
        return _stat->print( output );
    }


    std::int32_t width( PrettyPrintStream *output ) {
        return ( _hasReturn ? output->width_of_string( hat() ) : 0 ) + _stat->width( output );
    }
};


bool codeNode::print( PrettyPrintStream *output ) {

    bool first = true;

    for ( std::int32_t i = 0; i < _statements->length(); i++ ) {
        astNode *s = _statements->at( i );
        if ( not first ) {
            output->print( "." );
            output->newline();
        }
        s->print( output );
        if ( s->is_statement() and ( (statement *) s )->has_hat() )
            break;
        first = false;
    }
    return true;
}


class methodNode : public codeNode {

public:
    methodNode( std::int32_t byteCodeIndex, scopeNode *scope ) :
        codeNode( byteCodeIndex, scope ) {
    }


    methodNode() = default;
    virtual ~methodNode() = default;
    methodNode( const methodNode & ) = default;
    methodNode &operator=( const methodNode & ) = default;


    void operator delete( void *ptr ) { (void) ( ptr ); }


    bool print( PrettyPrintStream *output ) {
        output->inc_indent();
        output->indent();

        astNode *l = _scopeNode->temps();
        if ( l ) {
            l->print( output );
            output->newline();
        }
        codeNode::print( output );
        output->dec_newline();
        return true;
    }
};


class blockNode : public codeNode {

private:
    std::int32_t _numOfArgs;

public:
    blockNode( std::int32_t byteCodeIndex, scopeNode *scope, std::int32_t numOfArgs ) :
        codeNode( byteCodeIndex, scope ),
        _numOfArgs{ numOfArgs } {
    }


    blockNode() = default;
    virtual ~blockNode() = default;
    blockNode( const blockNode & ) = default;
    blockNode &operator=( const blockNode & ) = default;


    void operator delete( void *ptr ) { (void) ( ptr ); }


    bool print( PrettyPrintStream *output ) {
        HIGHLIGHT
        bool split = false;
        output->print( "[" );
        astNode *p = _scopeNode->params();
        if ( p ) {
            split = output->remaining() < p->width( output ) + output->width_of_space();
            if ( split ) {
                output->inc_newline();
                p->print( output );
                output->newline();
            } else {
                output->space();
                p->print( output );
            }
        }

        astNode *l = _scopeNode->temps();
        if ( l ) {
            if ( not split ) {
                if ( output->remaining() >= l->width( output ) + output->width_of_space() ) {
                    output->space();
                    l->print( output );
                } else {
                    output->inc_newline();
                    split = true;
                }
            }
            if ( split ) {
                l->print( output );
                output->newline();
            }
        }

        if ( not split ) {
            if ( output->remaining() >= codeNode::width( output ) ) {
                if ( l or p )
                    output->space();
                codeNode::print( output );
            } else {
                output->inc_newline();
                split = true;
            }
        }

        if ( split ) {
            codeNode::print( output );
        }
        output->print( "]" );
        if ( split )
            output->dec_indent();
        return split;
    }


    std::int32_t width( PrettyPrintStream *output ) {
        std::int32_t w  = output->width_of_string( "[" ) + output->width_of_string( "]" );
        astNode      *p = _scopeNode->params();
        if ( p )
            w += p->width( output ) + 2 * output->width_of_space();
        astNode *t = _scopeNode->temps();
        if ( t )
            w += t->width( output ) + output->width_of_space();
        w += codeNode::width( output );
        return w;
    }

};


class assignment : public astNode {

private:
    astNode *_variable;
    astNode *_e;


    const char *sym() {
        return ":=";
    }


public:
    assignment( std::int32_t byteCodeIndex, scopeNode *scope, astNode *variable, astNode *e ) :
        astNode( byteCodeIndex, scope ),
        _variable{ variable },
        _e{ e } {
    }


    assignment() = default;
    virtual ~assignment() = default;
    assignment( const assignment & ) = default;
    assignment &operator=( const assignment & ) = default;


    void operator delete( void *ptr ) { (void) ( ptr ); }


    bool print( PrettyPrintStream *output ) {
        HIGHLIGHT
        bool split = output->remaining() < width( output );
        _variable->print( output );
        output->space();
        output->print( sym() );
        astNode::print( output );
        if ( split )
            output->inc_newline();
        else
            output->space();
        _e->print( output );
        if ( split )
            output->dec_indent();
        return split;
    }


    std::int32_t width( PrettyPrintStream *output ) {
        return _variable->width( output ) + 2 * output->width_of_space() + output->width_of_string( sym() );
    }
};


class messageNode : public astNode {

private:
    astNode                  *_receiver;
    SymbolOop                _selector;
    GrowableArray<astNode *> *_arguments;
    bool                     _is_prim;

public:
    messageNode( std::int32_t byteCodeIndex, scopeNode *scope, SymbolOop selector, bool is_prim = false ) :
        _selector{ selector },
        _arguments{ new GrowableArray<astNode *>( 10 ) },
        _receiver{ nullptr },
        _is_prim{ is_prim },
        astNode( byteCodeIndex, scope ) {
    }


    messageNode() = default;
    virtual ~messageNode() = default;
    messageNode( const messageNode & ) = default;
    messageNode &operator=( const messageNode & ) = default;


    void operator delete( void *ptr ) { (void) ( ptr ); }


    bool is_unary() {
        return _selector->is_unary();
    }


    bool is_binary() {
        return _selector->is_binary();
    }


    bool is_keyword() {
        return _selector->is_keyword();
    }


    bool should_wrap_receiver() {
        if ( receiver() and receiver()->is_message() ) {
            if ( ( (messageNode *) _receiver )->is_keyword() ) {
                return true;
            }

            if ( ( (messageNode *) _receiver )->is_binary() and is_binary() ) {
                return true;
            }

        }
        return false;
    }


    bool should_wrap_argument( astNode *arg ) {
        if ( arg->is_message() ) {
            if ( ( (messageNode *) arg )->is_keyword() ) {
                return true;
            }
            if ( ( (messageNode *) arg )->is_binary() and is_binary() ) {
                return true;
            }
        }
        return false;
    }


    bool print_receiver( PrettyPrintStream *output ) {
        bool wrap = should_wrap_receiver();
        if ( wrap )
            output->print( "(" );
        _receiver->print( output );
        if ( wrap )
            output->print( ")" );
        output->print( " " );
        return false;
    }


    bool real_print( PrettyPrintStream *output ) {
        HIGHLIGHT
        bool split;
        if ( _is_prim )
            output->print( "{{" );
        if ( valid_receiver() )
            print_receiver( output );
        if ( output->remaining() < width_send( output ) ) {
            output->inc_newline();
            astNode::print( output );
            split = print_selector_with_arguments( output, _selector, _arguments, true );
            if ( _is_prim )
                output->print( "}}" );
            output->dec_indent();
            return split;
        }
        split = print_selector_with_arguments( output, _selector, _arguments, false );
        if ( _is_prim )
            output->print( "}}" );
        return split;
    }


    bool print( PrettyPrintStream *output ) {
        if ( receiver() and receiver()->is_cascade() )
            return receiver()->print( output );
        return real_print( output );
    }


    std::int32_t width_receiver( PrettyPrintStream *output ) {
        std::int32_t w = receiver() ? receiver()->width( output ) + output->width_of_space() : 0;
        if ( should_wrap_receiver() ) {
            w += output->width_of_string( "(" ) + output->width_of_string( ")" );
        }
        return w;
    }


    std::int32_t width_send( PrettyPrintStream *output ) {
        std::int32_t arg = _selector->number_of_arguments();
        std::int32_t w   = output->width_of_string( _selector->as_string() ) + arg * output->width_of_space();

        for ( std::int32_t i = 0; i < _arguments->length(); i++ ) {
            w += _arguments->at( i )->width( output );
        }

        return w;
    }


    std::int32_t real_width( PrettyPrintStream *output ) {
        return ( receiver() ? width_receiver( output ) + output->width_of_space() : 0 ) + width_send( output );
    }


    std::int32_t width( PrettyPrintStream *output ) {
        if ( receiver() and receiver()->is_cascade() ) {
            return receiver()->width( output );
        }

        return real_width( output );
    }


    void add_param( astNode *p ) {
        _arguments->push( p );
    }


    void set_receiver( astNode *p );


    astNode *receiver() {
        return _receiver;
    }


    bool valid_receiver() {
        return receiver() ? not receiver()->is_cascade() : false;
    }


    bool is_message() {
        return true;
    }
};


class selfNode : public leafNode {

public:
    selfNode( std::int32_t byteCodeIndex, scopeNode *scope ) :
        leafNode( byteCodeIndex, scope ) {
    }


    selfNode() = default;
    virtual ~selfNode() = default;
    selfNode( const selfNode & ) = default;
    selfNode &operator=( const selfNode & ) = default;


    void operator delete( void *ptr ) { (void) ( ptr ); }


    const char *string() {
        return "self";
    }
};


class ignoreReceiver : public leafNode {

public:
    ignoreReceiver( std::int32_t byteCodeIndex, scopeNode *scope ) :
        leafNode( byteCodeIndex, scope ) {
    }


    ignoreReceiver() = default;
    virtual ~ignoreReceiver() = default;
    ignoreReceiver( const ignoreReceiver & ) = default;
    ignoreReceiver &operator=( const ignoreReceiver & ) = default;


    void operator delete( void *ptr ) { (void) ( ptr ); }


    const char *string() {
        return "";
    }
};


class superNode : public leafNode {

public:
    superNode( std::int32_t byteCodeIndex, scopeNode *scope ) :
        leafNode( byteCodeIndex, scope ) {
    }


    superNode() = default;
    virtual ~superNode() = default;
    superNode( const superNode & ) = default;
    superNode &operator=( const superNode & ) = default;


    void operator delete( void *ptr ) { (void) ( ptr ); }


    const char *string() {
        return "super";
    }
};


class literalNode : public leafNode {

private:
    const char *_str;

public:
    literalNode( std::int32_t byteCodeIndex, scopeNode *scope, const char *str ) :
        leafNode( byteCodeIndex, scope ),
        _str{ str } {
    }


    literalNode() = default;
    virtual ~literalNode() = default;
    literalNode( const literalNode & ) = default;
    literalNode &operator=( const literalNode & ) = default;


    void operator delete( void *ptr ) { (void) ( ptr ); }


    const char *string() {
        return _str;
    }
};


class symbolNode : public astNode {

private:
    SymbolOop  _value;
    bool       _isOuter;
    const char *_str;

public:
    symbolNode( std::int32_t byteCodeIndex, scopeNode *scope, SymbolOop value, bool is_outer = true ) :
        astNode( byteCodeIndex, scope ),
        _value{ value },
        _isOuter{ is_outer },
        _str{ nullptr } {
        _str = value->as_string();
    }


    symbolNode() = default;
    virtual ~symbolNode() = default;
    symbolNode( const symbolNode & ) = default;
    symbolNode &operator=( const symbolNode & ) = default;


    void operator delete( void *ptr ) { (void) ( ptr ); }


    bool print( PrettyPrintStream *output ) {
        astNode::print( output );
        if ( _isOuter ) {
            output->print( "#" );
        }
        output->print( _str );
        return false;
    }


    std::int32_t width( PrettyPrintStream *output ) {
        return ( _isOuter ? output->width_of_string( "#" ) : 0 ) + output->width_of_string( _str );
    }
};


class doubleByteArrayNode : public astNode {

private:
    DoubleByteArrayOop _value;
    char               *_str;

public:
    doubleByteArrayNode( std::int32_t byteCodeIndex, scopeNode *scope, DoubleByteArrayOop value ) :
        astNode( byteCodeIndex, scope ),
        _value{ value },
        _str{ nullptr } {
        _value = value;
        _str   = value->as_string();
    }


    doubleByteArrayNode() = default;
    virtual ~doubleByteArrayNode() = default;
    doubleByteArrayNode( const doubleByteArrayNode & ) = default;
    doubleByteArrayNode &operator=( const doubleByteArrayNode & ) = default;


    void operator delete( void *ptr ) { (void) ( ptr ); }


    bool print( PrettyPrintStream *output ) {
        astNode::print( output );
        output->print( "'" );
        output->print( _str );
        output->print( "'" );
        return false;
    }


    std::int32_t width( PrettyPrintStream *output ) {
        return 2 * output->width_of_string( "'" ) + output->width_of_string( _str );
    }
};


class byteArrayNode : public astNode {

private:
    ByteArrayOop _value;
    const char   *_str;

public:
    byteArrayNode( std::int32_t byteCodeIndex, scopeNode *scope, ByteArrayOop value ) :
        astNode( byteCodeIndex, scope ),
        _value{ value },
        _str{ nullptr } {
        _str = value->as_string();
    }


    bool print( PrettyPrintStream *output ) {
        HIGHLIGHT
        astNode::print( output );
        output->print( "'" );
        output->print( _str );
        output->print( "'" );
        return false;
    }


    byteArrayNode() = default;
    virtual ~byteArrayNode() = default;
    byteArrayNode( const byteArrayNode & ) = default;
    byteArrayNode &operator=( const byteArrayNode & ) = default;


    void operator delete( void *ptr ) { (void) ( ptr ); }


    std::int32_t width( PrettyPrintStream *output ) {
        return 2 * output->width_of_string( "'" ) + output->width_of_string( _str );
    }
};


class smiNode : public leafNode {

private:
    std::int32_t _value;
    char         *_str;

public:
    smiNode( std::int32_t byteCodeIndex, scopeNode *scope, std::int32_t value ) :
        _value{ value },
        _str{ new_resource_array<char>( 10 ) },
        leafNode( byteCodeIndex, scope ) {
        sprintf( _str, "%d", value );
    }


    smiNode() = default;
    virtual ~smiNode() = default;
    smiNode( const smiNode & ) = default;
    smiNode &operator=( const smiNode & ) = default;


    void operator delete( void *ptr ) { (void) ( ptr ); }


    const char *string() {
        return _str;
    }
};


class doubleNode : public leafNode {

private:
    double _value;
    char   *_str;

public:
    doubleNode( std::int32_t byteCodeIndex, scopeNode *scope, double value ) :
        _value{ value },
        _str{ new_resource_array<char>( 10 ) },
        leafNode( byteCodeIndex, scope ) {
        sprintf( _str, "%1.10gd", value );
    }


    doubleNode() = default;
    virtual ~doubleNode() = default;
    doubleNode( const doubleNode & ) = default;
    doubleNode &operator=( const doubleNode & ) = default;


    void operator delete( void *ptr ) { (void) ( ptr ); }


    const char *string() {
        return _str;
    }
};


class CharacterNode : public leafNode {

private:
    Oop  _value;
    char *_str;

public:

    CharacterNode( std::int32_t byteCodeIndex, scopeNode *scope, Oop value ) :
        _value{ value },
        _str{ new_resource_array<char>( 3 ) },
        leafNode( byteCodeIndex, scope ) {

        if ( value->is_mem() ) {
            Oop ch = MemOop( value )->instVarAt( 2 );
            if ( ch->is_smi() ) {
                sprintf( _str, "$%c", SMIOop( ch )->value() );
                return;
            }
        }

        //
        _str[0] = '$';
        _str[1] = '%';
        _str[2] = 'c';

    }


    CharacterNode() = default;
    virtual ~CharacterNode() = default;
    CharacterNode( const CharacterNode & ) = default;
    CharacterNode &operator=( const CharacterNode & ) = default;


    void operator delete( void *ptr ) { (void) ( ptr ); }


    const char *string() {
        return _str;
    }
};


class ObjectArrayNode : public astNode {

private:
    ObjectArrayOop           _value;
    bool                     _isOuter;
    GrowableArray<astNode *> *_elements;

public:
    ObjectArrayNode( std::int32_t byteCodeIndex, scopeNode *scope, ObjectArrayOop value, bool is_outer = true ) :
        astNode( byteCodeIndex, scope ),
        _value{},
        _isOuter{ false },
        _elements{ nullptr } {
        _value    = value;
        _isOuter  = is_outer;
        _elements = new GrowableArray<astNode *>( 10 );

        for ( std::int32_t i = 1; i <= value->length(); i++ ) {
            _elements->push( get_literal_node( value->obj_at( i ), byteCodeIndex, scope ) );
        }
    }


    ObjectArrayNode() = default;
    virtual ~ObjectArrayNode() = default;
    ObjectArrayNode( const ObjectArrayNode & ) = default;
    ObjectArrayNode &operator=( const ObjectArrayNode & ) = default;


    void operator delete( void *ptr ) { (void) ( ptr ); }


    bool print( PrettyPrintStream *output ) {
        HIGHLIGHT
        astNode::print( output );
        output->print( "#(" );
        for ( std::int32_t i = 0; i < _elements->length(); i++ ) {
            _elements->at( i )->print( output );
            if ( i < _elements->length() - 1 )
                output->space();
        }
        output->print( ")" );
        return false;
    }


    std::int32_t width( PrettyPrintStream *output ) {
        std::int32_t w = output->width_of_string( "#(" ) + output->width_of_string( ")" );

        for ( std::int32_t i = 0; i < _elements->length(); i++ ) {
            w += _elements->at( i )->width( output );
            if ( i < _elements->length() - 1 )
                w += output->width_of_space();
        }
        return 0;
    }
};


class dllNode : public astNode {

private:
    astNode                  *_dllName;
    astNode                  *_funcName;
    GrowableArray<astNode *> *_arguments;
    astNode                  *_proxy;

public:
    dllNode( std::int32_t byteCodeIndex, scopeNode *scope, SymbolOop dll_name, SymbolOop func_name ) :
        astNode( byteCodeIndex, scope ),
        _dllName{ nullptr },
        _funcName{ nullptr },
        _arguments{ nullptr },
        _proxy{ nullptr } {
        _dllName   = new symbolNode( byteCodeIndex, scope, dll_name, false );
        _funcName  = new symbolNode( byteCodeIndex, scope, func_name, false );
        _arguments = new GrowableArray<astNode *>( 10 );
        _proxy     = nullptr;
    }


    dllNode() = default;
    virtual ~dllNode() = default;
    dllNode( const dllNode & ) = default;
    dllNode &operator=( const dllNode & ) = default;


    void operator delete( void *ptr ) { (void) ( ptr ); }


    void add_param( astNode *param ) {
        _arguments->push( param );
    }


    void set_proxy( astNode *p ) {
        _proxy = p;
    }


    bool print( PrettyPrintStream *output ) {
        HIGHLIGHT
        astNode::print( output );
        output->print( "{{<" );
        _dllName->print( output );
        output->space();
        _proxy->print( output );
        output->space();
        _funcName->print( output );
        output->print( ">" );
        for ( std::int32_t i = _arguments->length() - 1; i >= 0; i-- ) {
            _arguments->at( i )->print( output );
            if ( i < _arguments->length() - 1 )
                output->print( ", " );
        }
        output->print( "}}" );
        return false;
    }


    std::int32_t width( PrettyPrintStream *output ) {
        return output->width_of_string( "<" ) + output->width_of_string( "Printing dll call" ) + output->width_of_string( ">" );
    }
};


class stackTempNode : public leafNode {

private:
    std::int32_t _offset;
    const char   *_str;

public:
    stackTempNode( std::int32_t byteCodeIndex, scopeNode *scope, std::int32_t offset ) :
        _offset{ offset },
        _str{},
        leafNode( byteCodeIndex, scope ) {
        _str = scope->stack_temp_string( this_byteCodeIndex(), offset );
    }


    stackTempNode() = default;
    virtual ~stackTempNode() = default;
    stackTempNode( const stackTempNode & ) = default;
    stackTempNode &operator=( const stackTempNode & ) = default;


    void operator delete( void *ptr ) { (void) ( ptr ); }


    const char *string() {
        return _str;
    }
};


class heapTempNode : public leafNode {

private:
    std::int32_t _offset;
    std::int32_t _contextLevel;
    const char   *_str;

public:
    heapTempNode( std::int32_t byteCodeIndex, scopeNode *scope, std::int32_t offset, std::int32_t context_level ) :
        leafNode( byteCodeIndex, scope ),
        _str{ nullptr },
        _offset{ offset },
        _contextLevel{ context_level } {

        _str = scope->heap_temp_string( this_byteCodeIndex(), offset, context_level );
    }


    heapTempNode() = default;
    virtual ~heapTempNode() = default;
    heapTempNode( const heapTempNode & ) = default;
    heapTempNode &operator=( const heapTempNode & ) = default;


    void operator delete( void *ptr ) { (void) ( ptr ); }


    const char *string() {
        return _str;
    }
};


class floatNode : public leafNode {

private:
    const char *_str;

public:
    floatNode( std::int32_t no, std::int32_t byteCodeIndex, scopeNode *scope ) :
        leafNode( byteCodeIndex, scope ),
        _str{ nullptr } {
        _str = scope->stack_temp_string( this_byteCodeIndex(), no );
    }


    floatNode() = default;
    virtual ~floatNode() = default;
    floatNode( const floatNode & ) = default;
    floatNode &operator=( const floatNode & ) = default;


    void operator delete( void *ptr ) { (void) ( ptr ); }


    const char *string() {
        return _str;
    }
};


class instVarNode : public leafNode {

private:
    Oop        _obj;
    const char *_str;

public:
    instVarNode( std::int32_t byteCodeIndex, scopeNode *scope, Oop obj ) :
        leafNode( byteCodeIndex, scope ),
        _obj{ obj },
        _str{ nullptr } {

        if ( obj->is_smi() ) {
            _str = scope->inst_var_string( SMIOop( obj )->value() );
        } else {
            _str = SymbolOop( obj )->as_string();
        }
    }


    instVarNode() = default;
    virtual ~instVarNode() = default;
    instVarNode( const instVarNode & ) = default;
    instVarNode &operator=( const instVarNode & ) = default;


    void operator delete( void *ptr ) { (void) ( ptr ); }


    const char *string() {
        return _str;
    }
};


class classVarNode : public leafNode {

private:
    Oop        _obj;
    const char *_str;

public:
    classVarNode( std::int32_t byteCodeIndex, scopeNode *scope, Oop obj ) :
        leafNode( byteCodeIndex, scope ),
        _obj{ nullptr },
        _str{ nullptr } {
        _obj = obj;
        if ( obj->is_association() ) {
            _str = AssociationOop( obj )->key()->as_string();
        } else {
            _str = SymbolOop( obj )->as_string();
        }
    }


    classVarNode() = default;
    virtual ~classVarNode() = default;
    classVarNode( const classVarNode & ) = default;
    classVarNode &operator=( const classVarNode & ) = default;


    void operator delete( void *ptr ) { (void) ( ptr ); }


    const char *string() {
        return _str;
    }
};


class primitiveResultNode : public leafNode {

public:
    bool is_primitive_result() {
        return true;
    }


    primitiveResultNode( std::int32_t byteCodeIndex, scopeNode *scope ) :
        leafNode( byteCodeIndex, scope ) {
    }


    const char *string() {
        return "`primitive result`";
    }
};


class assocNode : public leafNode {

private:
    AssociationOop _assoc;
    const char     *_str;

public:
    assocNode( std::int32_t byteCodeIndex, scopeNode *scope, AssociationOop assoc ) :
        leafNode( byteCodeIndex, scope ),
        _assoc{ nullptr },
        _str{ nullptr } {
        _assoc = assoc;
        _str   = assoc->key()->as_string();
    }


    assocNode() = default;
    virtual ~assocNode() = default;
    assocNode( const assocNode & ) = default;
    assocNode &operator=( const assocNode & ) = default;


    void operator delete( void *ptr ) { (void) ( ptr ); }


    const char *string() {
        return _str;
    }
};


class cascadeSendNode : public astNode {

private:
    GrowableArray<messageNode *> *_messages;
    astNode                      *_receiver;

public:
    cascadeSendNode( std::int32_t byteCodeIndex, scopeNode *scope, messageNode *first ) :
        astNode( byteCodeIndex, scope ),
        _messages{ nullptr },
        _receiver{ nullptr } {
        _messages = new GrowableArray<messageNode *>( 10 );
        _receiver = first->receiver();
        first->set_receiver( this );
    }


    cascadeSendNode() = default;
    virtual ~cascadeSendNode() = default;
    cascadeSendNode( const cascadeSendNode & ) = default;
    cascadeSendNode &operator=( const cascadeSendNode & ) = default;


    void operator delete( void *ptr ) { (void) ( ptr ); }


    bool is_cascade() {
        return true;
    }


    void add_message( messageNode *m ) {
        _messages->push( m );
    }


    bool print( PrettyPrintStream *output ) {
        _receiver->print( output );
        output->inc_newline();
        for ( std::int32_t i = 0; i < _messages->length(); i++ ) {
            if ( i == 0 )
                output->print( "  " );
            else
                output->print( "; " );
            _messages->at( i )->real_print( output );
            if ( i not_eq _messages->length() - 1 )
                output->newline();
        }
        output->dec_indent();
        return true;
    }


    std::int32_t width( PrettyPrintStream *output ) {
        static_cast<void>(output); // unused
        return 0;
    }
};


void messageNode::set_receiver( astNode *p ) {
    if ( p->is_cascade() ) {
        ( (cascadeSendNode *) p )->add_message( this );
    }
    _receiver = p;
}


static astNode *get_literal_node( Oop obj, std::int32_t byteCodeIndex, scopeNode *scope ) {
    if ( obj == trueObject )
        return new literalNode( byteCodeIndex, scope, "true" );
    if ( obj == falseObject )
        return new literalNode( byteCodeIndex, scope, "false" );
    if ( obj == nilObject )
        return new literalNode( byteCodeIndex, scope, "nil" );
    if ( obj->is_doubleByteArray() )
        return new doubleByteArrayNode( byteCodeIndex, scope, DoubleByteArrayOop( obj ) );
    if ( obj->is_symbol() )
        return new symbolNode( byteCodeIndex, scope, SymbolOop( obj ) );
    if ( obj->is_byteArray() )
        return new byteArrayNode( byteCodeIndex, scope, ByteArrayOop( obj ) );
    if ( obj->is_smi() )
        return new smiNode( byteCodeIndex, scope, SMIOop( obj )->value() );
    if ( obj->is_double() )
        return new doubleNode( byteCodeIndex, scope, DoubleOop( obj )->value() );
    if ( obj->is_objArray() )
        return new ObjectArrayNode( byteCodeIndex, scope, ObjectArrayOop( obj ) );

    return new CharacterNode( byteCodeIndex, scope, obj );
}


void PrettyPrintStream::print() {
    _console->print( "pretty-printer stream" );
}


void DefaultPrettyPrintStream::indent() {
    for ( std::int32_t i = 0; i < _indentation; i++ ) {
        space();
        space();
    }
}


void DefaultPrettyPrintStream::print( const char *str ) {
    for ( std::int32_t i = 0; str[ i ]; i++ )
        print_char( str[ i ] );
}


void DefaultPrettyPrintStream::print_char( char c ) {
    _console->print( "%c", c );
    _position += width_of_char( c );
}


std::int32_t DefaultPrettyPrintStream::width_of_string( const char *str ) {
    std::int32_t w = 0;

    for ( std::int32_t i = 0; str[ i ]; i++ ) {
        w += width_of_char( str[ i ] );
    }

    return w;
}


void DefaultPrettyPrintStream::space() {
    print_char( ' ' );
}


void DefaultPrettyPrintStream::newline() {
    print_char( '\n' );
    _position = 0;
    indent();
}


void DefaultPrettyPrintStream::begin_highlight() {
    set_highlight( true );
    print( "{" );
}


void DefaultPrettyPrintStream::end_highlight() {
    set_highlight( false );
    print( "}" );
}


void ByteArrayPrettyPrintStream::newline() {
    print_char( '\r' );
    _position = 0;
    indent();
}


// Forward declaration
astNode *generateForBlock( MethodOop method, KlassOop klass, std::int32_t byteCodeIndex, std::int32_t numOfArgs );

astNode *generate( scopeNode *scope );

class MethodPrettyPrinter : public MethodClosure {
private:
    GrowableArray<astNode *> *_stack; // expression stack
    scopeNode                *_scope; // debugging scope

    // the receiver is on the stack
    void normal_send( SymbolOop selector, bool is_prim = false );

    // the receiver is provided
    void special_send( astNode *receiver, SymbolOop selector, bool is_prim = false );

    astNode *generateInlineBlock( MethodInterval *interval, bool produces_result, astNode *push_elem = nullptr );


    void store( astNode *node ) {
        _push( new assignment( byteCodeIndex(), scope(), node, _pop() ) );
    }


public:


    MethodPrettyPrinter() = default;
    virtual ~MethodPrettyPrinter() = default;
    MethodPrettyPrinter( const MethodPrettyPrinter & ) = default;
    MethodPrettyPrinter &operator=( const MethodPrettyPrinter & ) = default;


    void operator delete( void *ptr ) { (void) ( ptr ); }


    void _push( astNode *node ) {
        _stack->push( node );
    }


    astNode *_pop() {
        return _stack->pop();
    }


    astNode *_top() {
        return _stack->top();
    }


    std::int32_t _size() const {
        return _stack->length();
    }


    scopeNode *scope() const {
        return _scope;
    }


    MethodPrettyPrinter( scopeNode *scope );

    // node call backs
    void if_node( IfNode *node );

    void cond_node( CondNode *node );

    void while_node( WhileNode *node );

    void primitive_call_node( PrimitiveCallNode *node );

    void dll_call_node( DLLCallNode *node );

    // complicated call backs
    void pop();

    void method_return( std::int32_t nofArgs );

    void nonlocal_return( std::int32_t nofArgs );


    void allocate_closure( AllocationType type, std::int32_t nofArgs, MethodOop meth ) {
        if ( type == AllocationType::tos_as_scope )
            _pop();
        scopeNode *methodScope = _scope->scopeFor( meth );
        if ( methodScope )
            _push( generate( methodScope ) );
        else
            _push( generateForBlock( meth, scope()->get_klass(), -1, nofArgs ) );
    }


    void allocate_context( std::int32_t nofTemps, bool forMethod ) {
        static_cast<void>(nofTemps); // unused
        static_cast<void>(forMethod); // unused

        scope()->context_allocated();
    }


    // sends
    void normal_send( InterpretedInlineCache *ic ) {
        normal_send( ic->selector() );
    }


    void self_send( InterpretedInlineCache *ic ) {
        special_send( new selfNode( byteCodeIndex(), scope() ), ic->selector() );
    }


    void super_send( InterpretedInlineCache *ic ) {
        special_send( new superNode( byteCodeIndex(), scope() ), ic->selector() );
    }


    void double_equal() {
        normal_send( vmSymbols::double_equal() );
    }


    void double_not_equal() {
        normal_send( vmSymbols::double_tilde() );
    }


    // simple call backs
    void push_self() {
        _push( new selfNode( byteCodeIndex(), scope() ) );
    }


    void push_tos() {
        _push( _top() );
    }


    void push_literal( Oop obj ) {
        _push( get_literal_node( obj, byteCodeIndex(), scope() ) );
    }


    void push_argument( std::int32_t no ) {
        _push( new paramNode( byteCodeIndex(), scope(), no ) );
    }


    void push_temporary( std::int32_t no ) {
        _push( new stackTempNode( byteCodeIndex(), scope(), no ) );
    }


    void push_temporary( std::int32_t no, std::int32_t context ) {
        _push( new heapTempNode( byteCodeIndex(), scope(), no, context ) );
    }


    void push_instVar( std::int32_t offset ) {
        _push( new instVarNode( byteCodeIndex(), scope(), smiOopFromValue( offset ) ) );
    }


    void push_instVar_name( SymbolOop name ) {
        _push( new instVarNode( byteCodeIndex(), scope(), name ) );
    }


    void push_classVar( AssociationOop assoc ) {
        _push( new classVarNode( byteCodeIndex(), scope(), assoc ) );
    }


    void push_classVar_name( SymbolOop name ) {
        _push( new classVarNode( byteCodeIndex(), scope(), name ) );
    }


    void push_global( AssociationOop obj ) {
        _push( new assocNode( byteCodeIndex(), scope(), obj ) );
    }


    void store_temporary( std::int32_t no ) {
        store( new stackTempNode( byteCodeIndex(), scope(), no ) );
    }


    void store_temporary( std::int32_t no, std::int32_t context ) {
        store( new heapTempNode( byteCodeIndex(), scope(), no, context ) );
    }


    void store_instVar( std::int32_t offset ) {
        store( new instVarNode( byteCodeIndex(), scope(), smiOopFromValue( offset ) ) );
    }


    void store_instVar_name( SymbolOop name ) {
        store( new instVarNode( byteCodeIndex(), scope(), name ) );
    }


    void store_classVar( AssociationOop assoc ) {
        store( new classVarNode( byteCodeIndex(), scope(), assoc ) );
    }


    void store_classVar_name( SymbolOop name ) {
        store( new classVarNode( byteCodeIndex(), scope(), name ) );
    }


    void store_global( AssociationOop obj ) {
        store( new assocNode( byteCodeIndex(), scope(), obj ) );
    }


    // call backs to ignore
    void allocate_temporaries( std::int32_t nofTemps ) {
        static_cast<void>(nofTemps); // unused
    }


    void set_self_via_context() {
    }


    void copy_self_into_context() {
    }


    void copy_argument_into_context( std::int32_t argNo, std::int32_t no ) {
        static_cast<void>(argNo); // unused
        static_cast<void>(no); // unused
    }


    void zap_scope() {
    }


    void predict_primitive_call( PrimitiveDescriptor *pdesc, std::int32_t failure_start ) {
        static_cast<void>(pdesc); // unused
        static_cast<void>(failure_start); // unused
    }


    void float_allocate( std::int32_t nofFloatTemps, std::int32_t nofFloatExprs ) {
        static_cast<void>(nofFloatTemps); // unused
        static_cast<void>(nofFloatExprs); // unused
    }


    void float_floatify( Floats::Function f, std::int32_t tof ) {
        static_cast<void>(tof); // unused
        normal_send( Floats::selector_for( f ) );
        pop();
    }


    void float_move( std::int32_t tof, std::int32_t from ) {
        _push( new floatNode( from, byteCodeIndex(), scope() ) );
        store( new floatNode( tof, byteCodeIndex(), scope() ) );
        pop();
    }


    void float_set( std::int32_t tof, DoubleOop value ) {
        static_cast<void>(tof); // unused
        static_cast<void>(value); // unused
    }


    void float_nullary( Floats::Function f, std::int32_t tof ) {
        static_cast<void>(f); // unused
        static_cast<void>(tof); // unused
    }


    void float_unary( Floats::Function f, std::int32_t tof ) {
        _push( new floatNode( tof, byteCodeIndex(), scope() ) );
        normal_send( Floats::selector_for( f ) );
        pop();
    }


    void float_binary( Floats::Function f, std::int32_t tof ) {
        _push( new floatNode( tof - 1, byteCodeIndex(), scope() ) );
        _push( new floatNode( tof, byteCodeIndex(), scope() ) );
        normal_send( Floats::selector_for( f ) );
        pop();
    }


    void float_unaryToOop( Floats::Function f, std::int32_t tof ) {
        _push( new floatNode( tof, byteCodeIndex(), scope() ) );
        normal_send( Floats::selector_for( f ) );
    }


    void float_binaryToOop( Floats::Function f, std::int32_t tof ) {
        _push( new floatNode( tof, byteCodeIndex(), scope() ) );
        _push( new floatNode( tof - 1, byteCodeIndex(), scope() ) );
        normal_send( Floats::selector_for( f ) );
    }
};


MethodPrettyPrinter::MethodPrettyPrinter( scopeNode *scope ) :
    _stack{ nullptr },
    _scope{ scope } {
    _stack = new GrowableArray<astNode *>( 10 );

    if ( scope->method()->is_blockMethod() ) {
        _push( new blockNode( 1, scope, scope->method()->number_of_arguments() ) );
    } else {
        _push( new methodNode( 1, scope ) );
    }

}


void MethodPrettyPrinter::normal_send( SymbolOop selector, bool is_prim ) {
    std::int32_t nargs = selector->number_of_arguments();
    messageNode  *msg  = new messageNode( byteCodeIndex(), scope(), selector, is_prim );

    GrowableArray<astNode *> *arguments = new GrowableArray<astNode *>( 10 );
    for ( std::int32_t       i          = 0; i < nargs; i++ )
        arguments->push( _pop() );

    for ( std::int32_t i = 0; i < nargs; i++ )
        msg->add_param( arguments->at( i ) );

    msg->set_receiver( _pop() );

    _push( msg );
}


void MethodPrettyPrinter::special_send( astNode *receiver, SymbolOop selector, bool is_prim ) {
    std::int32_t nargs = selector->number_of_arguments();
    messageNode  *msg  = new messageNode( byteCodeIndex(), scope(), selector, is_prim );

    GrowableArray<astNode *> *arguments = new GrowableArray<astNode *>( 10 );
    for ( std::int32_t       i          = 0; i < nargs; i++ )
        arguments->push( _pop() );

    for ( std::int32_t i = 0; i < nargs; i++ )
        msg->add_param( arguments->at( i ) );

    msg->set_receiver( receiver );

    _push( msg );
}


void MethodPrettyPrinter::pop() {
    // This has to be on a statement boundary.
    astNode *expr = _pop();
    astNode *code = _top();

    if ( code->is_code() ) {
        code->add( new statement( byteCodeIndex(), scope(), expr, false ) );
        return;
    }

    if ( expr->is_message() ) {
        // This must be a cascade send
        //    - exp
        //    - receiver for the cascade send

        // Replace the receiver with a cascade code
        if ( not _top()->is_cascade() ) {
            _pop();
            _push( new cascadeSendNode( byteCodeIndex(), scope(), (messageNode *) expr ) );
        }
    } else {
        _pop();
        _push( expr );
    }
}


void MethodPrettyPrinter::method_return( std::int32_t nofArgs ) {
    static_cast<void>(nofArgs); // unused

    // This has to be on a statement boundary.
    if ( _size() == 1 ) {
        // Make the last statement in the method a nlr.
        astNode *code = _top();
        ( (codeNode *) code )->last_statement()->set_return();
    } else {
        astNode *expr = _pop();
        astNode *code = _top();
        if ( code->is_code() ) {
            code->add( new statement( byteCodeIndex(), scope(), expr, not scope()->is_block_method() ) );
        } else {
            st_fatal1( "Stack size = %d", _size() );
        }
    }
}


void MethodPrettyPrinter::nonlocal_return( std::int32_t nofArgs ) {
    static_cast<void>(nofArgs); // unused

    // This has to be on a statement boundary.
    astNode *expr = _pop();
    astNode *code = _top();
    if ( code->is_code() ) {
        ( (codeNode *) code )->add( new statement( byteCodeIndex(), scope(), expr, true ) );
    } else {
        st_fatal1( "Stack size = %d", _size() );
    }
}


class StackChecker {

public:
    MethodPrettyPrinter *_methodPrettyPrinter;
    std::int32_t        _size;
    const char          *_name;
    std::int32_t        _offset;

    StackChecker() = default;


    StackChecker( const char *name, MethodPrettyPrinter *pp, std::int32_t offset = 0 ) :
        _methodPrettyPrinter{ pp },
        _size{ pp->_size() },
        _name{ name },
        _offset{ offset } {
    }


    StackChecker( const StackChecker & ) = default;
    StackChecker &operator=( const StackChecker & ) = default;


    virtual ~StackChecker() {
        if ( _methodPrettyPrinter->_size() not_eq _size + _offset ) {
            SPDLOG_INFO( "StackTracer found misaligned stack" );
            SPDLOG_INFO( "Expecting {} but found {}", _size + _offset, _methodPrettyPrinter->_size() );
            st_fatal( "aborting" );
        }
    }


    void operator delete( void *ptr ) { (void) ( ptr ); }

};


astNode *MethodPrettyPrinter::generateInlineBlock( MethodInterval *interval, bool produces_result, astNode *push_elem ) {
    inlinedBlockNode *block = new inlinedBlockNode( byteCodeIndex(), scope() );
    _push( block );
    if ( push_elem )
        _push( push_elem );
    MethodIterator mi( interval, this );
    if ( produces_result )
        pop(); // discard the result value
    return _pop();
}


void MethodPrettyPrinter::if_node( IfNode *node ) {
    _push( generateInlineBlock( node->then_code(), node->produces_result() ) );
    if ( node->else_code() ) {
        _push( generateInlineBlock( node->else_code(), node->produces_result() ) );
    }
    normal_send( node->selector() );
    if ( not node->produces_result() )
        pop();
}


void MethodPrettyPrinter::cond_node( CondNode *node ) {
    _push( generateInlineBlock( node->expr_code(), true ) );
    normal_send( node->selector() );
}


void MethodPrettyPrinter::while_node( WhileNode *node ) {
    _push( generateInlineBlock( node->expr_code(), true ) );
    if ( node->body_code() )
        _push( generateInlineBlock( node->body_code(), false ) );
    normal_send( node->selector() );
    pop();
}


void MethodPrettyPrinter::primitive_call_node( PrimitiveCallNode *node ) {
    if ( node->failure_code() )
        _push( generateInlineBlock( node->failure_code(), true, new primitiveResultNode( byteCodeIndex(), scope() ) ) );
    if ( node->has_receiver() )
        normal_send( node->name(), true );
    else
        special_send( new ignoreReceiver( byteCodeIndex(), scope() ), node->name(), true );
}


void MethodPrettyPrinter::dll_call_node( DLLCallNode *node ) {
    dllNode *msg = new dllNode( byteCodeIndex(), scope(), node->dll_name(), node->function_name() );

    std::int32_t nargs = node->nofArgs();

    GrowableArray<astNode *> *arguments = new GrowableArray<astNode *>( 10 );
    for ( std::int32_t       i          = 0; i < nargs; i++ )
        arguments->push( _pop() );

    for ( std::int32_t i = 0; i < nargs; i++ )
        msg->add_param( arguments->at( i ) );

    msg->set_proxy( _pop() );

    _push( msg );
}


astNode *generate( scopeNode *scope ) {
    MethodPrettyPrinter blk( scope );
    MethodIterator      mi( scope->method(), &blk );
    st_assert( blk._size() == 1, "just checking" );
    return blk._top();
}


astNode *generateForActivation( DeltaVirtualFrame *fr, std::int32_t index ) {
    scopeNode *scope      = new scopeNode( fr, index );
    scopeNode *toGenerate = scope->homeScope();
    return generate( toGenerate );
}


astNode *generateForMethod( MethodOop method, KlassOop klass, std::int32_t byteCodeIndex ) {
    return generate( new scopeNode( method, klass, byteCodeIndex ) );
}


astNode *generateForBlock( MethodOop method, KlassOop klass, std::int32_t byteCodeIndex, std::int32_t nofArgs ) {
    static_cast<void>(nofArgs); // unused

    return generate( new scopeNode( method, klass, byteCodeIndex ) );
}


void PrettyPrinter::print( MethodOop method, KlassOop klass, std::int32_t byteCodeIndex, PrettyPrintStream *output ) {
    ResourceMark resourceMark;
    if ( not output )
        output = new DefaultPrettyPrintStream;
    astNode *root = generateForMethod( method, klass, byteCodeIndex );
    root->top_level_print( output );
    output->newline();
}


void PrettyPrinter::print( std::int32_t index, DeltaVirtualFrame *fr, PrettyPrintStream *output ) {
    ResourceMark resourceMark;
    if ( not output )
        output = new DefaultPrettyPrintStream;

    if ( ActivationShowCode ) {
        astNode *root = generateForActivation( fr, index );
        root->top_level_print( output );
    } else {
        scopeNode *scope = new scopeNode( fr, index );
        scope->print_header( output );
    }
    output->newline();
}


void PrettyPrinter::print_body( DeltaVirtualFrame *fr, PrettyPrintStream *output ) {
    ResourceMark resourceMark;
    if ( not output ) {
        output = new DefaultPrettyPrintStream;
    }

    astNode *root = generateForActivation( fr, 0 );
    root->print( output );
}


ByteArrayPrettyPrintStream::ByteArrayPrettyPrintStream() :
    DefaultPrettyPrintStream(), _buffer{ nullptr } {
    _buffer = new GrowableArray<std::int32_t>( 200 );
}


void ByteArrayPrettyPrintStream::print_char( char c ) {
    _buffer->append( (std::int32_t) c );
    _position += width_of_char( c );
}


ByteArrayOop ByteArrayPrettyPrintStream::asByteArray() {
    std::int32_t       l = _buffer->length();
    ByteArrayOop       a = oopFactory::new_byteArray( l );
    for ( std::int32_t i = 0; i < l; i++ ) {
        a->byte_at_put( i + 1, (char) _buffer->at( i ) );
    }
    return a;
}


ByteArrayOop PrettyPrinter::print_in_byteArray( MethodOop method, KlassOop klass, std::int32_t byteCodeIndex ) {
    ResourceMark               rm;
    ByteArrayPrettyPrintStream *stream = new ByteArrayPrettyPrintStream();
    PrettyPrinter::print( method, klass, byteCodeIndex, stream );
    return stream->asByteArray();
}


bool should_wrap( std::int32_t type, astNode *arg ) {
    if ( not arg->is_message() )
        return false;
    messageNode *msg = (messageNode *) arg;
    switch ( type ) {
        case 0:
            return false;
        case 1:
            return not msg->is_unary();
        case 2:
            return msg->is_keyword();
    }
    return false;
}


void PrintParams::parameter( ByteArrayOop name, std::int32_t index ) {
    static_cast<void>(name); // unused
    _elements->push( _scope->parameter_at( index, true ) );
}


void PrintTemps::stack_temp( ByteArrayOop name, std::int32_t offset ) {
    static_cast<void>(name); // unused
    _elements->push( _scope->stack_temp_at( offset ) );
}


void PrintTemps::heap_temp( ByteArrayOop name, std::int32_t offset ) {
    static_cast<void>(name); // unused
    _elements->push( _scope->heap_temp_at( offset ) );
}
