//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/memory/allocation.hpp"
#include "vm/utilities/GrowableArray.hpp"
#include "vm/memory/Universe.hpp"


class NativeMethod;

// NativeCodeBase is the superclass of all things containing native code.

class NativeCodeBase : public PrintableCHeapAllocatedObject {

    protected:
        int _instructionsLength;

    public:
        void * operator new( size_t size ) {
            SubclassResponsibility();
            return nullptr;
        }


        virtual char * instructionsStart() const = 0;       // beginning of instructions part

        virtual int size() const = 0;                       // size in bytes

        int instructionsLength() const {
            return _instructionsLength;
        }


        char * instructionsEnd() const {
            return instructionsStart() + instructionsLength();
        }


        bool_t contains( const void * p ) const {
            return ( void * ) instructionsStart() <= p and p < ( void * ) instructionsEnd();
        }


        virtual bool_t isNativeMethod() const {
            return false;
        }


        virtual bool_t isPIC() const {
            return false;
        }


        virtual bool_t isCountStub() const {
            return false;
        }


        virtual bool_t isAgingStub() const {
            return false;
        }


        virtual void moveTo( void * to, int size ) = 0; // (possibly overlapping) copy
        virtual void relocate() {
        }


        virtual void verify() = 0;

    protected:
        void verify2( const char * name );
};


// OopNativeCode is the base class of all code containing Oop references embedded in the code (e.g. "load constant" instructions).

//#define OOPNATIVECODE_FROM( fieldName, p ) ( (OopNativeCode*)((char*)p - (char*)&((OopNativeCode*)nullptr)->fieldName))


class RelocationInformation;

class OopNativeCode : public NativeCodeBase {

    protected:
        void check_store( Oop x, char * bound ) {
            if ( Universe::new_gen.is_new( x, bound ) )
                remember();
        }


        int _locsLen;                // relocation info length (bytes)

    public:

        RelocationInformation * locs() const {
            return ( RelocationInformation * ) instructionsEnd();
        }


        int locsLen() const {
            return _locsLen;
        }


        RelocationInformation * locsEnd() const {
            return ( RelocationInformation * ) ( ( const char * ) locs() + _locsLen );
        }


        virtual bool_t isNativeMethod() const {
            return false;
        }


        // Memory operations: return true if need to invalidate instruction cache
        virtual bool_t switch_pointers( Oop from, Oop to, GrowableArray <NativeMethod *> * nativeMethods_to_invalidate );

        void relocate();

        void remember();

        virtual void verify();
};

NativeCodeBase * findThing( void * addr );   // returns nullptr if addr not in a zone
