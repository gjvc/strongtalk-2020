//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/oops/OopDescriptor.hpp"
#include "vm/oops/MemOopKlass.hpp"
#include "vm/oops/VirtualFrameOopDescriptor.hpp"


class VirtualFrameKlass : public MemOopKlass {

    public:
        // testers
        bool_t oop_is_vframe() const {
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
            return Format::vframe_klass;
        }


        // memory operations
        int oop_scavenge_contents( Oop obj );

        int oop_scavenge_tenured_contents( Oop obj );

        void oop_follow_contents( Oop obj );

        // iterators
        void oop_layout_iterate( Oop obj, ObjectLayoutClosure * blk );

        void oop_oop_iterate( Oop obj, OopClosure * blk );


        // sizing
        int oop_header_size() const {
            return VirtualFrameOopDescriptor::header_size();
        }


        // printing support
        const char * name() const {
            return "VirtualFrame";
        }


        // class creation
        friend void setKlassVirtualTableFromVirtualFrameKlass( Klass * k );
};

void setKlassVirtualTableFromVirtualFrameKlass( Klass * k );
