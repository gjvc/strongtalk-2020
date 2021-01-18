//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/code/NativeMethod.hpp"
#include "vm/compiler/Compiler.hpp"
#include "vm/runtime/vmOperations.hpp"
#include "vm/code/ProgramCounterDescriptor.hpp"
#include "vm/code/StubRoutines.hpp"
#include "vm/oops/AssociationOopDescriptor.hpp"
#include "vm/memory/oopFactory.hpp"
#include "vm/interpreter/MissingMethodBuilder.hpp"
#include "vm/oops/KlassOopDescriptor.hpp"
#include "vm/oops/MethodOopDescriptor.hpp"
#include "vm/oops/MethodKlass.hpp"
#include "vm/oops/ObjectArrayOopDescriptor.hpp"
#include "vm/memory/Scavenge.hpp"


void MissingMethodBuilder::build() {
    BlockScavenge bs;

    int argCount = _selector->number_of_arguments();
    if ( argCount > 0 )
        _buffer.pushByte( static_cast<std::uint8_t>(ByteCodes::Code::allocate_temp_1) );

    _buffer.pushByte( static_cast<std::uint8_t>(ByteCodes::Code::push_global) );
    _buffer.pushOop( Universe::find_global_association( "Message" ) );
    _buffer.pushByte( static_cast<std::uint8_t>(ByteCodes::Code::push_self) );
    _buffer.pushByte( static_cast<std::uint8_t>(ByteCodes::Code::push_literal) );
    _buffer.pushOop( _selector );

    if ( argCount == 0 ) {
        _buffer.pushByte( static_cast<std::uint8_t>(ByteCodes::Code::push_literal) );
        _buffer.pushOop( oopFactory::new_objArray( 0 ) );

    } else {
        _buffer.pushByte( static_cast<std::uint8_t>(ByteCodes::Code::push_global) );
        _buffer.pushOop( Universe::find_global_association( "Array" ) );
        _buffer.pushByte( static_cast<std::uint8_t>(ByteCodes::Code::push_succ_n) );
        _buffer.pushByte( argCount - 1 );
        _buffer.pushByte( static_cast<std::uint8_t>(ByteCodes::Code::interpreted_send_1) );
        _buffer.pushOop( oopFactory::new_symbol( "new:" ) );
        _buffer.pushOop( smiOopFromValue( 0 ) );
        _buffer.pushByte( static_cast<std::uint8_t>(ByteCodes::Code::store_temp_n) );
        _buffer.pushByte( 0xFF );

        for ( int i = 0; i < argCount; i++ ) {
            _buffer.pushByte( static_cast<std::uint8_t>(ByteCodes::Code::push_succ_n) );
            _buffer.pushByte( i );
            _buffer.pushByte( static_cast<std::uint8_t>(ByteCodes::Code::push_arg_n) );
            _buffer.pushByte( argCount - i - 1 );
            _buffer.pushByte( static_cast<std::uint8_t>(ByteCodes::Code::interpreted_send_2_pop) );
            _buffer.pushOop( oopFactory::new_symbol( "at:put:" ) );
            _buffer.pushOop( smiOopFromValue( 0 ) );
            _buffer.pushByte( static_cast<std::uint8_t>(ByteCodes::Code::push_temp_0) );
        }
    }
    _buffer.pushByte( static_cast<std::uint8_t>(ByteCodes::Code::interpreted_send_n) );
    _buffer.pushByte( 3 );
    _buffer.pushOop( oopFactory::new_symbol( "receiver:selector:arguments:" ) );
    _buffer.pushOop( smiOopFromValue( 0 ) );
    _buffer.pushByte( static_cast<std::uint8_t>(ByteCodes::Code::interpreted_send_self) );
    _buffer.pushOop( oopFactory::new_symbol( "doesNotUnderstand:" ) );
    _buffer.pushOop( smiOopFromValue( 0 ) );

    switch ( argCount ) {
        case 0:
            _buffer.pushByte( static_cast<std::uint8_t>(ByteCodes::Code::return_tos_pop_0) );
            break;
        case 1:
            _buffer.pushByte( static_cast<std::uint8_t>(ByteCodes::Code::return_tos_pop_1) );
            break;
        case 2:
            _buffer.pushByte( static_cast<std::uint8_t>(ByteCodes::Code::return_tos_pop_2) );
            break;
        default:
            _buffer.pushByte( static_cast<std::uint8_t>(ByteCodes::Code::return_tos_pop_n) );
            _buffer.pushByte( argCount );
            break;
    }
    MethodKlass *k = (MethodKlass *) Universe::methodKlassObj()->klass_part();
    _method = k->constructMethod( _selector, 0,         // flags
                                  argCount,  // number of arguments
                                  oopFactory::new_objArray( 0 ), // debug info
                                  bytes(), oops() );
}


ByteArrayOop MissingMethodBuilder::bytes() {
    return _buffer.bytes();
}


ObjectArrayOop MissingMethodBuilder::oops() {
    return _buffer.oops();
}
