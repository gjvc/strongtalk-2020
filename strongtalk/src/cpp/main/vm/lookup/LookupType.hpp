//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once



// This code is unused

enum BaseLookupType {
    NormalLookupType, SelfLookupType, SuperLookupType
};

enum CountType {
    NonCounting,    // no counting at all
    Counting,       // incrementing a counter
    Comparing       // increment & test for reaching limit (recompilation)
};


typedef int LookupType;

const int LookupTypeSize = 2;
const int LookupTypeMask = 3;

const int CountTypeMask = NonCounting | Counting | Comparing;
const int CountTypeSize = 2;
const int CountSendBit  = LookupTypeSize + 1;

// the dirty bit records whether the inline cache has ever made a transition
// from non-empty to empty (e.g. through flushing)
const int DirtySendBit  = CountSendBit + CountTypeSize;
const int DirtySendMask = 1 << DirtySendBit;

// the optimized bit says that if no callee NativeMethod exists, an optimized
// method should be created immediately rather than going through an
// unoptimized version first
const int OptimizedSendBit  = DirtySendBit + 1;
const int OptimizedSendMask = 1 << OptimizedSendBit;

// the uninlinable bit says that the SIC has decided it's not worth
// inlining this send no matter how often it is executed.
const int UninlinableSendBit  = OptimizedSendBit + 1;
const int UninlinableSendMask = 1 << UninlinableSendBit;


inline LookupType withoutExtraBits( LookupType lookupType ) {
    return lookupType & LookupTypeMask;
}


inline LookupType withCountBits( LookupType l, CountType t ) {
    return LookupType( ( int( l ) & ~( CountTypeMask << CountSendBit ) ) | ( t << CountSendBit ) );
}


inline CountType countType( LookupType l ) {
    return CountType( ( int( l ) >> CountSendBit ) & CountTypeMask );
}


extern "C" {
void printLookupType( LookupType lookupType );
char * lookupTypeName( LookupType lookupType );
}
