//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once



// This code is unused

enum class LookupType {
    NormalLookupType,   //
    SelfLookupType,     //
    SuperLookupType     //
};

enum class CountType {
    NonCounting,    // no counting at all
    Counting,       // incrementing a counter
    Comparing       // increment & test for reaching limit (recompilation)
};


//typedef std::int32_t LookupType;

const std::int32_t LookupTypeSize = 2;
const std::int32_t LookupTypeMask = 3;

const std::int32_t CountTypeMask = static_cast<std::int32_t>(CountType::NonCounting) | static_cast<std::int32_t>(CountType::Counting) | static_cast<std::int32_t>(CountType::Comparing);
const std::int32_t CountTypeSize = 2;
const std::int32_t CountSendBit  = LookupTypeSize + 1;

// the dirty bit records whether the inline cache has ever made a transition
// from non-empty to empty (e.g. through flushing)
const std::int32_t DirtySendBit  = CountSendBit + CountTypeSize;
const std::int32_t DirtySendMask = 1 << DirtySendBit;

// the optimized bit says that if no callee NativeMethod exists, an optimized
// method should be created immediately rather than going through an
// unoptimized version first
const std::int32_t OptimizedSendBit  = DirtySendBit + 1;
const std::int32_t OptimizedSendMask = 1 << OptimizedSendBit;

// the uninlinable bit says that the SIC has decided it's not worth
// inlining this send no matter how often it is executed.
const std::int32_t UninlinableSendBit  = OptimizedSendBit + 1;
const std::int32_t UninlinableSendMask = 1 << UninlinableSendBit;


inline LookupType withoutExtraBits( LookupType lookupType ) {
    return static_cast<LookupType>(static_cast<std::int32_t>(lookupType) & static_cast<std::int32_t>(LookupTypeMask));
}


inline LookupType withCountBits( LookupType l, CountType t ) {
    return LookupType( ( std::int32_t( l ) & ~( CountTypeMask << CountSendBit ) ) | ( static_cast<std::int32_t>(t) << CountSendBit ) );
}


inline CountType countType( LookupType l ) {
    return CountType( ( std::int32_t( l ) >> CountSendBit ) & CountTypeMask );
}


extern "C" {
void printLookupType( LookupType lookupType );
char *lookupTypeName( LookupType lookupType );
}
