//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/system/platform.hpp"
#include "vm/system/asserts.hpp"
#include "vm/memory/util.hpp"
#include "vm/utilities/OutputStream.hpp"
#include "vm/runtime/flags.hpp"
#include "vm/utilities/GrowableArray.hpp"
#include "vm/runtime/Process.hpp"
#include "vm/memory/Universe.hpp"
#include "vm/oops/ByteArrayOopDescriptor.hpp"
#include "vm/oops/SymbolOopDescriptor.hpp"
#include "vm/lookup/LookupKey.hpp"
#include "vm/code/NativeInstruction.hpp"
#include "vm/code/CompiledInlineCache.hpp"
#include "vm/code/RelocationInformation.hpp"
#include "vm/code/NativeMethod.hpp"
#include "vm/code/NameDescriptor.hpp"
#include "vm/compiler/Expression.hpp"
#include "vm/code/ScopeDescriptor.hpp"
#include "vm/code/ScopeDescriptorRecorder.hpp"
#include "vm/code/NativeMethodScopes.hpp"


char *ScopeDescriptor::invalid_pc = (char *) -1;


int compareByteCodeIndex( int byteCodeIndex1, int byteCodeIndex2 ) {
    st_assert( byteCodeIndex1 not_eq IllegalByteCodeIndex and byteCodeIndex2 not_eq IllegalByteCodeIndex, "can't compare" );
    return byteCodeIndex1 - byteCodeIndex2;
}


ScopeDescriptor::ScopeDescriptor( const NativeMethodScopes *scopes, std::size_t offset, const char *pc ) {
    _scopes = scopes;
    _offset = offset;
    _pc     = pc;

    _name_desc_offset = offset;

    ScopeDescriptorHeaderByte b;
    b.unpack( _scopes->get_next_char( _name_desc_offset ) );

    _lite                  = b.is_lite();
    _hasTemporaries        = b.has_temps();
    _hasContextTemporaries = b.has_context_temps();
    _hasExpressionStack    = b.has_expr_stack();

    st_assert( offset not_eq 0 or not is_lite(), "Root scopeDesc cannot be lite" );

    if ( b.has_nameDescs() ) {
        _next = _scopes->unpackValueAt( _name_desc_offset ) + _offset;
    } else {
        _next = -1;
    }

    if ( _offset == 0 ) {
        _senderScopeOffset   = 0;
        _senderByteCodeIndex = IllegalByteCodeIndex;
    } else {
        _senderScopeOffset   = _scopes->unpackValueAt( _name_desc_offset );
        _senderByteCodeIndex = _scopes->unpackValueAt( _name_desc_offset );
    }

    _method = MethodOop( _scopes->unpackOopAt( _name_desc_offset ) );
    st_assert( _method->is_method(), "expecting a method" );

    _allocatesCompiledContext = b.has_compiled_context();
    _scopeID                  = _scopes->unpackValueAt( _name_desc_offset );
}


ScopeDescriptor *ScopeDescriptor::home( bool_t cross_NativeMethod_boundary ) const {
    ScopeDescriptor *p = (ScopeDescriptor *) this;
    for ( ; p and not p->isMethodScope(); p = p->parent( cross_NativeMethod_boundary ) );
    return p;
}


NameDescriptor *ScopeDescriptor::temporary( int index, bool_t canFail ) {
    std::size_t    pos     = _name_desc_offset;
    NameDescriptor *result = nullptr;

    if ( _hasTemporaries ) {
        NameDescriptor *current = nameDescAt( pos );
        std::size_t    i        = 0;
        while ( current not_eq nullptr ) {
            if ( i == index ) {
                result = current;
                break;
            }
            current = nameDescAt( pos );
            i++;
        }
    }

    if ( not result and not canFail ) {
        st_fatal1( "couldn't find temporary %d", index );
    }

    return result;
}


NameDescriptor *ScopeDescriptor::contextTemporary( int index, bool_t canFail ) {
    std::size_t    pos     = _name_desc_offset;
    NameDescriptor *result = nullptr;

    if ( _hasTemporaries ) {
        NameDescriptor *current = nameDescAt( pos );
        while ( current ) {
            current = nameDescAt( pos );
        }
    }

    if ( _hasContextTemporaries ) {
        NameDescriptor *current = nameDescAt( pos );
        std::size_t    i        = 0;
        while ( current ) {
            if ( i == index ) {
                result = current;
                break;
            }
            current = nameDescAt( pos );
            i++;
        }
    }

    if ( not result and not canFail ) {
        st_fatal1( "couldn't find context temporary %d", index );
    }

    return result;
}


NameDescriptor *ScopeDescriptor::exprStackElem( int byteCodeIndex ) {
    std::size_t pos = _name_desc_offset;

    if ( _hasTemporaries ) {
        NameDescriptor *current = nameDescAt( pos );
        while ( current ) {
            current = nameDescAt( pos );
        }
    }

    if ( _hasContextTemporaries ) {
        NameDescriptor *current = nameDescAt( pos );
        while ( current ) {
            current = nameDescAt( pos );
        }
    }

    if ( _hasExpressionStack ) {
        NameDescriptor *current = nameDescAt( pos );
        while ( current ) {
            int the_byteCodeIndex = valueAt( pos );
            if ( byteCodeIndex == the_byteCodeIndex )
                return current;
            current = nameDescAt( pos );
        }
    }

    return nullptr;
}


void ScopeDescriptor::iterate( NameDescriptorClosure *blk ) {
    std::size_t pos = _name_desc_offset;

    if ( _hasTemporaries ) {
        NameDescriptor *current = nameDescAt( pos );
        int            number   = 0;
        while ( current ) {
            blk->temp( number++, current, pc() );
            current = nameDescAt( pos );
        }
    }

    if ( _hasContextTemporaries ) {
        NameDescriptor *current = nameDescAt( pos );
        int            number   = 0;
        while ( current ) {
            blk->context_temp( number++, current, pc() );
            current = nameDescAt( pos );
        }
    }

    if ( _hasExpressionStack ) {
        NameDescriptor *current = nameDescAt( pos );
        while ( current ) {
            blk->stack_expr( valueAt( pos ), current, pc() );
            current = nameDescAt( pos );
        }
    }
}


// Wrapper class for NameDescriptorClosure, allows iteration over string of NameDescriptor instance with different pc information.

class IterationHelper : public UnpackClosure {
protected:
    int                   _no;
    NameDescriptorClosure *_blk;
    bool_t                _is_used;


    void use() {
        _is_used = true;
    }


public:
    void init( int no, NameDescriptorClosure *blk ) {
        _no      = no;
        _blk     = blk;
        _is_used = false;
    }


    bool_t is_used() const {
        return _is_used;
    }
};

class IH_arg : public IterationHelper {
    void nameDescAt( NameDescriptor *nd, const char *pc ) {
        use();
        _blk->arg( _no, nd, pc );
    }
};

class IH_temp : public IterationHelper {
    void nameDescAt( NameDescriptor *nd, const char *pc ) {
        use();
        _blk->temp( _no, nd, pc );
    }
};

class IH_context_temp : public IterationHelper {
    void nameDescAt( NameDescriptor *nd, const char *pc ) {
        use();
        _blk->context_temp( _no, nd, pc );
    }
};

class IH_stack_expr : public IterationHelper {
    void nameDescAt( NameDescriptor *nd, const char *pc ) {
        use();
        _blk->stack_expr( _no, nd, pc );
    }
};


void ScopeDescriptor::iterate_all( NameDescriptorClosure *blk ) {
    std::size_t pos = _name_desc_offset;
    if ( _hasTemporaries ) {
        int     no = 0;
        IH_temp helper;
        do {
            helper.init( no++, blk );
            _scopes->iterate( pos, &helper );
        } while ( helper.is_used() );
    }

    if ( _hasContextTemporaries ) {
        int             no = 0;
        IH_context_temp helper;
        do {
            helper.init( no++, blk );
            _scopes->iterate( pos, &helper );
        } while ( helper.is_used() );
    }

    if ( _hasExpressionStack ) {
        int           no = 0;
        IH_stack_expr helper;
        do {
            helper.init( no++, blk );
            _scopes->iterate( pos, &helper );
            if ( helper.is_used() )
                valueAt( pos ); // get byteCodeIndex (i.e., print-out is showing expr. index and not byteCodeIndex)
        } while ( helper.is_used() );
    }
}


bool_t ScopeDescriptor::allocates_interpreted_context() const {
    return method()->allocatesInterpretedContext();
}


NameDescriptor *ScopeDescriptor::compiled_context() {
    st_assert( allocates_compiled_context(), "must allocate a context" );
    constexpr int temporary_index_for_context = 0;
    return temporary( temporary_index_for_context );
}


bool_t ScopeDescriptor::s_equivalent( ScopeDescriptor *s ) const {
    return method() == s->method() and ( _senderByteCodeIndex == s->_senderByteCodeIndex or _senderByteCodeIndex < 0 or s->_senderByteCodeIndex < 0 );
    // don't check senderByteCodeIndex for pseudo ByteCodeIndexs
}


bool_t ScopeDescriptor::l_equivalent( LookupKey *l ) const {
    return selector() == l->selector();
}


ScopeDescriptor *ScopeDescriptor::sender() const {
    return _senderScopeOffset ? _scopes->at( _offset - _senderScopeOffset, pc() ) : nullptr;
}


NameDescriptor *ScopeDescriptor::nameDescAt( std::size_t &offset ) const {
    return _scopes->unpackNameDescAt( offset, pc() );
}


int ScopeDescriptor::valueAt( std::size_t &offset ) const {
    return _scopes->unpackValueAt( offset );
}


bool_t ScopeDescriptor::verify() {
    // verifies mostly structure, not contents
    bool_t ok = true;

    // don't do a full verify of parent/sender -- too many redundant verifies
    ScopeDescriptor *s = sender();
    if ( s and not s->shallow_verify() ) {
        _console->print( "invalid sender %#lx of ScopeDescriptor %#lx", s, this );
        ok = false;
    }
    ScopeDescriptor *p = parent();
    if ( p and not p->shallow_verify() ) {
        _console->print( "invalid parent %#lx of ScopeDescriptor %#lx", p, this );
        ok = false;
    }
    return ok;
}


// verify expression stack at a call or primitive call
void ScopeDescriptor::verify_expression_stack( int byteCodeIndex ) {
    GrowableArray<int> *mapping = method()->expression_stack_mapping( byteCodeIndex );
    for ( std::size_t  index    = 0; index < mapping->length(); index++ ) {
        NameDescriptor *nd = exprStackElem( mapping->at( index ) );
        if ( nd == nullptr ) {
            warning( "expression not found in NativeMethod" );
            continue;
        }
        // Fix this Lars (add parameter for checking registers
        // if (nd->hasLocation() and nd->location().isRegisterLocation()) {
        //   print(); nd->print(); method()->print_codes();
        //   error("expr stack element is in register at call at byteCodeIndex %d", byteCodeIndex);
        // }
    }
}


class PrintNameDescClosure : public NameDescriptorClosure {
private:
    int  _indent;
    char *_pc0;


    void print( const char *title, int no, NameDescriptor *nd, char *pc ) {
        _console->fill_to( _indent );
        if ( UseNewBackend ) {
            _console->print( "%5d: ", pc - _pc0 );
        }
        _console->print( "%s[%d]\t", title, no );
        nd->print();
        _console->cr();
    }


public:
    PrintNameDescClosure( int indent, char *pc0 ) {
        _indent = indent;
        _pc0    = pc0;
    }


    void arg( int no, NameDescriptor *a, char *pc ) {
        print( "arg   ", no, a, pc );
    }


    void temp( int no, NameDescriptor *t, char *pc ) {
        print( "temp  ", no, t, pc );
    }


    void context_temp( int no, NameDescriptor *c, char *pc ) {
        print( "c_temp", no, c, pc );
    }


    void stack_expr( int no, NameDescriptor *e, char *pc ) {
        print( "expr  ", no, e, pc );
    }
};


void ScopeDescriptor::print( int indent, bool_t all_pcs ) {
    _console->fill_to( indent );
    printName();
    _console->print( "ScopeDescriptor @%d%s: ", offset(), is_lite() ? ", lite" : "" );
    _console->print( " (ID %ld) ", scopeID() );
    method()->selector()->print_symbol_on();
    _console->print( " %#x", method() );
    _console->cr();
    ScopeDescriptor *s = sender();
    if ( s not_eq nullptr ) {
        _console->fill_to( indent );
        _console->print( "sender: (%d) @ %ld", s->offset(), std::int32_t( senderByteCodeIndex() ) );
    }
    ScopeDescriptor *p = parent();
    if ( p not_eq nullptr ) {
        if ( s not_eq nullptr ) {
            _console->print( "; " );
        } else {
            _console->fill_to( indent );
        }
        _console->print( "parent: (%d)", p->offset() );
    }
    if ( s not_eq nullptr or p not_eq nullptr ) {
        _console->cr();
    }
    _console->fill_to( indent );
    printSelf();
    PrintNameDescClosure blk( indent + 2, _scopes->my_nativeMethod()->instructionsStart() );
    if ( all_pcs ) {
        iterate_all( &blk );
    } else {
        iterate( &blk );
    }
}


void ScopeDescriptor::print_value_on( ConsoleOutputStream *stream ) const {
    // print offset
    if ( WizardMode )
        stream->print( " [%d]", offset() );
}


bool_t MethodScopeDescriptor::s_equivalent( ScopeDescriptor *s ) const {
    return s->isMethodScope() and ScopeDescriptor::s_equivalent( s ) and key()->equal( ( (MethodScopeDescriptor *) s )->key() );
}


bool_t MethodScopeDescriptor::l_equivalent( LookupKey *l ) const {
    return ScopeDescriptor::l_equivalent( l ) and selfKlass() == l->klass();
}


MethodScopeDescriptor::MethodScopeDescriptor( NativeMethodScopes *scopes, int offset, const char *pc ) :
        ScopeDescriptor( scopes, offset, pc ), _key() {
    Oop k = _scopes->unpackOopAt( _name_desc_offset );
    Oop s = _scopes->unpackOopAt( _name_desc_offset );
    _key.initialize( (KlassOop) k, s );
    _self_name = _scopes->unpackNameDescAt( _name_desc_offset, pc );
    if ( _next == -1 )
        _next  = _name_desc_offset;
}


void MethodScopeDescriptor::printName() {
    _console->print( "Method" );
}


void MethodScopeDescriptor::printSelf() {
    printIndent();
    _console->print( "self: " );
    self()->print();
    _console->cr();
}


void MethodScopeDescriptor::print_value_on( ConsoleOutputStream *stream ) const {
    key()->print_on( stream );
    ScopeDescriptor::print_value_on( stream );
}


void BlockScopeDescriptor::printSelf() {
    ScopeDescriptor::printSelf();
    _console->cr();
}


BlockScopeDescriptor::BlockScopeDescriptor( const NativeMethodScopes *scopes, int offset, const char *pc ) :
        ScopeDescriptor( scopes, offset, pc ) {
    _parentScopeOffset = _scopes->unpackValueAt( _name_desc_offset );

    if ( _next == -1 )
        _next = _name_desc_offset;
}


bool_t BlockScopeDescriptor::s_equivalent( ScopeDescriptor *s ) const {
    return s->isBlockScope() and ScopeDescriptor::s_equivalent( s );
}


void BlockScopeDescriptor::printName() {
    _console->print( "Block" );
}


KlassOop BlockScopeDescriptor::selfKlass() const {
    ScopeDescriptor *p = parent();
    return p ? p->selfKlass() : nullptr;
}


NameDescriptor *BlockScopeDescriptor::self() const {
    ScopeDescriptor *p = parent();
    return p ? p->self() : nullptr;
}


ScopeDescriptor *BlockScopeDescriptor::parent( bool_t cross_NativeMethod_boundary ) const {
    return _parentScopeOffset ? _scopes->at( _offset - _parentScopeOffset, pc() ) : nullptr;
}


void BlockScopeDescriptor::print_value_on( ConsoleOutputStream *stream ) const {
    stream->print( "block {parent %d}", _offset - _parentScopeOffset );
    ScopeDescriptor::print_value_on( stream );
}


LookupKey *BlockScopeDescriptor::key() const {
    return LookupKey::allocate( selfKlass(), method() );
}


LookupKey *TopLevelBlockScopeDescriptor::key() const {
    return LookupKey::allocate( selfKlass(), method() );
}


NonInlinedBlockScopeDescriptor::NonInlinedBlockScopeDescriptor( const NativeMethodScopes *scopes, std::size_t offset ) {
    _offset = offset;
    _scopes = scopes;

    ScopeDescriptorHeaderByte b;
    b.unpack( _scopes->get_next_char( offset ) );
    _method            = MethodOop( scopes->unpackOopAt( offset ) );
    _parentScopeOffset = scopes->unpackValueAt( offset );
}


void NonInlinedBlockScopeDescriptor::print() {
    _console->print_cr( "NonInlinedBlockScopeDescriptor" );
    _console->print( " - method: " );
    method()->print_value();
    _console->cr();
    _console->print_cr( " - parent offset: %d", _parentScopeOffset );
}


ScopeDescriptor *NonInlinedBlockScopeDescriptor::parent() const {
    return _parentScopeOffset ? _scopes->at( _offset - _parentScopeOffset, ScopeDescriptor::invalid_pc ) : nullptr;
}


TopLevelBlockScopeDescriptor::TopLevelBlockScopeDescriptor( const NativeMethodScopes *scopes, std::size_t offset, const char *pc ) :
        ScopeDescriptor( scopes, offset, pc ) {
    _self_name  = _scopes->unpackNameDescAt( _name_desc_offset, pc );
    _self_klass = KlassOop( scopes->unpackOopAt( _name_desc_offset ) );
    if ( _next == -1 )
        _next   = _name_desc_offset;
}


void TopLevelBlockScopeDescriptor::printSelf() {
    ScopeDescriptor::printSelf();
    _console->print( "self: " );
    self()->print();
    _console->cr();
}


ScopeDescriptor *TopLevelBlockScopeDescriptor::parent( bool_t cross_NativeMethod_boundary ) const {
    if ( not cross_NativeMethod_boundary )
        return nullptr;
    NativeMethod                   *nm     = _scopes->my_nativeMethod();
    int                            index;
    NativeMethod                   *parent = nm->jump_table_entry()->parent_nativeMethod( index );
    NonInlinedBlockScopeDescriptor *scope  = parent->noninlined_block_scope_at( index );
    return scope->parent();
}


bool_t TopLevelBlockScopeDescriptor::s_equivalent( ScopeDescriptor *s ) const {
    // programming can tranform a nested block to a top-level block
    return s->isBlockScope() and ScopeDescriptor::s_equivalent( s );
}


void TopLevelBlockScopeDescriptor::printName() {
    _console->print( "TopLevelBlock" );
}


void TopLevelBlockScopeDescriptor::print_value_on( ConsoleOutputStream *stream ) const {
    stream->print( "top block" );
    ScopeDescriptor::print_value_on( stream );
}


Expression *ScopeDescriptor::selfExpression( PseudoRegister *p ) const {
    KlassOop self_klass = selfKlass();
    if ( self_klass == trueObj->klass() )
        return new ConstantExpression( trueObj, p, nullptr );
    if ( self_klass == falseObj->klass() )
        return new ConstantExpression( falseObj, p, nullptr );
    if ( self_klass == nilObj->klass() )
        return new ConstantExpression( nilObj, p, nullptr );
    return new KlassExpression( self_klass, p, nullptr );
}
