#! /usr/bin/env gdb

# -----------------------------------------------------------------------------

#set logging file main-debug.log
#set logging overwrite on
#set logging redirect on
#set logging on
set pagination off
set confirm off
 

# -----------------------------------------------------------------------------

#info breakpoints
#info signal SIGUSR1
#info signal SIGQUIT
#info signal SIGSEGV

handle SIGUSR1 noprint pass
handle SIGQUIT noprint pass
handle SIGSEGV noprint pass
#handle SIGTRAP noprint pass


# -----------------------------------------------------------------------------

#file /usr/lib/wine/wine
#set args obj/make/gnu/debug/strongtalk.exe.so -script "Z:\home\gjvc\projects\SMALLTALK\strongtalk-2020\strongtalk\src\dlt\primGen.dlt"
#run

#-script "Z:\home\gjvc\projects\SMALLTALK\strongtalk-2020\strongtalk\src\st\StrongtalkSource"

