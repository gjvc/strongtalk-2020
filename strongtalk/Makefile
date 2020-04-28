#! /usr/bin/env make

##
##  (C) 1994 - 2020, The Strongtalk authors and contributors
##  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
##


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
runtime-variant     := -posix
gcc-version-major 	:= 9
gcc-version-minor 	:= 3
gcc-version     	:= $(gcc-version-major)-$(gcc-version-minor)
include-name        := mingw-w64
machine-name        := i686
runtime-platform    := w64-mingw32
cmake-system-name   := Windows

# i686-linux-gnu --------------------------------------------------------------

else ifeq ($(platform),linux)
machine-name        := i686
gcc-version-major 	:= 9
gcc-version-minor 	:= 3
gcc-version     	:= $(gcc-version-major)-$(gcc-version-minor)
runtime-platform    := linux-gnu
runtime-variant     := -9
cmake-system-name   := Linux

else
$(info )
$(info make platform=linux)
$(info make platform=mingw)
$(info )
$(error $(null))
endif


# gcc -------------------------------------------------------------------------

toolchain-name      := $(machine-name)-$(runtime-platform)
object-path         := obj/$(toolchain-name)$(runtime-variant)/$(build-type)


# gcc -------------------------------------------------------------------------

g++                 := /usr/bin/$(toolchain-name)-g++$(runtime-variant)
gcc                 := /usr/bin/$(toolchain-name)-gcc$(runtime-variant)
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


# mps -------------------------------------------------------------------------

mps-root-dir                    := ext/mps
mps-object-dir                  := $(object-path)/obj/ext/mps
mps-include-paths               := $(mps-object-dir)/include
libmps-a                        := $(mps-object-dir)/lib/libmps.a
libmps-a-flags                  := -L $(mps-object-dir)/lib -l mps


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
cxx-define-flags                = $(strongtalk-define-flags)

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



# strongtalk-{sources,objects,depends} -------------------------------------

strongtalk-exe                      := $(object-path)/strongtalk.exe
strongtalk-dirs                     := $(notdir $(wildcard src/cpp/main/vm/*))
strongtalk-sources                  := $(foreach dir,$(strongtalk-dirs),$(wildcard src/cpp/main/vm/$(dir)/*.cpp))
strongtalk-objects                  := $(patsubst %.cpp,$(object-path)/%.cpp.o,$(strongtalk-sources))
strongtalk-depends                  := $(patsubst %.cpp,$(object-path)/%.cpp.d,$(strongtalk-sources))
strongtalk-cxx-include-paths        := $(nasm-include-paths) $(udis86-include-paths) src/cpp/main
strongtalk-cxx-include-flags        := $(addprefix -I ,$(strongtalk-cxx-include-paths))
strongtalk-define-values            := 
strongtalk-define-flags             := $(addprefix -D ,$(strongtalk-define-values))


# strongtalktest-{sources,objects,depends} ---------------------------------

strongtalktest-exe                  := $(object-path)/strongtalktest.exe
strongtalktest-paths                := main primitives  interpreter
strongtalktest-paths                := compiler interpreter main memory primitives runtime utilities
strongtalktest-sources              := $(foreach dir,$(strongtalktest-paths),$(wildcard src/cpp/test/test/$(dir)/*.cpp))
#strongtalktest-sources              := $(filter-out $(strongtalk-exe-sources),$(strongtalktest-sources))
strongtalktest-objects              = $(patsubst %.cpp,$(object-path)/%.cpp.o,$(strongtalktest-sources))
strongtalktest-depends              = $(patsubst %.cpp,$(object-path)/%.cpp.d,$(strongtalktest-sources))
#src/cpp/test/test/main/main.cpp

strongtalktest-cxx-include-paths    := src/cpp/test src/cpp/main $(googletest-include-paths)
strongtalktest-cxx-include-flags    := $(addprefix -I ,$(strongtalktest-cxx-include-paths))

strongtalktest-define-values        := 
strongtalktest-define-flags         := $(addprefix -D ,$(strongtalktest-define-values))


# targets ---------------------------------------------------------------------

.PHONY : default strongtalk-exe strongtalktest-exe googletest 
.PHONY : init init-ext init-wine 

default : strongtalk-exe strongtalktest-exe

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
	@echo "    make wine-strongtalk-exe"
	@echo "        build and run strongtalk.exe under wine"
	@echo ""
	@echo "    make wine-strongtalktest-exe"
	@echo "        build and run strongtalk-test.exe under wine"
	@echo ""
	@echo "    make strongtalk-exe"
	@echo "        build strongtalk.exe"
	@echo ""
	@echo "    make strongtalk-exe-run"
	@echo "        build and run strongtalk.exe"
	@echo ""
	@echo "    make strongtalktest-exe"
	@echo "        build strongtalk-test.exe"
	@echo ""
	@echo "    make strongtalktest-exe-run"
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

strongtalk-exe : $(strongtalk-exe)

strongtalk-exe-run : $(strongtalk-exe)
	$(strongtalk-exe)

strongtalktest-exe : $(strongtalktest-exe)

strongtalktest-exe-run : $(strongtalktest-exe)
	$(strongtalktest-exe)


# wine ------------------------------------------------------------------------

wine-kill-all :
	-$(wineserver) --kill

wine-strongtalk-exe : $(strongtalk-exe)
	$(wine) $(strongtalk-exe)

wine-strongtalktest-exe : $(strongtalktest-exe)
	$(wine) $(strongtalktest-exe)


# valgrind --------------------------------------------------------------------

valgrind-strongtalk-exe :
	$(valgrind) $(valgrind-options) $(wine) $(strongtalk-exe)

valgrind-strongtalktest-exe :
	$(valgrind) $(valgrind-options) $(wine) $(strongtalktest-exe)


# tags ------------------------------------------------------------------------

tags : GPATH GRTAGS GTAGS

GPATH GRTAGS GTAGS : $(strongtalk-sources) $(strongtalktest-sources)
	gtags --verbose --statistics $<


# clean -----------------------------------------------------------------------

clean :
	-rm $(strongtalk-exe) $(strongtalk-exe-depends)
	-rm $(strongtalk-objects)
	-rm $(strongtalk-depends)
	-rm $(strongtalktest-exe) $(strongtalktest-exe-objects)
	-rm $(strongtalktest-objects)


# strongtalk.exe --------------------------------------------------------------

ifeq ($(toolchain-name),i686-w64-mingw32)
platform-flags :=  $(mingw-w64-link-flags)
endif

ifeq ($(toolchain-name),i686-linux-gnu)
platform-flags := -ldl -lpthread -lreadline
endif


$(strongtalk-exe) : $(strongtalk-objects)
	$(g++) $(cxx-link-flags) \
		$(libgcc-link-flags) \
		$(strongtalk-objects) \
		$(libnasm-a-flags) \
		$(libudis86-a-flags) \
		$(platform-flags) \
		-o $(strongtalk-exe)

$(strongtalk-exe-depends) :
include $(wildcard $(strongtalk-exe-depends))


$(strongtalk-objects) : $(object-path)/%.cpp.o : %.cpp $(object-path)/%.cpp.d
	-@mkdir -p $(dir $@)
	$(ccache) $(g++) $(cxx-compile-flags) $(strongtalk-cxx-include-flags) -c $< -o $@

$(strongtalk-depends) :
include $(wildcard $(strongtalk-depends))


# strongtalktest.exe ----------------------------------------------------------

$(strongtalktest-exe) : $(strongtalktest-exe-objects) $(strongtalktest-objects) $(strongtalk-objects)
	$(g++) $(cxx-link-flags) \
		$(libgcc-link-flags) \
		$(strongtalk-objects) \
		$(strongtalktest-objects) \
		$(libudis86-a-flags) \
		$(libgtest-a-flags) \
		$(platform-flags) \
		-o $(strongtalktest-exe)

$(strongtalktest-exe-objects) : $(object-path)/%.cpp.o : %.cpp $(object-path)/%.cpp.d
	-@mkdir -p $(dir $@)
	$(ccache) $(g++) $(cxx-compile-flags) $(strongtalktest-cxx-include-flags) -c $< -o $@

$(strongtalktest-exe-depends) :
include $(wildcard $(strongtalktest-exe-depends))


$(strongtalktest-objects) : $(object-path)/%.cpp.o : %.cpp $(object-path)/%.cpp.d
	-@mkdir -p $(dir $@)
	$(ccache) $(g++) $(cxx-compile-flags) $(strongtalktest-cxx-include-flags) -c $< -o $@

$(strongtalktest-depends) :
include $(wildcard $(strongtalktest-depends))


# ccache ----------------------------------------------------------------------

ccache-clear : $(CCACHE_DIR)/ccache.conf
	@ccache --clear

ccache-show-stats : $(CCACHE_DIR)/ccache.conf
	@ccache --show-stats

ccache-zero-stats : $(CCACHE_DIR)/ccache.conf
	@ccache --zero-stats

$(CCACHE_DIR)/ccache.conf : | $(CCACHE_DIR)
	@echo "max_size = 32G" > $(CCACHE_DIR)/ccache.conf

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


# mps -------------------------------------------------------------------------

mps :
	cd $(ROOT)/ext/mps && make install prefix=$(mps-object-dir)
#	cd $(ROOT)/ext/mps && ./configure --host $(machine-name)-$(runtime-platform) --prefix $(mps-object-dir)


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

