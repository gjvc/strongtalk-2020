//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/oops/Klass.hpp"
#include "vm/runtime/ResourceObject.hpp"


class memConverter;

class ClassChange : public ResourceObject {

    private:
        struct KlassOopDescriptor * _old_klass;
        struct MixinOopDescriptor * _new_mixin;
        Klass::Format             _new_format;
        struct KlassOopDescriptor * _new_klass;
        struct KlassOopDescriptor * _new_super;
        memConverter              * _converter;
        ClassChange               * _super_change;
        int32_t                   _is_schema_change_computed;
        int32_t                   _needs_schema_change;
        const char                * _reason_for_schema_change;

    public:
        ClassChange( struct KlassOopDescriptor * old_klass, struct MixinOopDescriptor * new_mixin, Klass::Format new_format, struct KlassOopDescriptor * new_super );


        ClassChange( struct KlassOopDescriptor * old_klass, Klass::Format new_format );


        struct KlassOopDescriptor * old_klass() const;


        struct MixinOopDescriptor * new_mixin() const;


        Klass::Format new_format() const;


        struct KlassOopDescriptor * new_klass() const;


        struct KlassOopDescriptor * new_super() const;


        memConverter * converter() const;


        ClassChange * super_change() const;


        struct MixinOopDescriptor * old_mixin() const;


        const char * reason_for_schema_change();


        void set_reason_for_schema_change( const char * msg );


        void set_super_change( ClassChange * change );


        void setup_schema_change();

        void recustomize_methods();

        int32_t compute_needed_schema_change();


        int32_t needs_schema_change();


        struct KlassOopDescriptor * new_class_from( struct KlassOopDescriptor * old_klass, struct KlassOopDescriptor * new_super_klass, struct MixinOopDescriptor * new_mixin, Klass::Format new_format, struct MixinOopDescriptor * old_mixin );

        memConverter * create_converter_for( struct KlassOopDescriptor * old_class, struct KlassOopDescriptor * new_class );

        void transfer_misc( struct MemOopDescriptor * src, struct MemOopDescriptor * dst );

        // Updating if no schema change is needed
        void update_class( int32_t class_vars_changed, int32_t instance_methods_changed, int32_t class_methods_changed );

        void update_class_vars();

        void update_methods( int32_t instance_side );
};

