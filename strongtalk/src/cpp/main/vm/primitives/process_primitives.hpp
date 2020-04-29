//
//  (C) 1994 - 2020, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#pragma once

#include "vm/system/platform.hpp"
#include "vm/memory/allocation.hpp"
#include "vm/primitives/primitive_tracing.hpp"

// Primitives for processes

class processOopPrimitives : AllStatic {
    private:
        static void inc_calls() {
            number_of_calls++;
        }


    public:
        static int number_of_calls;

        //%prim
        // <Process class> primitiveProcessCreate: block <BlockWithoutArguments>
        //                                 ifFail: failBlock <PrimFailBlock> ^<Process> =
        //   Internal { doc   = 'Creates a new process'
        //              error = #(ProcessAllocationFailed)
        //              name  = 'processOopPrimitives::create' }
        //%
        static Oop __CALLING_CONVENTION create( Oop receiver, Oop block );

        //%prim
        // <NoReceiver> primitiveProcessYield ^<Process> =
        //   Internal { doc   = 'Yields the control to the scheduler.'
        //              doc   = 'Does nothing if executed by the scheduler.'
        //              doc   = 'Returns current process when regaining control.'
        //              name  = 'processOopPrimitives::yield' }
        //%
        static Oop __CALLING_CONVENTION yield();

        //%prim
        // <NoReceiver> primitiveProcessStop ^<Process> =
        //   Internal { doc   = 'Yields the control to the scheduler.'
        //              doc   = 'Does nothing if executed by the scheduler.'
        //              doc   = 'Returns current process when regaining control.'
        //              name  = 'processOopPrimitives::stop' }
        //%
        static Oop __CALLING_CONVENTION stop();

        //%prim
        // <Process> primitiveRecordMainProcessIfFail: failBlock <PrimFailBlock> ^<Object> =
        //   Internal { doc   = 'Records the receiver as the main process'
        //              name  = 'processOopPrimitives::setMainProcess' }
        //%
        static Oop __CALLING_CONVENTION setMainProcess( Oop process );

        //%prim
        // <NoReceiver> primitiveProcessActiveProcess ^<Object> =
        //   Internal { doc   = 'Returns the active process'
        //              name  = 'processOopPrimitives::activeProcess' }
        //%
        static Oop __CALLING_CONVENTION activeProcess();

        //%prim
        // <NoReceiver> primitiveProcessTransferTo: process   <Process>
        //                                  ifFail: failBlock <PrimFailBlock> ^<Object> =
        //   Internal { doc   = 'Transfers the control from the scheduler to a process.'
        //              doc   = 'Returns the status of process (see primitiveProcessStatus).'
        //              error = #(NotInScheduler ProcessCannotContinue Dead)
        //              name  = 'processOopPrimitives::transferTo' }
        //%
        static Oop __CALLING_CONVENTION transferTo( Oop process );

        //%prim
        // <Process> primitiveProcessSetMode: mode       <Symbol>
        //                        activation: activation <Activation>
        //                       returnValue: value      <Object>
        //                            ifFail: failBlock  <PrimFailBlock> ^<Symbol> =
        //   Internal { doc   = 'Change the mode of the process. mode: #Normal #Step #StepNext #StepEnd #Revert #Return'
        //              doc   = 'Returns the old mode of the process.'
        //              error = #(InScheduler Dead)
        //              name  = 'processOopPrimitives::set_mode' }
        //%
        static Oop __CALLING_CONVENTION set_mode( Oop process, Oop mode, Oop value );

        //%prim
        // <NoReceiver> primitiveProcessStartEvaluator: process   <Process>
        //                                      ifFail: failBlock <PrimFailBlock> ^<Object> =
        //   Internal { doc   = 'Transfers the control from the scheduler to a process,'
        //              doc   = 'and enters the evaluator.'
        //              doc   = 'Returns the status of process (see primitiveProcessStatus).'
        //              error = #(NotInScheduler ProcessCannotContinue Dead)
        //              name  = 'processOopPrimitives::start_evaluator' }
        //%
        static Oop __CALLING_CONVENTION start_evaluator( Oop process );

        //%prim
        // <Process> primitiveProcessTerminateIfFail: failBlock <PrimFailBlock> ^<Self> =
        //   Internal { doc   = 'Terminates the process'
        //              error = #(Dead)
        //              flags = #(NonLocalReturn)
        //              name  = 'processOopPrimitives::terminate' }
        //%
        static Oop __CALLING_CONVENTION terminate( Oop receiver );

        //%prim
        // <Process> primitiveProcessStatus ^<Symbol> =
        //   Internal { doc  = 'Returns the process status:'
        //              doc  = '  #Initialized            - State right after creation.'
        //              doc  = '  #Yielded                - Gave up control by calling yield.'
        //              doc  = '  #Running                - Is running.'
        //              doc  = '  #InAsyncDLL             - Gave up control but continues to execute asynchronous DLL.'
        //              doc  = '  #Stopped                - Gave up control by calling stop.'
        //              doc  = '  #Preempted              - Was preempted by system.'
        //              doc  = '  #Completed              - Ran to completion.'
        //              doc  = '  #Dead                   - The process is dead.'
        //              doc  = '  #BooleanError           - A boolean was expected at hardcoded control structure.'
        //              doc  = '  #FloatError             - A float was expected at hardcoded float operation.'
        //              doc  = '  #LookupError            - The receiver does not understand doesNotUnderstand:.'
        //              doc  = '  #PrimitiveLookupError   - Binding a primitive failed.'
        //              doc  = '  #DLLLookupError         - Binding a DLL function failed.'
        //              doc  = '  #NonLocalReturnError               - Context for NonLocalReturn did not exist.'
        //              doc  = '  #StackOverflow          - Stack exhausted.'
        //              name = 'processOopPrimitives::status' }
        //%
        static Oop __CALLING_CONVENTION status( Oop process );

        //%prim
        // <Process> primitiveProcessSchedulerWait: milliseconds <SmallInteger>
        //                                  ifFail: failBlock <PrimFailBlock> ^<Boolean> =
        //   Internal { doc   = 'Waits until timer has expired or a asynchronous dll call has returned.'
        //              doc   = 'Returns whether the timer expired.'
        //              name  = 'processOopPrimitives::scheduler_wait' }
        //%
        static Oop __CALLING_CONVENTION scheduler_wait( Oop milliseconds );

        //%prim
        // <Process> primitiveProcessTraceStack: size <SmallInteger>
        //                               ifFail: failBlock <PrimFailBlock> ^<Self> =
        //   Internal { doc    = 'Prints the stack trace'
        //              errors = #(Dead)
        //              name   = 'processOopPrimitives::trace_stack' }
        //%
        static Oop __CALLING_CONVENTION trace_stack( Oop receiver, Oop size );

        //%prim
        // <NoReceiver> primitiveProcessEnterCritical ^<Process> =
        //   Internal { doc    = 'Disables preemption in active process.'
        //              name   = 'processOopPrimitives::enter_critical' }
        //%
        static Oop __CALLING_CONVENTION enter_critical();

        //%prim
        // <NoReceiver> primitiveProcessLeaveCritical ^<Process> =
        //   Internal { doc    = 'Enables preemption in active process'
        //              name   = 'processOopPrimitives::leave_critical' }
        //%
        static Oop __CALLING_CONVENTION leave_critical();

        //%prim
        // <NoReceiver> primitiveProcessYieldInCritical ^<Process> =
        //   Internal { doc   = 'Yields the control to the scheduler.'
        //              doc   = 'Does nothing if executed by the scheduler.'
        //              doc   = 'Returns current process when regaining control.'
        //              name  = 'processOopPrimitives::yield_in_critical' }
        //%
        static Oop __CALLING_CONVENTION yield_in_critical();

        //%prim
        // <Process> primitiveProcessUserTime ^<Float> =
        //   Internal { doc   = 'Returns time, in seconds, the process has spent in user code'
        //              name  = 'processOopPrimitives::user_time' }
        //%
        static Oop __CALLING_CONVENTION user_time( Oop receiver );

        //%prim
        // <Process> primitiveProcessSystemTime ^<Float> =
        //   Internal { doc   = 'Returns time, in seconds, the process has spent in system code'
        //              name  = 'processOopPrimitives::user_time' }
        //%
        static Oop __CALLING_CONVENTION system_time( Oop receiver );

        //%prim
        // <Process> primitiveProcessStackLimit: limit <SmallInteger>
        //                               ifFail: failBlock <PrimFailBlock> ^<IndexedInstanceVariables> =
        //   Internal { errors = #(Dead Running)
        //              doc    = 'Returns an array of the process activation.'
        //              doc    = 'The primitive fails if either the receiver is a dead process or'
        //              doc    = 'if it is the active process'
        //              name   = 'processOopPrimitives::stack' }
        //%
        static Oop __CALLING_CONVENTION stack( Oop receiver, Oop limit );

};

