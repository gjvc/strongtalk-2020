//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once


#if 0

// various Compiler functionality associated with splitting

// A split signature encodes the position of a SplitPReg or a node;
// 0 means the node/preg isn't involved in any split,
// 1 means it is in the first split path, etc.

const int MaxSplitDepth = 7;

class SplitSig {
        // "this" encodes 7 split 4-bit IDs plus nesting level in lowest bits
        // this == 0 means top level, not in any split
        static const std::uint32_t LevelMask;
    public:
        SplitSig() { ShouldNotCallThis(); }
        friend SplitSig * new_SplitSig( SplitSig * current, int splitID );

        int level() { return std::uint32_t( this ) & LevelMask; }
        bool_t contains( SplitSig * other ) {
            // other sig is in same branch iff the receiver is a prefix of other
            // NB: this is not symmetric, i.e. it's like <=, not ==
            int shift = ( MaxSplitDepth - level() + 1 ) << 2;
            if ( shift == 32 ) return true;    // because x >> 32 is undefined
            return ( ( std::uint32_t( this ) ^ std::uint32_t( other ) ) >> shift ) == 0;
        }
        void print();
        const char * prefix( const char * buf );
};

#endif
