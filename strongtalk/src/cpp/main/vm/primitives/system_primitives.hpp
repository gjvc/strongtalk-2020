//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/memory/allocation.hpp"
#include "vm/primitives/primitive_declarations.hpp"
#include "vm/primitives/primitive_tracing.hpp"
// Misc. system primitives

class SystemPrimitives : AllStatic {

    private:
        static void inc_calls() {
            number_of_calls++;
        }


    public:
        static int number_of_calls;

        // REFLECTIVE OPERATIONS

        //%prim
        // <NoReceiver> primitiveCreateInvocationOf: mixin      <Mixin>
        //                                    named: name       <Symbol>
        //                      isPrimaryInvocation: primary    <Boolean>
        //                               superclass: superclass <Behavior>
        //                                   format: format     <Symbol>
        //                                   ifFail: failBlock  <PrimFailBlock> ^<GlobalAssociation> =
        //   Internal { doc   = 'format: #Oops, #ExternalProxy #Process'
        //              doc   = '        #IndexedInstanceVariables #IndexedByteInstanceVariables'
        //              doc   = '        #IndexedDoubleByteInstanceVariables #IndexedNextOfKinInstanceVariables'
        //              error = #(WrongFormat)
        //              name  = 'systemPrimitives::createNamedInvocation' }
        //%
        static PRIM_DECL_5( createNamedInvocation, Oop mixin, Oop name, Oop primary, Oop superclass, Oop format );

        //%prim
        // <NoReceiver> primitiveCreateInvocationOf: mixin      <Mixin>
        //                               superclass: superclass <Behavior>
        //                                   format: format     <Symbol>
        //                                   ifFail: failBlock  <PrimFailBlock> ^<GlobalAssociation> =
        //   Internal { doc   = 'format: #Oops, #ExternalProxy #Process'
        //              doc   = '        #IndexedInstanceVariables #IndexedByteInstanceVariables'
        //              doc   = '        #IndexedDoubleByteInstanceVariables #IndexedNextOfKinInstanceVariables'
        //              error = #(WrongFormat)
        //              name  = 'systemPrimitives::createInvocation' }
        //%
        static PRIM_DECL_3( createInvocation, Oop mixin, Oop superclass, Oop format );

        //%prim
        // <NoReceiver> primitiveApplyChange: change        <IndexedInstanceVariables>
        //                            ifFail: failBlock     <PrimFailBlock> ^<Object> =
        //   Internal { doc  = 'Apply change to a mixin and their invocations.'
        //              doc  = 'The change is a <IndexedInstanceVariables> with the following structure:'
        //              doc  = '  [1]     = new-mixin   <Mixin>'
        //              doc  = '  [2]     = old-mixin   <Mixin>'
        //              doc  = '  [3 - n] = invocations <IndexedInstanceVariables>'
        //              doc  = 'Where the format for the a incovation is:'
        //              doc  = '  [1]     = incovation  <Class>'
        //              doc  = '  [2]     = format      <Symbol>'
        //              doc  = '  [3]     = superClass  <Class>'
        //              doc  = '  [4 - m] = {subclass, format}*  <IndexedInstanceVariables>, <Symbol>'
        //              doc  = 'The list of classes are sub classes of the invocation (topological sorted).'
        //              name = 'systemPrimitives::applyChange' }
        //%
        static PRIM_DECL_1( applyChange, Oop change );

        //%prim
        // <Object> primitiveScavenge ^<Self> =
        //   Internal { name  = 'systemPrimitives::scavenge' }
        //%
        static PRIM_DECL_1( scavenge, Oop receiver );

        //%prim
        // <NoReceiver> primitiveCanScavenge ^<Boolean> =
        //   Internal { name  = 'systemPrimitives::canScavenge' }
        //%
        static PRIM_DECL_0( canScavenge );

        //%prim
        // <Object> primitiveGarbageCollect ^<Self> =
        //   Internal { name  = 'systemPrimitives::garbageGollect' }
        //%
        static PRIM_DECL_1( garbageGollect, Oop receiver );

        //%prim
        // <NoReceiver> primitiveExpandMemory: size <SmallInteger> ^<Object> =
        //   Internal { name  = 'systemPrimitives::expandMemory' }
        //%
        static PRIM_DECL_1( expandMemory, Oop size );

        //%prim
        // <NoReceiver> primitiveShrinkMemory: size <SmallInteger> ^<Object> =
        //   Internal { name  = 'systemPrimitives::shrinkMemory' }
        //%
        static PRIM_DECL_1( shrinkMemory, Oop size );

        //%prim
        // <NoReceiver> primitiveSizeOfOop ^<SmallInteger> =
        //   Internal { name  = 'systemPrimitives::oopSize' }
        //%
        static PRIM_DECL_0( oopSize );

        //%prim
        // <NoReceiver> primitiveFreeSpace ^<SmallInteger> =
        //   Internal { name  = 'systemPrimitives::freeSpace'
        //              doc   = 'Returns the number of unused bytes in the old generation' }
        //%
        static PRIM_DECL_0( freeSpace );

        //%prim
        // <NoReceiver> primitiveNurseryFreeSpace ^<SmallInteger> =
        //   Internal { name  = 'systemPrimitives::nurseryFreeSpace'
        //              doc   = 'Returns the number of unused bytes in the new generation' }
        //%
        static PRIM_DECL_0( nurseryFreeSpace );

        //%prim
        // <NoReceiver> primitiveExpansions ^<SmallInteger> =
        //   Internal { name  = 'systemPrimitives::expansions'
        //              doc   = 'Returns the number of expansions of the old generation' }
        //%
        static PRIM_DECL_0( expansions );

        //%prim
        // <NoReceiver> primitiveBreakpoint ^<Object> =
        //   Internal { name  = 'systemPrimitives::breakpoint' }
        //%
        static PRIM_DECL_0( breakpoint );

        //%prim
        // <NoReceiver> primitiveVMBreakpoint ^<Object> =
        //   Internal { name  = 'systemPrimitives::vmbreakpoint' }
        //%
        static PRIM_DECL_0( vmbreakpoint );

        //%prim
        // <NoReceiver> primitiveGetLastError ^<Integer> =
        //   Internal { name  = 'systemPrimitives::getLastError' }
        //%
        static PRIM_DECL_0( getLastError );

        //%prim
        // <NoReceiver> primitiveHalt ^<Object> =
        //   Internal { name  = 'systemPrimitives::halt' }
        //%
        static PRIM_DECL_0( halt );

        //%prim
        // <NoReceiver> primitiveUserTime ^<Float> =
        //   Internal { name  = 'systemPrimitives::userTime' }
        //%
        static PRIM_DECL_0( userTime );

        //%prim
        // <NoReceiver> primitiveSystemTime ^<Float> =
        //   Internal { name  = 'systemPrimitives::systemTime' }
        //%
        static PRIM_DECL_0( systemTime );

        //%prim
        // <NoReceiver> primitiveElapsedTime ^<Float> =
        //   Internal { name = 'systemPrimitives::elapsedTime' }
        //%
        static PRIM_DECL_0( elapsedTime );

        //%prim
        // <NoReceiver> primitiveWriteSnapshot: fileName <String> ^<Object> =
        //   Internal { name  = 'systemPrimitives::writeSnapshot' }
        //%
        static PRIM_DECL_1( writeSnapshot, Oop fileName );

        //%prim
        // <NoReceiver> primitiveQuit ^<BottomType> =
        //   Internal { name  = 'systemPrimitives::quit' }
        //%
        static PRIM_DECL_0( quit );

        // GLOBAL ASSOCIATION

        //%prim
        // <GlobalAssociation> primitiveGlobalAssociationKey ^<Symbol> =
        //   Internal { name  = 'systemPrimitives::globalAssociationKey' }
        //%
        static PRIM_DECL_1( globalAssociationKey, Oop receiver );

        //%prim
        // <GlobalAssociation> primitiveGlobalAssociationSetKey: key <Symbol> ^<Object> =
        //   Internal { name  = 'systemPrimitives::globalAssociationSetKey' }
        //%
        static PRIM_DECL_2( globalAssociationSetKey, Oop receiver, Oop key );

        //%prim
        // <GlobalAssociation> primitiveGlobalAssociationValue ^<Object> =
        //   Internal { name  = 'systemPrimitives::globalAssociationValue' }
        //%
        static PRIM_DECL_1( globalAssociationValue, Oop receiver );

        //%prim
        // <GlobalAssociation> primitiveGlobalAssociationSetValue: value <Object> ^<Object> =
        //   Internal { name  = 'systemPrimitives::globalAssociationSetValue' }
        //%
        static PRIM_DECL_2( globalAssociationSetValue, Oop receiver, Oop value );

        //%prim
        // <GlobalAssociation> primitiveGlobalAssociationIsConstant ^<Boolean> =
        //   Internal { name  = 'systemPrimitives::globalAssociationIsConstant' }
        //%
        static PRIM_DECL_1( globalAssociationIsConstant, Oop receiver );

        //%prim
        // <GlobalAssociation> primitiveGlobalAssociationSetConstant: value <Boolean> ^<Boolean> =
        //   Internal { name  = 'systemPrimitives::globalAssociationSetConstant' }
        //%
        static PRIM_DECL_2( globalAssociationSetConstant, Oop receiver, Oop value );

        // THE SMALLTALK ARRAY

        //%prim
        // <NoReceiver> primitiveSmalltalkAt: index <SmallInteger>
        //                            ifFail: failBlock <PrimFailBlock>  ^<GlobalAssociation> =
        //   Internal { doc   = 'Returns the global association at index'
        //              error = #(OutOfBounds)
        //              name  = 'systemPrimitives::smalltalk_at' }
        //%
        static PRIM_DECL_1( smalltalk_at, Oop index );

        //%prim
        // <NoReceiver> primitiveSmalltalkAt: key       <Symbol>
        //                               Put: value     <Object>
        //                            ifFail: failBlock <PrimFailBlock>  ^<GlobalAssociation> =
        //   Internal { doc   = 'Add a new non-constant global association'
        //              name  = 'systemPrimitives::smalltalk_at_put' }
        //%
        static PRIM_DECL_2( smalltalk_at_put, Oop key, Oop value );

        //%prim
        // <NoReceiver> primitiveSmalltalkRemoveAt: index <SmallInteger>
        //                                  ifFail: failBlock <PrimFailBlock>  ^<GlobalAssociation> =
        //   Internal { doc   = 'Removes the association at index, and returns the removed element'
        //              name  = 'systemPrimitives::smalltalk_remove_at' }
        //%
        static PRIM_DECL_1( smalltalk_remove_at, Oop index );

        //%prim
        // <NoReceiver> primitiveSmalltalkSize ^<SmallInteger> =
        //   Internal { name  = 'systemPrimitives::smalltalk_size' }
        //%
        static PRIM_DECL_0( smalltalk_size );

        //%prim
        // <NoReceiver> primitiveSmalltalkArray ^<IndexedInstanceVariables> =
        //   Internal { name  = 'systemPrimitives::smalltalk_array' }
        //%
        static PRIM_DECL_0( smalltalk_array );

        //%prim
        // <NoReceiver> primitivePrintPrimitiveTable ^<Object> =
        //   Internal { name  = 'systemPrimitives::printPrimitiveTable' }
        //%
        static PRIM_DECL_0( printPrimitiveTable );

        //%prim
        // <NoReceiver> primitivePrintMemory ^<Object> =
        //   Internal { name  = 'systemPrimitives::print_memory' }
        //%
        static PRIM_DECL_0( print_memory );

        //%prim
        // <NoReceiver> primitivePrintZone ^<Object> =
        //   Internal { name  = 'systemPrimitives::print_zone' }
        //%
        static PRIM_DECL_0( print_zone );

        // Windows specific primitives

        //%prim
        // <NoReceiver> primitiveDefWindowProc: resultProxy <Proxy>
        //                              ifFail: failBlock <PrimFailBlock>  ^<Proxy> =
        //   Internal { name  = 'systemPrimitives::defWindowProc' }
        //%
        static PRIM_DECL_1( defWindowProc, Oop resultProxy );


        //%prim
        // <NoReceiver> primitiveWindowsHInstance: resultProxy <Proxy>
        //                                 ifFail: failBlock <PrimFailBlock>  ^<Proxy> =
        //   Internal { name  = 'systemPrimitives::windowsHInstance' }
        //%
        static PRIM_DECL_1( windowsHInstance, Oop resultProxy );

        //%prim
        // <NoReceiver> primitiveWindowsHPrevInstance: resultProxy <Proxy>
        //                                     ifFail: failBlock <PrimFailBlock>  ^<Proxy> =
        //   Internal { name  = 'systemPrimitives::windowsHPrevInstance' }
        //%
        static PRIM_DECL_1( windowsHPrevInstance, Oop resultProxy );

        //%prim
        // <NoReceiver> primitiveWindowsNCmdShow ^<Object> =
        //   Internal { name  = 'systemPrimitives::windowsNCmdShow' }
        //%
        static PRIM_DECL_0( windowsNCmdShow );

        //%prim
        // <NoReceiver> primitiveCharacterFor: value     <SmallInteger>
        //                             ifFail: failBlock <PrimFailBlock>  ^<Proxy> =
        //   Internal { error = #(OutOfBounds)
        //              name  = 'systemPrimitives::characterFor' }
        //%
        static PRIM_DECL_1( characterFor, Oop value );

        //%prim
        // <NoReceiver> primitiveTraceStack ^<Object> =
        //   Internal { name  = 'systemPrimitives::traceStack' }
        //%
        static PRIM_DECL_0( traceStack );

        // FLAT PROFILER

        //%prim
        // <NoReceiver> primitiveFlatProfilerReset ^<Object> =
        //   Internal { doc  = 'Resets the flat profiler to initial state.'
        //              name = 'systemPrimitives::flat_profiler_reset' }
        //%
        static PRIM_DECL_0( flat_profiler_reset );

        //%prim
        // <NoReceiver> primitiveFlatProfilerProcess ^<Process|nil> =
        //   Internal { doc  = 'Returns the process beeing profiler, nil otherwise.'
        //              name = 'systemPrimitives::flat_profiler_process' }
        //%
        static PRIM_DECL_0( flat_profiler_process );

        //%prim
        // <NoReceiver> primitiveFlatProfilerEngage: process <Process>
        //                                   ifFail: failBlock <PrimFailBlock> ^<Process> =
        //   Internal { doc  = 'Starts profiling process.'
        //              name = 'systemPrimitives::flat_profiler_engage' }
        //%
        static PRIM_DECL_1( flat_profiler_engage, Oop process );

        //%prim
        // <NoReceiver> primitiveFlatProfilerDisengage ^<Process|nil> =
        //   Internal { doc  = 'Stops profiling.'
        //              name = 'systemPrimitives::flat_profiler_disengage' }
        //%
        static PRIM_DECL_0( flat_profiler_disengage );

        //%prim
        // <NoReceiver> primitiveFlatProfilerPrint ^<Object> =
        //    Internal { doc   = 'Prints the collected profile information.'
        //               name  = 'systemPrimitives::flat_profiler_print' }
        //%
        static PRIM_DECL_0( flat_profiler_print );


        // SUPPORT FOR WEAK ARRAY NOTIFICATION

        //%prim
        // <NoReceiver> primitiveNotificationQueueGetIfFail: failBlock <PrimFailBlock>  ^<Object> =
        //   Internal { doc   = 'Returns the first element in the notification queue (FIFO).'
        //              error = #(EmptyQueue)
        //              name  = 'systemPrimitives::notificationQueueGet' }
        //%
        static PRIM_DECL_0( notificationQueueGet );

        //%prim
        // <NoReceiver> primitiveNotificationQueuePut: value <Object> ^<Object> =
        //   Internal { doc   = 'Appends the argument to the notification queue (FIFO).'
        //              name  = 'systemPrimitives::notificationQueuePut' }
        //%
        static PRIM_DECL_1( notificationQueuePut, Oop value );

        //%prim
        // <NoReceiver> primitiveHadNearDeathExperience: value <Object> ^<Boolean> =
        //   Internal { doc  = 'Tells whether the receiver had a near death experience.'
        //              name = 'systemPrimitives::hadNearDeathExperience' }
        //%
        static PRIM_DECL_1( hadNearDeathExperience, Oop value );

        // DLL support

        //%prim
        // <NoReceiver> primitiveDLLSetupLookup: receiver <Object>
        //                             selector: selector <Symbol>
        //                               ifFail: failBlock <PrimFailBlock> ^<Object> =
        //   Internal { doc    = 'Setup call back for DLL lookup. Fails if selector does not have right number of arguments'
        //              errors = #(failed)
        //              name   = 'systemPrimitives::dll_setup' }
        //%
        static PRIM_DECL_2( dll_setup, Oop receiver, Oop selector );

        //%prim
        // <NoReceiver> primitiveDLLLookup: name      <Symbol>
        //                              in: library   <Proxy>
        //                          result: entry     <Proxy>
        //                          ifFail: failBlock <PrimFailBlock> ^<Proxy> =
        //   Internal { doc    = 'Lookup name in library'
        //              errors = #(NotFound)
        //              name   = 'systemPrimitives::dll_lookup' }
        //%
        static PRIM_DECL_3( dll_lookup, Oop name, Oop library, Oop result );

        //%prim
        // <NoReceiver> primitiveDLLLoad: name      <Symbol>
        //                        result: library   <Proxy>
        //                        ifFail: failBlock <PrimFailBlock> ^<Proxy> =
        //   Internal { doc    = 'Load library'
        //              errors = #(NotFound)
        //              name   = 'systemPrimitives::dll_load' }
        //%
        static PRIM_DECL_2( dll_load, Oop name, Oop library );

        //%prim
        // <NoReceiver> primitiveDLLUnload: library <Proxy>
        //                          ifFail: failBlock <PrimFailBlock> ^<Object> =
        //   Internal { doc    = 'Unload the library'
        //              errors = #(Failed)
        //              name   = 'systemPrimitives::dll_unload' }
        //%
        static PRIM_DECL_1( dll_unload, Oop library );

        // Inlining database

        //%prim
        // <NoReceiver> primitiveInliningDatabaseDirectory ^<Symbol> =
        //   Internal { doc    = 'Returns the directory for the external inlining database.'
        //              errors = #(Failed)
        //              name   = 'systemPrimitives::inlining_database_directory' }
        //%
        static PRIM_DECL_0( inlining_database_directory );

        //%prim
        // <NoReceiver> primitiveInliningDatabaseSetDirectory: name      <Symbol>
        //                                             ifFail: failBlock <PrimFailBlock> ^<Symbol> =
        //   Internal { doc    = 'Sets the directory for external inlining database.'
        //              doc    = 'Returns the old directory.'
        //              errors = #(Failed)
        //              name   = 'systemPrimitives::inlining_database_set_directory' }
        //%
        static PRIM_DECL_1( inlining_database_set_directory, Oop name );


        //%prim
        // <NoReceiver>  primitiveInliningDatabaseFileOutClass: receiverClass <Behavior>
        //                                              ifFail: failBlock     <PrimFailBlock> ^<SmallInteger> =
        //   Internal { doc    = 'Adds inlining information to the external database for all compiled methods with the specific receiver class.'
        //              doc    = 'Returns the number of filed out structures.'
        //              errors = #(Failed)
        //              name   = 'systemPrimitives::inlining_database_file_out_class' }
        //%
        static PRIM_DECL_1( inlining_database_file_out_class, Oop receiver_class );

        //%prim
        // <NoReceiver>  primitiveInliningDatabaseFileOutAllIfFail: failBlock ^<SmallInteger> =
        //   Internal { doc    = 'Adds inlining information to the external database for all compiled methods.'
        //              doc    = 'Returns the number of filed out structures.'
        //              errors = #(Failed)
        //              name   = 'systemPrimitives::inlining_database_file_out_all' }
        //%
        static PRIM_DECL_0( inlining_database_file_out_all );

        //%prim
        // <NoReceiver>  primitiveInliningDatabaseCompile: fileName  <String>
        //                                         ifFail: failBlock <PrimFailBlock> ^<Object> =
        //   Internal { doc    = 'Compiles a method described in fileName.'
        //              errors = #(Failed)
        //              name   = 'systemPrimitives::inlining_database_compile' }
        //%
        static PRIM_DECL_1( inlining_database_compile, Oop file_name );

        //%prim
        // <NoReceiver>  primitiveInliningDatabaseCompile ^<Boolean> =
        //   Internal { doc    = 'Compiles a method for the inlining database (for background compilation).'
        //              name   = 'systemPrimitives::inlining_database_compile_next' }
        //%
        static PRIM_DECL_0( inlining_database_compile_next );

        //%prim
        // <NoReceiver>  primitiveInliningDatabaseMangle: name      <String>
        //                                        ifFail: failBlock <PrimFailBlock> ^<IndexedByteInstanceVariables> =
        //   Internal { doc    = 'Returns the mangled name'
        //              name   = 'systemPrimitives::inlining_database_mangle' }
        //%
        static PRIM_DECL_1( inlining_database_mangle, Oop name );

        //%prim
        // <NoReceiver>  primitiveInliningDatabaseCompileDemangled: name <String>
        //                                                  ifFail: failBlock <PrimFailBlock> ^<IndexedByteInstanceVariables> =
        //   Internal { doc    = 'Returns the demangled name'
        //              name   = 'systemPrimitives::inlining_database_demangle' }
        //%
        static PRIM_DECL_1( inlining_database_demangle, Oop name );

        //%prim
        // <NoReceiver>  primitiveInliningDatabaseAddLookupEntryClass: class      <Behavior>
        //                                                   selector: selector   <Symbol>
        //                                                     ifFail: failBlock <PrimFailBlock> ^<Boolean> =
        //   Internal { name   = 'systemPrimitives::inlining_database_add_entry' }
        //%
        static PRIM_DECL_2( inlining_database_add_entry, Oop receiver_class, Oop method_selector );


        //%prim
        // <NoReceiver>  primitiveSlidingSystemAverageIfFail: failBlock <PrimFailBlock> ^<IndexedInstanceVariables> =
        //   Internal { errors = #(NotActive)
        //              name   = 'systemPrimitives::sliding_system_average' }
        //%
        static PRIM_DECL_0( sliding_system_average );

        // Enumeration primitives

        //%prim
        // <NoReceiver> primitiveInstancesOf: class <Class>
        //                             limit: limit <SmallInteger>
        //                            ifFail: failBlock <PrimFailBlock> ^<IndexedInstanceVariables> =
        //   Internal { doc    = 'Returns an array with instances of class.'
        //              doc    = 'limit specifies the maximum number of elements.'
        //              errors = #(OutOfMemory)
        //              name   = 'systemPrimitives::instances_of' }
        //%
        static PRIM_DECL_2( instances_of, Oop klass, Oop limit );

        //%prim
        // <NoReceiver> primitiveReferencesTo: obj <Object>
        //                              limit: limit <SmallInteger>
        //                             ifFail: failBlock <PrimFailBlock> ^<IndexedInstanceVariables> =
        //   Internal { doc    = 'Returns an array with all objects referring obj.'
        //              doc    = 'limit specifies the maximum number of elements.'
        //              errors = #(OutOfMemory)
        //              name   = 'systemPrimitives::references_to' }
        //%
        static PRIM_DECL_2( references_to, Oop obj, Oop limit );

        //%prim
        // <NoReceiver> primitiveReferencesToInstancesOf: class <Class>
        //                                         limit: limit <SmallInteger>
        //                                        ifFail: failBlock <PrimFailBlock> ^<IndexedInstanceVariables> =
        //   Internal { doc    = 'Returns an array with all objects referring instances of class.'
        //              doc    = 'limit specifies the maximum number of elements.'
        //              errors = #(OutOfMemory)
        //              name   = 'systemPrimitives::references_to_instances_of' }
        //%
        static PRIM_DECL_2( references_to_instances_of, Oop klass, Oop limit );

        //%prim
        // <NoReceiver> primitiveAllObjectsLimit: limit <SmallInteger>
        //                                ifFail: failBlock <PrimFailBlock> ^<IndexedInstanceVariables> =
        //   Internal { doc    = 'Returns an array containing all objects.'
        //              doc    = 'limit specifies the maximum number of elements.'
        //              errors = #(OutOfMemory)
        //              name   = 'systemPrimitives::all_objects' }
        //%
        static PRIM_DECL_1( all_objects, Oop limit );

        //%prim
        // <NoReceiver> primitiveFlushCodeCache ^<Object> =
        //   Internal { doc    = 'Flushes all compiled code.'
        //              name   = 'systemPrimitives::flush_code_cache' }
        //%
        static PRIM_DECL_0( flush_code_cache );

        //%prim
        // <NoReceiver> primitiveFlushDeadCode ^<Object> =
        //   Internal { doc    = 'Flushes all invalidate compiled code.'
        //              name   = 'systemPrimitives::flush_dead_code' }
        //%
        static PRIM_DECL_0( flush_dead_code );

        //%prim
        // <NoReceiver> primitiveCommandLineArgs ^<Array[String]> =
        //   Internal { doc    = 'Retrieves the command line arguments as an array of strings.'
        //              name   = 'systemPrimitives::command_line_args' }
        //%
        static PRIM_DECL_0( command_line_args );

        //%prim
        // <NoReceiver> primitiveCurrentThreadId ^<SmallInteger> =
        //   Internal { doc    = 'Retrieves an identifier for the currently executing thread.'
        //              name   = 'systemPrimitives::current_thread_id' }
        //%
        static PRIM_DECL_0( current_thread_id );

        //%prim
        // <NoReceiver> primitiveObjectMemorySize ^<Float> =
        //   Internal { doc    = 'Retrieves the current size of old Space in bytes.'
        //              name   = 'systemPrimitives::object_memory_size' }
        //%
        static PRIM_DECL_0( object_memory_size );

        // Aliens support

        //%prim
        // <NoReceiver> primitiveAlienMalloc: size <SmallInteger>
        //                            ifFail: failBlock <PrimFailBlock> ^<Float> =
        //   Internal { doc    = 'Allocate "size" bytes of storage from the C heap using malloc.'
        //              errors = #(OutOfMemory)
        //              name   = 'systemPrimitives::alienMalloc' }
        //%
        static PRIM_DECL_1( alienMalloc, Oop size );

        //%prim
        // <NoReceiver> primitiveAlienCalloc: size <SmallInteger>
        //                            ifFail: failBlock <PrimFailBlock> ^<Float> =
        //   Internal { doc    = 'Allocate "size" bytes of storage from the C heap using calloc.'
        //              errors = #(OutOfMemory)
        //              name   = 'systemPrimitives::alienCalloc' }
        //%
        static PRIM_DECL_1( alienCalloc, Oop size );

        //%prim
        // <NoReceiver> primitiveAlienFree: address <SmallInteger|LargeInteger>
        //                          ifFail: failBlock <PrimFailBlock> ^<Float> =
        //   Internal { doc    = 'Free the storage allocated at "address" from the C heap.'
        //              errors = #(OutOfMemory)
        //              name   = 'systemPrimitives::alienFree' }
        //%
        static PRIM_DECL_1( alienFree, Oop address );
};

