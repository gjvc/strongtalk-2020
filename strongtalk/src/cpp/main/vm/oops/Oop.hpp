
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"


// -----------------------------------------------------------------------------

typedef class OopDescriptor *Oop;
constexpr std::int32_t      OOP_SIZE = sizeof( Oop );


// -----------------------------------------------------------------------------

typedef Oop     (__CALLING_CONVENTION *primitiveFunctionType)( ... );
//typedef void    (__CALLING_CONVENTION * oopsDoFn)( Oop * p );
typedef void    (__CALLING_CONVENTION *doFn)();


// -----------------------------------------------------------------------------

typedef class OopDescriptor                 *Oop;
typedef class MarkOopDescriptor             *MarkOop;
typedef class MemOopDescriptor              *MemOop;
typedef class AssociationOopDescriptor      *AssociationOop;
typedef class BlockClosureOopDescriptor     *BlockClosureOop;
typedef class ByteArrayOopDescriptor        *ByteArrayOop;
typedef class SymbolOopDescriptor           *SymbolOop;
typedef class ContextOopDescriptor          *ContextOop;
typedef class DoubleByteArrayOopDescriptor  *DoubleByteArrayOop;
typedef class DoubleOopDescriptor           *DoubleOop;
typedef class DoubleValueArrayOopDescriptor *doubleValueArrayOop;
typedef class KlassOopDescriptor            *KlassOop;
typedef class MethodOopDescriptor           *MethodOop;
typedef class MixinOopDescriptor            *MixinOop;
typedef class ObjectArrayOopDescriptor      *ObjectArrayOop;
typedef class WeakArrayOopDescriptor        *WeakArrayOop;
typedef class ProcessOopDescriptor          *ProcessOop;
typedef class ProxyOopDescriptor            *ProxyOop;
typedef class VirtualFrameOopDescriptor     *VirtualFrameOop;
typedef class SMIOopDescriptor              *SMIOop;


// -----------------------------------------------------------------------------

typedef class ScopeDescriptorNode *ScopeInfo;

//
// oopDescriptor ("object-orientated pointer descriptor") is the top of the Oop hierarchy.
//

// The "*Descriptor" classes describe the format of ST objects so the fields can be accessed from C++.
// "*Oop" pointers to "*Descriptor" structures (e.g., Oop, proxyOop) are TAGGED and thus should not be used to access the fields.
// Instead, convert the xxxOop to a xxxDescriptor* with the ->addr() function, then work with the xxxDescriptor* pointer.

// xxxOop pointers are tagged.
// xxxDescriptor* pointers are not tagged.
// convert xxxOop to a xxxDescriptor* with the xxxOop->addr() function

// NB: the above is true only for memOops

extern "C" Oop nilObject;

class Generation;

class Klass;

class ConsoleOutputStream;
