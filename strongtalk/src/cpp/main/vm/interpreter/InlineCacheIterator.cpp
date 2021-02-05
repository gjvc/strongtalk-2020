//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/interpreter/InlineCacheIterator.hpp"
#include "vm/code/PolymorphicInlineCache.hpp"
#include "vm/oops/SymbolOopDescriptor.hpp"
#include "vm/interpreter/InterpretedInlineCache.hpp"
#include "vm/utilities/OutputStream.hpp"
#include "vm/oops/Klass.hpp"
#include "vm/oops/KlassOopDescriptor.hpp"


InlineCache::InlineCache( CompiledInlineCache *ic ) :
    _iter( new CompiledInlineCacheIterator( ic ) ) {
}


InlineCache::InlineCache( InterpretedInlineCache *ic ) :
    _iter( new InterpretedInlineCacheIterator( ic ) ) {
}


GrowableArray<KlassOop> *InlineCache::receiver_klasses() const {

    GrowableArray<KlassOop> *result = new GrowableArray<KlassOop>();
    InlineCacheIterator     *it     = iterator();
    it->init_iteration();

    while ( not it->at_end() ) {
        result->append( it->klass() );
        it->advance();
    }
    return result;
}


void InlineCache::replace( NativeMethod *nm ) {
    static_cast<void>(nm); // unused
    Unimplemented();
    InlineCacheIterator *it = iterator();
    it->init_iteration();
    while ( not it->at_end() ) {
        // replace if found
        it->advance();
    }
}


void InlineCache::print() {
    const char *s;
    switch ( shape() ) {
        case InlineCacheShape::ANAMORPHIC:
            s = "Anamorphic";
            break;
        case InlineCacheShape::MONOMORPHIC:
            s = "Monomorphic";
            break;
        case InlineCacheShape::POLYMORPHIC:
            s = "Polymorphic";
            break;
        case InlineCacheShape::MEGAMORPHIC:
            s = "Megamorphic";
            break;
        default         : ShouldNotReachHere();
    }
    spdlog::info( "%s InlineCache: {} entries", s, number_of_targets() );

    InlineCacheIterator *it = iterator();
    it->init_iteration();
    while ( not it->at_end() ) {
        spdlog::info( "\t- klass: " );
        it->klass()->print_value();
        if ( it->is_interpreted() ) {
            spdlog::info( ";\tmethod  0x{0:x}", static_cast<void *>( it->interpreted_method() ) );
        } else {
            spdlog::info( ";\tnativeMethod 0x{0:x}", static_cast<void *>(  it->compiled_method() ) );
        }
        it->advance();
    }
}


void InlineCacheIterator::goto_elem( std::int32_t n ) {
    init_iteration();
    for ( std::int32_t i = 0; i < n; i++ )
        advance();
}


MethodOop InlineCacheIterator::interpreted_method( std::int32_t i ) {
    goto_elem( i );
    return interpreted_method();
}


NativeMethod *InlineCacheIterator::compiled_method( std::int32_t i ) {
    goto_elem( i );
    return compiled_method();
}


KlassOop InlineCacheIterator::klass( std::int32_t i ) {
    goto_elem( i );
    return klass();
}


CompiledInlineCacheIterator::CompiledInlineCacheIterator( CompiledInlineCache *ic ) {
    _ic = ic;
    init_iteration();
}


void CompiledInlineCacheIterator::init_iteration() {
    _picit = nullptr;
    _index = 0;
    if ( _ic->is_empty() ) {
        _number_of_targets = 0;
        _shape             = InlineCacheShape::ANAMORPHIC;
    } else if ( _ic->is_monomorphic() ) {
        _number_of_targets = 1;
        _shape             = InlineCacheShape::MONOMORPHIC;
        PolymorphicInlineCache *pic = _ic->pic();
        if ( pic )
            _picit = new PolymorphicInlineCacheIterator( pic );  // calls an interpreted method
    } else if ( _ic->is_polymorphic() ) {
        PolymorphicInlineCache *pic = _ic->pic();
        _number_of_targets = pic->number_of_targets();
        _shape             = InlineCacheShape::POLYMORPHIC;
        _picit             = new PolymorphicInlineCacheIterator( pic );
    } else if ( _ic->is_megamorphic() ) {
        _number_of_targets = 0;
        _shape             = InlineCacheShape::MEGAMORPHIC;
    } else {
        ShouldNotReachHere();
    }
}


void CompiledInlineCacheIterator::advance() {
    st_assert( not at_end(), "iterated over the end" );
    if ( _picit not_eq nullptr ) {
        _picit->advance();
    }
    _index++;
}


KlassOop CompiledInlineCacheIterator::klass() const {
    st_assert( not at_end(), "iterated over the end" );
    if ( _picit not_eq nullptr ) {
        return _picit->get_klass();
    } else {
        return _ic->get_klass( 0 );
    }
}


bool CompiledInlineCacheIterator::is_interpreted() const {
    st_assert( not at_end(), "iterated over the end" );
    if ( _picit not_eq nullptr ) {
        return _picit->is_interpreted();
    } else {
        return false;
    }
}


bool CompiledInlineCacheIterator::is_compiled() const {
    st_assert( not at_end(), "iterated over the end" );
    if ( _picit not_eq nullptr ) {
        return _picit->is_compiled();
    } else {
        return true;
    }
}


bool CompiledInlineCacheIterator::is_super_send() const {
    extern bool SuperSendsAreAlwaysInlined;
    st_assert( SuperSendsAreAlwaysInlined, "fix this" );
    return false;        // for now, super sends are always inlined
}


MethodOop CompiledInlineCacheIterator::interpreted_method() const {
    st_assert( not at_end(), "iterated over the end" );
    if ( _picit not_eq nullptr ) {
        return _picit->interpreted_method();
    } else {
        return compiled_method()->method();
    }
}


NativeMethod *CompiledInlineCacheIterator::compiled_method() const {
    st_assert( not at_end(), "iterated over the end" );
    if ( _picit not_eq nullptr ) {
        return _picit->compiled_method();
    } else {
        st_assert( number_of_targets() == 1, "must be MONOMORPHIC" );
        return _ic->target();
    }
}


void CompiledInlineCacheIterator::print() {
    spdlog::info( "CompiledInlineCacheIterator for ((CompiledInlineCache*)0x{0:x}) (%s)", static_cast<void *>( _ic ), selector()->as_string() );
}
