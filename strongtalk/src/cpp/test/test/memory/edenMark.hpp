//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/system/platform.hpp"
#include "vm/system/asserts.hpp"
#include "vm/system/macros.hpp"
#include "vm/memory/util.hpp"


extern "C" Oop * eden_top;
extern "C" Oop * eden_end;

class EdenMark : ValueObject {

    private:
        Oop * old_eden_top;

    public:
        EdenMark() {
            old_eden_top = eden_top;
        }


        ~EdenMark() {
            eden_top = old_eden_top;
        }


        void setToEnd() {
            eden_top = eden_end;
        }
};
