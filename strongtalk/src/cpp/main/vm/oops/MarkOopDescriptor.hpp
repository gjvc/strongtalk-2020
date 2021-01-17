//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/oops/OopDescriptor.hpp"


//
// Bit-format of markOop:
//
//  sentinel:1 near_death:1 tagged_contents:1 age:7 hash:20 tag:2 = 32 bits
//
//  - sentinel is needed during pointer reversal in garbage collection
//    to distinguish markOops from roots of oops.
//    markOops are used termination elements in the pointer list.
//
//    During normal execution the sentinel is 1 but the but may be used for
//    marking during special vm operations like dependency checking.
//
//  - near_death is set by the memory system iff weak pointers keep the object alive.
//
//  - tagged_contents indicates the is no untagged data in the object.
//
//  - age contains the age of the object when residing in the new generation.
//    (during garbage collection, age is used to store the size of the object).
//    PLEASE DO NOT CHANGE THE AGE FIELD since the garbage collector relies on the size.
//
//  - hash contains the identity hash value.
//
//  - tag contains the special mark tag
//

class MarkOopDescriptor : public OopDescriptor {

    private:
        uint32_t value() const {
            return ( uint32_t ) this;
        }


        friend int assign_hash( MarkOop & m );

        enum {
            no_hash    = 0,    //
            first_hash = 1  //
        };

        enum {
            sentinel_bits        = 1, //
            near_death_bits      = 1, //
            tagged_contents_bits = 1, //
            age_bits             = 7, //
            hash_bits            = BitsPerWord - sentinel_bits - near_death_bits - tagged_contents_bits - age_bits - TAG_SIZE
        };

        enum {
            hash_shift            = TAG_SIZE, //
            age_shift             = hash_bits + hash_shift, //
            tagged_contents_shift = age_bits + age_shift, //
            near_death_shift      = tagged_contents_bits + tagged_contents_shift, //
            sentinel_shift        = near_death_bits + near_death_shift //
        };

        enum {
            hash_mask                     = nthMask( hash_bits ), //
            hash_mask_in_place            = hash_mask << hash_shift, //
            age_mask                      = nthMask( age_bits ), //
            age_mask_in_place             = age_mask << age_shift, //
            tagged_contents_mask          = nthMask( tagged_contents_bits ), //
            tagged_contents_mask_in_place = tagged_contents_mask << tagged_contents_shift, //
            near_death_mask               = nthMask( near_death_bits ), //
            near_death_mask_in_place      = near_death_mask << near_death_shift, //
            sentinel_mask                 = nthMask( sentinel_bits ), //
            sentinel_mask_in_place        = sentinel_mask << sentinel_shift //
        };

        enum {
            no_hash_in_place           = no_hash << hash_shift, //
            first_hash_in_place        = first_hash << hash_shift, //
            untagged_contents_in_place = 1 << tagged_contents_shift
        };

        enum {
            sentinel_is_place = 1 << sentinel_shift //
        };

    public:
        enum {
            max_age = age_mask
        };


        // accessors
        bool_t has_sentinel() const {
            return maskBits( value(), sentinel_mask_in_place ) not_eq 0;
        }


        MarkOop set_sentinel() const {
            return MarkOop( sentinel_is_place | value() );
        }


        MarkOop clear_sentinel() const {
            return MarkOop( ~sentinel_is_place & value() );
        }


        bool_t has_tagged_contents() const {
            return maskBits( value(), tagged_contents_mask_in_place ) not_eq 0;
        }


        bool_t is_near_death() const {
            return maskBits( value(), near_death_mask_in_place ) not_eq 0;
        }


        MarkOop set_near_death() const {
            return MarkOop( near_death_mask_in_place | value() );
        }


        MarkOop clear_near_death() const {
            return MarkOop( ~near_death_mask_in_place & value() );
        }


        // klass invalidation (via sentinel bit)
        bool_t is_klass_invalid() const {
            return not has_sentinel();
        }


        MarkOop set_klass_invalid() const {
            return clear_sentinel();
        }


        MarkOop clear_klass_invalid() const {
            return set_sentinel();
        }


        // tells if context has forward reference to an unoptimized context(via sentinel bit)
        static int context_forward_bit_mask() {
            return near_death_mask_in_place;
        }


        bool_t has_context_forward() const {
            return is_near_death();
        }


        MarkOop set_context_forward() const {
            return set_near_death();
        }


        // notification queue check
        bool_t is_queued() const {
            return not has_sentinel();
        }


        MarkOop set_queued() const {
            return clear_sentinel();
        }


        MarkOop clear_queued() const {
            return set_sentinel();
        }


        // age operations
        int age() const {
            return maskBits( value(), age_mask_in_place ) >> age_shift;
        }


        MarkOop set_age( int v ) const {
            st_assert( ( v & ~age_mask ) == 0, "shouldn't overflow field" );
            return MarkOop( ( value() & ~age_mask_in_place ) | ( ( v & age_mask ) << age_shift ) );
        }


        MarkOop incr_age() const {
            return age() == max_age ? MarkOop( this ) : set_age( age() + 1 );
        }


        // hash operations
        int hash() const {
            return maskBits( value(), hash_mask_in_place ) >> hash_shift;
        }


        MarkOop set_hash( int v ) const {
            if ( ( v & hash_mask ) == 0 )
                v       = first_hash; // avoid no_hash
            MarkOop val = MarkOop( ( value() & ~hash_mask_in_place ) | ( ( v & hash_mask ) << hash_shift ) );
            st_assert( val->hash() not_eq no_hash, "should have hash now" );
            return val;
        }


        bool_t has_valid_hash() const {
            return hash() not_eq no_hash;
        }


        // markOop prototypes
        static MarkOop tagged_prototype() {
            return MarkOop( sentinel_is_place | no_hash_in_place | MARK_TAG );
        }


        static MarkOop untagged_prototype() {
            return MarkOop( sentinel_is_place | untagged_contents_in_place | no_hash_in_place | MARK_TAG );
        }


        // badOop
        static MarkOop bad() {
            return MarkOop( sentinel_is_place | first_hash_in_place | MARK_TAG );
        }


        friend int hash_markOop( MarkOop & m ) {
            int v = m->hash();
            return v == no_hash ? assign_hash( m ) : v;
        }


        // printing
        void print_on( ConsoleOutputStream * stream );


        static int masked_hash( int v ) {
            return v & hash_mask;
        }
};

#define badOop MarkOopDescriptor::bad()


// tells whether p is a root to an Oop or a markOop
// used during pointer reversal during GC.
// inline bool_t is_oop_root(Oop* p) { return not markOop(p)->has_sentinel(); }
inline bool_t is_oop_root( Oop * p ) {
    return not MarkOop( p )->is_mark();
}
