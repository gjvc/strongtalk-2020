//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/lookup/LookupResult.hpp"

const std::uint32_t primary_cache_size   = 16 * 1024;
const std::uint32_t secondary_cache_size = 2 * 1024;

extern LookupResult interpreter_normal_lookup( KlassOop receiver_klass, SymbolOop selector );

extern LookupResult interpreter_super_lookup( SymbolOop selector );

class LookupCache : AllStatic {

private:
    static address_t primary_cache_address();

    static address_t secondary_cache_address();

    static std::uint32_t hash_value( LookupKey *key );

    static std::size_t number_of_primary_hits;
    static std::size_t number_of_secondary_hits;
    static std::size_t number_of_misses;

    static LookupResult ic_lookup( KlassOop receiver_klass, Oop selector_or_method );

    static LookupResult lookup( LookupKey *key, bool_t compile );

    static LookupResult cache_miss_lookup( LookupKey *key, bool_t compile );

    static NativeMethod *compile_method( LookupKey *key, MethodOop method );

public:
    // Lookup probe into the lookup cache
    static LookupResult lookup_probe( LookupKey *key );

    // Lookup support for inline cache (returns methodOop or jump_table_entry).
    static LookupResult ic_normal_lookup( KlassOop receiver_klass, SymbolOop selector );

    static LookupResult ic_super_lookup( KlassOop receiver_klass, KlassOop sending_method_holder, SymbolOop selector );

    // Lookup support for interpreter  (returns methodOop or jump_table_entry).
    friend LookupResult interpreter_normal_lookup( KlassOop receiver_klass, SymbolOop selector );

    friend LookupResult interpreter_super_lookup( SymbolOop selector );

    // Lookup support for compiler
    static MethodOop method_lookup( KlassOop receiver_klass, SymbolOop selector );

    static MethodOop compile_time_normal_lookup( KlassOop receiver_klass, SymbolOop selector );    // returns nullptr if not found
    static MethodOop compile_time_super_lookup( KlassOop receiver_klass, SymbolOop selector );    // returns nullptr if not found

    // Lookup support for LookupKey
    static LookupResult lookup( LookupKey *key );

    // Lookup support for megamorphic sends (no super sends)
    static Oop normal_lookup( KlassOop receiver_klass, SymbolOop selector );            // returns {methodOop or jump table entry}

    // Flushing
    static void flush( LookupKey *key );

    static void flush();

    // Clear all entries in the cache.
    static void verify();

    static void clear_statistics();

    static void print_statistics();

    friend class InterpreterGenerator;

    friend class StubRoutines;

    friend class debugPrimitives;
};
