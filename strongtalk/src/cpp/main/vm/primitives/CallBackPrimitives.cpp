
//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/system/platform.hpp"
#include "vm/runtime/flags.hpp"
#include "vm/primitives/CallBackPrimitives.hpp"
#include "vm/memory/vmSymbols.hpp"
#include "vm/oops/ProxyOopDescriptor.hpp"
#include "vm/runtime/CallBack.hpp"


TRACE_FUNC( TraceCallBackPrims, "callBack" )


std::int32_t CallBackPrimitives::number_of_calls;


PRIM_DECL_2( CallBackPrimitives::initialize, Oop receiver, Oop selector ) {
    PROLOGUE_2( "initialize", receiver, selector );
    if ( not selector->isSymbol() )
        return markSymbol( vmSymbols::second_argument_has_wrong_type() );
    CallBack::initialize( receiver, SymbolOop( selector ) );
    return receiver;
}


PRIM_DECL_3( CallBackPrimitives::registerPascalCall, Oop index, Oop nofArgs, Oop result ) {
    PROLOGUE_3( "registerPascalCall", index, nofArgs, result );
    if ( not index->isSmallIntegerOop() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );
    if ( not nofArgs->isSmallIntegerOop() )
        return markSymbol( vmSymbols::second_argument_has_wrong_type() );
    if ( not result->is_proxy() )
        return markSymbol( vmSymbols::third_argument_has_wrong_type() );
    ProxyOop( result )->set_pointer( CallBack::registerPascalCall( SmallIntegerOop( index )->value(), SmallIntegerOop( nofArgs )->value() ) );
    return result;
}


PRIM_DECL_2( CallBackPrimitives::registerCCall, Oop index, Oop result ) {
    PROLOGUE_2( "registerCCall", index, result );
    if ( not index->isSmallIntegerOop() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );
    if ( not result->is_proxy() )
        return markSymbol( vmSymbols::second_argument_has_wrong_type() );
    ProxyOop( result )->set_pointer( CallBack::registerCCall( SmallIntegerOop( index )->value() ) );
    return result;
}


PRIM_DECL_1( CallBackPrimitives::unregister, Oop proxy ) {
    PROLOGUE_1( "unregister", proxy );
    if ( not proxy->is_proxy() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );
    CallBack::unregister( ProxyOop( proxy )->get_pointer() );
    ProxyOop( proxy )->set_pointer( nullptr );
    return proxy;
}




PRIM_DECL_1( CallBackPrimitives::invokePascal, Oop proxy ) {
    PROLOGUE_1( "invokePascal", proxy );
    if ( not proxy->is_proxy() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );
    mytype f = (mytype) ProxyOop( proxy )->get_pointer();
    return smiOopFromValue( ( *f )( 10, 5 ) );
}


PRIM_DECL_1( CallBackPrimitives::invokeC, Oop proxy ) {
    PROLOGUE_1( "invokeC", proxy );
    if ( not proxy->is_proxy() )
        return markSymbol( vmSymbols::first_argument_has_wrong_type() );
    primitiveFunctionType f = (primitiveFunctionType) ProxyOop( proxy )->get_pointer();
    return smiOopFromValue( (std::int32_t) ( *f )( 10, 5 ) );
}
