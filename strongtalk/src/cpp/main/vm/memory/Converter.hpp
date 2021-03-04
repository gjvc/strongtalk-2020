//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/system/platform.hpp"
#include "vm/system/asserts.hpp"
#include "allocation.hpp"
#include "vm/memory/Reflection.hpp"
#include "vm/oop/KlassOopDescriptor.hpp"
#include "vm/runtime/flags.hpp"
#include "vm/oop/ProcessOopDescriptor.hpp"
#include "vm/oop/ProxyOopDescriptor.hpp"
#include "vm/oop/DoubleOopDescriptor.hpp"
#include "vm/oop/DoubleByteArrayOopDescriptor.hpp"
#include "vm/oop/ObjectArrayOopDescriptor.hpp"
#include "vm/oop/DoubleValueArrayOopDescriptor.hpp"
#include "vm/runtime/ResourceObject.hpp"


// Converter hierarchy:
// - memConverter
//   - byteArrayConverter
//   - doubleByteArrayConverter
//   - doubleValueArrayConverter
//   - klassConverter
//   - mixinConverter
//   - objectArrayConverter
//   - processConverter
//   - proxyConverter

class memConverter : public ResourceObject {

protected:
    KlassOop                    _oldKlass;
    KlassOop                    _newKlass;
    GrowableArray<std::int32_t> *_mapping;


    void compute_mapping() {
        _mapping                     = new GrowableArray<std::int32_t>( 20 );
        std::int32_t old_header_size = _oldKlass->klass_part()->oop_header_size();
//        std::int32_t new_header_size = _newKlass->klass_part()->oop_header_size();
        std::int32_t n               = _oldKlass->klass_part()->number_of_instance_variables();

        for ( std::int32_t old_index = 0; old_index < n; old_index++ ) {
            SymbolOop name = _oldKlass->klass_part()->inst_var_name_at( old_index + old_header_size );
            st_assert( name->isSymbol(), "instance variable name must be symbol" );
            std::int32_t new_index = _newKlass->klass_part()->lookup_inst_var( name );
            if ( new_index > 0 ) {
                if ( TraceApplyChange ) {
                    _console->print( "map " );
                    name->print_symbol_on( _console );
                    SPDLOG_INFO( " {} -> {}", old_index + old_header_size, new_index );
                }
                // We found a match between a old and new class
                _mapping->push( old_index + old_header_size );
                _mapping->push( new_index );
            }
        }
    }


public:
    memConverter( KlassOop old_klass, KlassOop new_klass ) :
        _oldKlass{ old_klass },
        _newKlass{ new_klass },
        _mapping{ nullptr } {
        compute_mapping();
    }


    memConverter() = default;
    virtual ~memConverter() = default;
    memConverter( const memConverter & ) = default;
    memConverter &operator=( const memConverter & ) = default;
    void operator delete( void *ptr ) { (void)(ptr); }



    MemOop convert( MemOop src ) {
        MemOop dst = allocate( src );
        // Transfer contents from src to dst
        transfer( src, dst );
        Reflection::forward( src, dst );
        return dst;
    }


    virtual void transfer( MemOop src, MemOop dst ) {
        // header information
        if ( src->mark()->is_near_death() )
            dst->mark_as_dying();
        dst->set_identity_hash( src->identity_hash() );

        // Instance variables
        for ( std::size_t i = 0; i < _mapping->length(); i += 2 ) {
            std::int32_t from = _mapping->at( i );
            std::int32_t to   = _mapping->at( i + 1 );
            dst->raw_at_put( to, src->raw_at( from ) );
        }
    }


    virtual MemOop allocate( MemOop src ) {
        static_cast<void>(src); // unused
        return MemOop( _newKlass->klass_part()->allocateObject() );
    }
};


class proxyConverter : public memConverter {
private:
    bool _sourceIsProxy;

public:
    proxyConverter( KlassOop old_klass, KlassOop new_klass ) :
        memConverter( old_klass, new_klass ),
        _sourceIsProxy{ false } {

        st_assert( new_klass->klass_part()->oopIsProxy(), "new_klass must be a proxy klass" );
        _sourceIsProxy = old_klass->klass_part()->oopIsProxy();
    }


    void transfer( MemOop src, MemOop dst ) {
        if ( _sourceIsProxy ) {
            ProxyOop( dst )->set_pointer( ProxyOop( src )->get_pointer() );
        }
        memConverter::transfer( src, dst );
    }
};


class processConverter : public memConverter {
private:
    bool _sourceIsProcess;
public:
    processConverter( KlassOop old_klass, KlassOop new_klass ) :
        memConverter( old_klass, new_klass ),
        _sourceIsProcess{ false } {

        st_assert( new_klass->klass_part()->oopIsProcess(), "new_klass must be a process klass" );
        _sourceIsProcess = old_klass->klass_part()->oopIsProcess();
    }


    void transfer( MemOop src, MemOop dst ) {
        if ( _sourceIsProcess ) {
            ProcessOop( dst )->set_process( ProcessOop( src )->process() );
        }
        memConverter::transfer( src, dst );
    }
};


class byteArrayConverter : public memConverter {

private:
    bool _sourceIsByteArray;

public:
    byteArrayConverter( KlassOop old_klass, KlassOop new_klass ) :
        memConverter( old_klass, new_klass ),
        _sourceIsByteArray{ false } {

        st_assert( new_klass->klass_part()->oopIsByteArray(), "new_klass must be a byteArray klass" );
        _sourceIsByteArray = old_klass->klass_part()->oopIsByteArray();
    }


    void transfer( MemOop src, MemOop dst ) {
        memConverter::transfer( src, dst );
        if ( _sourceIsByteArray ) {
            std::size_t length = ByteArrayOop( src )->length();

            for ( std::size_t i = 1; i <= length; i++ ) {
                ByteArrayOop( dst )->byte_at_put( i, ByteArrayOop( src )->byte_at( i ) );
            }
        }
    }


    MemOop allocate( MemOop src ) {
        std::int32_t len = _sourceIsByteArray ? ByteArrayOop( src )->length() : 0;
        return MemOop( _newKlass->klass_part()->allocateObjectSize( len ) );
    }
};


class doubleByteArrayConverter : public memConverter {

private:
    bool _sourceIsDoubleByteArray;

public:
    doubleByteArrayConverter( KlassOop old_klass, KlassOop new_klass ) :
        memConverter( old_klass, new_klass ),
        _sourceIsDoubleByteArray{ false } {
        st_assert( new_klass->klass_part()->oopIsDoubleByteArray(), "new_klass must be a byteArray klass" );
        _sourceIsDoubleByteArray = old_klass->klass_part()->oopIsDoubleByteArray();
    }


    void transfer( MemOop src, MemOop dst ) {
        memConverter::transfer( src, dst );
        if ( _sourceIsDoubleByteArray ) {
            std::size_t length = DoubleByteArrayOop( src )->length();

            for ( std::size_t i = 1; i <= length; i++ ) {
                DoubleByteArrayOop( dst )->doubleByte_at_put( i, DoubleByteArrayOop( src )->doubleByte_at( i ) );
            }

        }
    }


    MemOop allocate( MemOop src ) {
        std::int32_t len = _sourceIsDoubleByteArray ? DoubleByteArrayOop( src )->length() : 0;
        return MemOop( _newKlass->klass_part()->allocateObjectSize( len ) );
    }
};


class objectArrayConverter : public memConverter {

private:
    bool _sourceIsObjectArray;

public:
    objectArrayConverter( KlassOop old_klass, KlassOop new_klass ) :
        memConverter( old_klass, new_klass ),
        _sourceIsObjectArray{ false } {
        st_assert( new_klass->klass_part()->oopIsObjectArray(), "new_klass must be a objectArray klass" );
        _sourceIsObjectArray = old_klass->klass_part()->oopIsObjectArray();
    }


    void transfer( MemOop src, MemOop dst ) {
        memConverter::transfer( src, dst );
        if ( _sourceIsObjectArray ) {

            std::size_t length = ObjectArrayOop( src )->length();

            for ( std::size_t i = 1; i <= length; i++ ) {
                ObjectArrayOop( dst )->obj_at_put( i, ObjectArrayOop( src )->obj_at( i ) );
            }

        }
    }


    MemOop allocate( MemOop src ) {
        std::int32_t len = _sourceIsObjectArray ? ObjectArrayOop( src )->length() : 0;
        return MemOop( _newKlass->klass_part()->allocateObjectSize( len ) );
    }
};


class doubleValueArrayConverter : public memConverter {
private:
    bool source_is_obj_array;
public:
    doubleValueArrayConverter( KlassOop old_klass, KlassOop new_klass ) :
        memConverter( old_klass, new_klass ),
        source_is_obj_array{ false } {
        st_assert( new_klass->klass_part()->oopIsDoubleValueArray(), "new_klass must be a doubleValueArray klass" );
        source_is_obj_array = old_klass->klass_part()->oopIsDoubleValueArray();
    }


    void transfer( MemOop src, MemOop dst ) {
        memConverter::transfer( src, dst );
        if ( source_is_obj_array ) {
            std::size_t length = DoubleValueArrayOop( src )->length();

            for ( std::size_t i = 1; i <= length; i++ ) {
                DoubleValueArrayOop( dst )->double_at_put( i, DoubleValueArrayOop( src )->double_at( i ) );
            }
        }
    }


    MemOop allocate( MemOop src ) {
        std::int32_t len = source_is_obj_array ? DoubleValueArrayOop( src )->length() : 0;
        return MemOop( _newKlass->klass_part()->allocateObjectSize( len ) );
    }
};


class klassConverter : public memConverter {

public:
    klassConverter( KlassOop old_klass, KlassOop new_klass ) :
        memConverter( old_klass, new_klass ) {
        st_assert( old_klass->klass_part()->oopIsKlass(), "new_klass must be a klass klass" );
        st_assert( new_klass->klass_part()->oopIsKlass(), "new_klass must be a klass klass" );
    }


    void transfer( MemOop src, MemOop dst ) {
        memConverter::transfer( src, dst );
    }


    MemOop allocate( MemOop src ) {
        static_cast<void>(src); // unused

        Unimplemented();
        return nullptr;
    }
};


class mixinConverter : public memConverter {

public:
    mixinConverter( KlassOop old_klass, KlassOop new_klass ) :
        memConverter( old_klass, new_klass ) {
        st_assert( old_klass->klass_part()->oopIsMixin(), "new_klass must be a mixin klass" );
        st_assert( new_klass->klass_part()->oopIsMixin(), "new_klass must be a mixin klass" );
    }


    void transfer( MemOop src, MemOop dst ) {
        //Unimplemented();
        memConverter::transfer( src, dst );
    }
};


class ConvertClosure : public OopClosure {

private:
    void do_oop( Oop *o ) {
        Reflection::convert( o );
    }
};


class ConvertOopClosure : public ObjectClosure {

public:
    void do_object( MemOop obj ) {
        if ( obj->klass()->klass_part()->is_marked_for_schema_change() ) {
            return;
        }

        ConvertClosure blk;
        obj->oop_iterate( &blk );
    }
};
