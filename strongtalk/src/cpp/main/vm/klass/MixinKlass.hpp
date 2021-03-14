//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/platform/platform.hpp"
#include "vm/oop/OopDescriptor.hpp"
#include "vm/klass/MemOopKlass.hpp"
#include "vm/oop/MixinOopDescriptor.hpp"


template<typename T>
concept oopIsMixin = requires( T a ) {
    a.get();
};

//template<oopIsMixin T>
class MixinKlass : public MemOopKlass {

public:

    // testers
    bool oopIsMixin() const {
        return true;
    }


    // allocation properties
    bool can_inline_allocation() const {
        return false;
    }


    // allocation operations
    Oop allocateObject( bool permit_scavenge = true, bool tenured = false );

    Oop oop_shallow_copy( Oop obj, bool tenured );

    // creates invocation
    KlassOop create_subclass( MixinOop mixin, Format format );

    static KlassOop create_class( KlassOop super_class, MixinOop mixin );


    // Format
    Format format() {
        return Format::mixin_klass;
    }


    // memory operations
    std::int32_t oop_scavenge_contents( Oop obj );

    std::int32_t oop_scavenge_tenured_contents( Oop obj );

    void oop_follow_contents( Oop obj );

    // iterators
    void oop_layout_iterate( Oop obj, ObjectLayoutClosure *blk );

    void oop_oop_iterate( Oop obj, OopClosure *blk );


    // sizing
    std::int32_t oop_header_size() const {
        return MixinOopDescriptor::header_size();
    }


    // printing support
    const char *name() const {
        return "mixin";
    }


    // class creation
    friend void setKlassVirtualTableFromMixinKlass( Klass *k );
};

void setKlassVirtualTableFromMixinKlass( Klass *k );
