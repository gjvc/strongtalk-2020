//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/code/PolymorphicInlineCache.hpp"
#include "vm/code/StubRoutines.hpp"
#include "vm/utilities/EventLog.hpp"
#include "vm/lookup/LookupCache.hpp"
#include "vm/lookup/LookupKey.hpp"
#include "vm/oops/SymbolOopDescriptor.hpp"
#include "vm/oops/KlassOopDescriptor.hpp"
#include "vm/runtime/ResourceMark.hpp"

// A PolymorphicInlineCache implements a Polymorphic Inline Cache for compiled code.
//
// The layout comes in 3 variants:
//
// a) PICs that contain only m methodOop entries,
// b) PICs that contain both, n NativeMethod and m methodOop entries
// c) MICs (Megamorphic Inline Caches) which keep only the selector so lookup is fast.
//
// The 3 formats can be distinguished by looking at
// the first instruction.
//
// Layout a)		(methodOop entries only)
//
//  0			: call PolymorphicInlineCache stub routine
//  5			: m methodOop entries a 8 bytes each
//  5 + m*8		: <end of PolymorphicInlineCache>
//
// i.th methodOop entry (0 <= i < m; m > 0)
//
//  5 + i*8		: klass(i)
//  9 + i*8		: methodOop(i)
//
//
// Layout b)		(NativeMethod & methodOop entries)
//
//  0			: test al, MEMOOP_TAG
//  2			: jz smi_nativeMethod/jz methodOop call stub/jz cache_miss
//  8			: mov edx, [eax.klass] (always here to simplify iteration, even if n = 0)
// 11			: n NativeMethod entries a 12 bytes each
// 11 + n*12		: call PolymorphicInlineCache stub routine (m > 0)/jmp cache_miss (m = 0)
// 16 + n*12		: m methodOop entries a 8 bytes each
// 16 + n*12 + m*8	: <end of PolymorphicInlineCache>
//
// i.th NativeMethod entry (0 <= i < n; n >= 0):
//
// 11 + i*12		: cmp edx, klass(i)
// 11 + i*12 + 6	: je NativeMethod(i)
//
// i.th methodOop (0 <= i < m; m >= 0):
//
// 16 + n*12 + i*8	: klass
// 16 + n*12 + i*8 + 4	: methodOop
//
//
// Layout c)		(MEGAMORPHIC inline cache, selector only)
//
//  0			: call MIC stub routine
//  5			: selector
//  9			: <end of MIC>
//
// The PolymorphicInlineCache stub routine interprets the remaining entries of the PolymorphicInlineCache; there
// are different stub routines for different m (starting point for interpretation
// is the return address). Entries for smis are treated especially in the sense
// that an initial check for them is always there.
//
// NB: The reason for interpreting the interpreter entries is Space, a smallest
// native code implementation for these cases requires 18 bytes per entry (cmp,
// je and load of methodOop). Furthermore, in each case it would be necessary
// to call a stub routine to setup an interpreter frame anyway.
//
// The MIC stub routine interprets the one entry in the MIC. In case of a miss,
// no new PolymorphicInlineCache/MIC is generated but the current entries are updated. The reason
// for having a specialized PolymorphicInlineCache allocated in the MIC case is only so the selector
// can be stored. It is used for fast lookup (usually the selector is recomputed
// via debug-info from the corresponding interpreted method).


// Opcodes for code pattern generation/parsing
static const char             test_opcode     = '\xa8';
static const char             call_opcode     = '\xe8';
static const char             jmp_opcode      = '\xe9';
static const std::uint16_t    jz_opcode       = 0x840f;
static const std::uint16_t    mov_opcode      = 0x508b;
static const std::uint16_t    cmp_opcode      = 0xfa81;
static constexpr std::int32_t cmp_opcode_size = sizeof( std::uint16_t );


// -----------------------------------------------------------------------------

// Helper routines for code pattern generation/parsing
static inline void put_byte( char *&p, std::uint8_t b ) {
    *p++ = b;
}


static inline void put_shrt( char *&p, std::uint16_t s ) {
    *(std::uint16_t *) p = s;
    p += sizeof( std::uint16_t );
}


static inline void put_word( char *&p, std::int32_t w ) {
    *(std::int32_t *) p = w;
    p += sizeof( std::int32_t );
}


static inline void put_disp( char *&p, const char *d ) {
    put_word( p, (std::int32_t) ( d - p - sizeof( std::int32_t ) ) );
}


// -----------------------------------------------------------------------------

static inline std::int32_t get_shrt( const char *p ) {
    return *(std::uint16_t *) p;
}


static inline const char *get_disp( const char *p ) {
    return *(std::int32_t *) p + p + sizeof( std::int32_t );
}


// -----------------------------------------------------------------------------


// Structure for storing the entries of a PolymorphicInlineCache
class PolymorphicInlineCacheContents {
public:
    // smi_t case
    char      *smi_nativeMethod;
    MethodOop smi_methodOop;

    // NativeMethod entries
    KlassOop     nativeMethod_klasses[static_cast<std::int32_t>(PolymorphicInlineCache::Constant::max_nof_entries)];
    char         *nativeMethods[static_cast<std::int32_t>(PolymorphicInlineCache::Constant::max_nof_entries)];
    std::int32_t n;    // nativeMethods index

    // methodOop entries
    KlassOop     methodOop_klasses[static_cast<std::int32_t>(PolymorphicInlineCache::Constant::max_nof_entries)];
    MethodOop    methodOops[static_cast<std::int32_t>(PolymorphicInlineCache::Constant::max_nof_entries)];
    std::int32_t m;    // methodOops index

    void append_NativeMethod_entry( KlassOop klass, char *entry );

    void append_method( KlassOop klass, MethodOop method );


    std::int32_t number_of_compiled_targets() const {
        return ( smi_nativeMethod ? 1 : 0 ) + n;
    }


    std::int32_t number_of_interpreted_targets() const {
        return ( smi_methodOop ? 1 : 0 ) + m;
    }


    std::int32_t number_of_targets() const {
        return number_of_compiled_targets() + number_of_interpreted_targets();
    }


    bool has_smi_case() const {
        return ( smi_methodOop not_eq nullptr ) or ( smi_nativeMethod not_eq nullptr );
    }


    bool has_nativeMethods() const {
        return ( n > 0 ) or ( smi_nativeMethod not_eq nullptr );
    }


    std::int32_t code_size() const {
        std::int32_t methodOop_size = number_of_interpreted_targets() * static_cast<std::int32_t>( PolymorphicInlineCache::Constant::PolymorphicInlineCache_methodOop_entry_size );
        if ( has_nativeMethods() ) {
            return static_cast<std::int32_t>( PolymorphicInlineCache::Constant::PolymorphicInlineCache_methodOop_entry_offset ) + n * static_cast<std::int32_t>( PolymorphicInlineCache::Constant::PolymorphicInlineCache_NativeMethod_entry_size ) + methodOop_size;
        } else {
            return static_cast<std::int32_t>( PolymorphicInlineCache::Constant::PolymorphicInlineCache_methodOop_only_offset ) + methodOop_size;
        }
    }


    //
    PolymorphicInlineCacheContents() :
        smi_nativeMethod{ nullptr },
        smi_methodOop{ nullptr },
        n{ 0 },
        m{ 0 } {
    }


};


void PolymorphicInlineCacheContents::append_NativeMethod_entry( KlassOop klass, char *entry ) {
    // add new entry
    if ( klass == smiKlassObject ) {
        st_assert( not has_smi_case(), "cannot overwrite smi_t case" );
        smi_nativeMethod = entry;
    } else {
        nativeMethod_klasses[ n ] = klass;
        nativeMethods[ n ]        = entry;
        n++;
    }
}


void PolymorphicInlineCacheContents::append_method( KlassOop klass, MethodOop method ) {
    // add new entry
    st_assert( method->is_method(), "must be methodOop" );
    if ( klass == smiKlassObject ) {
        st_assert( not has_smi_case(), "cannot overwrite smi_t case" );
        smi_methodOop = method;
    } else {
        methodOop_klasses[ m ] = klass;
        methodOops[ m ]        = method;
        m++;
    }
}


// Implementation of PolymorphicInlineCache_Iterators

PolymorphicInlineCacheIterator::PolymorphicInlineCacheIterator( PolymorphicInlineCache *pic ) :

    _methodOop_counter{ 0 },
    _state{},
    _pic{ pic },
    _pos{ nullptr } {

    //
    _pos = pic->entry();

    // determine initial state
    if ( pic->is_megamorphic() ) {
        // MIC -> do not use cached information
        st_assert( get_disp( _pos + 1 ) == StubRoutines::megamorphic_ic_entry(), "MIC stub expected" );
        _state = InlineState::AT_THE_END;

    } else if ( *_pos == call_opcode ) {
        // PolymorphicInlineCache without nativeMethods
        _state             = InlineState::AT_METHOD_OOP;
        _methodOop_counter = PolymorphicInlineCache::nof_entries( get_disp( _pos + 1 ) );
        _pos += static_cast<std::int32_t>(PolymorphicInlineCache::Constant::PolymorphicInlineCache_methodOop_only_offset);

    } else {
        // nativeMethods -> handle smis first
        const char *dest = get_disp( _pos + static_cast<std::int32_t>( PolymorphicInlineCache::Constant::PolymorphicInlineCache_smi_nativeMethodOffset ) );
        if ( dest == CompiledInlineCache::normalLookupRoutine() or _pic->contains( dest ) ) {
            // no smis or smi_t case is treated in methodOop section
            _state = InlineState::AT_NATIVE_METHOD;
            _pos += static_cast<std::int32_t>( PolymorphicInlineCache::Constant::PolymorphicInlineCache_NativeMethod_entry_offset );
        } else {
            // smi_t entry is treated here
            _state = InlineState::AT_SMI_NATIVE_METHOD;
        }
    }
}


void PolymorphicInlineCacheIterator::computeNextState() {
    if ( get_shrt( _pos ) == cmp_opcode ) {
        // same state
    } else if ( *_pos == call_opcode ) {
        _state             = InlineState::AT_METHOD_OOP;
        _methodOop_counter = PolymorphicInlineCache::nof_entries( get_disp( _pos + 1 ) );
        _pos += static_cast<std::int32_t>( PolymorphicInlineCache::Constant::PolymorphicInlineCache_methodOop_entry_offset ) - static_cast<std::int32_t>( PolymorphicInlineCache::Constant::PolymorphicInlineCache_NativeMethod_entry_offset );
    } else {
        st_assert( *_pos == jmp_opcode, "jump to lookup routine expected" );
        _state = InlineState::AT_THE_END;
    }
}


void PolymorphicInlineCacheIterator::advance() {
    switch ( _state ) {
        case InlineState::AT_SMI_NATIVE_METHOD:
            st_assert( _pos == _pic->entry(), "must be at beginning" );
            _pos += static_cast<std::int32_t>( PolymorphicInlineCache::Constant::PolymorphicInlineCache_NativeMethod_entry_offset );
            _state = InlineState::AT_NATIVE_METHOD;
            computeNextState();
            break;
        case InlineState::AT_NATIVE_METHOD:
            _pos += static_cast<std::int32_t>( PolymorphicInlineCache::Constant::PolymorphicInlineCache_NativeMethod_entry_size );
            computeNextState();
            break;
        case InlineState::AT_METHOD_OOP:
            if ( --_methodOop_counter > 0 ) {
                _pos += static_cast<std::int32_t>( PolymorphicInlineCache::Constant::PolymorphicInlineCache_methodOop_entry_size );
            } else {
                _state = InlineState::AT_THE_END;
            }
            break;
        case InlineState::AT_THE_END: ShouldNotCallThis();
        default: ShouldNotReachHere();
    }
}


KlassOop *PolymorphicInlineCacheIterator::klass_addr() const {
    std::int32_t offs{ 0 };
    switch ( state() ) {
        case InlineState::AT_SMI_NATIVE_METHOD: ShouldNotCallThis();            // no klass stored -> no klass address available
        case InlineState::AT_NATIVE_METHOD:
            offs = static_cast<std::int32_t>(PolymorphicInlineCache::Constant::PolymorphicInlineCache_NativeMethod_klass_offset);
            break;
        case InlineState::AT_METHOD_OOP:
            offs = static_cast<std::int32_t>(PolymorphicInlineCache::Constant::PolymorphicInlineCache_methodOop_klass_offset);
            break;
        case InlineState::AT_THE_END: ShouldNotCallThis();            // no klass stored -> no klass address available
        default: ShouldNotReachHere();
    }
    return (KlassOop *) ( _pos + offs );
}


std::int32_t *PolymorphicInlineCacheIterator::nativeMethod_disp_addr() const {
    std::int32_t offs{ 0 };
    switch ( state() ) {
        case InlineState::AT_SMI_NATIVE_METHOD:
            offs = static_cast<std::int32_t>(PolymorphicInlineCache::Constant::PolymorphicInlineCache_smi_nativeMethodOffset);
            break;
        case InlineState::AT_NATIVE_METHOD:
            offs = static_cast<std::int32_t>(PolymorphicInlineCache::Constant::PolymorphicInlineCache_nativeMethodOffset);
            break;
        case InlineState::AT_METHOD_OOP: ShouldNotCallThis();            // no NativeMethod stored -> no NativeMethod address available
        case InlineState::AT_THE_END: ShouldNotCallThis();            // no NativeMethod stored -> no NativeMethod address available
        default: ShouldNotReachHere();
    }
    return (std::int32_t *) ( _pos + offs );
}


MethodOop *PolymorphicInlineCacheIterator::methodOop_addr() const {
    std::int32_t offs{ 0 };
    switch ( state() ) {
        case InlineState::AT_SMI_NATIVE_METHOD: ShouldNotCallThis();            // no methodOop stored -> no methodOop address available
        case InlineState::AT_NATIVE_METHOD    : ShouldNotCallThis();            // no methodOop stored -> no methodOop address available
        case InlineState::AT_METHOD_OOP:
            offs = static_cast<std::int32_t>(PolymorphicInlineCache::Constant::PolymorphicInlineCache_methodOop_offset);
            break;
        case InlineState::AT_THE_END    : ShouldNotCallThis();
        default            : ShouldNotReachHere();
    }
    return (MethodOop *) ( _pos + offs );
}


void PolymorphicInlineCacheIterator::print() {
    SPDLOG_INFO( "a PolymorphicInlineCacheIterator" );
}


// Implementation of PICs

bool PolymorphicInlineCache::in_heap( const char *addr ) {
    return Universe::code->_picHeap->contains( addr );
}


PolymorphicInlineCache *PolymorphicInlineCache::find( const char *addr ) {
    if ( Universe::code->_picHeap->contains( addr ) ) {
        PolymorphicInlineCache *result = (PolymorphicInlineCache *) Universe::code->_picHeap->findStartOfBlock( addr );
        return result;
    }
    return nullptr;
}


// Accessing PolymorphicInlineCache entries
KlassOop PolymorphicInlineCacheIterator::get_klass() const {
    return state() == InlineState::AT_SMI_NATIVE_METHOD ? smiKlassObject : *klass_addr();
}


char *PolymorphicInlineCacheIterator::get_call_addr() const {
    std::int32_t *a = nativeMethod_disp_addr();
    return (char *) a + sizeof( std::int32_t ) + *a;
}


bool PolymorphicInlineCacheIterator::is_compiled() const {
    switch ( state() ) {
        case InlineState::AT_SMI_NATIVE_METHOD:
            return true;
        case InlineState::AT_NATIVE_METHOD:
            return true;
        case InlineState::AT_METHOD_OOP:
            return false;
        case InlineState::AT_THE_END: ShouldNotCallThis();
        default: ShouldNotReachHere();
    }
    return false;
}


bool PolymorphicInlineCacheIterator::is_interpreted() const {
    return not is_compiled();
}


NativeMethod *PolymorphicInlineCacheIterator::compiled_method() const {
    if ( not is_compiled() ) {
        return nullptr;
    }
    return findNativeMethod( get_call_addr() - sizeof( NativeMethod ) );
}


MethodOop PolymorphicInlineCacheIterator::interpreted_method() const {
    if ( is_interpreted() ) {
        return *methodOop_addr();
    } else {
        return compiled_method()->method();
    }
}


// Modifying PolymorphicInlineCache entries
void PolymorphicInlineCacheIterator::set_klass( KlassOop klass ) {
    st_assert( state() not_eq InlineState::AT_SMI_NATIVE_METHOD, "cannot be set" );
    *klass_addr() = klass;
}


void PolymorphicInlineCacheIterator::set_nativeMethod( NativeMethod *nm ) {
    st_assert( get_klass() == nm->_lookupKey.klass(), "mismatched receiver klass" );
    std::int32_t *a = nativeMethod_disp_addr();
    *a = nm->verifiedEntryPoint() - (const char *) a - sizeof( std::int32_t );
}


void PolymorphicInlineCacheIterator::set_methodOop( MethodOop method ) {
    *methodOop_addr() = method;
}


SymbolOop *PolymorphicInlineCache::MegamorphicInlineCache_selector_address() const {
    st_assert( is_megamorphic(), "not a MIC" );
    return (SymbolOop *) ( entry() + static_cast<std::int32_t>( PolymorphicInlineCache::Constant::MegamorphicInlineCache_selector_offset ) );
}


PolymorphicInlineCache *PolymorphicInlineCache::replace( NativeMethod *nm ) {
    // nothing to do in MEGAMORPHIC case
    if ( is_megamorphic() )
        return this;

    SPDLOG_INFO( "compiled PolymorphicInlineCache at 0x{0:x}: new NativeMethod 0x{0:x} for klass 0x{0:x} replaces old entry",
                  static_cast<const void *>( this ),
                  static_cast<const void *>( nm ),
                  static_cast<const void *>( nm->_lookupKey.klass() ) );

    { // do the replace without creating a new PolymorphicInlineCache if possible
        PolymorphicInlineCacheIterator it( this );
        while ( it.get_klass() not_eq nm->_lookupKey.klass() ) {
            it.advance();
        }

        st_assert( not it.at_end(), "unexpected end during replace" );
        if ( it.is_compiled() ) {
            it.set_nativeMethod( nm );
            return this;
        }
    }

    { // Create a new PolymorphicInlineCache
        PolymorphicInlineCacheContents contents;
        PolymorphicInlineCacheIterator it( this );
        while ( not it.at_end() ) {
            KlassOop receiver_klass = it.get_klass();
            if ( receiver_klass == nm->_lookupKey.klass() ) {
                contents.append_NativeMethod_entry( nm->_lookupKey.klass(), nm->verifiedEntryPoint() );
            } else {
                if ( it.is_interpreted() ) {
                    contents.append_method( it.get_klass(), it.interpreted_method() );
                } else {
                    contents.append_NativeMethod_entry( it.get_klass(), it.get_call_addr() );
                }
            }
            it.advance();
        }

        std::int32_t allocated_code_size = contents.code_size();

        return new( allocated_code_size ) PolymorphicInlineCache( _ic, &contents, allocated_code_size );
    }
}


PolymorphicInlineCache *PolymorphicInlineCache::cleanup( NativeMethod **nm ) {
    // nothing to do in MEGAMORPHIC case
    if ( is_megamorphic() )
        return this;

    bool pic_layout_has_changed = false;

    // Iterate over the PolymorphicInlineCache and
    //  - patch the PolymorphicInlineCache if possible
    //  - check if the layout has changed
    //  - collect the PolymorphicInlineCache information.

    PolymorphicInlineCacheContents contents;
    PolymorphicInlineCacheIterator it( this );
    while ( not it.at_end() ) {
//        KlassOop receiver_klass = it.get_klass();

        if ( it.is_interpreted() ) {
            // Interpreted methodOop
            if ( compiled_ic()->isSuperSend() ) {
                contents.append_method( it.get_klass(), it.interpreted_method() );
            } else {
                LookupKey    key( it.get_klass(), it.interpreted_method()->selector() );
                LookupResult result = LookupCache::lookup( &key );
                if ( result.matches( it.interpreted_method() ) ) {
                    contents.append_method( it.get_klass(), it.interpreted_method() );
                } else {
                    if ( result.is_method() ) {
                        contents.append_method( it.get_klass(), result.method() );
                        it.set_methodOop( result.method() );
                    } else if ( result.is_entry() ) {
                        contents.append_NativeMethod_entry( it.get_klass(), result.get_nativeMethod()->verifiedEntryPoint() );
                        pic_layout_has_changed = true;
                    } else {
                        pic_layout_has_changed = true;
                    }
                }
            }
        } else {
            // Compiled NativeMethod
            NativeMethod *nm    = it.compiled_method();
            LookupResult result = LookupCache::lookup( &nm->_lookupKey );
            if ( result.matches( nm ) ) {
                contents.append_NativeMethod_entry( it.get_klass(), it.get_call_addr() );
            } else {
                if ( result.is_method() ) {
                    contents.append_method( it.get_klass(), result.method() );
                    pic_layout_has_changed = true;
                } else if ( result.is_entry() ) {
                    contents.append_NativeMethod_entry( it.get_klass(), result.get_nativeMethod()->verifiedEntryPoint() );
                    it.set_nativeMethod( result.get_nativeMethod() );
                } else {
                    pic_layout_has_changed = true;
                }
            }
        }
        it.advance();
    }

    *nm = nullptr;
    if ( pic_layout_has_changed ) {

        if ( contents.number_of_targets() == 0 ) {
            // no targets left
            return nullptr;
        }

        if ( contents.number_of_targets() == 1 and contents.has_nativeMethods() ) {
            // 1 NativeMethod target
            *nm = findNativeMethod( contents.has_smi_case() ? contents.smi_nativeMethod : contents.nativeMethods[ 0 ] );
            st_assert( *nm, "NativeMethod must be present" );
            return nullptr;
        }

        std::int32_t allocated_code_size = contents.code_size();
        return new( allocated_code_size ) PolymorphicInlineCache( _ic, &contents, allocated_code_size );
    }

    // The layout has not changed so return the patched self
    return this;
}


std::int32_t PolymorphicInlineCache::nof_entries( const char *pic_stub ) {
    std::int32_t i = 1;
    while ( true ) {
        if ( pic_stub == StubRoutines::PolymorphicInlineCache_stub_entry( i ) ) {
            return i;
        }
        i++;
    }
    ShouldNotReachHere();
    return 0;
}


std::int32_t PolymorphicInlineCache::code_for_methodOops_only( const char *entry, PolymorphicInlineCacheContents *c ) {
    char *p = const_cast<char *>(entry);
    put_byte( p, call_opcode );
    if ( c->smi_methodOop == nullptr ) {
        // no smi_t methodOop
        put_disp( p, StubRoutines::PolymorphicInlineCache_stub_entry( c->m ) );
        st_assert( entry + static_cast<std::int32_t>( PolymorphicInlineCache::Constant::PolymorphicInlineCache_methodOop_only_offset ) == p, "constant inconsistent" );

    } else {
        // handle smi_t methodOop first
        put_disp( p, StubRoutines::PolymorphicInlineCache_stub_entry( 1 + c->m ) );
        put_word( p, std::int32_t( smiKlassObject ) );
        put_word( p, std::int32_t( c->smi_methodOop ) );
        st_assert( entry + static_cast<std::int32_t>( PolymorphicInlineCache::Constant::PolymorphicInlineCache_methodOop_only_offset ) + static_cast<std::int32_t>( PolymorphicInlineCache::Constant::PolymorphicInlineCache_methodOop_entry_size ) == p, "constant value inconsistent with code pattern" );

    }

    char               *p1 = p;
    for ( std::int32_t i   = 0; i < c->m; i++ ) {
        st_assert( c->methodOop_klasses[ i ] not_eq smiKlassObject, "should not be smiKlassObject" );
        put_word( p, std::int32_t( c->methodOop_klasses[ i ] ) );
        put_word( p, std::int32_t( c->methodOops[ i ] ) );
    }

    st_assert( p1 + c->m * static_cast<std::int32_t>( PolymorphicInlineCache::Constant::PolymorphicInlineCache_methodOop_entry_size ) == p, "constant value inconsistent with code pattern" );
    return p - entry;
}


std::int32_t PolymorphicInlineCache::code_for_polymorphic_case( char *entry, PolymorphicInlineCacheContents *c ) {

    if ( c->has_nativeMethods() ) {
        // nativeMethods & methodOops
        // test al, MEMOOP_TAG

        char *p     = entry;
        char *fixup = nullptr;
        put_byte( p, test_opcode );
        put_byte( p, MEMOOP_TAG );
        // jz ...
        put_shrt( p, jz_opcode );
        if ( c->smi_nativeMethod not_eq nullptr ) {
            st_assert( c->smi_methodOop == nullptr, "can only have one method for smis" );
            put_disp( p, c->smi_nativeMethod );
        } else if ( c->smi_methodOop not_eq nullptr ) {
            // smi_t method is methodOop -> handle it in methodOop section
            fixup = p;
            put_disp( p, 0 );
        } else {
            // no smi_t entries
            put_disp( p, const_cast<char *>( CompiledInlineCache::normalLookupRoutine() ) ); // Fix this for super sends!
        }
        // always load klass to simplify decoding/iteration of PolymorphicInlineCache
        // mov edx, [eax.klass]
        put_shrt( p, mov_opcode );
        put_byte( p, MemOopDescriptor::klass_byte_offset() );
        st_assert( entry + static_cast<std::int32_t>( PolymorphicInlineCache::Constant::PolymorphicInlineCache_NativeMethod_entry_offset ) == p, "constant value inconsistent with code pattern" );
        // handle nativeMethods
        for ( std::int32_t i = 0; i < c->n; i++ ) {
            // cmp edx, klass(i)
            st_assert( c->nativeMethod_klasses[ i ] not_eq smiKlassObject, "should not be smiKlassObject" );
            put_shrt( p, cmp_opcode );
            st_assert( entry + static_cast<std::int32_t>( PolymorphicInlineCache::Constant::PolymorphicInlineCache_NativeMethod_entry_offset ) + i * static_cast<std::int32_t>( PolymorphicInlineCache::Constant::PolymorphicInlineCache_NativeMethod_entry_size ) + static_cast<std::int32_t>( PolymorphicInlineCache::Constant::PolymorphicInlineCache_NativeMethod_klass_offset ) == p, "constant value inconsistent with code pattern" );
            put_word( p, std::int32_t( c->nativeMethod_klasses[ i ] ) );
            // je NativeMethod(j)
            put_shrt( p, jz_opcode );
            st_assert( entry + static_cast<std::int32_t>( PolymorphicInlineCache::Constant::PolymorphicInlineCache_NativeMethod_entry_offset ) + i * static_cast<std::int32_t>( PolymorphicInlineCache::Constant::PolymorphicInlineCache_NativeMethod_entry_size ) + static_cast<std::int32_t>( PolymorphicInlineCache::Constant::PolymorphicInlineCache_nativeMethodOffset ) == p, "constant value inconsistent with code pattern" );
            put_disp( p, c->nativeMethods[ i ] );
        }
        st_assert( entry + static_cast<std::int32_t>( PolymorphicInlineCache::Constant::PolymorphicInlineCache_NativeMethod_entry_offset ) + c->n * static_cast<std::int32_t>( PolymorphicInlineCache::Constant::PolymorphicInlineCache_NativeMethod_entry_size ) == p, "constant value inconsistent with code pattern" );
        if ( c->smi_methodOop not_eq nullptr or c->m > 0 ) {
            // handle methodOops
            if ( fixup not_eq nullptr )
                put_disp( fixup, p );
            p += code_for_methodOops_only( p, c );
        } else {
            // jmp cache_miss
            put_byte( p, jmp_opcode );
            put_disp( p, CompiledInlineCache::normalLookupRoutine() );
        }
        return p - entry;
    } else {
        // no nativeMethods -> call PolymorphicInlineCache stub routine directly
        return code_for_methodOops_only( entry, c );
    }
}


std::int32_t PolymorphicInlineCache::code_for_megamorphic_case( char *entry ) {

    char *p = entry;

    put_byte( p, call_opcode );
    put_disp( p, StubRoutines::megamorphic_ic_entry() );
    st_assert( entry + static_cast<std::int32_t>( PolymorphicInlineCache::Constant::MegamorphicInlineCache_selector_offset ) == p, "layout constant inconsistent with code pattern" );
    put_word( p, std::int32_t( selector() ) );    // used for fast lookup
    st_assert( entry + static_cast<std::int32_t>( PolymorphicInlineCache::Constant::MegamorphicInlineCache_code_size ) == p, "layout constant inconsistent with code pattern" );

    return p - entry;
}


void PolymorphicInlineCache::shrink_and_generate( PolymorphicInlineCache *pic, KlassOop klass, void *method ) {
    static_cast<void>(pic); // unused
    static_cast<void>(klass); // unused
    static_cast<void>(method); // unused
    Unimplemented();
}


void *PolymorphicInlineCache::operator new( std::size_t size, std::int32_t code_size ) {
    return Universe::code->_picHeap->allocate( size + code_size );
}


void PolymorphicInlineCache::operator delete( void *p ) {
    Universe::code->_picHeap->deallocate( p, 0 );
}


PolymorphicInlineCache *PolymorphicInlineCache::allocate( CompiledInlineCache *ic, KlassOop klass, LookupResult result ) {
    st_assert( not result.is_empty(), "lookup result cannot be empty" );

    PolymorphicInlineCacheContents contents;

    // Always add the new entry first
    if ( result.is_entry() ) {
        contents.append_NativeMethod_entry( klass, result.get_nativeMethod()->verifiedEntryPoint() );
    } else {
        contents.append_method( klass, result.method() );
    }

    PolymorphicInlineCache *old_pic          = ic->pic();
    NativeMethod           *old_nativeMethod = ic->target();
    bool                   switch_to_MIC     = false;

    // 3 possible cases:
    //
    // 1. InlineCache is empty but lookup result returns methodOop -> needs 1-element PolymorphicInlineCache to call methodOop
    // 2. InlineCache contains 1 NativeMethod -> generate 2-element PolymorphicInlineCache
    // 3. InlineCache contains pic -> grow PolymorphicInlineCache or switch to MIC

    if ( old_pic not_eq nullptr ) {
        // ic contains pic
        st_assert( old_nativeMethod == nullptr, "just checking" );
        st_assert( not old_pic->is_megamorphic(), "MICs should not change anymore" );
        if ( old_pic->number_of_targets() >= static_cast<std::int32_t>( PolymorphicInlineCache::Constant::max_nof_entries ) ) {
            if ( UseMICs ) {
                // switch to MIC, keep only no lookup result
                switch_to_MIC = true;
            } else {
                ic->resetOptimized();    // make sure it doesn't force the creation of nativeMethods
                return nullptr;
            }
        } else {
            // append old PolymorphicInlineCache entries
            PolymorphicInlineCacheIterator it( old_pic );
            while ( not it.at_end() ) {
                if ( it.is_interpreted() ) {
                    contents.append_method( it.get_klass(), it.interpreted_method() );
                } else {
                    contents.append_NativeMethod_entry( it.get_klass(), it.get_call_addr() );
                }
                it.advance();
            }
        }
    } else if ( old_nativeMethod not_eq nullptr ) {
        // ic contains 1 NativeMethod
        contents.append_NativeMethod_entry( ic->get_klass( 0 ), old_nativeMethod->verifiedEntryPoint() );
    } else {
        // empty ic -> nothing to do
        st_assert( ic->is_empty(), "just checking" );
    }

    st_assert( switch_to_MIC or contents.number_of_interpreted_targets() > 0 or contents.number_of_compiled_targets() > 1, "no PolymorphicInlineCache required for only 1 compiled target" );

    PolymorphicInlineCache *new_pic = nullptr;
    if ( switch_to_MIC ) {
        new_pic = new( static_cast<std::int32_t>( PolymorphicInlineCache::Constant::MegamorphicInlineCache_code_size ) ) PolymorphicInlineCache( ic );
    } else {
        std::int32_t allocated_code_size = contents.code_size();
        new_pic = new( allocated_code_size ) PolymorphicInlineCache( ic, &contents, allocated_code_size );
    }

    new_pic->verify();

    return new_pic;
}


PolymorphicInlineCache::PolymorphicInlineCache( CompiledInlineCache *ic, PolymorphicInlineCacheContents *contents, std::int32_t allocated_code_size ) :
    _ic{ ic },
    _codeSize{ 0 },
    _numberOfTargets{ 0 } {
    st_assert( contents->number_of_targets() >= 1, "at least one entry needed for non-MEGAMORPHIC case" );
    _numberOfTargets = contents->number_of_targets();
    _codeSize        = code_for_polymorphic_case( entry(), contents );
    st_assert( code_size() == allocated_code_size, "Please adjust PolymorphicInlineCacheContents::code_size()" );
}


PolymorphicInlineCache::PolymorphicInlineCache( CompiledInlineCache *ic ) :
    _ic{ ic },
    _codeSize{ 0 },
    _numberOfTargets{ 0 } {
//    _numberOfTargets = 0; // indicates MEGAMORPHIC case
    _codeSize = code_for_megamorphic_case( entry() );
    st_assert( code_size() == static_cast<std::int32_t>( PolymorphicInlineCache::Constant::MegamorphicInlineCache_code_size ), "Please adjust PolymorphicInlineCacheContents::code_size()" );
}


GrowableArray<KlassOop> *PolymorphicInlineCache::klasses() const {
    GrowableArray<KlassOop>        *k = new GrowableArray<KlassOop>( 2 );
    PolymorphicInlineCacheIterator it( (PolymorphicInlineCache *) this );

    while ( not it.at_end() ) {
        k->append( it.get_klass() );
        it.advance();
    }

    return k;
}


void PolymorphicInlineCache::oops_do( void f( Oop * ) ) {
    if ( is_megamorphic() ) {
        // cannot use PolymorphicInlineCacheIterator (0 entries) -> deal with MIC directly
        f( (Oop *) MegamorphicInlineCache_selector_address() );
    } else {
        PolymorphicInlineCacheIterator it( this );
        while ( not it.at_end() ) {
            switch ( it.state() ) {
                case InlineState::AT_METHOD_OOP:
                    f( (Oop *) it.methodOop_addr() );
                    [[fallthrough]];
                case InlineState::AT_NATIVE_METHOD:
                    f( (Oop *) it.klass_addr() );
                default:
                    nullptr;
            }
            it.advance();
        }
    }
}


void PolymorphicInlineCache::print() {
    SPDLOG_INFO( "\tPolymorphicInlineCache with %d entr%s", number_of_targets(), number_of_targets() == 1 ? "y" : "ies" );
    SPDLOG_INFO( "\t- selector    : {}", selector()->print_value_string() );
    selector()->print_symbol_on();
//    SPDLOG_INFO( "" );

    // Disassembler::decode(entry(), entry() + code_size());


    PolymorphicInlineCacheIterator it( this );

    std::int32_t i = 1;
    while ( not it.at_end() ) {
        SPDLOG_INFO( "\t- %d. klass    : ", i );
        it.get_klass()->print_value();
        SPDLOG_INFO( "" );
        switch ( it.state() ) {
            case InlineState::AT_SMI_NATIVE_METHOD:
                [[fallthrough]];
            case InlineState::AT_NATIVE_METHOD:
                SPDLOG_INFO( "\t-    NativeMethod  : 0x{0:x} (entry 0x{0:x})\n", (std::int32_t) it.compiled_method(), (std::int32_t) it.get_call_addr() );
                break;
            case InlineState::AT_METHOD_OOP:
                SPDLOG_INFO( "\t-    methodOop: %s\n", it.interpreted_method()->print_value_string() );
                break;
            default: ShouldNotReachHere();
        }
        i++;
        it.advance();
    }
}


void PolymorphicInlineCache::verify() {
    // check for multiple entries for same class
    ResourceMark            rm;
    GrowableArray<KlassOop> *k = klasses();

    for ( std::int32_t i = 0; i < k->length() - 1; i++ ) {
        for ( std::int32_t j = i + 1; j < k->length(); j++ ) {
            if ( k->at( i ) == k->at( j ) ) {
                _console->print( "The class " );
                k->at( i )->klass_part()->print_name_on( _console );
                SPDLOG_INFO( "is twice in PolymorphicInlineCache 0x%lx", static_cast<const void *>(this) );
                spdlog::warn( "PolymorphicInlineCache verify error" );
            }
        }
    }
}


// Implementation of CompiledInlineCacheIterator
