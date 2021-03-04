//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/code/NativeInstruction.hpp"
#include "vm/runtime/Frame.hpp"
#include "vm/primitive/PrimitiveDescriptor.hpp"
#include "vm/runtime/DeltaProcess.hpp"
#include "vm/runtime/VMProcess.hpp"


// ICs describe the interface to a send in an NativeMethod.
// A InlineCache can either describe:
//  - an inline cache for a Delta send (isDeltaSend() == true), or
//  - a primitive call                  (isPrimCall() == true).
//
//
// The memory layout of a Delta send (inline cache) is:
//  (offsets from return address)
// -5: call { _icLookupStub     (if not filled)
//          | NativeMethod address   (if filled with compiled    method)
//          | pic entry point}  (if filled with PolymorphicInlineCache/MegamorphicInlineCache)
//   				<--- CompiledInlineCache* pointers point here
//  0: test reg,		<--- dummy test instruction used to hold ic info
//  1: ic info
//  5: ...			<--- first instruction after move
//
//
// The memory layout of a primitive call is (offsets from return address):
//
// -5: call ...
//   				<--- PrimitiveInlineCache* pointers point here
//  0: test reg,		<--- dummy test instruction used to hold ic info (only if NonLocalReturn is possible)
//  1: ic info
//  5: ...			<--- first instruction after move
//
//
// InlineCache info layout (see als InlineCacheInfo for layout constants)
//
// [NonLocalReturn offset|flags]		32bit immediate of the dummy test instruction
//  31.......8|7...0
//
// The NonLocalReturn offset is a signed std::int32_t, the NonLocalReturn target destination is computed
// from the (call's) return address + NonLocalReturn offset.
//
// Calling nativeMethods through a jump table will cost 10% of the total execution speed
// (data provided by Urs 4-21-95)


class AbstractCompiledInlineCache : public NativeCall {

public:
    char *NonLocalReturn_testcode() const {
        return ic_info_at( next_instruction_address() )->NonLocalReturn_target();
    }


    // returns the beginning of the inline cache.
    char *begin_addr() const {
        return instruction_address();
    }
};


// Flags

constexpr std::int32_t dirty_send_bit_no      = 0;
constexpr std::int32_t optimized_bit_no       = 1;
constexpr std::int32_t uninlinable_bit_no     = 2;
constexpr std::int32_t super_send_bit_no      = 3;
constexpr std::int32_t megamorphic_bit_no     = 4;
constexpr std::int32_t receiver_static_bit_no = 5;


// CompiledInlineCache isn't a real object; the 'this' pointer points into the compiled code
// (more precisely, it's the same as the return address of the callee)

class LookupKey;

class PolymorphicInlineCache;

class InterpretedInlineCache;

class CompiledInlineCache : public AbstractCompiledInlineCache {

protected:
    std::int32_t compiler_info() const {
        return ic_info_at( next_instruction_address() )->flags();
    }


    void set_compiler_info( std::int32_t info ) {
        ic_info_at( next_instruction_address() )->set_flags( info );
    }


public:
    // lookup routines for empty inline cache
    static const char *normalLookupRoutine();

    static const char *superLookupRoutine();

    // conversion (machine PC to CompiledInlineCache*)
    friend CompiledInlineCache *CompiledIC_from_return_addr( const char *return_addr );

    friend CompiledInlineCache *CompiledIC_from_relocInfo( const char *displacement_address );

    // Accessors

    // isDirty() --> has had misses
    bool isDirty() const {
        return isBitSet( compiler_info(), dirty_send_bit_no );
    }


    void setDirty() {
        set_compiler_info( addNthBit( compiler_info(), dirty_send_bit_no ) );
    }


    // isOptimized() --> nativeMethods called from here should be optimized
    bool isOptimized() const {
        return isBitSet( compiler_info(), optimized_bit_no );
    }


    void setOptimized() {
        set_compiler_info( addNthBit( compiler_info(), optimized_bit_no ) );
    }


    void resetOptimized() {
        set_compiler_info( subNthBit( compiler_info(), optimized_bit_no ) );
    }


    // isUninlinable() --> compiler says don't try to inline this send
    bool isUninlinable() const {
        return isBitSet( compiler_info(), uninlinable_bit_no );
    }


    // isSuperSend() --> send is a super send
    bool isSuperSend() const {
        return isBitSet( compiler_info(), super_send_bit_no );
    }


    // isMegamorphic() --> send is MEGAMORPHIC
    bool isMegamorphic() const {
        return isBitSet( compiler_info(), megamorphic_bit_no );
    }


    void setMegamorphic() {
        set_compiler_info( addNthBit( compiler_info(), megamorphic_bit_no ) );
    }


    // isReceiverStatic() --> receiver klass is known statically (connect to verifiedEntryPoint)
    bool isReceiverStatic() const {
        return isBitSet( compiler_info(), receiver_static_bit_no );
    }


    void setReceiverStatic() {
        set_compiler_info( addNthBit( compiler_info(), receiver_static_bit_no ) );
    }


    bool wasNeverExecuted() const;

    InterpretedInlineCache *inlineCache() const;    // corresponding source-level inline cache (nullptr if none, e.g. perform)
    PolymorphicInlineCache *pic() const;            // nullptr if 0 or 1 targets

    // returns the first address after this primitive ic
    char *end_addr() {
        return ic_info_at( next_instruction_address() )->next_instruction_address();
    }


    // sets the destination of the call instruction
    void set_call_destination( const char *entry_point );

    // Does a lookup in the receiver and patches the inline cache
    const char *normalLookup( Oop receiver );

    const char *superLookup( Oop receiver );

    // Returns the class that holds the current method
    KlassOop sending_method_holder();

    // replace appropriate target (with key nm->key) by nm
    void replace( NativeMethod *nm );

public:
    SymbolOop selector() const;

    bool is_empty() const;

    bool is_monomorphic() const;

    bool is_polymorphic() const;

    bool is_megamorphic() const;

    NativeMethod *target() const;    // directly called NativeMethod or nullptr if none/PolymorphicInlineCache
    KlassOop targetKlass() const;    // klass of compiled or interpreted target;
    // can only call if single target
    std::int32_t ntargets() const;    // number of targets in inline cache or PolymorphicInlineCache
    KlassOop get_klass( std::int32_t i ) const; // receiver klass of ith target (i=0..ntargets()-1)

    // returns the lookup key for PolymorphicInlineCache index
    LookupKey *key( std::int32_t which, bool is_normal_send ) const;

    void reset_jump_addr();

    void link( PolymorphicInlineCache *s );

    void clear();        // clear inline cache
    void cleanup();        // cleanup inline cache

    bool verify();

    void print();
};


class PrimitiveInlineCache : public AbstractCompiledInlineCache {
public:
    // returns the primitive descriptor (based on the destination of the call).
    PrimitiveDescriptor *primitive();

    // conversion (machine PC to PrimitiveInlineCache*)
    friend PrimitiveInlineCache *PrimitiveIC_from_return_addr( const char *return_addr );

    friend PrimitiveInlineCache *PrimitiveIC_from_relocInfo( const char *displacement_address );

    // returns the first address after this primitive ic.
    char *end_addr();

    void print();
};
