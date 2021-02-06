
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


# git -------------------------------------------------------------------------

git-current-branch      := $(shell git branch --show-current)
git-current-revision    := $(shell git rev-parse $(git-current-branch))
export GIT_CURRENT_BRANCH=$(git-current-branch)
export GIT_CURRENT_REVISION=$(git-current-revision)


# wine ------------------------------------------------------------------------

wine   	                := /usr/bin/wine
wine-windows-version    := win95
wine-winedbg            := /usr/bin/winedbg
wine-wineserver         := /usr/bin/wineserver
wine-winetricks         := /usr/bin/winetricks

export WINEARCH=win32


# toolchain -------------------------------------------------------------------

compiler-family := gnu

ifeq ($(compiler-family),gnu)
machine             := i686
platform            := w64-mingw32
compiler-family     := gnu
compiler-version    := 10
runtime             := posix
toolchain-name      := $(machine)-$(platform)-$(compiler-family)-$(compiler-version)-$(runtime)

ar   		        := /usr/bin/$(machine)-$(platform)-gcc-ar-$(runtime)
ar-flags            := Dcrs

cc-compiler         := /usr/bin/$(machine)-$(platform)-gcc-$(runtime)
cxx-compiler        := /usr/bin/$(machine)-$(platform)-g++-$(runtime)

export CC=$(ccache) $(cc-compiler)
export CXX=$(ccache) $(cxx-compiler)
endif


ifeq ($(compiler-family),llvm)
machine             := i686
platform            := linux
compiler-family     := llvm
compiler-version    := 11
runtime             := posix
toolchain-name      := $(machine)-$(platform)-$(compiler-family)-$(compiler-version)-$(runtime)

ar   		        := /usr/bin/$(machine)-$(platform)-gcc-ar-$(runtime)
ar-flags            := Dcrs

cc-compiler         := /usr/bin/clang-$(compiler-version)
cxx-compiler        := /usr/bin/clang++-$(compiler-version)

run-clang-tidy-py   := /usr/lib/$(compiler-family)-$(compiler-version)/share/clang/run-clang-tidy.py

export CC=$(ccache) $(cc-compiler)
export CXX=$(ccache) $(cxx-compiler)
endif


# cmake -----------------------------------------------------------------------

ifeq ($(compiler-family),gnu)
cmake-system-name   := Windows
endif

ifeq ($(compiler-family),llvm)
cmake-system-name   := Linux
endif

cmake-program           := /usr/bin/cmake
cmake-generator-name    := Ninja
ninja-program           := /usr/bin/ninja


# object files ----------------------------------------------------------------

build-type      ?= debug
build-system    ?= gnu-make
object-path     := $(toolchain-name)/$(build-system)/$(build-type)
object-root     := obj/$(object-path)

object-root-cmake := obj/$(toolchain-name)/cmake/$(build-type)




# ==== EXTERNAL PACKAGES / DEPENDENCIES =======================================

# third-party package object roots --------------------------------------------

external-package-path               := ext
external-package-root               := $(ROOT)/ext
external-package-object-root        := $(ROOT)/obj/$(toolchain-name)/gnu-make/$(build-type)/ext
external-package-object-root-cmake  := $(ROOT)/obj/$(toolchain-name)/cmake/$(build-type)/ext


# fmt -------------------------------------------------------------------------

fmt-git-url         := https://github.com/fmtlib/fmt.git
fmt-root-dir        := $(external-package-root)/fmt
fmt-include-paths   := $(fmt-root-dir)/include
fmt-object-dir      := $(external-package-object-root-cmake)/fmt
libfmt-a            := $(fmt-object-dir)/lib/libfmt.a
libfmt-a-flags      := -L $(fmt-object-dir)/lib -l fmt


# googletest ------------------------------------------------------------------

googletest-git-url          := https://github.com/google/googletest
googletest-root-dir         := $(external-package-root)/googletest
googletest-include-paths    := $(googletest-root-dir)/googletest/include
googletest-object-dir       := $(external-package-object-root-cmake)/googletest
libgtest-a                  := $(googletest-object-dir)/lib/libgtest.a
libgtest-a-flags            := -L $(googletest-object-dir)/lib -l gtest


# nasm ------------------------------------------------------------------------

nasm-git-url        := https://github.com/netwide-assembler/nasm
nasm-root-dir       := $(external-package-root)/nasm
nasm-object-dir     := $(external-package-object-root)/nasm
nasm-include-paths  := $(external-package-object-root)/nasm/include
libnasm-a           := $(external-package-object-root)/nasm/lib/libnasm.a
libnasm-a-flags     := -L $(nasm-object-dir)/lib -l nasm


# spdlog ----------------------------------------------------------------------

spdlog-git-url          := https://github.com/gabime/spdlog.git
spdlog-root-dir         := $(external-package-root)/spdlog
spdlog-include-paths    := $(spdlog-root-dir)/include
spdlog-object-dir       := $(external-package-object-root-cmake)/spdlog
libspdlog-a             := $(spdlog-object-dir)/libspdlog.a
libspdlog-a-flags       := -L $(spdlog-object-dir) -l spdlog




# ==== BUILDING ===============================================================

# compiler flags --------------------------------------------------------------

cxx-debug-options           := gdb 3 dwarf-2
cxx-debug-flags             := $(addprefix -g,$(cxx-debug-options))

cxx-define-options          :=
cxx-define-flags            := $(addprefix -D,$(cxx-define-options))

cxx-depends-options         := 
cxx-depends-flags           = -MMD -MP -MT $@ -MF $(object-root)/$*.cpp.depends

cxx-feature-options         := permissive
cxx-feature-flags           := $(addprefix -f,$(cxx-feature-options))

cxx-language-options        := c++2a
cxx-language-flags          := $(addprefix -std=,$(cxx-language-options))

cxx-machine-options         := arch=native
cxx-machine-flags           := $(addprefix -m,$(cxx-machine-options))

cxx-no-warning-options      := 
cxx-no-warning-flags        := $(addprefix -Wno-,$(cxx-no-warning-options))

cxx-optimisation-options    := 3
cxx-optimisation-flags      := $(addprefix -O,$(cxx-optimisation-options))

cxx-other-options           := 
cxx-other-flags             := $(addprefix -,$(cxx-other-options))

cxx-other-long-options      := 
cxx-other-long-flags        := $(addprefix --,$(cxx-other-long-options))

cxx-warning-options         := all extra effc++
cxx-warning-flags           := $(addprefix -W,$(cxx-warning-options))

cxx-compile-flags = \
    $(cxx-define-flags) \
    $(cxx-machine-flags) \
    $(cxx-debug-flags) \
    $(cxx-depends-flags) \
    $(cxx-feature-flags) \
    $(cxx-language-flags) \
    $(cxx-no-warning-flags) \
    $(cxx-optimisation-flags) \
    $(cxx-other-flags) \
    $(cxx-other-long-flags) \
    $(cxx-warning-flags) \

cxx-link-flags = \
    -static \
    $(cxx-machine-flags)

libgcc-link-flags := \
    -static-libgcc \
    -static-libstdc++


# ==== RULES ====================================================================

# st20-main-lib -----------------------------------------------------------------

st20-main-lib               := $(object-root)/st20-main-lib.a
st20-main-lib-sources       := $(wildcard src/cpp/auto/vm/*/*.cpp src/cpp/main/vm/*/*.cpp src/cpp/main/vm/*/*/*.cpp)
st20-main-lib-sources       := $(filter-out src/cpp/main/vm/main/%.cpp,$(st20-main-lib-sources))
st20-main-lib-objects       := $(patsubst %.cpp,$(object-root)/%.cpp.o,$(st20-main-lib-sources))
st20-main-lib-depends       := $(patsubst %.cpp,$(object-root)/%.cpp.depends,$(st20-main-lib-sources))

st20-main-lib-cxx-include-paths-external := $(nasm-include-paths) $(fmt-include-paths) $(spdlog-include-paths)
st20-main-lib-cxx-include-flags-external := $(addprefix -isystem ,$(st20-main-lib-cxx-include-paths-external))
st20-main-lib-cxx-include-paths-internal := src/cpp/auto src/cpp/main
st20-main-lib-cxx-include-flags-internal := $(addprefix -I ,$(st20-main-lib-cxx-include-paths-internal))
st20-main-lib-cxx-include-flags          := $(st20-main-lib-cxx-include-flags-external) $(st20-main-lib-cxx-include-flags-internal)

st20-main-lib-define-values := 
st20-main-lib-define-flags  := $(addprefix -D ,$(st20-main-lib-define-values))


# st20-main-exe -----------------------------------------------------------------

st20-main-exe               := $(object-root)/st20-main.exe
st20-main-exe-sources       := $(wildcard src/cpp/main/vm/main/*.cpp)
st20-main-exe-objects       := $(patsubst %.cpp,$(object-root)/%.cpp.o,$(st20-main-exe-sources))
st20-main-exe-depends       := $(patsubst %.cpp,$(object-root)/%.cpp.depends,$(st20-main-exe-sources))

st20-main-lib-define-values := GIT_CURRENT_REVISION=$(GIT_CURRENT_REVISION) GIT_CURRENT_BRANCH=$(GIT_CURRENT_BRANCH)
st20-main-lib-define-flags  := $(addprefix -D ,$(st20-main-lib-define-values))


# st20-test-exe -----------------------------------------------------------------

st20-test-lib               := $(object-root)/st20-test-lib.a
st20-test-lib-sources       := $(wildcard src/cpp/test/test/*/*.cpp)
st20-test-lib-sources       := $(filter-out src/cpp/test/test/main/%.cpp,$(st20-test-lib-sources))
st20-test-lib-objects       := $(patsubst %.cpp,$(object-root)/%.cpp.o,$(st20-test-lib-sources))
st20-test-lib-depends       := $(patsubst %.cpp,$(object-root)/%.cpp.depends,$(st20-test-lib-sources))

st20-test-exe               := $(object-root)/st20-test.exe
st20-test-exe-sources       := $(wildcard src/cpp/test/test/main/*.cpp)
st20-test-exe-objects       := $(patsubst %.cpp,$(object-root)/%.cpp.o,$(st20-test-exe-sources))
st20-test-exe-depends       := $(patsubst %.cpp,$(object-root)/%.cpp.depends,$(st20-test-exe-sources))

st20-test-lib-cxx-include-paths-external := $(googletest-include-paths) $(fmt-include-paths) $(spdlog-include-paths)
st20-test-lib-cxx-include-flags-external := $(addprefix -isystem ,$(st20-test-lib-cxx-include-paths-external))
st20-test-lib-cxx-include-paths-internal := src/cpp/auto src/cpp/main src/cpp/test
st20-test-lib-cxx-include-flags-internal := $(addprefix -I ,$(st20-test-lib-cxx-include-paths-internal))
st20-test-lib-cxx-include-flags          := $(st20-test-lib-cxx-include-flags-external) $(st20-test-lib-cxx-include-flags-internal)

st20-test-define-values     := 
st20-test-define-flags      := $(addprefix -D ,$(st20-test-define-values))


# ==== TARGETS ==================================================================

# targets -----------------------------------------------------------------------

.PHONY : default st20-main-exe st20-test-exe 
.PHONY : init init-3rd-party init-wine 

default : st20-main-exe st20-test-exe

cmake-build : 
	$(cmake-program) \
        -G $(cmake-generator-name) \
        -D CMAKE_C_COMPILER=$(cc-compiler) \
        -D CMAKE_C_COMPILER_LAUNCHER=$(ccache) \
        -D CMAKE_CXX_COMPILER=$(cxx-compiler) \
        -D CMAKE_CXX_COMPILER_LAUNCHER=$(ccache) \
        -D CMAKE_SYSTEM_NAME=$(cmake-system-name) \
        -S $(ROOT) \
        -B $(object-root-cmake)
	@$(ninja-program) -k 0 -C $(object-root-cmake)

run-clang-tidy :
	$(run-clang-tidy-py) -p $(object-root) 

help :
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


settings :
	@echo ""
	@echo "ar                      $(ar)"
	@echo "cc-compiler             $(cc-compiler)"
	@echo "cmake-generator-name    $(cmake-generator-name)"
	@echo "cmake-program           $(cmake-program)"
	@echo "cmake-system-name       $(cmake-system-name)"
	@echo "compiler-family         $(compiler-family)"
	@echo "compiler-version        $(compiler-version)"
	@echo "cxx-compiler            $(cxx-compiler)"
	@echo "git-current-branch      $(git-current-branch)"
	@echo "git-current-revision    $(git-current-revision)"
	@echo "object-root-cmake       $(object-root-cmake)"
	@echo "runtime                 $(runtime)"
	@echo "toolchain-name          $(toolchain-name)"
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

tags : $(st20-main-lib-sources) $(st20-test-lib-sources)
	@ctags-exuberant -R src/cpp/main/

GPATH GRTAGS GTAGS : $(st20-main-lib-sources) $(st20-test-lib-sources)
	gtags --verbose --statistics $<


# clean -----------------------------------------------------------------------

clean :
	-@rm $(st20-main-lib-objects)
	-@rm $(st20-main-lib-depends)
	-@rm $(st20-main-exe-objects)
	-@rm $(st20-main-exe-depends)
	-@rm $(st20-main-exe)
	-@rm $(st20-test-lib-depends)
	-@rm $(st20-test-exe-objects)
	-@rm $(st20-test-exe-depends)
	-@rm $(st20-test-exe)

.PHONY : clean


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

$(st20-main-exe-objects) $(st20-main-lib-objects) : $(object-root)/%.cpp.o : %.cpp $(object-root)/%.cpp.depends
	-@mkdir -p $(dir $@)
	@$(ccache) $(cxx-compiler) $(cxx-compile-flags) $(st20-main-lib-cxx-include-flags) -c $< -o $@ && echo ok: object: $@

$(st20-main-exe) : $(st20-main-exe-objects) $(st20-main-lib-a)
	-@mkdir -p $(dir $@)
	@$(cxx-compiler) \
		$(cxx-link-flags) \
		$(libgcc-link-flags) \
		$(st20-main-exe-objects) \
		$(st20-main-lib-a) \
		$(libnasm-a-flags) \
		$(libspdlog-a-flags) \
		-o $(st20-main-exe) \
    && echo ok: binary: $@

$(st20-main-exe-depends) :
include $(wildcard $(st20-main-exe-depends))

$(st20-main-lib-depends) :
include $(wildcard $(st20-main-lib-depends))


# st20-test.exe ----------------------------------------------------------

$(st20-test-exe-objects) $(st20-test-lib-objects) : $(object-root)/%.cpp.o : %.cpp $(object-root)/%.cpp.depends
	-@mkdir -p $(dir $@)
	@$(ccache) $(cxx-compiler) $(cxx-compile-flags) $(st20-test-lib-cxx-include-flags) -c $< -o $@ && echo ok: object: $@

$(st20-test-exe) : $(st20-test-exe-objects) $(st20-main-lib-a) $(st20-test-lib-a)
	-@mkdir -p $(dir $@)
	@$(cxx-compiler) \
		$(cxx-link-flags) \
		$(libgcc-link-flags) \
		$(st20-test-exe-objects) \
		$(st20-test-lib-a) \
		$(st20-main-lib-a) \
		$(libgtest-a-flags) \
		$(libspdlog-a-flags) \
		-o $(st20-test-exe) \
	&& echo ok: binary: $@

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


ext : fmt googletest spdlog nasm


# fmt -------------------------------------------------------------------------

$(fmt-root-dir) :
	git clone $(fmt-git-url) $(fmt-root-dir)

fmt : $(fmt-root-dir)
	$(cmake-program) \
        -G $(cmake-generator-name) \
        -D CMAKE_C_COMPILER=$(cc-compiler) \
        -D CMAKE_C_COMPILER_LAUNCHER=$(ccache) \
        -D CMAKE_CXX_COMPILER=$(cxx-compiler) \
        -D CMAKE_CXX_COMPILER_LAUNCHER=$(ccache) \
        -D CMAKE_SYSTEM_NAME=$(cmake-system-name) \
        -S $(fmt-root-dir) \
        -B $(fmt-object-dir)
	$(ninja-program) -C $(fmt-object-dir)


# spdlog ----------------------------------------------------------------------

$(spdlog-root-dir) :
	git clone $(spdlog-git-url) $(spdlog-root-dir)

spdlog : $(spdlog-root-dir)
	$(cmake-program) \
        -G $(cmake-generator-name) \
        -D CMAKE_C_COMPILER=$(cc-compiler) \
        -D CMAKE_C_COMPILER_LAUNCHER=$(ccache) \
        -D CMAKE_CXX_COMPILER=$(cxx-compiler) \
        -D CMAKE_CXX_COMPILER_LAUNCHER=$(ccache) \
        -D CMAKE_SYSTEM_NAME=$(cmake-system-name) \
        -S $(spdlog-root-dir) \
        -B $(spdlog-object-dir)
	$(ninja-program) -C $(spdlog-object-dir)


# googletest ------------------------------------------------------------------

$(googletest-root-dir) :
	git clone $(googletest-git-url) $(googletest-root-dir)

googletest : $(googletest-root-dir)
	$(cmake-program) \
        -G $(cmake-generator-name) \
        -D CMAKE_C_COMPILER=$(cc-compiler) \
        -D CMAKE_C_COMPILER_LAUNCHER=$(ccache) \
        -D CMAKE_CXX_COMPILER=$(cxx-compiler) \
        -D CMAKE_CXX_COMPILER_LAUNCHER=$(ccache) \
        -D CMAKE_SYSTEM_NAME=$(cmake-system-name) \
        -S $(googletest-root-dir) \
        -B $(googletest-object-dir)
	$(ninja-program) -C $(googletest-object-dir)


# nasm ------------------------------------------------------------------------

$(nasm-root-dir) :
	git clone $(nasm-git-url) $(nasm-root-dir)

nasm : $(nasm-root-dir)
	cd $(nasm-root-dir) && ./autogen.sh
	cd $(nasm-root-dir) && ./configure --host $(machine)-$(platform) --prefix $(nasm-object-dir)
	cd $(nasm-root-dir) && make libnasm.a
	objcopy --remove-leading-char $(nasm-root-dir)/libnasm.a $(nasm-root-dir)/libnasm.a
	install --verbose $(nasm-root-dir)/config/* 	-D --target-directory $(nasm-object-dir)/include/config
	install --verbose $(nasm-root-dir)/disasm/* 	-D --target-directory $(nasm-object-dir)/include
	install --verbose $(nasm-root-dir)/include/* 	-D --target-directory $(nasm-object-dir)/include
	install --verbose $(nasm-root-dir)/x86/* 		-D --target-directory $(nasm-object-dir)/include
	install --verbose $(nasm-root-dir)/libnasm.a 	-D --target-directory $(nasm-object-dir)/lib

