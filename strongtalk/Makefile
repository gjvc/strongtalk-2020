
####
####  (C) 1994 - 2021, The Strongtalk authors and contributors
####  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
####


# ==== ESSENTIALS =============================================================

# GNU make --------------------------------------------------------------------

.SHELLFLAGS := -eu -o pipefail -c
SHELL       := /bin/bash

MAKEFLAGS   += --jobs $(shell nproc)
MAKEFLAGS   += --keep-going
MAKEFLAGS   += --no-builtin-rules
MAKEFLAGS   += --warn-undefined-variables

null 		:=
space   	:= $(null) $(null)


# location --------------------------------------------------------------------

this-file       := $(abspath $(firstword $(MAKEFILE_LIST)))
this-file-dir   := $(dir $(this-file))
ROOT            := $(this-file-dir:/=)


# ==== TOOLS ==================================================================

# ccache ----------------------------------------------------------------------

ccache := /usr/bin/ccache
export CCACHE_DIR=$(ROOT)/.ccache


# cmake -----------------------------------------------------------------------

cmake-program           := /usr/bin/cmake
cmake-system-name       := Windows
cmake-generator-name    := Ninja
ninja-program           := /usr/bin/ninja


# git -------------------------------------------------------------------------

git-current-branch      := $(shell git branch --show-current)
git-current-revision    := $(shell git rev-parse $(git-branch))


# wine ------------------------------------------------------------------------

wine   	                := /usr/bin/wine
wine-windows-version    := win95
wine-winedbg            := /usr/bin/winedbg
wine-wineserver         := /usr/bin/wineserver
wine-winetricks         := /usr/bin/winetricks

export WINEARCH=win32


# toolchain -------------------------------------------------------------------

arch                := i686
platform            := w64-mingw32
compiler-family     := gnu
compiler-version    := 10
runtime             := win32
toolchain-name      := $(arch)-$(platform)-$(compiler-family)-$(compiler-version)-$(runtime)

ar   		        := /usr/bin/$(arch)-$(platform)-gcc-ar-$(runtime)
ar-flags            := Dcrs

cc-compiler         := /usr/bin/$(arch)-$(platform)-gcc-$(runtime)
cxx-compiler        := /usr/bin/$(arch)-$(platform)-g++-$(runtime)

export CC=$(ccache) $(cc-compiler)
export CXX=$(ccache) $(cxx-compiler)


# object files ----------------------------------------------------------------

build-type          ?= debug
object-path         := $(toolchain-name)/$(build-type)
object-root         := obj/$(object-path)


# ==== EXTERNAL PACKAGES / DEPENDENCIES =======================================

# third-party packages --------------------------------------------------------

external-package-path           := src/ext
external-package-root           := $(ROOT)/$(external-package-path)
external-package-object-root    := $(ROOT)/$(object-root)/$(external-package-path)


# googletest ------------------------------------------------------------------

googletest-root-dir         := $(external-package-root)/googletest
googletest-include-paths    := $(googletest-root-dir)/googletest/include
googletest-object-dir       := $(external-package-object-root)/googletest
libgtest-a                  := $(googletest-object-dir)/lib/libgtest.a
libgtest-a-flags            := -L $(googletest-object-dir)/lib -l gtest


# nasm ------------------------------------------------------------------------

nasm-root-dir               := $(external-package-root)/nasm
nasm-object-dir             := $(external-package-object-root)/nasm
nasm-include-paths          := $(external-package-object-root)/nasm/include
libnasm-a                   := $(external-package-object-root)/nasm/lib/libnasm.a
libnasm-a-flags             := -L $(nasm-object-dir)/lib -l nasm


# ==== BUILDING ===============================================================

# compiler flags --------------------------------------------------------------

cxx-arch-options            := 
cxx-arch-flags            	:= $(addprefix -march=,$(cxx-arch-options))

cxx-debug-options           := dwarf
cxx-debug-flags             := $(addprefix -g,$(cxx-debug-options))

cxx-depends-options         := 
cxx-depends-flags           = -MMD -MP -MT $@ -MF $(object-root)/$*.cpp.d

cxx-feature-options         := permissive
cxx-feature-flags           := $(addprefix -f,$(cxx-feature-options))

cxx-language-options        := c++2a
cxx-language-flags          := $(addprefix -std=,$(cxx-language-options))

cxx-machine-options         := 32
cxx-machine-flags           := $(addprefix -m,$(cxx-machine-options))

cxx-no-warning-options      := unused-parameter
cxx-no-warning-flags        := $(addprefix -Wno-,$(cxx-no-warning-options))

ifeq ($(build-type),release)
cxx-optimisation-options    := 3
else
cxx-optimisation-options    := 0
endif
cxx-optimisation-flags      := $(addprefix -O,$(cxx-optimisation-options))

cxx-other-options           := pthread
cxx-other-flags             := $(addprefix -,$(cxx-other-options))

cxx-other-long-options      := diagnostics-color=always
cxx-other-long-flags        := $(addprefix --,$(cxx-other-long-options))

cxx-warning-options-all-all := $(shell $(cxx-compiler) -Q --help=warning | sed -e 's/^\s*\-W\(\S*\)\s*\[\w*\]/\1 /gp;d' | tr -d '\n' )
cxx-warning-options         := all extra
cxx-warning-flags           := $(addprefix -W,$(cxx-warning-options))

cxx-compile-flags = \
    $(cxx-arch-flags) \
    $(cxx-debug-flags) \
    $(cxx-depends-flags) \
    $(cxx-feature-flags) \
    $(cxx-language-flags) \
    $(cxx-machine-flags) \
    $(cxx-no-warning-flags) \
    $(cxx-optimisation-flags) \
    $(cxx-other-flags) \
    $(cxx-other-long-flags) \
    $(cxx-warning-flags) \

cxx-link-flags = \
    $(cxx-machine-flags)

libgcc-link-flags := \
    -static-libgcc \
    -static-libstdc++

ifeq ($(runtime),posix)
    libgcc-link-flags := $(libgcc-link-flags) \
        -Wl,-Bstatic \
        -lstdc++ -lwinpthread \
        -Wl,-Bdynamic
endif


# ==== RULES ====================================================================

# st20-main-lib -----------------------------------------------------------------

st20-main-lib               := $(object-root)/st20-main-lib.a
st20-main-lib-sources       := $(wildcard src/cpp/main/vm/*/*.cpp src/cpp/main/vm/*/*/*.cpp)
st20-main-lib-sources       := $(filter-out src/cpp/main/vm/main/%.cpp,$(st20-main-lib-sources))
st20-main-lib-objects       := $(patsubst %.cpp,$(object-root)/%.cpp.o,$(st20-main-lib-sources))
st20-main-lib-depends       := $(patsubst %.cpp,$(object-root)/%.cpp.d,$(st20-main-lib-sources))

st20-main-lib-cxx-include-paths-external := $(nasm-include-paths) $(udis86-include-paths)
st20-main-lib-cxx-include-flags-external := $(addprefix -isystem ,$(st20-main-lib-cxx-include-paths-external))
st20-main-lib-cxx-include-paths-internal := src/cpp/main
st20-main-lib-cxx-include-flags-internal := $(addprefix -I ,$(st20-main-lib-cxx-include-paths-internal))
st20-main-lib-cxx-include-flags          := $(st20-main-lib-cxx-include-flags-external) $(st20-main-lib-cxx-include-flags-internal)

st20-main-lib-define-values := 
st20-main-lib-define-flags  := $(addprefix -D ,$(st20-main-lib-define-values))


# st20-main-exe -----------------------------------------------------------------

st20-main-exe               := $(object-root)/st20-main.exe
st20-main-exe-sources       := $(wildcard src/cpp/main/vm/main/*.cpp)
st20-main-exe-objects       := $(patsubst %.cpp,$(object-root)/%.cpp.o,$(st20-main-exe-sources))
st20-main-exe-depends       := $(patsubst %.cpp,$(object-root)/%.cpp.d,$(st20-main-exe-sources))


# st20-test-exe -----------------------------------------------------------------

st20-test-lib               := $(object-root)/st20-test-lib.a
st20-test-lib-sources       := $(wildcard src/cpp/test/test/*/*.cpp)
st20-test-lib-sources       := $(filter-out src/cpp/test/test/main/%.cpp,$(st20-test-lib-sources))
st20-test-lib-objects       := $(patsubst %.cpp,$(object-root)/%.cpp.o,$(st20-test-lib-sources))
st20-test-lib-depends       := $(patsubst %.cpp,$(object-root)/%.cpp.d,$(st20-test-lib-sources))

st20-test-exe               := $(object-root)/st20-test.exe
st20-test-exe-sources       := $(wildcard src/cpp/test/test/main/*.cpp)
st20-test-exe-objects       := $(patsubst %.cpp,$(object-root)/%.cpp.o,$(st20-test-exe-sources))
st20-test-exe-depends       := $(patsubst %.cpp,$(object-root)/%.cpp.d,$(st20-test-exe-sources))

st20-test-lib-cxx-include-paths-external := $(googletest-include-paths)
st20-test-lib-cxx-include-flags-external := $(addprefix -isystem ,$(st20-test-lib-cxx-include-paths-external))
st20-test-lib-cxx-include-paths-internal := src/cpp/main src/cpp/test
st20-test-lib-cxx-include-flags-internal := $(addprefix -I ,$(st20-test-lib-cxx-include-paths-internal))
st20-test-lib-cxx-include-flags          := $(st20-test-lib-cxx-include-flags-external) $(st20-test-lib-cxx-include-flags-internal)

st20-test-define-values := 
st20-test-define-flags  := $(addprefix -D ,$(st20-test-define-values))


# ==== TARGETS ==================================================================

# targets -----------------------------------------------------------------------

.PHONY : default st20-main-exe st20-test-exe 
.PHONY : init init-3rd-party init-wine 

default : st20-main-exe st20-test-exe

help  : 
	@echo ""
	@echo "NAME"
	@echo ""
	@echo "    st20-2020"
	@echo ""
	@echo ""
	@echo "TARGETS"
	@echo ""
	@echo "    make wine-kill-all"
	@echo "        kill all wine processex via wine-wineserver --kill"
	@echo ""
	@echo "    make wine-st20-main-exe"
	@echo "        build and run strongtalk.exe under wine"
	@echo ""
	@echo "    make wine-st20-test-exe"
	@echo "        build and run st20-test.exe under wine"
	@echo ""
	@echo "    make st20-main-exe"
	@echo "        build strongtalk.exe"
	@echo ""
	@echo "    make st20-main-exe-run"
	@echo "        build and run strongtalk.exe"
	@echo ""
	@echo "    make st20-test-exe"
	@echo "        build st20-test.exe"
	@echo ""
	@echo "    make st20-test-exe-run"
	@echo "        build and run st20-test.exe"
	@echo ""
	@echo "META"
	@echo ""
	@echo "    this file is [$(THIS-FILE)]"
	@echo ""


# environment -----------------------------------------------------------------

wine-init :
	-$(wine-wineserver) --kill=9
	-rm -rf ~/.wine/
	$(wine-winetricks) $(wine-windows-version)


# executables -----------------------------------------------------------------

st20-main-exe : $(st20-main-exe)

st20-test-exe : $(st20-test-exe)

st20-main-lib : st20-main-lib-a

st20-test-lib : st20-test-lib-a


# wine ------------------------------------------------------------------------

wine-kill-all :
	-$(wine-wineserver) --kill

wine-st20-main-exe : $(st20-exe)
	$(wine) $(st20-main-exe)

wine-st20-test-exe : $(st20-test-exe)
	$(wine) $(st20-test-exe)


# tags ------------------------------------------------------------------------

tags : GPATH GRTAGS GTAGS

GPATH GRTAGS GTAGS : $(st20-sources) $(st20-test-lib-sources)
	gtags --verbose --statistics $<


# clean -----------------------------------------------------------------------

clean : clean-main clean-test

clean-main :
	-@rm $(st20-main-lib-objects)
	-@rm $(st20-main-lib-depends)
	-@rm $(st20-main-exe-objects)
	-@rm $(st20-main-exe-depends)
	-@rm $(st20-main-exe)

clean-test :
	-@rm $(st20-test-lib-depends)
	-@rm $(st20-test-exe-objects)
	-@rm $(st20-test-exe-depends)
	-@rm $(st20-test-exe)

.PHONY : clean clean-main


# st20-main-lib-a -------------------------------------------------------

st20-main-lib-a := $(object-root)/libst20-main-lib-static.a
st20-main-lib-a : $(st20-main-lib-a)

$(st20-main-lib-a) : $(st20-main-lib-objects)
	@$(ar) $(ar-flags) $(st20-main-lib-a) $(st20-main-lib-objects)


# st20-test-lib-a -------------------------------------------------------

st20-test-lib-a := $(object-root)/libst20-test-lib-static.a
st20-test-lib-a : $(st20-test-lib-a)

$(st20-test-lib-a) : $(st20-test-lib-objects)
	@$(ar) $(ar-flags) $(st20-test-lib-a) $(st20-test-lib-objects)


# st20-main.exe ---------------------------------------------------------

$(st20-main-exe-objects) $(st20-main-lib-objects) : $(object-root)/%.cpp.o : %.cpp $(object-root)/%.cpp.d
	-@mkdir -p $(dir $@)
	@$(ccache) $(cxx-compiler) $(cxx-compile-flags) $(st20-main-lib-cxx-include-flags) -c $< -o $@
	@echo ok: $@ from $<

$(st20-main-exe) : $(st20-main-exe-objects) $(st20-main-lib-a)
	@$(cxx-compiler) \
		$(cxx-link-flags) \
		$(libgcc-link-flags) \
		$(st20-main-exe-objects) \
		$(st20-main-lib-a) \
		$(libnasm-a-flags) \
		-o $(st20-main-exe)
	@echo ok: $@ linked 

$(st20-main-exe-depends) :
include $(wildcard $(st20-main-exe-depends))

$(st20-main-lib-depends) :
include $(wildcard $(st20-main-lib-depends))


# st20-test.exe ----------------------------------------------------------

$(st20-test-exe-objects) $(st20-test-lib-objects) : $(object-root)/%.cpp.o : %.cpp $(object-root)/%.cpp.d
	-@mkdir -p $(dir $@)
	@$(ccache) $(cxx-compiler) $(cxx-compile-flags) $(st20-test-lib-cxx-include-flags) -c $< -o $@
	@echo ok: $@ from $<

$(st20-test-exe) : $(st20-test-exe-objects) $(st20-main-lib-a) $(st20-test-lib-a)
	@$(cxx-compiler) \
		$(cxx-link-flags) \
		$(libgcc-link-flags) \
		$(st20-test-exe-objects) \
		$(st20-test-lib-a) \
		$(st20-main-lib-a) \
		$(libgtest-a-flags) \
		-o $(st20-test-exe)
	@echo ok: $@ linked

$(st20-test-exe-depends) :
include $(wildcard $(st20-test-exe-depends))

$(st20-test-lib-depends) :
include $(wildcard $(st20-test-lib-depends))


# ccache ----------------------------------------------------------------------

ccache-clear : $(CCACHE_DIR)/ccache.conf
	@ccache --clear

ccache-show-stats : $(CCACHE_DIR)/ccache.conf
	@ccache --show-stats

ccache-zero-stats : $(CCACHE_DIR)/ccache.conf
	@ccache --zero-stats

$(CCACHE_DIR)/ccache.conf : | $(CCACHE_DIR)
	touch $(CCACHE_DIR)/ccache.conf

$(CCACHE_DIR) :
	-@mkdir -p $(CCACHE_DIR)


# googletest ------------------------------------------------------------------

googletest :
	$(cmake-program) \
        -G $(cmake-generator-name) \
        -D CMAKE_CXX_COMPILER=$(cxx-compiler) \
        -D CMAKE_CXX_COMPILER_LAUNCHER=$(ccache) \
        -D CMAKE_SYSTEM_NAME=$(cmake-system-name) \
        -S $(googletest-root-dir) \
        -B $(googletest-object-dir)
	$(ninja-program) -C $(googletest-object-dir)


# nasm ------------------------------------------------------------------------

nasm :
	cd $(nasm-root-dir) && make distclean || /bin/true
	cd $(nasm-root-dir) && ./configure --host $(arch)-$(platform) --prefix $(nasm-object-dir)
	cd $(nasm-root-dir) && make libnasm.a
	objcopy --remove-leading-char $(nasm-root-dir)/libnasm.a $(nasm-root-dir)/libnasm.a
	install --verbose $(nasm-root-dir)/config/* 	-D --target-directory $(nasm-object-dir)/include/config
	install --verbose $(nasm-root-dir)/disasm/* 	-D --target-directory $(nasm-object-dir)/include
	install --verbose $(nasm-root-dir)/include/* 	-D --target-directory $(nasm-object-dir)/include
	install --verbose $(nasm-root-dir)/x86/* 		-D --target-directory $(nasm-object-dir)/include
	install --verbose $(nasm-root-dir)/libnasm.a 	-D --target-directory $(nasm-object-dir)/lib


# ==== UNUSED =================================================================

## capstone --------------------------------------------------------------------
#
#capstone-root-dir           := $(external-package-root)/capstone
#capstone-object-dir         := $(object-root)/$(external-package-root)/capstone
#capstone-include-paths      := $(capstone-root-dir)/capstone/include
#libcapstone-a               := $(capstone-object-dir)/libcapstone.a
#libcapstone-a-flags         := -L $(capstone-object-dir) -l capstone
#
## udis86 ----------------------------------------------------------------------
#
#udis86-root-dir             := $(external-package-root)/udis86
#udis86-object-dir           := $(external-package-object-root)/udis86
#udis86-include-paths        := $(external-package-object-root)/udis86/include
#udis86-library-paths        := $(external-package-object-root)/udis86/lib
#libudis86-a                 := $(external-package-object-root)/udis86/lib/libudis86.a
#libudis86-a-flags           := -L $(udis86-library-paths) -l udis86
#
## zydis -----------------------------------------------------------------------
#
#zydis-root-dir              := $(external-package-root)/zydis
#zydis-object-dir            := $(external-package-object-root)/zydis
#zydis-include-paths         := $(zydis-root-dir)/zydis/include
#libzydis-a                  := $(zydis-object-dir)/libzydis.a
#libzydis-a-flags            := -L $(zydis-object-dir) -l zydis
## capstone --------------------------------------------------------------------
#
#capstone :
#	$(cmake-program) \
#        -G $(cmake-generator-name) \
#        -D CMAKE_CXX_COMPILER=$(cxx-compiler) \
#        -D CMAKE_CXX_COMPILER_LAUNCHER=$(ccache) \
#        -D CMAKE_SYSTEM_NAME=$(cmake-system-name) \
#        -S $(capstone-root-dir) \
#        -B $(capstone-object-dir)
#	ninja -C $(capstone-object-dir)
#
#
## doxygen ---------------------------------------------------------------------
#
#doxygen-generate :
#	doxygen etc/doxygen/strongtalk.conf


## spdlog ----------------------------------------------------------------------
#
#spdlog :
#	$(cmake-program) \
#        -G $(cmake-generator-name) \
#        -D CMAKE_CXX_COMPILER=$(cxx-compiler) \
#        -D CMAKE_CXX_COMPILER_LAUNCHER=$(ccache) \
#        -D CMAKE_SYSTEM_NAME=$(cmake-system-name) \
#        -S $(spdlog-root-dir) \
#        -B $(spdlog-object-dir)
#	$(ninja-program) -C $(spdlog-object-dir)
#
#
#
## udis86 ----------------------------------------------------------------------
#
#udis86 :
#	cd $(udis86-root-dir) && make distclean || /bin/true
#	cd $(udis86-root-dir) && ./configure --host $(arch)-$(platform) --prefix $(ROOT)/$(udis86-object-dir)
#	cd $(udis86-root-dir) && make install
#
#
## zydis -----------------------------------------------------------------------
#
#zydis :
#	$(cmake-program) \
#        -G $(cmake-generator-name) \
#        -D CMAKE_CXX_COMPILER=$(cxx-compiler) \
#        -D CMAKE_CXX_COMPILER_LAUNCHER=$(ccache) \
#        -D CMAKE_SYSTEM_NAME=$(cmake-system-name) \
#        -S $(zydis-root-dir) \
#        -B $(zydis-object-dir)
#	$(ninja-program) -C $(zydis-object-dir)
#
