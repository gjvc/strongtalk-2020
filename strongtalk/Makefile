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
runtime-variant     := posix
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
runtime-variant     := $(gcc-version-major)
endif


# gcc -------------------------------------------------------------------------

toolchain-name  := $(machine-name)-$(runtime-platform)
object-path 	:= obj/$(toolchain-name)-$(runtime-variant)/$(build-type)


# gcc -------------------------------------------------------------------------

g++ := /usr/bin/$(toolchain-name)-g++-$(runtime-variant)
gcc := /usr/bin/$(toolchain-name)-gcc-$(runtime-variant)
ar  := /usr/bin/$(toolchain-name)-ar
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

cxx-feature-options             := diagnostics-color=always permissive concepts check-new
cxx-feature-flags               := $(addprefix -f,$(cxx-feature-options))

cxx-language-options            := c++2a
cxx-language-flags              := $(addprefix -std=,$(cxx-language-options))

cxx-machine-options             := 32
cxx-machine-flags               := $(addprefix -m,$(cxx-machine-options))

cxx-no-warning-options          := comment unused-parameter
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


ifeq ($(runtime-variant),posix)
mingw-w64-link-flags = \
	-L /usr/$(machine-name)-$(runtime-platform)/lib -Wl,-Bstatic,--whole-archive -l winpthread -Wl,-Bdynamic,--no-whole-archive
endif



# strongtalk-main-{sources,objects,depends} -------------------------------------

default : strongtalk-main-exe strongtalk-test-exe

clean : strongtalk-main-lib-clean strongtalk-main-exe-clean 


# strongtalk-main -----------------------------------------------------------

strongtalk-main-cxx-include-paths   := $(nasm-include-paths) $(udis86-include-paths) src/cpp/main
strongtalk-main-cxx-include-flags   := $(addprefix -I ,$(strongtalk-main-cxx-include-paths))

strongtalk-main-define-values    	:= 
strongtalk-main-define-flags        := $(addprefix -D ,$(strongtalk-main-define-values))


# strongtalk-main-lib -------------------------------------------------------

strongtalk-main-lib-sources         := $(wildcard src/cpp/main/vm/*/*.cpp)
strongtalk-main-lib-sources         := $(filter-out src/cpp/main/vm/main/%.cpp,$(strongtalk-main-lib-sources))
strongtalk-main-lib-objects         := $(patsubst %.cpp,$(object-path)/%.cpp.o,$(strongtalk-main-lib-sources))
strongtalk-main-lib-depends         := $(patsubst %.cpp,$(object-path)/%.cpp.d,$(strongtalk-main-lib-sources))
strongtalk-main-lib        			:= $(object-path)/libstrongtalk.a
strongtalk-main-lib-link-flags      := -L $(object-path) -Wl,-Bstatic,--whole-archive -l strongtalk -Wl,--no-whole-archive,--Bdynamic

strongtalk-main-lib : $(strongtalk-main-lib)

$(strongtalk-main-lib-objects) : $(object-path)/%.cpp.o : %.cpp $(object-path)/%.cpp.d
	-@mkdir -p $(dir $@)
	$(ccache) $(g++) $(cxx-compile-flags) $(strongtalk-main-cxx-include-flags) -c $< -o $@

$(strongtalk-main-lib-depends) :
include $(wildcard $(strongtalk-main-lib-depends))

$(strongtalk-main-lib) : $(strongtalk-main-lib-objects)
	@$(ar) rcs $(strongtalk-main-lib)  $(strongtalk-main-lib-objects)

strongtalk-main-lib-clean : 
	-rm $(strongtalk-main-lib)
	-rm $(strongtalk-main-lib-depends)
	-rm $(strongtalk-main-lib-objects)


# strongtalk-main-exe -------------------------------------------------------

strongtalk-main-exe-sources         := $(wildcard src/cpp/main/vm/main/*.cpp)
strongtalk-main-exe-objects         := $(patsubst %.cpp,$(object-path)/%.cpp.o,$(strongtalk-main-exe-sources))
strongtalk-main-exe-depends         := $(patsubst %.cpp,$(object-path)/%.cpp.d,$(strongtalk-main-exe-sources))
strongtalk-main-exe                 := $(object-path)/strongtalk-main.exe

strongtalk-main-exe : $(strongtalk-main-exe)

$(strongtalk-main-exe-objects) : $(object-path)/%.cpp.o : %.cpp $(object-path)/%.cpp.d
	-@mkdir -p $(dir $@)
	$(ccache) $(g++) $(cxx-compile-flags) $(strongtalk-main-cxx-include-flags) -c $< -o $@

$(strongtalk-main-exe-depends) :
include $(wildcard $(strongtalk-main-exe-depends))

$(strongtalk-main-exe) : $(strongtalk-main-exe-objects) $(strongtalk-main-lib)
	$(g++) \
		$(cxx-link-flags) \
		$(libgcc-link-flags) \
		$(strongtalk-main-exe-objects) \
		$(strongtalk-main-lib-link-flags) \
		$(libnasm-a-flags) \
		$(platform-flags) \
		-o $(strongtalk-main-exe)

strongtalk-main-exe-clean : 
	-rm $(strongtalk-main-exe)
	-rm $(strongtalk-main-exe-depends)
	-rm $(strongtalk-main-exe-objects)


# strongtalk-test-lib -------------------------------------------------------

strongtalk-test-cxx-include-paths   := src/cpp/test src/cpp/main $(googletest-include-paths)
strongtalk-test-cxx-include-flags   := $(addprefix -I ,$(strongtalk-test-cxx-include-paths))

strongtalk-test-define-values       := 
strongtalk-test-define-flags        := $(addprefix -D ,$(strongtalk-test-define-values))

strongtalk-test-lib-sources         := $(wildcard src/cpp/test/vm/*/*.cpp)
strongtalk-test-lib-sources         := $(filter-out src/cpp/test/test/main/%.cpp,$(strongtalk-test-lib-sources))
strongtalk-test-lib-objects         := $(patsubst %.cpp,$(object-path)/%.cpp.o,$(strongtalk-test-lib-sources))
strongtalk-test-lib-depends         := $(patsubst %.cpp,$(object-path)/%.cpp.d,$(strongtalk-test-lib-sources))
strongtalk-test-lib                 := $(object-path)/libstrongtalktest.a
strongtalk-test-lib-link-flags      := -L $(object-path) -Wl,-Bstatic,--whole-archive -l strongtalktest -Wl,--no-whole-archive,--Bdynamic

strongtalk-test-lib : $(strongtalk-test-lib)

$(strongtalk-test-lib-depends) :
include $(wildcard $(strongtalk-test-lib-depends))

$(strongtalk-test-lib) : $(strongtalk-test-lib-objects)
	$(ar) rcs $(strongtalk-test-lib) $(strongtalk-test-lib-objects)

strongtalk-test-lib-clean : 
	-rm $(strongtalk-test-lib)
	-rm $(strongtalk-test-lib-depends)
	-rm $(strongtalk-test-lib-objects)


# strongtalk-test-exe -------------------------------------------------------

strongtalk-test-exe-sources 		:= $(wildcard src/cpp/test/test/main/*.cpp)
strongtalk-test-exe-objects 		:= $(patsubst %.cpp,$(object-path)/%.cpp.o,$(strongtalk-test-exe-sources))
strongtalk-test-exe-depends 		:= $(patsubst %.cpp,$(object-path)/%.cpp.d,$(strongtalk-test-exe-sources))
strongtalk-test-exe     			:= $(object-path)/strongtalk-test.exe

$(strongtalk-test-exe-objects) : $(object-path)/%.cpp.o : %.cpp $(object-path)/%.cpp.d
	-@mkdir -p $(dir $@)
	$(ccache) $(g++) $(cxx-compile-flags) $(strongtalk-test-cxx-include-flags) -c $< -o $@

$(strongtalk-test-exe-depends) :
include $(wildcard $(strongtalk-test-exe-depends))

strongtalk-test-exe : $(strongtalk-test-exe)

$(strongtalk-test-exe) : $(strongtalk-test-exe-objects) $(strongtalk-test-lib) $(strongtalk-main-lib)
	$(g++) \
		$(cxx-link-flags) \
		$(libgcc-link-flags) \
		$(strongtalk-test-exe-objects) \
		$(strongtalk-test-lib-link-flags) \
		$(strongtalk-main-lib-link-flags) \
		$(libgtest-a-flags) \
		$(platform-flags) \
		-o $(strongtalk-test-exe)

strongtalk-test-exe-clean : 
	-rm $(strongtalk-test-exe)
	-rm $(strongtalk-test-exe-depends)
	-rm $(strongtalk-test-exe-objects)


# environment -----------------------------------------------------------------

init : init-ext init-wine

init-ext : capstone googletest nasm udis86 zydis

init-wine :
	-$(wineserver) --kill=9
	$(wineboot) --init


# wine ------------------------------------------------------------------------

.PHONY : wine-kill-all wine-strongtalk-main-exe wine-strongtalk-test-exe

wine-kill-all :
	-$(wineserver) --kill

wine-strongtalk-main-exe : $(strongtalk-exe)
	$(wine) $(strongtalk-main-exe)

wine-strongtalk-test-exe : $(strongtalk-test-exe)
	$(wine) $(strongtalk-test-exe)


# valgrind --------------------------------------------------------------------

.PHONY : valgrind-strongtalk-main-exe valgrind-strongtalk-test-exe

valgrind-strongtalk-main-exe :
	$(valgrind) $(valgrind-options) $(wine) $(strongtalk-main-exe)

valgrind-strongtalk-test-exe :
	$(valgrind) $(valgrind-options) $(wine) $(strongtalk-test-exe)


# tags ------------------------------------------------------------------------

.PHONY : tags

tags : GPATH GRTAGS GTAGS

GPATH GRTAGS GTAGS : $(strongtalk-main-lib-sources) $(strongtalk-test-lib-sources)
	gtags --verbose --statistics $<


# strongtalk.exe --------------------------------------------------------------

ifeq ($(toolchain-name),i686-w64-mingw32)
platform-flags :=  $(mingw-w64-link-flags)
endif

ifeq ($(toolchain-name),i686-linux-gnu)
platform-flags := -ldl -lpthread -lreadline
endif


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
	cmake \
		-G Ninja \
		-D CMAKE_SYSTEM_NAME=$(cmake-system-name) \
		-D CMAKE_CXX_COMPILER_LAUNCHER=$(ccache) \
		-D CMAKE_CXX_COMPILER=$(g++) \
		-S $(googletest-root-dir) \
		-B $(googletest-object-dir)
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
	cmake \
		-G Ninja \
		-D CMAKE_SYSTEM_NAME=$(cmake-system-name) \
		-D CMAKE_CXX_COMPILER_LAUNCHER=$(ccache) \
		-D CMAKE_CXX_COMPILER=$(g++) \
		-D CMAKE_C_COMPILER=$(gcc) \
		-S $(spdlog-root-dir) \
		-B $(spdlog-object-dir)
	ninja -C $(spdlog-object-dir)



# udis86 ----------------------------------------------------------------------

udis86 :
	-cd $(ROOT)/ext/udis86 && aclocal
	-cd $(ROOT)/ext/udis86 && automake
	cd $(ROOT)/ext/udis86 && ./configure --host $(machine-name)-$(runtime-platform) --prefix $(ROOT)/$(udis86-object-dir)
	cd $(ROOT)/ext/udis86 && make 


# zydis -----------------------------------------------------------------------

zydis :
	cmake \
		-G Ninja \
		-D CMAKE_CXX_COMPILER=$(g++) \
		-D CMAKE_CXX_COMPILER_LAUNCHER=$(ccache) \
		-D CMAKE_SYSTEM_NAME=$(cmake-system-name) \
		-S $(zydis-root-dir) \
		-B $(zydis-object-dir)
	ninja -C $(zydis-object-dir)

