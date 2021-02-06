
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/system/platform.hpp"
#include "vm/system/asserts.hpp"
#include "vm/memory/util.hpp"
#include "vm/runtime/flags.hpp"
#include "vm/utilities/GrowableArray.hpp"
#include "vm/memory/Universe.hpp"
#include "vm/lookup/LookupKey.hpp"
#include "vm/code/CompiledInlineCache.hpp"
#include "vm/code/RelocationInformation.hpp"
#include "vm/code/NativeMethod.hpp"
#include "vm/code/NameDescriptor.hpp"
#include "vm/compiler/Expression.hpp"
#include "vm/code/ScopeDescriptor.hpp"
#include "vm/code/ScopeDescriptorRecorder.hpp"
#include "vm/code/NativeMethodScopes.hpp"
#include "vm/utilities/ConsoleOutputStream.hpp"


char *ScopeDescriptor::invalid_pc = (char *) -1;


std::int32_t compareByteCodeIndex( std::int32_t byteCodeIndex1, std::int32_t byteCodeIndex2 ) {
    st_assert( byteCodeIndex1 not_eq IllegalByteCodeIndex and byteCodeIndex2 not_eq IllegalByteCodeIndex, "can't compare" );
    return byteCodeIndex1 - byteCodeIndex2;
}


ScopeDescriptor::ScopeDescriptor( const NativeMethodScopes *scopes, std::int32_t offset, const char *pc ) :


//    _next{ 0 },
//    _name_desc_offset{ 0 },

    _hasTemporaries{ false },
    _hasContextTemporaries{ false },
    _hasExpressionStack{ false },
    _method{ nullptr },
    _scopeID{ 0 },
    _lite{ false },
    _senderScopeOffset{ 0 },
    _senderByteCodeIndex{ 0 },
    _allocatesCompiledContext{ false },
    _next{ 0 },
    _scopes{ scopes },
    _offset{ offset },
    _pc{ pc },
    _name_desc_offset{ offset } {

    //
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


ScopeDescriptor *ScopeDescriptor::home( bool cross_NativeMethod_boundary ) const {
    ScopeDescriptor *p = (ScopeDescriptor *) this;
    for ( ; p and not p->isMethodScope(); p = p->parent( cross_NativeMethod_boundary ) );
    return p;
}


NameDescriptor *ScopeDescriptor::temporary( std::int32_t index, bool canFail ) {
    std::int32_t   pos     = _name_desc_offset;
    NameDescriptor *result = nullptr;

    if ( _hasTemporaries ) {
        NameDescriptor *current = nameDescAt( pos );
        std::int32_t   i        = 0;
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


NameDescriptor *ScopeDescriptor::contextTemporary( std::int32_t index, bool canFail ) {
    std::int32_t   pos     = _name_desc_offset;
    NameDescriptor *result = nullptr;

    if ( _hasTemporaries ) {
        NameDescriptor *current = nameDescAt( pos );
        while ( current ) {
            current = nameDescAt( pos );
        }
    }

    if ( _hasContextTemporaries ) {
        NameDescriptor *current = nameDescAt( pos );
        std::int32_t   i        = 0;
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


NameDescriptor *ScopeDescriptor::exprStackElem( std::int32_t byteCodeIndex ) {
    std::int32_t pos = _name_desc_offset;

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
            std::int32_t the_byteCodeIndex = valueAt( pos );
            if ( byteCodeIndex == the_byteCodeIndex )
                return current;
            current = nameDescAt( pos );
        }
    }

    return nullptr;
}


void ScopeDescriptor::iterate( NameDescriptorClosure *blk ) {
    std::int32_t pos = _name_desc_offset;

    if ( _hasTemporaries ) {
        NameDescriptor *current = nameDescAt( pos );
        std::int32_t   number   = 0;
        while ( current ) {
            blk->temp( number++, current, pc() );
            current = nameDescAt( pos );
        }
    }

    if ( _hasContextTemporaries ) {
        NameDescriptor *current = nameDescAt( pos );
        std::int32_t   number   = 0;
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
    std::int32_t          _no;
    NameDescriptorClosure *_blk;
    bool                  _is_used;


    void use() {
        _is_used = true;
    }


    IterationHelper() : UnpackClosure(), _no{ 0 }, _blk{ nullptr }, _is_used{ false } {

    }


public:
    void init( std::int32_t no, NameDescriptorClosure *blk ) {
        _no      = no;
        _blk     = blk;
        _is_used = false;
    }


    bool is_used() const {
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
    std::int32_t pos = _name_desc_offset;
    if ( _hasTemporaries ) {
        std::int32_t no = 0;
        IH_temp      helper;
        do {
            helper.init( no++, blk );
            _scopes->iterate( pos, &helper );
        } while ( helper.is_used() );
    }

    if ( _hasContextTemporaries ) {
        std::int32_t    no = 0;
        IH_context_temp helper;
        do {
            helper.init( no++, blk );
            _scopes->iterate( pos, &helper );
        } while ( helper.is_used() );
    }

    if ( _hasExpressionStack ) {
        std::int32_t  no = 0;
        IH_stack_expr helper;
        do {
            helper.init( no++, blk );
            _scopes->iterate( pos, &helper );
            if ( helper.is_used() )
                valueAt( pos ); // get byteCodeIndex (i.e., print-out is showing expr. index and not byteCodeIndex)
        } while ( helper.is_used() );
    }
}


bool ScopeDescriptor::allocates_interpreted_context() const {
    return method()->allocatesInterpretedContext();
}


NameDescriptor *ScopeDescriptor::compiled_context() {
    st_assert( allocates_compiled_context(), "must allocate a context" );
    constexpr std::int32_t temporary_index_for_context = 0;
    return temporary( temporary_index_for_context );
}


bool ScopeDescriptor::s_equivalent( ScopeDescriptor *s ) const {
    return method() == s->method() and ( _senderByteCodeIndex == s->_senderByteCodeIndex or _senderByteCodeIndex < 0 or s->_senderByteCodeIndex < 0 );
    // don't check senderByteCodeIndex for pseudo ByteCodeIndexs
}


bool ScopeDescriptor::l_equivalent( LookupKey *l ) const {
    return selector() == l->selector();
}


ScopeDescriptor *ScopeDescriptor::sender() const {
    return _senderScopeOffset ? _scopes->at( _offset - _senderScopeOffset, pc() ) : nullptr;
}


NameDescriptor *ScopeDescriptor::nameDescAt( std::int32_t &offset ) const {
    return _scopes->unpackNameDescAt( offset, pc() );
}


std::int32_t ScopeDescriptor::valueAt( std::int32_t &offset ) const {
    return _scopes->unpackValueAt( offset );
}


bool ScopeDescriptor::verify() {
    // verifies mostly structure, not contents
    bool ok = true;

    // don't do a full verify of parent/sender -- too many redundant verifies
    ScopeDescriptor *s = sender();
    if ( s and not s->shallow_verify() ) {
        _console->print( "invalid sender 0x{0:x} of ScopeDescriptor 0x{0:x}", s, this );
        ok = false;
    }
    ScopeDescriptor *p = parent();
    if ( p and not p->shallow_verify() ) {
        _console->print( "invalid parent 0x{0:x} of ScopeDescriptor 0x{0:x}", p, this );
        ok = false;
    }
    return ok;
}


// verify expression stack at a call or primitive call
void ScopeDescriptor::verify_expression_stack( std::int32_t byteCodeIndex ) {
    GrowableArray<std::int32_t> *mapping = method()->expression_stack_mapping( byteCodeIndex );
    for ( std::int32_t          index    = 0; index < mapping->length(); index++ ) {
        NameDescriptor *nd = exprStackElem( mapping->at( index ) );
        if ( nd == nullptr ) {
            spdlog::warn( "expression not found in NativeMethod" );
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
    std::int32_t _indent;
    char         *_pc0;


    void print( const char *title, std::int32_t no, NameDescriptor *nd, char *pc ) {
        _console->fill_to( _indent );
        if ( UseNewBackend ) {
            _console->print( "%5d: ", pc - _pc0 );
        }
        _console->print( "%s[%d]\t", title, no );
        nd->print();
        _console->cr();
    }


public:
    PrintNameDescClosure( std::int32_t indent, char *pc0 ) :
        _indent{ indent },
        _pc0{ pc0 } {
    }


    void arg( std::int32_t no, NameDescriptor *a, char *pc ) {
        print( "arg   ", no, a, pc );
    }


    void temp( std::int32_t no, NameDescriptor *t, char *pc ) {
        print( "temp  ", no, t, pc );
    }


    void context_temp( std::int32_t no, NameDescriptor *c, char *pc ) {
        print( "c_temp", no, c, pc );
    }


    void stack_expr( std::int32_t no, NameDescriptor *e, char *pc ) {
        print( "expr  ", no, e, pc );
    }
};


void ScopeDescriptor::print( std::int32_t indent, bool all_pcs ) {
    _console->fill_to( indent );
    printName();
    _console->print( "ScopeDescriptor @%d%s: ", offset(), is_lite() ? ", lite" : "" );
    _console->print( " (ID %ld) ", scopeID() );
    method()->selector()->print_symbol_on();
    _console->print( " 0x{0:x}", method() );
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


bool MethodScopeDescriptor::s_equivalent( ScopeDescriptor *s ) const {
    return s->isMethodScope() and ScopeDescriptor::s_equivalent( s ) and key()->equal( ( (MethodScopeDescriptor *) s )->key() );
}


bool MethodScopeDescriptor::l_equivalent( LookupKey *l ) const {
    return ScopeDescriptor::l_equivalent( l ) and selfKlass() == l->klass();
}


MethodScopeDescriptor::MethodScopeDescriptor( NativeMethodScopes *scopes, std::int32_t offset, const char *pc ) :
    ScopeDescriptor( scopes, offset, pc ),
//    _next{ 0 },
//    _name_desc_offset{ 0 },
    _self_name{ nullptr },
    _key{ nullptr } {

    Oop k = _scopes->unpackOopAt( _name_desc_offset );
    Oop s = _scopes->unpackOopAt( _name_desc_offset );
    _key.initialize( (KlassOop) k, s );
    _self_name = _scopes->unpackNameDescAt( _name_desc_offset, pc );
    if ( _next == -1 ) {
        _next = _name_desc_offset;
    }

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


BlockScopeDescriptor::BlockScopeDescriptor( const NativeMethodScopes *scopes, std::int32_t offset, const char *pc ) :
    ScopeDescriptor( scopes, offset, pc ),
    _parentScopeOffset{ 0 } {

    _parentScopeOffset = _scopes->unpackValueAt( _name_desc_offset );

    if ( _next == -1 ) {
        _next = _name_desc_offset;
    }

}


bool BlockScopeDescriptor::s_equivalent( ScopeDescriptor *s ) const {
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


ScopeDescriptor *BlockScopeDescriptor::parent( bool cross_NativeMethod_boundary ) const {
    static_cast<void>(cross_NativeMethod_boundary); // unused
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


NonInlinedBlockScopeDescriptor::NonInlinedBlockScopeDescriptor( const NativeMethodScopes *scopes, std::int32_t offset ) :
    _method{},
    _parentScopeOffset{ 0 },
    _offset{ offset },
    _scopes{ scopes } {

    ScopeDescriptorHeaderByte b;
    b.unpack( _scopes->get_next_char( offset ) );
    _method            = MethodOop( scopes->unpackOopAt( offset ) );
    _parentScopeOffset = scopes->unpackValueAt( offset );
}


void NonInlinedBlockScopeDescriptor::print() {
    spdlog::info( "NonInlinedBlockScopeDescriptor" );
    _console->print( " - method: " );
    method()->print_value();
    _console->cr();
    spdlog::info( " - parent offset: {}", _parentScopeOffset );
}


ScopeDescriptor *NonInlinedBlockScopeDescriptor::parent() const {
    return _parentScopeOffset ? _scopes->at( _offset - _parentScopeOffset, ScopeDescriptor::invalid_pc ) : nullptr;
}


TopLevelBlockScopeDescriptor::TopLevelBlockScopeDescriptor( const NativeMethodScopes *scopes, std::int32_t offset, const char *pc ) :
    ScopeDescriptor( scopes, offset, pc ),
    _self_klass{ nullptr },
    _self_name{ nullptr } {

    _self_name  = _scopes->unpackNameDescAt( _name_desc_offset, pc );
    _self_klass = KlassOop( scopes->unpackOopAt( _name_desc_offset ) );

    if ( _next == -1 ) {
        _next = _name_desc_offset;
    }

}


void TopLevelBlockScopeDescriptor::printSelf() {
    ScopeDescriptor::printSelf();
    _console->print( "self: " );
    self()->print();
    _console->cr();
}


ScopeDescriptor *TopLevelBlockScopeDescriptor::parent( bool cross_NativeMethod_boundary ) const {
    if ( not cross_NativeMethod_boundary )
        return nullptr;
    NativeMethod                   *nm     = _scopes->my_nativeMethod();
    std::int32_t                   index;
    NativeMethod                   *parent = nm->jump_table_entry()->parent_nativeMethod( index );
    NonInlinedBlockScopeDescriptor *scope  = parent->noninlined_block_scope_at( index );
    return scope->parent();
}


bool TopLevelBlockScopeDescriptor::s_equivalent( ScopeDescriptor *s ) const {
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
    if ( self_klass == trueObject->klass() )
        return new ConstantExpression( trueObject, p, nullptr );
    if ( self_klass == falseObject->klass() )
        return new ConstantExpression( falseObject, p, nullptr );
    if ( self_klass == nilObject->klass() )
        return new ConstantExpression( nilObject, p, nullptr );
    return new KlassExpression( self_klass, p, nullptr );
}
