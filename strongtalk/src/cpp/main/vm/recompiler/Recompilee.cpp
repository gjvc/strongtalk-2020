
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/recompiler/Recompilation.hpp"
#include "vm/recompiler/Recompilee.hpp"
#include "vm/recompiler/RecompilerFrame.hpp"


Recompilee *Recompilee::new_Recompilee( RecompilerFrame *recompilerFrame ) {
    if ( recompilerFrame->is_compiled() ) {
        return new CompiledRecompilee( recompilerFrame, ( (CompiledRecompilerFrame *) recompilerFrame )->nm() );
    } else {
        InterpretedRecompilerFrame *irf = (InterpretedRecompilerFrame *) recompilerFrame;
        // Urs, please check this!
        st_assert( irf->key() not_eq nullptr, "must have a key" );
        return new InterpretedRecompilee( irf, irf->key(), irf->top_method() );
    }
}
