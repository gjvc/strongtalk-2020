//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/system/platform.hpp"
#include "vm/system/asserts.hpp"
#include "vm/system/macros.hpp"
#include "vm/oops/ByteArrayOopDescriptor.hpp"
#include "vm/oops/Klass.hpp"
#include "vm/oops/ObjectArrayOopDescriptor.hpp"
#include "vm/oops/MethodOopDescriptor.hpp"
#include "vm/runtime/TempDecoder.hpp"

#define NEXT                        \
  pos++;                            \
  if (pos > len) return;            \
  current = tempInfo->obj_at(pos)


void TempDecoder::decode( MethodOop method, int byteCodeIndex ) {
    // Format:
    //   name*                     parameters
    //   offset name*              stack-allocated temporaries
    //   offset name*              stack-allocated floats (offset is 0)
    //   offset name*              heap-allocated temporaries
    //   (begin end offset name+)* stack-allocated temporaries of inlined scopes

    ObjectArrayOop tempInfo = method->tempInfo();
    if ( tempInfo == nullptr ) {
        no_debug_info();
        return;
    }
    int len = tempInfo->length();
    if ( len == 0 )
        return;

    int pos     = 1;
    Oop current = tempInfo->obj_at( pos );

    { // scan parameters
        _num_of_params = 0;
        while ( current->is_byteArray() ) {
            parameter( ByteArrayOop( current ), _num_of_params++ );
            NEXT;
        }
    }

    { // scan global stack temps
        st_assert_smi( current, "expecting smi_t" );
        int offset = SMIOop( current )->value();
        NEXT;
        while ( current->is_byteArray() ) {
            stack_temp( ByteArrayOop( current ), offset++ );
            NEXT;
        }
    }

    { // scan global stack float temps
        st_assert_smi( current, "expecting smi_t" );
        st_assert( SMIOop(current)->value() == 0, "should be zero" )
        int fno = SMIOop( current )->value();
        NEXT;
        while ( current->is_byteArray() ) {
            stack_float_temp( ByteArrayOop( current ), fno++ );
            NEXT;
        }
    }

    { // scan global heap temps
        st_assert_smi( current, "expecting smi_t" );
        int offset = SMIOop( current )->value();
        NEXT;
        while ( current->is_byteArray() ) {
            if ( is_heap_parameter( ByteArrayOop( current ), tempInfo ) ) {
                heap_parameter( ByteArrayOop( current ), offset++ );
            } else {
                heap_temp( ByteArrayOop( current ), offset++ );
            }
            NEXT;
        }
    }
    { // scan inlined temps
        while ( 1 ) {
            st_assert_smi( current, "expecting smi_t" );
            int begin = SMIOop( current )->value();
            NEXT;
            st_assert_smi( current, "expecting smi_t" );
            int end = SMIOop( current )->value();
            NEXT;
            // Oop temps
            st_assert_smi( current, "expecting smi_t" );
            int offset = SMIOop( current )->value();
            NEXT;
            while ( current->is_byteArray() ) {
                if ( ( begin <= byteCodeIndex ) and ( byteCodeIndex <= end ) ) {
                    stack_temp( ByteArrayOop( current ), offset );
                }
                offset++;
                NEXT;
            }
            // Floats
            st_assert_smi( current, "expecting smi_t" );
            offset = SMIOop( current )->value();
            NEXT;
            while ( current->is_byteArray() ) {
                if ( ( begin <= byteCodeIndex ) and ( byteCodeIndex <= end ) ) {
                    stack_float_temp( ByteArrayOop( current ), offset );
                }
                offset++;
                NEXT;
            }
        }
    }
}


bool_t TempDecoder::is_heap_parameter( ByteArrayOop name, ObjectArrayOop tempInfo ) {
    st_assert( name->is_symbol(), "Must be symbol" );
    for ( int i = 1; i <= _num_of_params; i++ ) {
        ByteArrayOop par = ByteArrayOop( tempInfo->obj_at( i ) );
        st_assert( par->is_symbol(), "Must be symbol" );
        if ( name == par )
            return true;
    }
    return false;
}


void TempPrinter::decode( MethodOop method, int byteCodeIndex ) {
    _console->print_cr( "TempDecoding:" );
    TempDecoder::decode( method, byteCodeIndex );
}


void TempPrinter::parameter( ByteArrayOop name, int index ) {
    _console->print_cr( "  param:      %s@%d", name->as_string(), index );
}


void TempPrinter::stack_temp( ByteArrayOop name, int no ) {
    _console->print_cr( "  stack temp: %s@%d", name->as_string(), no );
}


void TempPrinter::stack_float_temp( ByteArrayOop name, int fno ) {
    _console->print_cr( "  stack float temp: %s@%d", name->as_string(), fno );
}


void TempPrinter::heap_temp( ByteArrayOop name, int no ) {
    _console->print_cr( "  heap temp:  %s@%d", name->as_string(), no );
}


void TempPrinter::heap_parameter( ByteArrayOop name, int no ) {
    _console->print_cr( "  heap param:  %s@%d", name->as_string(), no );
}


void TempPrinter::no_debug_info() {
    _console->print_cr( "method has no debug information" );
}


class FindParam : public TempDecoder {
    private:
        int the_no;
    public:
        ByteArrayOop result;


        void find( MethodOop method, int no ) {
            result = nullptr;
            the_no = no;
            decode( method, 0 );
        }


        void parameter( ByteArrayOop name, int no ) {
            if ( the_no == no )
                result = name;
        }
};


ByteArrayOop find_parameter_name( MethodOop method, int no ) {
    FindParam p;
    p.find( method, no );
    return p.result;
}


class FindStackTemp : public TempDecoder {
    private:
        int the_no;
    public:
        ByteArrayOop result;


        void find( MethodOop method, int byteCodeIndex, int no ) {
            result = nullptr;
            the_no = no;
            TempDecoder::decode( method, byteCodeIndex );
        }


        void stack_temp( ByteArrayOop name, int no ) {
            if ( the_no == no )
                result = name;
        }
};

class FindStackFloatTemp : public TempDecoder {
    private:
        int the_fno;
    public:
        ByteArrayOop result;


        void find( MethodOop method, int byteCodeIndex, int fno ) {
            result  = nullptr;
            the_fno = fno;
            TempDecoder::decode( method, byteCodeIndex );
        }


        void stack_float_temp( ByteArrayOop name, int fno ) {
            if ( the_fno == fno )
                result = name;
        }
};

class FindHeapTemp : public TempDecoder {
    private:
        int the_no;
    public:
        ByteArrayOop result;


        void find( MethodOop method, int byteCodeIndex, int no ) {
            result = nullptr;
            the_no = no;
            TempDecoder::decode( method, byteCodeIndex );
        }


        void heap_temp( ByteArrayOop name, int no ) {
            if ( the_no == no )
                result = name;
        }


        void heap_parameter( ByteArrayOop name, int no ) {
            if ( the_no == no )
                result = name;
        }

};


ByteArrayOop find_stack_temp( MethodOop method, int byteCodeIndex, int no ) {
    FindStackTemp p;
    p.find( method, byteCodeIndex, no );
    return p.result;
}


ByteArrayOop find_heap_temp( MethodOop method, int byteCodeIndex, int no ) {
    FindHeapTemp p;
    p.find( method, byteCodeIndex, no );
    return p.result;
}


ByteArrayOop find_stack_float_temp( MethodOop method, int byteCodeIndex, int fno ) {
    FindStackFloatTemp p;
    p.find( method, byteCodeIndex, fno );
    return p.result;
}
