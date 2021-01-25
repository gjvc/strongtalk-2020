//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/oops/MemOopKlass.hpp"
#include "vm/oops/ProxyOopDescriptor.hpp"
#include "vm/runtime/Process.hpp"


class ProxyKlass : public MemOopKlass {
public:
    // testers
    bool_t oop_is_proxy() const {
        return true;
    }


    // allocation properties
    bool_t can_inline_allocation() const {
        return false;
    }


    // allocation operations
    Oop allocateObject( bool_t permit_scavenge = true, bool_t tenured = false );

    // creates invocation
    KlassOop create_subclass( MixinOop mixin, Format format );

    static KlassOop create_class( KlassOop super_class, MixinOop mixin );


    // Format
    Format format() {
        return Format::proxy_klass;
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
        return ProxyOopDescriptor::header_size();
    }


    // printing support
    const char *name() const {
        return "proxy";
    }


    // class creation
    friend void setKlassVirtualTableFromProxyKlass( Klass *k );
};

void setKlassVirtualTableFromProxyKlass( Klass *k );
