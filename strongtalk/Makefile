#! /usr/bin/env make

##
##  (C) 1994 - 2020, The Strongtalk authors and contributors
##  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
##


# make ------------------------------------------------------------------------

ifeq ($(platform),)
platform := mingw
endif


# make ------------------------------------------------------------------------

THIS-FILE   := $(abspath $(firstword $(MAKEFILE_LIST)))
THIS-DIR    := $(dir $(THIS-FILE))
ROOT        := $(THIS-DIR:/=)

.SHELLFLAGS := -eu -o pipefail -c
SHELL       := /bin/bash

MAKEFLAGS   += --no-builtin-rules
MAKEFLAGS   += --warn-undefined-variables
MAKEFLAGS   += --jobs $(shell nproc)

null 		:=
space   	:= ${null} ${null}


# ccache ----------------------------------------------------------------------

ccache := /usr/bin/ccache
export CCACHE_DIR=$(ROOT)/var/ccache


# mingw-w64 -------------------------------------------------------------------

ifeq ($(platform),mingw)
build-type          := debug
cmake-system-name   := Windows
gcc-version-major 	:= 9
gcc-version-minor 	:= 3
gcc-version     	:= $(gcc-version-major)-$(gcc-version-minor)
machine-name        := i686
runtime-platform    := w64-mingw32
runtime-variant     := -posix
endif


# i686-linux-gnu --------------------------------------------------------------

ifeq ($(platform),linux)
build-type          := debug
cmake-system-name   := Linux
gcc-version-major 	:= 9
gcc-version-minor 	:= 3
gcc-version     	:= $(gcc-version-major)-$(gcc-version-minor)
machine-name        := i686
runtime-platform    := linux-gnu
runtime-variant     := -$(gcc-version-major)
endif


# gcc -------------------------------------------------------------------------

toolchain-name  := $(machine-name)-$(runtime-platform)
object-path 	:= obj/$(toolchain-name)$(runtime-variant)/$(build-type)


# gcc -------------------------------------------------------------------------

g++ := /usr/bin/$(toolchain-name)-g++$(runtime-variant)
gcc := /usr/bin/$(toolchain-name)-gcc$(runtime-variant)
export CC=$(ccache) $(gcc)
export CXX=$(ccache) $(g++)


# wine ------------------------------------------------------------------------

wine   		:= /usr/bin/wine
wineboot    := /usr/bin/wineboot
wineserver  := /usr/lib/wine/wineserver64
export WINEARCH=win32


# valgrind --------------------------------------------------------------------

valgrind            := /usr/bin/valgrind
valgrind-options 	:= --trace-children=yes --vex-iropt-register-updates=allregs-at-mem-access


# googletest ------------------------------------------------------------------

googletest-root-dir             := ext/googletest
googletest-cmake-toolchain-file := src/cmake/$(runtime-platform).cmake
googletest-include-paths        := $(googletest-root-dir)/googletest/include
googletest-object-dir           := $(object-path)/ext/googletest
libgtest-a                      := $(googletest-object-dir)/lib/libgtest.a
libgtest-a-flags                := -L $(googletest-object-dir)/lib -l gtest


# capstone --------------------------------------------------------------------

capstone-cmake-toolchain-file   := src/cmake/$(runtime-platform).cmake
capstone-include-paths          := $(capstone-root-dir)/capstone/include
capstone-object-dir             := $(object-path)/ext/capstone
capstone-root-dir               := ext/capstone
libcapstone-a                   := $(capstone-object-dir)/libcapstone.a
libcapstone-a-flags             := -L $(capstone-object-dir) -l capstone


# zydis -----------------------------------------------------------------------

zydis-cmake-toolchain-file      := src/cmake/$(runtime-platform).cmake
zydis-include-paths             := $(zydis-root-dir)/zydis/include
zydis-object-dir                := $(object-path)/ext/zydis
zydis-root-dir                  := ext/zydis
libzydis-a                      := $(zydis-object-dir)/libzydis.a
libzydis-a-flags                := -L $(zydis-object-dir) -l zydis


# nasm ------------------------------------------------------------------------

nasm-root-dir                   := ext/nasm
nasm-object-dir                 := $(object-path)/ext/nasm
nasm-include-paths              := $(object-path)/ext/nasm/include
libnasm-a                       := $(object-path)/ext/nasm/lib/libnasm.a
libnasm-a-flags                 := -L $(nasm-object-dir)/lib -l nasm


# udis86 ----------------------------------------------------------------------

udis86-root-dir                 := ext/udis86
udis86-object-dir               := $(object-path)/ext/udis86
udis86-include-paths            := $(object-path)/ext/udis86/include
udis86-library-paths            := $(object-path)/ext/udis86/lib
libudis86-a                     := $(object-path)/ext/udis86/lib/libudis86.a
libudis86-a-flags               := -L $(udis86-library-paths) -l udis86


# compiler flags --------------------------------------------------------------

cxx-arch-options             	:= native
cxx-arch-flags            		:= $(addprefix -march=,$(cxx-arch-options))

cxx-debug-options               := dwarf-2 gdb 3
cxx-debug-flags                 := $(addprefix -g,$(cxx-debug-options))

cxx-feature-options             := diagnostics-color=always permissive concepts
cxx-feature-flags               := $(addprefix -f,$(cxx-feature-options))

cxx-language-options            := c++2a
cxx-language-flags              := $(addprefix -std=,$(cxx-language-options))

cxx-machine-options             := 32
cxx-machine-flags               := $(addprefix -m,$(cxx-machine-options))

cxx-no-warning-options          := unused-parameter
cxx-no-warning-flags            := $(addprefix -Wno-,$(cxx-no-warning-options))

cxx-optimisation-options        := 0
cxx-optimisation-flags          := $(addprefix -O,$(cxx-optimisation-options))

cxx-other-options               := pipe pedantic
cxx-other-flags                 := $(addprefix -,$(cxx-other-options))

cxx-warning-options             := all extra
cxx-warning-flags               := $(addprefix -W,$(cxx-warning-options))

cxx-depends-flags               = -MMD -MP -MT $@ -MF $(object-path)/$*.cpp.d
cxx-define-flags                = 

cxx-compile-flags = \
    $(cxx-arch-flags) \
    $(cxx-debug-flags) \
    $(cxx-define-flags) \
    $(cxx-depends-flags) \
    $(cxx-feature-flags) \
    $(cxx-language-flags) \
    $(cxx-machine-flags) \
    $(cxx-no-warning-flags) \
    $(cxx-optimisation-flags) \
    $(cxx-other-flags) \
    $(cxx-warning-flags) \

cxx-link-flags = \
    $(cxx-machine-flags)

libgcc-link-flags = \
	-static-libgcc \
    -static-libstdc++

mingw-w64-link-flags = \
	-L /usr/$(machine-name)-$(runtime-platform)/lib -Wl,-Bstatic,--whole-archive -l winpthread -Wl,-Bdynamic,--no-whole-archive


# strongtalk-main-{sources,objects,depends} -------------------------------------

strongtalk-main-lib-sources             := $(wildcard src/cpp/main/vm/*/*.cpp)
strongtalk-main-lib-sources             := $(filter-out src/cpp/main/vm/main/%.cpp,$(strongtalk-main-lib-sources))
strongtalk-main-lib-objects             := $(patsubst %.cpp,$(object-path)/%.cpp.o,$(strongtalk-main-lib-sources))
strongtalk-main-lib-depends             := $(patsubst %.cpp,$(object-path)/%.cpp.d,$(strongtalk-main-lib-sources))

strongtalk-main-lib-cxx-include-paths   := $(nasm-include-paths) $(udis86-include-paths) src/cpp/main
strongtalk-main-lib-cxx-include-flags   := $(addprefix -I ,$(strongtalk-main-lib-cxx-include-paths))
strongtalk-main-lib-define-values    	:= 
strongtalk-main-lib-define-flags        := $(addprefix -D ,$(strongtalk-main-lib-define-values))

strongtalk-main-exe                     := $(object-path)/strongtalk-main.exe
strongtalk-main-exe-sources             := $(wildcard src/cpp/main/vm/main/*.cpp)
strongtalk-main-exe-objects             := $(patsubst %.cpp,$(object-path)/%.cpp.o,$(strongtalk-main-exe-sources))
strongtalk-main-exe-depends             := $(patsubst %.cpp,$(object-path)/%.cpp.d,$(strongtalk-main-exe-sources))


# strongtalk-test-{sources,objects,depends} ---------------------------------

strongtalk-test-exe-sources             := $(wildcard src/cpp/test/test/*/*.cpp)
strongtalk-test-exe-objects             := $(patsubst %.cpp,$(object-path)/%.cpp.o,$(strongtalk-test-exe-sources))
strongtalk-test-exe-depends             := $(patsubst %.cpp,$(object-path)/%.cpp.d,$(strongtalk-test-exe-sources))

strongtalk-test-cxx-include-paths    	:= src/cpp/test src/cpp/main $(googletest-include-paths)
strongtalk-test-cxx-include-flags    	:= $(addprefix -I ,$(strongtalk-test-cxx-include-paths))
strongtalk-test-define-values        	:= 
strongtalk-test-define-flags         	:= $(addprefix -D ,$(strongtalk-test-define-values))

strongtalk-test-exe                  	:= $(object-path)/strongtalk-test.exe


# targets ---------------------------------------------------------------------

.PHONY : default strongtalk-main-exe strongtalk-test-exe googletest 
.PHONY : init init-ext init-wine 

default : strongtalk-main-exe strongtalk-test-exe

help  : 
	@echo ""
	@echo "NAME"
	@echo ""
	@echo "    strongtalk-2020"
	@echo ""
	@echo ""
	@echo "TARGETS"
	@echo ""
	@echo "    make wine-kill-all"
	@echo "        kill all wine processex via wineserver --kill"
	@echo ""
	@echo "    make wine-strongtalk-main-exe"
	@echo "        build and run strongtalk.exe under wine"
	@echo ""
	@echo "    make wine-strongtalk-test-exe"
	@echo "        build and run strongtalk-test.exe under wine"
	@echo ""
	@echo "    make strongtalk-main-exe"
	@echo "        build strongtalk.exe"
	@echo ""
	@echo "    make strongtalk-main-exe-run"
	@echo "        build and run strongtalk.exe"
	@echo ""
	@echo "    make strongtalk-test-exe"
	@echo "        build strongtalk-test.exe"
	@echo ""
	@echo "    make strongtalk-test-exe-run"
	@echo "        build and run strongtalk-test.exe"
	@echo ""
	@echo "META"
	@echo ""
	@echo "    this file is [$(THIS-FILE)]"
	@echo ""


# environment -----------------------------------------------------------------

init : init-ext init-wine

init-ext : capstone googletest nasm udis86 zydis

init-wine :
	-$(wineserver) --kill=9
	$(wineboot) --init


# executables -----------------------------------------------------------------

strongtalk-main-exe : $(strongtalk-main-exe)

strongtalk-main-exe-run : $(strongtalk-exe)
	$(strongtalk-main-exe)

strongtalk-test-exe : $(strongtalk-test-exe)

strongtalk-test-exe-run : $(strongtalk-test-exe)
	$(strongtalk-test-exe)


# wine ------------------------------------------------------------------------

wine-kill-all :
	-$(wineserver) --kill

wine-strongtalk-main-exe : $(strongtalk-exe)
	$(wine) $(strongtalk-main-exe)

wine-strongtalk-test-exe : $(strongtalk-test-exe)
	$(wine) $(strongtalk-test-exe)


# valgrind --------------------------------------------------------------------

valgrind-strongtalk-main-exe :
	$(valgrind) $(valgrind-options) $(wine) $(strongtalk-main-exe)

valgrind-strongtalk-test-exe :
	$(valgrind) $(valgrind-options) $(wine) $(strongtalk-test-exe)


# tags ------------------------------------------------------------------------

tags : GPATH GRTAGS GTAGS

GPATH GRTAGS GTAGS : $(strongtalk-sources) $(strongtalk-test-lib-sources)
	gtags --verbose --statistics $<


# clean -----------------------------------------------------------------------

clean : clean-main clean-test

clean-main :
	-rm $(strongtalk-main-lib-objects)
	-rm $(strongtalk-main-lib-depends)
	-rm $(strongtalk-main-exe-objects)
	-rm $(strongtalk-main-exe-depends)
	-rm $(strongtalk-main-exe)

clean-test :
	-rm $(strongtalk-test-lib-depends)
	-rm $(strongtalk-test-exe-objects)
	-rm $(strongtalk-test-exe-depends)
	-rm $(strongtalk-test-exe)

.PHONY : clean clean-main


# strongtalk.exe --------------------------------------------------------------

ifeq ($(toolchain-name),i686-w64-mingw32)
platform-flags :=  $(mingw-w64-link-flags)
endif

ifeq ($(toolchain-name),i686-linux-gnu)
platform-flags := -ldl -lpthread -lreadline
endif

$(strongtalk-main-exe-objects) $(strongtalk-main-lib-objects) : $(object-path)/%.cpp.o : %.cpp $(object-path)/%.cpp.d
	-@mkdir -p $(dir $@)
	$(ccache) $(g++) $(cxx-compile-flags) $(strongtalk-main-lib-cxx-include-flags) -c $< -o $@

$(strongtalk-main-lib-depends) :
include $(wildcard $(strongtalk-main-lib-depends))

# make: *** No rule to make target 'src/cpp/main/vm/main/*.cpp', needed by 'obj/i686-linux-gnu-9//src/cpp/main/vm/main/*.cpp.o'.  Stop.
 
$(strongtalk-main-exe) : $(strongtalk-main-lib-objects) $(strongtalk-main-exe-objects)
	$(g++) \
		-o $(strongtalk-main-exe) \
		$(cxx-link-flags) \
		$(libgcc-link-flags) \
		$(strongtalk-main-lib-objects) \
		$(strongtalk-main-exe-objects) \
		$(libnasm-a-flags) \
		$(libudis86-a-flags) \
		$(platform-flags)

$(strongtalk-main-exe-depends) :
include $(wildcard $(strongtalk-main-exe-depends))

$(strongtalk-main-lib-depends) :
include $(wildcard $(strongtalk-main-lib-depends))


# strongtalk-test.exe ----------------------------------------------------------

$(strongtalk-test-exe-objects) : $(object-path)/%.cpp.o : %.cpp $(object-path)/%.cpp.d
	-@mkdir -p $(dir $@)
	$(ccache) $(g++) $(cxx-compile-flags) $(strongtalk-test-cxx-include-flags) -c $< -o $@

$(strongtalk-test-exe-depends) :
include $(wildcard $(strongtalk-test-exe-depends))

$(strongtalk-test-exe) : $(strongtalk-test-exe-objects) $(strongtalk-main-lib-objects)
	$(g++) \
		$(cxx-link-flags) \
		$(libgcc-link-flags) \
		$(strongtalk-objects) \
		$(strongtalk-test-exe-objects) \
		$(strongtalk-main-lib-objects) \
		$(libudis86-a-flags) \
		$(libgtest-a-flags) \
		$(platform-flags) \
		-o $(strongtalk-test-exe)


# ccache ----------------------------------------------------------------------

ccache-clear : $(CCACHE_DIR)/ccache.conf
	@ccache --clear

ccache-show-stats : $(CCACHE_DIR)/ccache.conf
	@ccache --show-stats

ccache-zero-stats : $(CCACHE_DIR)/ccache.conf
	@ccache --zero-stats

$(CCACHE_DIR)/ccache.conf : | $(CCACHE_DIR)
	@echo "max_size = 32G" | tee $(CCACHE_DIR)/ccache.conf

$(CCACHE_DIR) :
	-@mkdir -p $(CCACHE_DIR)


# capstone --------------------------------------------------------------------

capstone :
	cmake -G Ninja -D CMAKE_SYSTEM_NAME=$(cmake-system-name) -S $(capstone-root-dir) -B $(capstone-object-dir)
	ninja -C $(capstone-object-dir)


# doxygen ---------------------------------------------------------------------

doxygen-generate :
	doxygen etc/doxygen/strongtalk.conf


# googletest ------------------------------------------------------------------

googletest :
	cmake -G Ninja -D CMAKE_SYSTEM_NAME=$(cmake-system-name) -D CMAKE_CXX_COMPILER_LAUNCHER=$(ccache) -D CMAKE_CXX_COMPILER=$(g++) -S $(googletest-root-dir) -B $(googletest-object-dir)
	ninja -C $(googletest-object-dir)


# nasm ------------------------------------------------------------------------

nasm :
	-cd $(ROOT)/ext/nasm && make distclean
	cd $(ROOT)/ext/nasm && ./configure --host $(machine-name)-$(runtime-platform) --prefix $(ROOT)/$(nasm-object-dir)
	cd $(ROOT)/ext/nasm && make libnasm.a
	objcopy --remove-leading-char $(ROOT)/ext/nasm/libnasm.a $(ROOT)/ext/nasm/libnasm.a
	install --verbose $(ROOT)/ext/nasm/config/* 	-D --target-directory $(nasm-object-dir)/include/config
	install --verbose $(ROOT)/ext/nasm/include/* 	-D --target-directory $(nasm-object-dir)/include
	install --verbose $(ROOT)/ext/nasm/disasm/* 	-D --target-directory $(nasm-object-dir)/include
	install --verbose $(ROOT)/ext/nasm/x86/* 		-D --target-directory $(nasm-object-dir)/include
	install --verbose $(ROOT)/ext/nasm/libnasm.a 	-D --target-directory $(nasm-object-dir)/lib


# spdlog ----------------------------------------------------------------------

spdlog :
	cmake -G Ninja -D CMAKE_SYSTEM_NAME=$(cmake-system-name) -D CMAKE_CXX_COMPILER_LAUNCHER=$(ccache) -D CMAKE_CXX_COMPILER=$(g++) -D CMAKE_C_COMPILER=$(gcc) -S $(spdlog-root-dir) -B $(spdlog-object-dir)
	ninja -C $(spdlog-object-dir)



# udis86 ----------------------------------------------------------------------

udis86 :
	cd $(ROOT)/ext/udis86 && make distclean
	cd $(ROOT)/ext/udis86 && ./configure --host $(machine-name)-$(runtime-platform) --prefix $(ROOT)/$(udis86-object-dir)
	cd $(ROOT)/ext/udis86 && make install


# zydis -----------------------------------------------------------------------

zydis :
	cmake -G Ninja -D CMAKE_SYSTEM_NAME=$(cmake-system-name) -D CMAKE_CXX_COMPILER_LAUNCHER=$(ccache) -D CMAKE_CXX_COMPILER=$(g++) -S $(zydis-root-dir) -B $(zydis-object-dir)
	ninja -C $(zydis-object-dir)

