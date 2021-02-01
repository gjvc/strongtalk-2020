//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/system/platform.hpp"
#include "vm/system/asserts.hpp"
#include "vm/memory/util.hpp"
#include "vm/utilities/GrowableArray.hpp"
#include "vm/runtime/Process.hpp"
#include "vm/memory/Universe.hpp"
#include "vm/oops/Klass.hpp"
#include "vm/oops/KlassOopDescriptor.hpp"
#include "vm/code/NativeCode.hpp"
#include "vm/code/NativeMethod.hpp"


void OopNativeCode::remember() {
    Unimplemented(); //  if (rememberLink.isEmpty()) Memory->code->rememberLink.add(&rememberLink);
}


void OopNativeCode::relocate() {
    Unimplemented();
}


bool OopNativeCode::switch_pointers( Oop from, Oop to, GrowableArray<NativeMethod *> *nativeMethods_to_invalidate ) {
    static_cast<void>(from); // unused
    static_cast<void>(to); // unused
    static_cast<void>(nativeMethods_to_invalidate); // unused
    bool needToInvalICache = false;
    Unimplemented();
    return needToInvalICache;
}


void NativeCodeBase::verify2( const char *name ) {
    if ( (std::int32_t) this & ( OOP_SIZE - 1 ) ) {
        error( "alignment error in %s at 0x{0:x}", name, this );
    }

    if ( instructionsLength() > 256 * 1024 ) {
        error( "instr length of %s at 0x{0:x} seems too big (%ld)", name, this, instructionsLength() );
    }
}


void OopNativeCode::verify() {
    const char *name = isNativeMethod() ? "NativeMethod" : ( isPIC() ? " PolymorphicInlineCache" : "count stub" );
    NativeCodeBase::verify2( name );
    // %fix: Verify via RelocationInformationIterator
}


NativeCodeBase *findThing( void *addr ) {
    if ( Universe::code->contains( addr ) ) {
        NativeMethod *n = (NativeMethod *) nativeMethod_from_insts( (const char *) addr );
        return n;
    } else {
        return nullptr;
    }
}
