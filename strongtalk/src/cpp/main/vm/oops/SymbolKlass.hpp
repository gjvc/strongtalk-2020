//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/system/bits.hpp"
#include "vm/oops/Klass.hpp"
#include "vm/oops/OopDescriptor.hpp"
#include "vm/oops/ByteArrayKlass.hpp"


// symbolOops are canonical symbols; all symbolOops are registered in the symbol table

class SymbolKlass : public ByteArrayKlass {
    public:
        // testers
        bool_t oop_is_symbol() const {
            return true;
        }


        // allocation properties
        bool_t can_inline_allocation() const {
            return false;
        }


        // creates invocation
        KlassOop create_subclass( MixinOop mixin, Format format );

        static KlassOop create_class( KlassOop super_class, MixinOop mixin );


        // Format
        Format format() {
            return Format::symbol_klass;
        }


        // reflective properties
        bool_t can_have_instance_variables() const {
            return false;
        }


        bool_t can_be_subclassed() const {
            return false;
        }


        // allocation operation
        SymbolOop allocateSymbol( const char * name, int len );

        // copy operation
        Oop oop_shallow_copy( Oop obj, bool_t tenured );

        // memory operations
        Oop scavenge( Oop obj );

        bool_t verify( Oop obj );

        // printing operations
        void oop_print_value_on( Oop obj, ConsoleOutputStream * stream );


        const char * name() const {
            return "symbol";
        }


        void print( Oop obj );

        // class creation
        friend void setKlassVirtualTableFromSymbolKlass( Klass * k );
};

void setKlassVirtualTableFromSymbolKlass( Klass * k );

