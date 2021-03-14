//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/platform/platform.hpp"
#include "vm/system/asserts.hpp"
#include "vm/system/macros.hpp"
#include "vm/klass/Klass.hpp"
#include "vm/oop/ObjectArrayOopDescriptor.hpp"
#include "vm/oop/MethodOopDescriptor.hpp"
#include "vm/oop/ByteArrayOopDescriptor.hpp"
#include "vm/runtime/TempDecoder.hpp"


void TempDecoder::decode( MethodOop method, std::int32_t byteCodeIndex ) {

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

    std::int32_t len = tempInfo->length();
    if ( len == 0 )
        return;

    std::int32_t pos     = 1;
    Oop          current = tempInfo->obj_at( pos );

    { // scan parameters
        _num_of_params = 0;
        while ( current->isByteArray() ) {
            parameter( ByteArrayOop( current ), _num_of_params++ );
            {
                pos++;
                if ( pos > len ) { return; }
                current = tempInfo->obj_at( pos );
            } // advance-to-next
        }
    }

    { // scan global stack temps
        st_assert_smi( current, "expecting small_int_t" );
        std::int32_t offset = SmallIntegerOop( current )->value();
        {
            pos++;
            if ( pos > len ) return;
            current = tempInfo->obj_at( pos );
        } // advance-to-next

        while ( current->isByteArray() ) {
            stack_temp( ByteArrayOop( current ), offset++ );
            {
                pos++;
                if ( pos > len ) return;
                current = tempInfo->obj_at( pos );
            } // advance-to-next
        }
    }

    { // scan global stack float temps
        st_assert_smi( current, "expecting small_int_t" );
        st_assert( SmallIntegerOop(current)->value() == 0, "should be zero" )
        std::int32_t fno = SmallIntegerOop( current )->value();
        {
            pos++;
            if ( pos > len ) return;
            current = tempInfo->obj_at( pos );
        } // advance-to-next

        while ( current->isByteArray() ) {
            stack_float_temp( ByteArrayOop( current ), fno++ );
            {
                pos++;
                if ( pos > len ) return;
                current = tempInfo->obj_at( pos );
            } // advance-to-next
        }
    }

    { // scan global heap temps
        st_assert_smi( current, "expecting small_int_t" );
        std::int32_t offset = SmallIntegerOop( current )->value();
        {
            pos++;
            if ( pos > len ) return;
            current = tempInfo->obj_at( pos );
        } // advance-to-next
        while ( current->isByteArray() ) {
            if ( is_heap_parameter( ByteArrayOop( current ), tempInfo ) ) {
                heap_parameter( ByteArrayOop( current ), offset++ );
            } else {
                heap_temp( ByteArrayOop( current ), offset++ );
            }
            {
                pos++;
                if ( pos > len ) return;
                current = tempInfo->obj_at( pos );
            } // advance-to-next
        }
    }
    { // scan inlined temps
        while ( 1 ) {
            st_assert_smi( current, "expecting small_int_t" );
            std::int32_t begin = SmallIntegerOop( current )->value();
            {
                pos++;
                if ( pos > len ) return;
                current = tempInfo->obj_at( pos );
            } // advance-to-next
            st_assert_smi( current, "expecting small_int_t" );
            std::int32_t end = SmallIntegerOop( current )->value();
            {
                pos++;
                if ( pos > len ) return;
                current = tempInfo->obj_at( pos );
            } // advance-to-next
            // Oop temps
            st_assert_smi( current, "expecting small_int_t" );
            std::int32_t offset = SmallIntegerOop( current )->value();
            {
                pos++;
                if ( pos > len ) return;
                current = tempInfo->obj_at( pos );
            } // advance-to-next
            while ( current->isByteArray() ) {
                if ( ( begin <= byteCodeIndex ) and ( byteCodeIndex <= end ) ) {
                    stack_temp( ByteArrayOop( current ), offset );
                }
                offset++;
                {
                    pos++;
                    if ( pos > len ) return;
                    current = tempInfo->obj_at( pos );
                } // advance-to-next
            }
            // Floats
            st_assert_smi( current, "expecting small_int_t" );
            offset = SmallIntegerOop( current )->value();
            {
                pos++;
                if ( pos > len ) return;
                current = tempInfo->obj_at( pos );
            } // advance-to-next
            while ( current->isByteArray() ) {
                if ( ( begin <= byteCodeIndex ) and ( byteCodeIndex <= end ) ) {
                    stack_float_temp( ByteArrayOop( current ), offset );
                }
                offset++;
                {
                    pos++;
                    if ( pos > len ) return;
                    current = tempInfo->obj_at( pos );
                } // advance-to-next
            }
        }
    }
}


bool TempDecoder::is_heap_parameter( ByteArrayOop name, ObjectArrayOop tempInfo ) {
    st_assert( name->isSymbol(), "Must be symbol" );
    for ( std::size_t i = 1; i <= _num_of_params; i++ ) {
        ByteArrayOop par = ByteArrayOop( tempInfo->obj_at( i ) );
        st_assert( par->isSymbol(), "Must be symbol" );
        if ( name == par )
            return true;
    }
    return false;
}


void TempPrinter::decode( MethodOop method, std::int32_t byteCodeIndex ) {
    SPDLOG_INFO( "TempDecoding:" );
    TempDecoder::decode( method, byteCodeIndex );
}


void TempPrinter::parameter( ByteArrayOop name, std::int32_t index ) {
    SPDLOG_INFO( "  param:      {}@{}", name->as_string(), index );
}


void TempPrinter::stack_temp( ByteArrayOop name, std::int32_t no ) {
    SPDLOG_INFO( "  stack temp: {}@{}", name->as_string(), no );
}


void TempPrinter::stack_float_temp( ByteArrayOop name, std::int32_t fno ) {
    SPDLOG_INFO( "  stack float temp: {}@{}", name->as_string(), fno );
}


void TempPrinter::heap_temp( ByteArrayOop name, std::int32_t no ) {
    SPDLOG_INFO( "  heap temp:  {}@{}", name->as_string(), no );
}


void TempPrinter::heap_parameter( ByteArrayOop name, std::int32_t no ) {
    SPDLOG_INFO( "  heap param:  {}@{}", name->as_string(), no );
}


void TempPrinter::no_debug_info() {
    SPDLOG_INFO( "method has no debug information" );
}


class FindParam : public TempDecoder {
private:
    std::int32_t the_no;
public:
    ByteArrayOop result;

    FindParam() : the_no{0}, result{} {}
    virtual ~FindParam() =default;
    FindParam( const FindParam & ) = default;
    FindParam &operator=( const FindParam & ) = default;
    void operator delete( void *ptr ) { (void)(ptr); }



    void find( MethodOop method, std::int32_t no ) {
        result = nullptr;
        the_no = no;
        decode( method, 0 );
    }


    void parameter( ByteArrayOop name, std::int32_t no ) {
        if ( the_no == no )
            result = name;
    }
};


ByteArrayOop find_parameter_name( MethodOop method, std::int32_t no ) {
    FindParam p;
    p.find( method, no );
    return p.result;
}


class FindStackTemp : public TempDecoder {
private:
    std::int32_t the_no;
public:
    ByteArrayOop result;


    FindStackTemp() : the_no{0}, result{} {

    }
    virtual ~FindStackTemp() = default;
    FindStackTemp( const FindStackTemp & ) = default;
    FindStackTemp &operator=( const FindStackTemp & ) = default;
    void operator delete( void *ptr ) { (void)(ptr); }



    void find( MethodOop method, std::int32_t byteCodeIndex, std::int32_t no ) {
        result = nullptr;
        the_no = no;
        TempDecoder::decode( method, byteCodeIndex );
    }


    void stack_temp( ByteArrayOop name, std::int32_t no ) {
        if ( the_no == no )
            result = name;
    }
};


class FindStackFloatTemp : public TempDecoder {
private:
    std::int32_t the_fno;
public:
    ByteArrayOop result;

    FindStackFloatTemp() : the_fno{0}, result{} {}
    virtual ~FindStackFloatTemp() = default;
    FindStackFloatTemp( const FindStackFloatTemp & ) = default;
    FindStackFloatTemp &operator=( const FindStackFloatTemp & ) = default;
    void operator delete( void *ptr ) { (void)(ptr); }




    void find( MethodOop method, std::int32_t byteCodeIndex, std::int32_t fno ) {
        result  = nullptr;
        the_fno = fno;
        TempDecoder::decode( method, byteCodeIndex );
    }


    void stack_float_temp( ByteArrayOop name, std::int32_t fno ) {
        if ( the_fno == fno ) {
            result = name;
        }
    }
};


class FindHeapTemp : public TempDecoder {

private:
    std::int32_t the_no;

public:
    ByteArrayOop result;


    FindHeapTemp() : the_no{0}, result{} {

    }
    virtual ~FindHeapTemp() = default;
    FindHeapTemp( const FindHeapTemp & ) = default;
    FindHeapTemp &operator=( const FindHeapTemp & ) = default;
    void operator delete( void *ptr ) { (void)(ptr); }



    void find( MethodOop method, std::int32_t byteCodeIndex, std::int32_t no ) {
        result = nullptr;
        the_no = no;
        TempDecoder::decode( method, byteCodeIndex );
    }


    void heap_temp( ByteArrayOop name, std::int32_t no ) {
        if ( the_no == no )
            result = name;
    }


    void heap_parameter( ByteArrayOop name, std::int32_t no ) {
        if ( the_no == no )
            result = name;
    }

};


ByteArrayOop find_stack_temp( MethodOop method, std::int32_t byteCodeIndex, std::int32_t no ) {
    FindStackTemp p;
    p.find( method, byteCodeIndex, no );
    return p.result;
}


ByteArrayOop find_heap_temp( MethodOop method, std::int32_t byteCodeIndex, std::int32_t no ) {
    FindHeapTemp p;
    p.find( method, byteCodeIndex, no );
    return p.result;
}


ByteArrayOop find_stack_float_temp( MethodOop method, std::int32_t byteCodeIndex, std::int32_t fno ) {
    FindStackFloatTemp p;
    p.find( method, byteCodeIndex, fno );
    return p.result;
}
