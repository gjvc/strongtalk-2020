//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "allocation.hpp"
#include "vm/oop/MixinOopDescriptor.hpp"
#include "vm/oop/SymbolOopDescriptor.hpp"
#include "vm/utility/GrowableArray.hpp"
#include "vm/memory/ClassChange.hpp"

class memConverter;

class ClassChange;


class Reflection : AllStatic {

private:
    // Variables used for schema change
    static GrowableArray<ClassChange *> *_classChanges; // Class changes
    static GrowableArray<MemOop>        *_converted;     // Converted objects

    static std::int32_t invocations_offset() {
        return 3;
    }


private:
    // registers all changes in 'class_changes'
    static void register_class_changes( MixinOop new_mixin, ObjectArrayOop invocations );

    // sets or resets the invalid bit in the header for old_klass in class_changes
    static void invalidate_classes( bool value );

    // find the change structure for a given class
    static ClassChange *find_change_for( KlassOop klass );

    // computes if a schema change is necessary
    static bool needs_schema_change();

    // FOR NO SCHEMA CHANFES
    static void update_classes( bool class_vars_changed, bool instance_methods_changed, bool class_methods_changed );

    // FOR SCHEMA CHANGES

    // builds the new classes and converters
    static void setup_schema_change();

    static void update_class( KlassOop klass, MixinOop new_mixin, MixinOop old_mixin, bool instance_methods_changed, bool class_methods_changed, bool class_vars_changed );


    static bool has_methods_changed( MixinOop new_mixin, MixinOop old_mixin );

    static bool has_class_vars_changed( MixinOop new_mixin, MixinOop old_mixin );

    static void apply_change( MixinOop new_mixin, MixinOop old_mixin, ObjectArrayOop invocations );

    static MemOop convert_object( MemOop obj );

public:
    // Entry for primitiveApplyChange:ifFail:
    static Oop apply_change( ObjectArrayOop change );

    // Converts an object if necessary (used when scanning the object heap)
    static void convert( Oop *p );

    // place forward pointer and
    static void forward( MemOop old_obj, MemOop new_obj );
};
