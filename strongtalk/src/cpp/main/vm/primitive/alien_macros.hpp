
#pragma once



#define checkAlienReceiver( receiver ) \
  if (not receiver->isByteArray()) \
    return markSymbol(vmSymbols::receiver_has_wrong_type())

#define isUnsafe( argument ) \
  (not Oop(argument)->isSmallIntegerOop() \
  and MemOop(argument)->klass_field() == unsafeAlienClass() \
  and unsafeContents(argument)->isByteArray())

#define alienArg( argument )      (void*)argument

#define alienArray( receiver )    ((std::int32_t*)ByteArrayOop(receiver)->bytes())

#define alienSize( receiver )     (alienArray(receiver)[0])

#define alienAddress( receiver )  ((void**)alienArray(receiver))[1]

#define alienResult( handle )     (handle.as_oop() == nilObject ? nullptr : (void*)handle.asPointer())
#define alienResult2( handle )     (handle->as_oop() == nilObject ? nullptr : (void*)handle->asPointer())

#define checkAlienCalloutReceiver( receiver ) \
  checkAlienReceiver(receiver); \
  if ( alienSize(receiver) > 0 or  alienAddress(receiver) == nullptr) \
    return markSymbol(vmSymbols::illegal_state())

#define checkAlienCalloutResult( argument ) \
  if (not (argument->isByteArray() or argument == nilObject)) \
    return markSymbol(vmSymbols::argument_has_wrong_type())

#define checkAlienCalloutResultArgs( argument ) \
  if (not (argument->isByteArray() or argument == nilObject)) \
    return markSymbol(vmSymbols::first_argument_has_wrong_type())

#define checkAlienCalloutArg( argument, symbol ) \
  if (not ((argument->isByteArray() and MemOop(argument)->klass() not_eq largeIntegerClass()) \
      or argument->isSmallIntegerOop() or isUnsafe(argument))) \
    return markSymbol(symbol)

#define checkAlienCalloutArg1( argument ) \
  checkAlienCalloutArg(argument, vmSymbols::second_argument_has_wrong_type())

#define checkAlienCalloutArg2( argument ) \
  checkAlienCalloutArg(argument, vmSymbols::third_argument_has_wrong_type())

#define checkAlienCalloutArg3( argument ) \
  checkAlienCalloutArg(argument, vmSymbols::fourth_argument_has_wrong_type())

#define checkAlienCalloutArg4( argument ) \
  checkAlienCalloutArg(argument, vmSymbols::fifth_argument_has_wrong_type())

#define checkAlienCalloutArg5( argument ) \
  checkAlienCalloutArg(argument, vmSymbols::sixth_argument_has_wrong_type())

#define checkAlienCalloutArg6( argument ) \
  checkAlienCalloutArg(argument, vmSymbols::seventh_argument_has_wrong_type())

#define checkAlienCalloutArg7( argument ) \
  checkAlienCalloutArg(argument, vmSymbols::eighth_argument_has_wrong_type())

#define checkAlienCalloutArg8( argument ) \
  checkAlienCalloutArg(argument, vmSymbols::ninth_argument_has_wrong_type())

#define alienIndex( argument ) (SmallIntegerOop(argument)->value())

#define checkAlienAtIndex( receiver, argument, type ) \
  if (not argument->isSmallIntegerOop()) \
    return markSymbol(vmSymbols::argument_has_wrong_type()); \
  if (alienIndex(argument) < 1 or \
      (alienSize(receiver) not_eq 0 and ((std::uint32_t)alienIndex(argument)) > abs(alienSize(receiver)) - sizeof(type) + 1)) \
    return markSymbol(vmSymbols::index_not_valid())

#define checkAlienAtPutIndex( receiver, argument, type ) \
  if (not argument->isSmallIntegerOop()) \
    return markSymbol(vmSymbols::first_argument_has_wrong_type()); \
  if (alienIndex(argument) < 1 or \
      (alienSize(receiver) not_eq 0 and ((std::uint32_t)alienIndex(argument)) > abs(alienSize(receiver)) - sizeof(type) + 1)) \
    return markSymbol(vmSymbols::index_not_valid())

#define checkAlienAtPutValue( receiver, argument, type, min, max ) \
  if (not argument->isSmallIntegerOop()) \
    return markSymbol(vmSymbols::second_argument_has_wrong_type()); \
  { \
    std::int32_t value = SmallIntegerOop(argument)->value(); \
    if (value < min or value > max) \
      return markSymbol(vmSymbols::argument_is_invalid()); \
  }

#define checkAlienAtReceiver( receiver ) \
  checkAlienReceiver(receiver); \
  if (alienSize(receiver) <= 0 and alienAddress(receiver) == nullptr) \
    return markSymbol(vmSymbols::illegal_state())

#define alienContents( receiver ) \
  (alienSize(receiver) > 0 \
    ? ((void*)(alienArray(receiver) + 1)) \
    : (alienAddress(receiver)))

#define alienAt( receiver, argument, type ) \
  *((type*)(((char*)alienContents(receiver)) + alienIndex(argument) - 1))
