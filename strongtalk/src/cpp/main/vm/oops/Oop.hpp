
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"


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

constexpr std::int32_t OOP_SIZE = sizeof( Oop );


// -----------------------------------------------------------------------------

typedef std::int32_t    (__CALLING_CONVENTION *mytype)( std::int32_t a, std::int32_t b );
typedef Oop             (__CALLING_CONVENTION *primitiveFunctionType)( ... );
typedef void            (__CALLING_CONVENTION *oopsDoFn)( Oop *p );
typedef void            (__CALLING_CONVENTION *doFn)();


// -----------------------------------------------------------------------------

typedef class ScopeDescriptorNode *ScopeInfo;
