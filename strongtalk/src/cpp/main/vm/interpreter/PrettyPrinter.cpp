
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
#include "vm/interpreter/InterpretedInlineCache.hpp"
#include "vm/memory/vmSymbols.hpp"
#include "vm/runtime/ResourceMark.hpp"
#include "vm/memory/oopFactory.hpp"


// ToDo list for pretty printer
// - convert the stream stuff to ostream.
// - improve the namedesc printout for temps etc.

class statement;

class codeNode;

class scopeNode;

class astNode : public ResourceObject {

protected:
    int _byteCodeIndex;
    scopeNode *_scopeNode;

public:
    astNode( int byteCodeIndex, scopeNode *scope ) {
        this->_byteCodeIndex = byteCodeIndex;
        this->_scopeNode     = scope;
    }


    int this_byteCodeIndex() {
        return _byteCodeIndex;
    }


    scopeNode *this_scope() {
        return _scopeNode;
    }


    virtual bool_t is_message() {
        return false;
    }


    virtual bool_t is_statement() {
        return false;
    }


    virtual bool_t is_code() {
        return false;
    }


    virtual bool_t is_cascade() {
        return false;
    }


    virtual void add( astNode *statement ) {
        st_fatal( "subclass should implement add" );
    }


    void top_level_print( prettyPrintStream *output );

    virtual bool_t print( prettyPrintStream *output );


    virtual int width( prettyPrintStream *output ) {
        return output->infinity();
    }


    virtual SymbolOop selector() {
        return nullptr;
    }


    virtual bool_t should_wrap_argument( astNode *argument ) {
        return false;
    };


    virtual astNode *argument_at( std::size_t i ) {
        return nullptr;
    }
};


class PrintWrapper {
private:
    astNode *_astNode;
    bool_t _hit;
    prettyPrintStream *_output;
public:
    PrintWrapper( astNode *astNode, prettyPrintStream *output );

    ~PrintWrapper();
};


#define HIGHLIGHT PrintWrapper pw(this, output);

bool_t should_wrap( int type, astNode *arg );


static bool_t print_selector_with_arguments( prettyPrintStream *output, SymbolOop selector, GrowableArray<astNode *> *arguments, bool_t split ) {

    if ( selector->is_binary() ) {
        // binary send
        output->print( selector->as_string() );
        output->space();
        astNode *arg = arguments->at( 0 );
        bool_t wrap = should_wrap( 1, arg );
        if ( wrap )
            output->print( "(" );
        bool_t result = arg->print( output );
        if ( wrap )
            output->print( ")" );
        return result;
    }

    int arg = arguments->length();

    if ( arg == 0 ) {
        output->print( selector->as_string() );
        return false;
    }

    for ( std::size_t i = 1; i <= selector->length(); i++ ) {
        int c = selector->byte_at( i );
        output->print_char( c );
        if ( c == ':' ) {
            output->space();
            astNode *a = arguments->at( --arg );
            bool_t wrap = should_wrap( 2, a );
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


static astNode *get_literal_node( Oop obj, int byteCodeIndex, scopeNode *scope );

class PrintTemps : public TempDecoder {

public:
    GrowableArray<astNode *> *_elements;
    scopeNode *_scope;


    void decode( MethodOop method, scopeNode *scope ) {
        this->_scope = scope;
        _elements = new GrowableArray<astNode *>( 10 );
        TempDecoder::decode( method );
    }


    void stack_temp( ByteArrayOop name, int offset );

    void heap_temp( ByteArrayOop name, int offset );
};


class PrintParams : public TempDecoder {

private:
    scopeNode *_scope;

public:
    GrowableArray<astNode *> *_elements;


    void decode( MethodOop method, scopeNode *scope ) {
        this->_scope = scope;
        _elements = new GrowableArray<astNode *>( 10 );
        TempDecoder::decode( method );
    }


    void parameter( ByteArrayOop name, int index );
};


class leafNode : public astNode {

public:
    leafNode( int byteCodeIndex, scopeNode *scope ) :
            astNode( byteCodeIndex, scope ) {
    }


    bool_t print( prettyPrintStream *output ) {
        HIGHLIGHT
        astNode::print( output );
        output->print( string() );
        return false;
    }


    int width( prettyPrintStream *output ) {
        return output->width_of_string( string() );
    }


    virtual const char *string() = 0;
};


class paramNode : public leafNode {

private:
    int _no;
    const char *_str;

public:
    paramNode( int byteCodeIndex, scopeNode *scope, int no );


    const char *string() {
        return _str;
    }


    bool_t is_param() {
        return true;
    }
};


class nameValueNode : public astNode {

private:
    const char *_name;
    const char *_value;

public:
    nameValueNode( const char *name, char *value ) :
            astNode( 0, 0 ) {
        this->_name  = name;
        this->_value = value;
    }


    bool_t print( prettyPrintStream *output ) {
        output->print( _name );
        output->print( " \"" );
        output->print( _value );
        output->print( "\"" );
        return false;
    }


    int width( prettyPrintStream *output ) {
        return output->width_of_string( _name ) + 3 + output->width_of_string( _value );
    }
};


class nameNode : public leafNode {

private:
    const char *_str;

public:
    nameNode( const char *str ) :
            leafNode( 0, 0 ) {
        this->_str = str;
    }


    const char *string() {
        return _str;
    }
};


class listNode : public astNode {

protected:
    GrowableArray<astNode *> *_elements;
    const char *_beginSym;
    const char *_endSym;

public:
    listNode( const char *begin_sym, const char *end_sym ) :
            astNode( 0, 0 ) {
        this->_beginSym = begin_sym;
        this->_endSym   = end_sym;
        _elements = new GrowableArray<astNode *>( 10 );
    }


    void add( astNode *element ) {
        _elements->push( element );
    }


    bool_t print( prettyPrintStream *output ) {
        if ( _beginSym ) {
            output->print( _beginSym );
            output->space();
        }

        for ( std::size_t i = 0; i < _elements->length(); i++ ) {
            _elements->at( i )->print( output );
            output->space();
        }

        if ( _endSym )
            output->print( _endSym );
        return false;
    }


    int width( prettyPrintStream *output ) {
        int w = 0;
        if ( _beginSym )
            w += output->width_of_string( _beginSym ) + output->width_of_space();

        for ( std::size_t i = 0; i < _elements->length(); i++ ) {
            w += _elements->at( i )->width( output ) + output->width_of_space();
        }

        if ( _endSym )
            w += output->width_of_string( _endSym );

        return w;
    }

};


class scopeNode : public astNode {

protected:
    MethodOop _methodOop;
    KlassOop  _klassOop;
    int       _in;
    int       _hotByteCodeIndex;
    int       _frameIndex;
    DeltaVirtualFrame *_deltaVirtualFrame;
    ScopeDescriptor   *_scopeDescriptor;
    scopeNode         *_parentScope;
    scopeNode         *_innerScope;

public:
    scopeNode( DeltaVirtualFrame *fr, int index, scopeNode *scope = nullptr ) :
            astNode( 0, nullptr ) {

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


    scopeNode( MethodOop method, KlassOop klass, int hot_byteCodeIndex, scopeNode *scope = nullptr ) :
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


    int hot_byteCodeIndex() {
        return _hotByteCodeIndex;
    }


    KlassOop get_klass() {
        return _klassOop;
    }


    bool_t is_block_method() {
        return method()->is_blockMethod();
    }


    void context_allocated() {
        st_assert( is_context_allocated(), "just checking" );
    }


    bool_t is_context_allocated() {
        return method()->allocatesInterpretedContext();
    }


    scopeNode *scopeFor( MethodOop method ) {
        return _innerScope and _innerScope->isFor( method ) ? _innerScope : nullptr;
    }


    bool_t isFor( MethodOop method ) {
        return _methodOop == method;
    }


    scopeNode *homeScope() {
        return _parentScope ? _parentScope->homeScope() : this;
    }


    const char *param_string( int index, bool_t in_block = false ) {
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


    const char *stack_temp_string( int byteCodeIndex, int no ) {
        ByteArrayOop res = find_stack_temp( method(), byteCodeIndex, no );
        return res ? res->as_string() : create_name( "t", no );
    }


    const char *stack_float_temp_string( int byteCodeIndex, int fno ) {
        ByteArrayOop res = find_stack_float_temp( method(), byteCodeIndex, fno );
        return res ? res->as_string() : create_name( "f", fno );
    }


    const char *create_name( const char *prefix, int no ) {
        char *str = new_resource_array<char>( 7 );
        sprintf( str, "%s_%d", prefix, no );
        return str;
    }


    const char *heap_temp_string( int byteCodeIndex, int no, int context_level ) {
        if ( is_block_method() ) {
            int level = context_level;
            if ( is_context_allocated() ) {
                level--;
                if ( context_level == 0 ) {
                    ByteArrayOop res = find_heap_temp( method(), byteCodeIndex, no );
                    return res ? res->as_string() : create_name( "ht", no );
                }
            }
            scopeNode *n = new scopeNode( method()->parent(), _klassOop, 0 );
            return n->heap_temp_string( byteCodeIndex, no, level );
        } else {
            ByteArrayOop res = find_heap_temp( method(), byteCodeIndex, no );
            return res ? res->as_string() : create_name( "ht", no );
        }
    }


    const char *inst_var_string( int offset ) {
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
        listNode *l = new listNode( "|", "|" );
        for ( std::size_t i = 0; i < p._elements->length(); i++ )
            l->add( p._elements->at( i ) );
        return l;
    }


    astNode *params() {
        PrintParams p;
        p.decode( method(), this );
        if ( p._elements->length() == 0 )
            return nullptr;

        listNode *l = new listNode( nullptr, "|" );
        for ( std::size_t i = 0; i < p._elements->length(); i++ ) {
            l->add( p._elements->at( i ) );
        }
        return l;
    }


    astNode *heap_temp_at( int no ) {
        const char *name = heap_temp_string( 0, no, 0 );
        if ( not fr() )
            return new nameNode( name );
        char *value = fr()->context_temp_at( no )->print_value_string();
        return new nameValueNode( name, value );
    }


    astNode *stack_temp_at( int no ) {
        ByteArrayOop res = find_stack_temp( method(), 0, no );
        const char *name = res ? res->as_string() : create_name( "t", no );
        if ( not fr() )
            return new nameNode( name );
        char *value = fr()->temp_at( no )->print_value_string();
        return new nameValueNode( name, value );
    }


    astNode *parameter_at( int index, bool_t in_block = false ) {
        const char *name = param_string( index, in_block );
        if ( not fr() )
            return new nameNode( name );
        char *value = fr()->argument_at( index )->print_value_string();
        return new nameValueNode( name, value );
    }


    void print_frame_header( prettyPrintStream *output ) {
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
                for ( std::size_t index = 0; index < stack->length(); index++ ) {
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


    void print_header( prettyPrintStream *output ) {
        if ( fr() ) {
            print_frame_header( output );
        } else {
            print_method_header( output );
        }
    }


    void print_method_header( prettyPrintStream *output ) {
        GrowableArray<astNode *> *arguments = new GrowableArray<astNode *>( 10 );
        for ( int                i          = method()->number_of_arguments() - 1; i >= 0; i-- ) {
            arguments->push( parameter_at( i ) );
        }
        print_selector_with_arguments( output, method()->selector(), arguments, false );
    }
};


paramNode::paramNode( int byteCodeIndex, scopeNode *scope, int no ) :
        leafNode( byteCodeIndex, scope ) {
    this->_no  = no;
    this->_str = scope->param_string( no );
}


PrintWrapper::PrintWrapper( astNode *astNode, prettyPrintStream *output ) {
    this->_astNode = astNode;
    this->_output  = output;
    this->_hit     = false;
    if ( not astNode->this_scope() )
        return;
    if ( astNode->this_byteCodeIndex() == astNode->this_scope()->hot_byteCodeIndex() and not output->in_highlight() ) {
        output->begin_highlight();
        _hit = true;
    }
}


PrintWrapper::~PrintWrapper() {
    if ( _hit )
        _output->end_highlight();
}


bool_t astNode::print( prettyPrintStream *output ) {
    if ( ActivationShowNameDescs ) {
        if ( _scopeNode and _scopeNode->sd() ) {
            NameDescriptor *nd = _scopeNode->sd()->exprStackElem( _byteCodeIndex );
            if ( nd )
                nd->print();
        }
    }
    return false;
}


void astNode::top_level_print( prettyPrintStream *output ) {
    _scopeNode->print_header( output );
    print( output );
}


class codeNode : public astNode {

protected:
    GrowableArray<astNode *> *_statements;

public:
    codeNode( int byteCodeIndex, scopeNode *scope ) :
            astNode( byteCodeIndex, scope ) {
        _statements = new GrowableArray<astNode *>( 10 );
    }


    void add( astNode *statement ) {
        _statements->push( statement );
    }


    bool_t print( prettyPrintStream *output );


    bool_t is_code() {
        return true;
    }


    statement *last_statement() {
        if ( _statements->length() > 0 ) {
            st_assert( _statements->at( _statements->length() - 1 )->is_statement(), "must be statement" );
            return (statement *) _statements->at( _statements->length() - 1 );
        }
        return nullptr;
    }


    int width( prettyPrintStream *output ) {
        int len = _statements->length();
        if ( len == 0 )
            return 0;
        if ( len == 1 )
            return _statements->at( 0 )->width( output );
        return output->infinity();
    }
};


class inlinedBlockNode : public codeNode {

public:
    inlinedBlockNode( int byteCodeIndex, scopeNode *scope ) :
            codeNode( byteCodeIndex, scope ) {
    }


    void add( astNode *statement ) {
        _statements->push( statement );
    }


    bool_t print( prettyPrintStream *output ) {
        bool_t split = output->remaining() < width( output );
        output->print( "[" );
        if ( split )
            output->inc_newline();
        codeNode::print( output );
        if ( split )
            output->dec_indent();
        output->print( "]" );
        return split;
    }


    int width( prettyPrintStream *output ) {
        return output->width_of_string( "[" ) + codeNode::width( output ) + output->width_of_string( "]" );
    }
};


class statement : public astNode {

private:
    bool_t _hasReturn;
    astNode *_stat;


    const char *hat() {
        return "^";
    }


public:

    bool_t is_statement() {
        return true;
    }


    statement( int byteCodeIndex, scopeNode *scope, astNode *stat, bool_t has_return ) :
            astNode( byteCodeIndex, scope ) {
        this->_stat      = stat;
        this->_hasReturn = has_return;
    }


    void set_return() {
        _hasReturn = true;
    }


    bool_t has_hat() {
        return _hasReturn;
    }


    bool_t print( prettyPrintStream *output ) {
        HIGHLIGHT
        if ( _hasReturn )
            output->print( hat() );
        return _stat->print( output );
    }


    int width( prettyPrintStream *output ) {
        return ( _hasReturn ? output->width_of_string( hat() ) : 0 ) + _stat->width( output );
    }
};


bool_t codeNode::print( prettyPrintStream *output ) {

    bool_t first = true;

    for ( std::size_t i = 0; i < _statements->length(); i++ ) {
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
    methodNode( int byteCodeIndex, scopeNode *scope ) :
            codeNode( byteCodeIndex, scope ) {
    }


    bool_t print( prettyPrintStream *output ) {
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
    int _numOfArgs;

public:
    blockNode( int byteCodeIndex, scopeNode *scope, int numOfArgs ) :
            codeNode( byteCodeIndex, scope ) {
        this->_numOfArgs = numOfArgs;
    }


    bool_t print( prettyPrintStream *output ) {
        HIGHLIGHT
        bool_t split = false;
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


    int width( prettyPrintStream *output ) {
        int w = output->width_of_string( "[" ) + output->width_of_string( "]" );
        astNode *p = _scopeNode->params();
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
    assignment( int byteCodeIndex, scopeNode *scope, astNode *variable, astNode *e ) :
            astNode( byteCodeIndex, scope ) {
        this->_variable = variable;
        this->_e        = e;
    }


    bool_t print( prettyPrintStream *output ) {
        HIGHLIGHT
        bool_t split = output->remaining() < width( output );
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


    int width( prettyPrintStream *output ) {
        return _variable->width( output ) + 2 * output->width_of_space() + output->width_of_string( sym() );
    }
};


class messageNode : public astNode {

private:
    astNode *_receiver;
    SymbolOop                _selector;
    GrowableArray<astNode *> *_arguments;
    bool_t                   _is_prim;

public:
    messageNode( int byteCodeIndex, scopeNode *scope, SymbolOop selector, bool_t is_prim = false ) :
            astNode( byteCodeIndex, scope ) {
        this->_selector = selector;
        _arguments = new GrowableArray<astNode *>( 10 );
        _receiver  = nullptr;
        _is_prim   = is_prim;
    }


    bool_t is_unary() {
        return _selector->is_unary();
    }


    bool_t is_binary() {
        return _selector->is_binary();
    }


    bool_t is_keyword() {
        return _selector->is_keyword();
    }


    bool_t should_wrap_receiver() {
        if ( receiver() and receiver()->is_message() ) {
            if ( ( (messageNode *) _receiver )->is_keyword() )
                return true;
            if ( ( (messageNode *) _receiver )->is_binary() and is_binary() )
                return true;
        }
        return false;
    }


    bool_t should_wrap_argument( astNode *arg ) {
        if ( arg->is_message() ) {
            if ( ( (messageNode *) arg )->is_keyword() )
                return true;
            if ( ( (messageNode *) arg )->is_binary() and is_binary() )
                return true;
        }
        return false;
    }


    bool_t print_receiver( prettyPrintStream *output ) {
        bool_t wrap = should_wrap_receiver();
        if ( wrap )
            output->print( "(" );
        _receiver->print( output );
        if ( wrap )
            output->print( ")" );
        output->print( " " );
        return false;
    }


    bool_t real_print( prettyPrintStream *output ) {
        HIGHLIGHT
        bool_t split;
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


    bool_t print( prettyPrintStream *output ) {
        if ( receiver() and receiver()->is_cascade() )
            return receiver()->print( output );
        return real_print( output );
    }


    int width_receiver( prettyPrintStream *output ) {
        int w = receiver() ? receiver()->width( output ) + output->width_of_space() : 0;
        if ( should_wrap_receiver() ) {
            w += output->width_of_string( "(" ) + output->width_of_string( ")" );
        }
        return w;
    }


    int width_send( prettyPrintStream *output ) {
        int arg = _selector->number_of_arguments();
        int w   = output->width_of_string( _selector->as_string() ) + arg * output->width_of_space();

        for ( std::size_t i = 0; i < _arguments->length(); i++ )
            w += _arguments->at( i )->width( output );

        return w;
    }


    int real_width( prettyPrintStream *output ) {
        return ( receiver() ? width_receiver( output ) + output->width_of_space() : 0 ) + width_send( output );
    }


    int width( prettyPrintStream *output ) {
        if ( receiver() and receiver()->is_cascade() )
            return receiver()->width( output );
        return real_width( output );
    }


    void add_param( astNode *p ) {
        _arguments->push( p );
    }


    void set_receiver( astNode *p );


    astNode *receiver() {
        return _receiver;
    }


    bool_t valid_receiver() {
        return receiver() ? not receiver()->is_cascade() : false;
    }


    bool_t is_message() {
        return true;
    }
};


class selfNode : public leafNode {

public:
    selfNode( int byteCodeIndex, scopeNode *scope ) :
            leafNode( byteCodeIndex, scope ) {
    }


    const char *string() {
        return "self";
    }
};


class ignoreReceiver : public leafNode {

public:
    ignoreReceiver( int byteCodeIndex, scopeNode *scope ) :
            leafNode( byteCodeIndex, scope ) {
    }


    const char *string() {
        return "";
    }
};


class superNode : public leafNode {

public:
    superNode( int byteCodeIndex, scopeNode *scope ) :
            leafNode( byteCodeIndex, scope ) {
    }


    const char *string() {
        return "super";
    }
};


class literalNode : public leafNode {

private:
    const char *_str;

public:
    literalNode( int byteCodeIndex, scopeNode *scope, const char *str ) :
            leafNode( byteCodeIndex, scope ) {
        this->_str = str;
    }


    const char *string() {
        return _str;
    }
};


class symbolNode : public astNode {

private:
    SymbolOop _value;
    bool_t    _isOuter;
    const char *_str;

public:
    symbolNode( int byteCodeIndex, scopeNode *scope, SymbolOop value, bool_t is_outer = true ) :
            astNode( byteCodeIndex, scope ) {
        this->_value   = value;
        this->_isOuter = is_outer;
        this->_str     = value->as_string();
    }


    bool_t print( prettyPrintStream *output ) {
        astNode::print( output );
        if ( _isOuter )
            output->print( "#" );
        output->print( _str );
        return false;
    }


    int width( prettyPrintStream *output ) {
        return ( _isOuter ? output->width_of_string( "#" ) : 0 ) + output->width_of_string( _str );
    }
};


class doubleByteArrayNode : public astNode {

private:
    DoubleByteArrayOop _value;
    char *_str;

public:
    doubleByteArrayNode( int byteCodeIndex, scopeNode *scope, DoubleByteArrayOop value ) :
            astNode( byteCodeIndex, scope ) {
        this->_value = value;
        this->_str   = value->as_string();
    }


    bool_t print( prettyPrintStream *output ) {
        astNode::print( output );
        output->print( "'" );
        output->print( _str );
        output->print( "'" );
        return false;
    }


    int width( prettyPrintStream *output ) {
        return 2 * output->width_of_string( "'" ) + output->width_of_string( _str );
    }
};


class byteArrayNode : public astNode {

private:
    ByteArrayOop _value;
    const char *_str;

public:
    byteArrayNode( int byteCodeIndex, scopeNode *scope, ByteArrayOop value ) :
            astNode( byteCodeIndex, scope ) {
        this->_value = value;
        this->_str   = value->as_string();
    }


    bool_t print( prettyPrintStream *output ) {
        HIGHLIGHT
        astNode::print( output );
        output->print( "'" );
        output->print( _str );
        output->print( "'" );
        return false;
    }


    int width( prettyPrintStream *output ) {
        return 2 * output->width_of_string( "'" ) + output->width_of_string( _str );
    }
};


class smiNode : public leafNode {

private:
    int _value;
    char *_str;

public:
    smiNode( int byteCodeIndex, scopeNode *scope, int value ) :
            leafNode( byteCodeIndex, scope ) {
        this->_value = value;
        this->_str   = new_resource_array<char>( 10 );
        sprintf( this->_str, "%d", value );
    }


    const char *string() {
        return _str;
    }
};

class doubleNode : public leafNode {

private:
    double _value;
    char *_str;

public:
    doubleNode( int byteCodeIndex, scopeNode *scope, double value ) :
            leafNode( byteCodeIndex, scope ) {
        this->_value = value;
        this->_str   = new_resource_array<char>( 30 );
        sprintf( this->_str, "%1.10gd", value );
    }


    const char *string() {
        return _str;
    }
};


class characterNode : public leafNode {

private:
    Oop _value;
    char *_str;

public:
    characterNode( int byteCodeIndex, scopeNode *scope, Oop value ) :
            leafNode( byteCodeIndex, scope ) {
        this->_value = value;
        this->_str   = new_resource_array<char>( 3 );
        if ( value->is_mem() ) {
            Oop ch = MemOop( value )->instVarAt( 2 );
            if ( ch->is_smi() ) {
                sprintf( this->_str, "$%c", SMIOop( ch )->value() );
                return;
            }
        }
        this->_str = "$%c";
    }


    const char *string() {
        return _str;
    }
};


class objArrayNode : public astNode {

private:
    ObjectArrayOop           _value;
    bool_t                   _isOuter;
    GrowableArray<astNode *> *_elements;

public:
    objArrayNode( int byteCodeIndex, scopeNode *scope, ObjectArrayOop value, bool_t is_outer = true ) :
            astNode( byteCodeIndex, scope ) {
        this->_value    = value;
        this->_isOuter  = is_outer;
        this->_elements = new GrowableArray<astNode *>( 10 );

        for ( std::size_t i = 1; i <= value->length(); i++ )
            _elements->push( get_literal_node( value->obj_at( i ), byteCodeIndex, scope ) );
    }


    bool_t print( prettyPrintStream *output ) {
        HIGHLIGHT
        astNode::print( output );
        output->print( "#(" );
        for ( std::size_t i = 0; i < _elements->length(); i++ ) {
            _elements->at( i )->print( output );
            if ( i < _elements->length() - 1 )
                output->space();
        }
        output->print( ")" );
        return false;
    }


    int width( prettyPrintStream *output ) {
        int w = output->width_of_string( "#(" ) + output->width_of_string( ")" );

        for ( std::size_t i = 0; i < _elements->length(); i++ ) {
            w += _elements->at( i )->width( output );
            if ( i < _elements->length() - 1 )
                w += output->width_of_space();
        }
        return 0;
    }
};


class dllNode : public astNode {

private:
    astNode *_dllName;
    astNode *_funcName;
    GrowableArray<astNode *> *_arguments;
    astNode *_proxy;

public:
    dllNode( int byteCodeIndex, scopeNode *scope, SymbolOop dll_name, SymbolOop func_name ) :
            astNode( byteCodeIndex, scope ) {
        this->_dllName  = new symbolNode( byteCodeIndex, scope, dll_name, false );
        this->_funcName = new symbolNode( byteCodeIndex, scope, func_name, false );
        _arguments = new GrowableArray<astNode *>( 10 );
        _proxy     = nullptr;
    }


    void add_param( astNode *param ) {
        _arguments->push( param );
    }


    void set_proxy( astNode *p ) {
        _proxy = p;
    }


    bool_t print( prettyPrintStream *output ) {
        HIGHLIGHT
        astNode::print( output );
        output->print( "{{<" );
        _dllName->print( output );
        output->space();
        _proxy->print( output );
        output->space();
        _funcName->print( output );
        output->print( ">" );
        for ( std::size_t i = _arguments->length() - 1; i >= 0; i-- ) {
            _arguments->at( i )->print( output );
            if ( i < _arguments->length() - 1 )
                output->print( ", " );
        }
        output->print( "}}" );
        return false;
    }


    int width( prettyPrintStream *output ) {
        return output->width_of_string( "<" ) + output->width_of_string( "Printing dll call" ) + output->width_of_string( ">" );
    }
};


class stackTempNode : public leafNode {

private:
    std::size_t _offset;
    const char *_str;

public:
    stackTempNode( int byteCodeIndex, scopeNode *scope, int offset ) :
            leafNode( byteCodeIndex, scope ) {
        this->_offset = offset;
        this->_str    = scope->stack_temp_string( this_byteCodeIndex(), offset );
    }


    const char *string() {
        return _str;
    }
};


class heapTempNode : public leafNode {

private:
    std::size_t _offset;
    int _contextLevel;
    const char *_str;

public:
    heapTempNode( int byteCodeIndex, scopeNode *scope, int offset, int context_level ) :
            leafNode( byteCodeIndex, scope ) {
        this->_offset       = offset;
        this->_contextLevel = context_level;
        this->_str          = scope->heap_temp_string( this_byteCodeIndex(), offset, context_level );
    }


    const char *string() {
        return _str;
    }
};

class floatNode : public leafNode {

private:
    const char *_str;

public:
    floatNode( int no, int byteCodeIndex, scopeNode *scope ) :
            leafNode( byteCodeIndex, scope ) {
        this->_str = scope->stack_temp_string( this_byteCodeIndex(), no );
    }


    const char *string() {
        return _str;
    }
};


class instVarNode : public leafNode {

private:
    Oop _obj;
    const char *_str;

public:
    instVarNode( int byteCodeIndex, scopeNode *scope, Oop obj ) :
            leafNode( byteCodeIndex, scope ) {
        this->_obj = obj;
        if ( obj->is_smi() ) {
            _str = scope->inst_var_string( SMIOop( obj )->value() );
        } else {
            _str = SymbolOop( obj )->as_string();
        }
    }


    const char *string() {
        return _str;
    }
};


class classVarNode : public leafNode {

private:
    Oop _obj;
    const char *_str;

public:
    classVarNode( int byteCodeIndex, scopeNode *scope, Oop obj ) :
            leafNode( byteCodeIndex, scope ) {
        this->_obj = obj;
        if ( obj->is_association() ) {
            _str = AssociationOop( obj )->key()->as_string();
        } else {
            _str = SymbolOop( obj )->as_string();
        }
    }


    const char *string() {
        return _str;
    }
};


class primitiveResultNode : public leafNode {

public:
    bool_t is_primitive_result() {
        return true;
    }


    primitiveResultNode( int byteCodeIndex, scopeNode *scope ) :
            leafNode( byteCodeIndex, scope ) {
    }


    const char *string() {
        return "`primitive result`";
    }
};


class assocNode : public leafNode {

private:
    AssociationOop _assoc;
    const char *_str;

public:
    assocNode( int byteCodeIndex, scopeNode *scope, AssociationOop assoc ) :
            leafNode( byteCodeIndex, scope ) {
        this->_assoc = assoc;
        this->_str   = assoc->key()->as_string();
    }


    const char *string() {
        return _str;
    }
};


class cascadeSendNode : public astNode {

private:
    GrowableArray<messageNode *> *_messages;
    astNode *_receiver;

public:
    cascadeSendNode( int byteCodeIndex, scopeNode *scope, messageNode *first ) :
            astNode( byteCodeIndex, scope ) {
        _messages = new GrowableArray<messageNode *>( 10 );
        _receiver = first->receiver();
        first->set_receiver( this );
    }


    bool_t is_cascade() {
        return true;
    }


    void add_message( messageNode *m ) {
        _messages->push( m );
    }


    bool_t print( prettyPrintStream *output ) {
        _receiver->print( output );
        output->inc_newline();
        for ( std::size_t i = 0; i < _messages->length(); i++ ) {
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


    int width( prettyPrintStream *output ) {
        return 0;
    }
};


void messageNode::set_receiver( astNode *p ) {
    if ( p->is_cascade() ) {
        ( (cascadeSendNode *) p )->add_message( this );
    }
    _receiver = p;
}


static astNode *get_literal_node( Oop obj, int byteCodeIndex, scopeNode *scope ) {
    if ( obj == trueObj )
        return new literalNode( byteCodeIndex, scope, "true" );
    if ( obj == falseObj )
        return new literalNode( byteCodeIndex, scope, "false" );
    if ( obj == nilObj )
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
        return new objArrayNode( byteCodeIndex, scope, ObjectArrayOop( obj ) );
    return new characterNode( byteCodeIndex, scope, obj );
}


void prettyPrintStream::print() {
    _console->print( "pretty-printer stream" );
}


void defaultPrettyPrintStream::indent() {
    for ( std::size_t i = 0; i < _indentation; i++ ) {
        space();
        space();
    }
}


void defaultPrettyPrintStream::print( const char *str ) {
    for ( std::size_t i = 0; str[ i ]; i++ )
        print_char( str[ i ] );
}


void defaultPrettyPrintStream::print_char( char c ) {
    _console->print( "%c", c );
    pos += width_of_char( c );
}


int defaultPrettyPrintStream::width_of_string( const char *str ) {
    int       w = 0;
    for ( std::size_t i = 0; str[ i ]; i++ )
        w += width_of_char( str[ i ] );
    return w;
}


void defaultPrettyPrintStream::space() {
    print_char( ' ' );
}


void defaultPrettyPrintStream::newline() {
    print_char( '\n' );
    pos = 0;
    indent();
}


void defaultPrettyPrintStream::begin_highlight() {
    set_highlight( true );
    print( "{" );
}


void defaultPrettyPrintStream::end_highlight() {
    set_highlight( false );
    print( "}" );
}


void byteArrayPrettyPrintStream::newline() {
    print_char( '\r' );
    pos = 0;
    indent();
}


// Forward declaration
astNode *generateForBlock( MethodOop method, KlassOop klass, int byteCodeIndex, int numOfArgs );

astNode *generate( scopeNode *scope );

class MethodPrettyPrinter : public MethodClosure {
private:
    GrowableArray<astNode *> *_stack; // expression stack
    scopeNode *_scope; // debugging scope

    // the receiver is on the stack
    void normal_send( SymbolOop selector, bool_t is_prim = false );

    // the receiver is provided
    void special_send( astNode *receiver, SymbolOop selector, bool_t is_prim = false );

    astNode *generateInlineBlock( MethodInterval *interval, bool_t produces_result, astNode *push_elem = nullptr );


    void store( astNode *node ) {
        _push( new assignment( byteCodeIndex(), scope(), node, _pop() ) );
    }


public:
    void _push( astNode *node ) {
        _stack->push( node );
    }


    astNode *_pop() {
        return _stack->pop();
    }


    astNode *_top() {
        return _stack->top();
    }


    std::size_t _size() const {
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

    void method_return( int nofArgs );

    void nonlocal_return( int nofArgs );


    void allocate_closure( AllocationType type, int nofArgs, MethodOop meth ) {
        if ( type == AllocationType::tos_as_scope )
            _pop();
        scopeNode *methodScope = _scope->scopeFor( meth );
        if ( methodScope )
            _push( generate( methodScope ) );
        else
            _push( generateForBlock( meth, scope()->get_klass(), -1, nofArgs ) );
    }


    void allocate_context( int nofTemps, bool_t forMethod ) {
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


    void push_argument( int no ) {
        _push( new paramNode( byteCodeIndex(), scope(), no ) );
    }


    void push_temporary( int no ) {
        _push( new stackTempNode( byteCodeIndex(), scope(), no ) );
    }


    void push_temporary( int no, int context ) {
        _push( new heapTempNode( byteCodeIndex(), scope(), no, context ) );
    }


    void push_instVar( int offset ) {
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


    void store_temporary( int no ) {
        store( new stackTempNode( byteCodeIndex(), scope(), no ) );
    }


    void store_temporary( int no, int context ) {
        store( new heapTempNode( byteCodeIndex(), scope(), no, context ) );
    }


    void store_instVar( int offset ) {
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
    void allocate_temporaries( int nofTemps ) {
    }


    void set_self_via_context() {
    }


    void copy_self_into_context() {
    }


    void copy_argument_into_context( int argNo, int no ) {
    }


    void zap_scope() {
    }


    void predict_primitive_call( PrimitiveDescriptor *pdesc, int failure_start ) {
    }


    void float_allocate( int nofFloatTemps, int nofFloatExprs ) {
    }


    void float_floatify( Floats::Function f, int tof ) {
        normal_send( Floats::selector_for( f ) );
        pop();
    }


    void float_move( int tof, int from ) {
        _push( new floatNode( from, byteCodeIndex(), scope() ) );
        store( new floatNode( tof, byteCodeIndex(), scope() ) );
        pop();
    }


    void float_set( int tof, DoubleOop value ) {
    }


    void float_nullary( Floats::Function f, int tof ) {
    }


    void float_unary( Floats::Function f, int tof ) {
        _push( new floatNode( tof, byteCodeIndex(), scope() ) );
        normal_send( Floats::selector_for( f ) );
        pop();
    }


    void float_binary( Floats::Function f, int tof ) {
        _push( new floatNode( tof - 1, byteCodeIndex(), scope() ) );
        _push( new floatNode( tof, byteCodeIndex(), scope() ) );
        normal_send( Floats::selector_for( f ) );
        pop();
    }


    void float_unaryToOop( Floats::Function f, int tof ) {
        _push( new floatNode( tof, byteCodeIndex(), scope() ) );
        normal_send( Floats::selector_for( f ) );
    }


    void float_binaryToOop( Floats::Function f, int tof ) {
        _push( new floatNode( tof, byteCodeIndex(), scope() ) );
        _push( new floatNode( tof - 1, byteCodeIndex(), scope() ) );
        normal_send( Floats::selector_for( f ) );
    }
};


MethodPrettyPrinter::MethodPrettyPrinter( scopeNode *scope ) {
    _stack = new GrowableArray<astNode *>( 10 );
    _scope = scope;
    if ( scope->method()->is_blockMethod() )
        _push( new blockNode( 1, scope, scope->method()->number_of_arguments() ) );
    else
        _push( new methodNode( 1, scope ) );
}


void MethodPrettyPrinter::normal_send( SymbolOop selector, bool_t is_prim ) {
    int nargs = selector->number_of_arguments();
    messageNode *msg = new messageNode( byteCodeIndex(), scope(), selector, is_prim );

    GrowableArray<astNode *> *arguments = new GrowableArray<astNode *>( 10 );
    for ( int                i          = 0; i < nargs; i++ )
        arguments->push( _pop() );

    for ( std::size_t i = 0; i < nargs; i++ )
        msg->add_param( arguments->at( i ) );

    msg->set_receiver( _pop() );

    _push( msg );
}


void MethodPrettyPrinter::special_send( astNode *receiver, SymbolOop selector, bool_t is_prim ) {
    int nargs = selector->number_of_arguments();
    messageNode *msg = new messageNode( byteCodeIndex(), scope(), selector, is_prim );

    GrowableArray<astNode *> *arguments = new GrowableArray<astNode *>( 10 );
    for ( int                i          = 0; i < nargs; i++ )
        arguments->push( _pop() );

    for ( std::size_t i = 0; i < nargs; i++ )
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


void MethodPrettyPrinter::method_return( int nofArgs ) {
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


void MethodPrettyPrinter::nonlocal_return( int nofArgs ) {
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
    std::size_t _size;
    const char *_name;
    std::size_t _offset;


    StackChecker( const char *name, MethodPrettyPrinter *pp, int offset = 0 ) {
        this->_methodPrettyPrinter = pp;
        this->_size                = pp->_size();
        this->_name                = name;
        this->_offset              = offset;
    }


    ~StackChecker() {
        if ( _methodPrettyPrinter->_size() not_eq _size + _offset ) {
            _console->print_cr( "StackTracer found misaligned stack" );
            _console->print_cr( "Expecting %d but found %d", _size + _offset, _methodPrettyPrinter->_size() );
            st_fatal( "aborting" );
        }
    }
};


astNode *MethodPrettyPrinter::generateInlineBlock( MethodInterval *interval, bool_t produces_result, astNode *push_elem ) {
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

    int nargs = node->nofArgs();

    GrowableArray<astNode *> *arguments = new GrowableArray<astNode *>( 10 );
    for ( int                i          = 0; i < nargs; i++ )
        arguments->push( _pop() );

    for ( std::size_t i = 0; i < nargs; i++ )
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


astNode *generateForActivation( DeltaVirtualFrame *fr, int index ) {
    scopeNode *scope      = new scopeNode( fr, index );
    scopeNode *toGenerate = scope->homeScope();
    return generate( toGenerate );
}


astNode *generateForMethod( MethodOop method, KlassOop klass, int byteCodeIndex ) {
    return generate( new scopeNode( method, klass, byteCodeIndex ) );
}


astNode *generateForBlock( MethodOop method, KlassOop klass, int byteCodeIndex, int numOfArgs ) {
    return generate( new scopeNode( method, klass, byteCodeIndex ) );
}


void PrettyPrinter::print( MethodOop method, KlassOop klass, int byteCodeIndex, prettyPrintStream *output ) {
    ResourceMark resourceMark;
    if ( not output )
        output = new defaultPrettyPrintStream;
    astNode *root = generateForMethod( method, klass, byteCodeIndex );
    root->top_level_print( output );
    output->newline();
}


void PrettyPrinter::print( int index, DeltaVirtualFrame *fr, prettyPrintStream *output ) {
    ResourceMark resourceMark;
    if ( not output )
        output = new defaultPrettyPrintStream;

    if ( ActivationShowCode ) {
        astNode *root = generateForActivation( fr, index );
        root->top_level_print( output );
    } else {
        scopeNode *scope = new scopeNode( fr, index );
        scope->print_header( output );
    }
    output->newline();
}


void PrettyPrinter::print_body( DeltaVirtualFrame *fr, prettyPrintStream *output ) {
    ResourceMark resourceMark;
    if ( not output )
        output = new defaultPrettyPrintStream;

    astNode *root = generateForActivation( fr, 0 );
    root->print( output );
}


byteArrayPrettyPrintStream::byteArrayPrettyPrintStream() :
        defaultPrettyPrintStream() {
    _buffer = new GrowableArray<int>( 200 );
}


void byteArrayPrettyPrintStream::print_char( char c ) {
    _buffer->append( (int) c );
    pos += width_of_char( c );
}


ByteArrayOop byteArrayPrettyPrintStream::asByteArray() {
    int          l = _buffer->length();
    ByteArrayOop a = oopFactory::new_byteArray( l );
    for ( int    i = 0; i < l; i++ ) {
        a->byte_at_put( i + 1, (char) _buffer->at( i ) );
    }
    return a;
}


ByteArrayOop PrettyPrinter::print_in_byteArray( MethodOop method, KlassOop klass, int byteCodeIndex ) {
    ResourceMark rm;
    byteArrayPrettyPrintStream *stream = new byteArrayPrettyPrintStream();
    PrettyPrinter::print( method, klass, byteCodeIndex, stream );
    return stream->asByteArray();
}


bool_t should_wrap( int type, astNode *arg ) {
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


void PrintParams::parameter( ByteArrayOop name, int index ) {
    _elements->push( _scope->parameter_at( index, true ) );
}


void PrintTemps::stack_temp( ByteArrayOop name, int offset ) {
    _elements->push( _scope->stack_temp_at( offset ) );
}


void PrintTemps::heap_temp( ByteArrayOop name, int offset ) {
    _elements->push( _scope->heap_temp_at( offset ) );
}
