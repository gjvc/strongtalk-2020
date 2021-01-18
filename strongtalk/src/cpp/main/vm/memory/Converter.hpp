//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/system/platform.hpp"
#include "vm/system/asserts.hpp"
#include "allocation.hpp"
#include "vm/memory/Reflection.hpp"
#include "vm/oops/KlassOopDescriptor.hpp"
#include "vm/runtime/flags.hpp"
#include "vm/oops/ProcessOopDescriptor.hpp"
#include "vm/oops/ProxyOopDescriptor.hpp"
#include "vm/oops/DoubleOopDescriptor.hpp"
#include "vm/oops/DoubleByteArrayOopDescriptor.hpp"
#include "vm/oops/ObjectArrayOopDescriptor.hpp"
#include "vm/oops/DoubleValueArrayOopDescriptor.hpp"
#include "vm/runtime/ResourceObject.hpp"


// Converter hierarchy:
// - memConverter
//   - byteArrayConverter
//   - doubleByteArrayConverter
//   - doubleValueArrayConverter
//   - klassConverter
//   - mixinConverter
//   - objArrayConverter
//   - processConverter
//   - proxyConverter

class memConverter : public ResourceObject {

protected:
    KlassOop _oldKlass;
    KlassOop _newKlass;
    GrowableArray<int> *_mapping;


    void compute_mapping() {
        _mapping            = new GrowableArray<int>( 20 );
        int old_header_size = _oldKlass->klass_part()->oop_header_size();
        int new_header_size = _newKlass->klass_part()->oop_header_size();
        int n               = _oldKlass->klass_part()->number_of_instance_variables();

        for ( int old_index = 0; old_index < n; old_index++ ) {
            SymbolOop name = _oldKlass->klass_part()->inst_var_name_at( old_index + old_header_size );
            st_assert( name->is_symbol(), "instance variable name must be symbol" );
            int new_index = _newKlass->klass_part()->lookup_inst_var( name );
            if ( new_index > 0 ) {
                if ( TraceApplyChange ) {
                    _console->print( "map " );
                    name->print_symbol_on( _console );
                    _console->print_cr( " %d -> %d", old_index + old_header_size, new_index );
                }
                // We found a match between a old and new class
                _mapping->push( old_index + old_header_size );
                _mapping->push( new_index );
            }
        }
    }


public:
    memConverter( KlassOop old_klass, KlassOop new_klass ) {
        _oldKlass = old_klass;
        _newKlass = new_klass;
        compute_mapping();
    }


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
        for ( int i = 0; i < _mapping->length(); i += 2 ) {
            int from = _mapping->at( i );
            int to   = _mapping->at( i + 1 );
            dst->raw_at_put( to, src->raw_at( from ) );
        }
    }


    virtual MemOop allocate( MemOop src ) {
        return MemOop( _newKlass->klass_part()->allocateObject() );
    }
};

class proxyConverter : public memConverter {
private:
    bool_t _sourceIsProxy;
public:
    proxyConverter( KlassOop old_klass, KlassOop new_klass ) :
            memConverter( old_klass, new_klass ) {
        st_assert( new_klass->klass_part()->oop_is_proxy(), "new_klass must be a proxy klass" );
        _sourceIsProxy = old_klass->klass_part()->oop_is_proxy();
    }


    void transfer( MemOop src, MemOop dst ) {
        if ( _sourceIsProxy )
            ProxyOop( dst )->set_pointer( ProxyOop( src )->get_pointer() );
        memConverter::transfer( src, dst );
    }
};

class processConverter : public memConverter {
private:
    bool_t _sourceIsProcess;
public:
    processConverter( KlassOop old_klass, KlassOop new_klass ) :
            memConverter( old_klass, new_klass ) {
        st_assert( new_klass->klass_part()->oop_is_process(), "new_klass must be a process klass" );
        _sourceIsProcess = old_klass->klass_part()->oop_is_process();
    }


    void transfer( MemOop src, MemOop dst ) {
        if ( _sourceIsProcess )
            ProcessOop( dst )->set_process( ProcessOop( src )->process() );
        memConverter::transfer( src, dst );
    }
};

class byteArrayConverter : public memConverter {

private:
    bool_t _sourceIsByteArray;

public:
    byteArrayConverter( KlassOop old_klass, KlassOop new_klass ) :
            memConverter( old_klass, new_klass ) {
        st_assert( new_klass->klass_part()->oop_is_byteArray(), "new_klass must be a byteArray klass" );
        _sourceIsByteArray = old_klass->klass_part()->oop_is_byteArray();
    }


    void transfer( MemOop src, MemOop dst ) {
        memConverter::transfer( src, dst );
        if ( _sourceIsByteArray ) {
            int length = ByteArrayOop( src )->length();

            for ( int i = 1; i <= length; i++ )
                ByteArrayOop( dst )->byte_at_put( i, ByteArrayOop( src )->byte_at( i ) );
        }
    }


    MemOop allocate( MemOop src ) {
        int len = _sourceIsByteArray ? ByteArrayOop( src )->length() : 0;
        return MemOop( _newKlass->klass_part()->allocateObjectSize( len ) );
    }
};

class doubleByteArrayConverter : public memConverter {

private:
    bool_t _sourceIsDoubleByteArray;

public:
    doubleByteArrayConverter( KlassOop old_klass, KlassOop new_klass ) :
            memConverter( old_klass, new_klass ) {
        st_assert( new_klass->klass_part()->oop_is_doubleByteArray(), "new_klass must be a byteArray klass" );
        _sourceIsDoubleByteArray = old_klass->klass_part()->oop_is_doubleByteArray();
    }


    void transfer( MemOop src, MemOop dst ) {
        memConverter::transfer( src, dst );
        if ( _sourceIsDoubleByteArray ) {
            int length = DoubleByteArrayOop( src )->length();

            for ( int i = 1; i <= length; i++ )
                DoubleByteArrayOop( dst )->doubleByte_at_put( i, DoubleByteArrayOop( src )->doubleByte_at( i ) );
        }
    }


    MemOop allocate( MemOop src ) {
        int len = _sourceIsDoubleByteArray ? DoubleByteArrayOop( src )->length() : 0;
        return MemOop( _newKlass->klass_part()->allocateObjectSize( len ) );
    }
};

class objArrayConverter : public memConverter {

private:
    bool_t _sourceIsObjArray;

public:
    objArrayConverter( KlassOop old_klass, KlassOop new_klass ) :
            memConverter( old_klass, new_klass ) {
        st_assert( new_klass->klass_part()->oop_is_objArray(), "new_klass must be a objArray klass" );
        _sourceIsObjArray = old_klass->klass_part()->oop_is_objArray();
    }


    void transfer( MemOop src, MemOop dst ) {
        memConverter::transfer( src, dst );
        if ( _sourceIsObjArray ) {

            int length = ObjectArrayOop( src )->length();

            for ( int i = 1; i <= length; i++ )
                ObjectArrayOop( dst )->obj_at_put( i, ObjectArrayOop( src )->obj_at( i ) );
        }
    }


    MemOop allocate( MemOop src ) {
        int len = _sourceIsObjArray ? ObjectArrayOop( src )->length() : 0;
        return MemOop( _newKlass->klass_part()->allocateObjectSize( len ) );
    }
};

class doubleValueArrayConverter : public memConverter {
private:
    bool_t source_is_obj_array;
public:
    doubleValueArrayConverter( KlassOop old_klass, KlassOop new_klass ) :
            memConverter( old_klass, new_klass ) {
        st_assert( new_klass->klass_part()->oop_is_doubleValueArray(), "new_klass must be a doubleValueArray klass" );
        source_is_obj_array = old_klass->klass_part()->oop_is_doubleValueArray();
    }


    void transfer( MemOop src, MemOop dst ) {
        memConverter::transfer( src, dst );
        if ( source_is_obj_array ) {
            int length = doubleValueArrayOop( src )->length();

            for ( int i = 1; i <= length; i++ )
                doubleValueArrayOop( dst )->double_at_put( i, doubleValueArrayOop( src )->double_at( i ) );
        }
    }


    MemOop allocate( MemOop src ) {
        int len = source_is_obj_array ? doubleValueArrayOop( src )->length() : 0;
        return MemOop( _newKlass->klass_part()->allocateObjectSize( len ) );
    }
};

class klassConverter : public memConverter {

public:
    klassConverter( KlassOop old_klass, KlassOop new_klass ) :
            memConverter( old_klass, new_klass ) {
        st_assert( old_klass->klass_part()->oop_is_klass(), "new_klass must be a klass klass" );
        st_assert( new_klass->klass_part()->oop_is_klass(), "new_klass must be a klass klass" );
    }


    void transfer( MemOop src, MemOop dst ) {
        memConverter::transfer( src, dst );
    }


    MemOop allocate( MemOop src ) {
        Unimplemented();
        return nullptr;
    }
};

class mixinConverter : public memConverter {

public:
    mixinConverter( KlassOop old_klass, KlassOop new_klass ) :
            memConverter( old_klass, new_klass ) {
        st_assert( old_klass->klass_part()->oop_is_mixin(), "new_klass must be a mixin klass" );
        st_assert( new_klass->klass_part()->oop_is_mixin(), "new_klass must be a mixin klass" );
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
        if ( obj->klass()->klass_part()->is_marked_for_schema_change() )
            return;
        ConvertClosure blk;
        obj->oop_iterate( &blk );
    }
};
