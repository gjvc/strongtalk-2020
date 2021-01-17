//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once


// Run-time system code to handle uncommon traps
//
// An uncommon trap can be in two states:
//  - unused; when it has never been executed.
//  - used;   when the trap has been executed before.
//
// Initially a uncommon trap is in the unused state
// but the first time it is executed the state will be changed to used.
// The state is reflected in the call destination. See StubRoutines.hpp.

// This routine is called by either used_uncommon_trap & unused_uncommon_trap
// (see StubRoutines.hpp).

extern void uncommon_trap();
