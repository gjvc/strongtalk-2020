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
#include "vm/system/sizes.hpp"


void OopNativeCode::remember() {
    Unimplemented(); //  if (rememberLink.isEmpty()) Memory->code->rememberLink.add(&rememberLink);
}


void OopNativeCode::relocate() {
    Unimplemented();
}


bool_t OopNativeCode::switch_pointers( Oop from, Oop to, GrowableArray<NativeMethod *> *nativeMethods_to_invalidate ) {
    bool_t needToInvalICache = false;
    Unimplemented();
    return needToInvalICache;
}


void NativeCodeBase::verify2( const char *name ) {
    if ( (int) this & ( oopSize - 1 ) )
        error( "alignment error in %s at %#lx", name, this );
    if ( instructionsLength() > 256 * 1024 )
        error( "instr length of %s at %#lx seems too big (%ld)", name, this, instructionsLength() );
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
