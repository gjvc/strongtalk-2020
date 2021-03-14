
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/lookup/LookupResult.hpp"
#include "vm/interpreter/InlineCacheIterator.hpp"
#include "vm/code/CompiledInlineCache.hpp"
#include "vm/utility/GrowableArray.hpp"
#include "vm/runtime/ResourceObject.hpp"
#include "vm/code/CompiledInlineCache.hpp"


// A PolymorphicInlineCache implements a PolymorphicInlineCache for compiled code.
// It may be MEGAMORPHIC, in which case it may cache only the last method and do a lookup whenever there is a cache miss.

//class PolymorphicInlineCacheIterator;

class PolymorphicInlineCacheContents;


class PolymorphicInlineCache {

public:
    enum class Constant {
        max_nof_entries = 4,            // the maximal number of PolymorphicInlineCache entries

        // PolymorphicInlineCache layout constants
        PolymorphicInlineCache_methodOop_only_offset     = 5,   //
        PolymorphicInlineCache_smi_nativeMethodOffset    = 4,   //
        PolymorphicInlineCache_NativeMethod_entry_offset = 11,  //
        PolymorphicInlineCache_NativeMethod_entry_size   = 12,  //
        PolymorphicInlineCache_NativeMethod_klass_offset = 2,   //
        PolymorphicInlineCache_nativeMethodOffset        = 8,   //
        PolymorphicInlineCache_methodOop_entry_offset    = 16,  //
        PolymorphicInlineCache_methodOop_entry_size      = 8,   //
        PolymorphicInlineCache_methodOop_klass_offset    = 0,   //
        PolymorphicInlineCache_methodOop_offset          = 4,   //

        // MegamorphicInlineCache layout constants
        MegamorphicInlineCache_selector_offset = 5, //
        MegamorphicInlineCache_code_size       = 9, //
    };

private:
    CompiledInlineCache *_ic;      // the ic linked to this PolymorphicInlineCache
    std::int16_t        _codeSize;              // size of code in bytes
    std::int16_t        _numberOfTargets;       // the total number of PolymorphicInlineCache entries, 0 indicates a MonomorphicInlineCache

    static std::int32_t nof_entries( const char *pic_stub );    // the no. of methodOop entries for a given stub routine

    std::int32_t code_for_methodOops_only( const char *entry, PolymorphicInlineCacheContents *c );

    std::int32_t code_for_polymorphic_case( char *entry, PolymorphicInlineCacheContents *c );

    std::int32_t code_for_megamorphic_case( char *entry );

    void shrink_and_generate( PolymorphicInlineCache *pic, KlassOop klass, void *method );


    bool contains( const char *addr ) {
        return entry() <= addr and addr < entry() + code_size();
    }


    // Creation / access of PolymorphicInlineCache instances
    PolymorphicInlineCache( CompiledInlineCache *ic, PolymorphicInlineCacheContents *contents, std::int32_t allocated_code_size ); // creation of PolymorphicInlineCache
    PolymorphicInlineCache( CompiledInlineCache *ic ); // creation of MegamorphicInlineCache

public:
    void *operator new( std::size_t size, std::int32_t code_size );

    // Deallocates this pic from the pic heap
    void operator delete( void *p );


    void operator delete( void *p, std::int32_t size ) {
        st_unused( p ); // unused
        st_unused( size ); // unused
    };

    // Allocates and returns a new ready to execute pic.
    static PolymorphicInlineCache *allocate( CompiledInlineCache *ic, KlassOop klass, LookupResult result );

    // Tells whether addr inside the PolymorphicInlineCache area
    static bool in_heap( const char *addr );

    // Returns the PolymorphicInlineCache containing addr, nullptr otherwise
    static PolymorphicInlineCache *find( const char *addr );


    // Returns the code size of the PolymorphicInlineCache
    std::int32_t code_size() const {
        return _codeSize;
    }


    // Retrieving PolymorphicInlineCache information
    CompiledInlineCache *compiled_ic() const {
        return _ic;
    }


    std::int32_t number_of_targets() const {
        return _numberOfTargets;
    }


    SymbolOop selector() const {
        return compiled_ic()->selector();
    }


    char *entry() const {
        return (char *) ( this + 1 );
    }


    bool is_monomorphic() const {
        return number_of_targets() == 1;
    }


    bool is_polymorphic() const {
        return number_of_targets() > 1;
    }


    bool is_megamorphic() const {
        return number_of_targets() == 0;
    }


    // For MegamorphicInlineCache instances only
    SymbolOop *MegamorphicInlineCache_selector_address() const;    // the address of the selector in the MegamorphicInlineCache

    // replace appropriate target (with key nm->key) by nm.
    // this is returned if we could patch the current PolymorphicInlineCache.
    // a new PolymorphicInlineCache is returned if we could not patch this PolymorphicInlineCache.
    PolymorphicInlineCache *replace( NativeMethod *nm );

    // Cleans up the pic and returns:
    //  1) A PolymorphicInlineCache			(still POLYMORPHIC or MEGAMORPHIC)
    //  2) A NativeMethod		(now   MONOMORPHIC)
    //  3) nothing		(now   ANAMORPHIC)
    PolymorphicInlineCache *cleanup( NativeMethod **nm );

    GrowableArray<KlassOop> *klasses() const;

    // Iterate over all oops in the pic
    void oops_do( void f( Oop * ) );

    // printing operation
    void print();

    // verify operation
    void verify();

    friend class PolymorphicInlineCacheIterator;
};
