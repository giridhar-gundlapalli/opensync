# Copyright (c) 2015, Plume Design Inc. All rights reserved.
# 
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#    1. Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#    2. Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in the
#       documentation and/or other materials provided with the distribution.
#    3. Neither the name of the Plume Design Inc. nor the
#       names of its contributors may be used to endorse or promote products
#       derived from this software without specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL Plume Design Inc. BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#
# native configuration
#

BUILD_SHARED_LIB = n

SDK_ROOTFS     = $(OBJDIR)/rootfs

CC             ?= gcc
CXX            ?= g++
AR             ?= ar
STRIP          ?= strip -g

# Includes
CFLAGS += -I/usr/include/protobuf-c
CFLAGS += -I/$(OVS_SOURCE)
CFLAGS += -I/$(OVS_SOURCE)/include

# Flags
CFLAGS += -fno-strict-aliasing
CFLAGS += -fasynchronous-unwind-tables
CFLAGS += -Wno-error=deprecated-declarations
CFLAGS += -Wno-error=cpp
CFLAGS += -fPIC

# GCC specific flags
ifneq (,$(findstring gcc,$(CC)))
	CFLAGS += -O3 -pipe
	CFLAGS += -Wno-error=unused-but-set-variable
	CFLAGS += -fno-caller-saves
endif

# clang specific flags. Enable address sanitizer and coverage
# instrumentation if requested
ifneq (,$(findstring clang,$(CC)))
	CFLAGS += -O0 -pipe
	CFLAGS += -fno-omit-frame-pointer
	CFLAGS += -fno-optimize-sibling-calls
	CFLAGS += -fsanitize=address
	LDFLAGS += -fsanitize=address
	CLANG_VERSION = $(subst clang,,${CC})
	ASAN_SYMBOLIZER_PATH = /usr/lib/llvm${CLANG_VERSION}/bin/llvm-symbolizer
	ASAN_OPTIONS = symbolized=1
ifneq (,$(findstring yes,$(COVERAGE)))
	CFLAGS += -fprofile-instr-generate -fcoverage-mapping
	LDFLAGS += -fprofile-instr-generate -fcoverage-mapping
endif
endif

# Defines
CFLAGS += -D_U_="__attribute__((unused))"
CFLAGS += -DARCH_X86

LDFLAGS += -lssl -lcrypto -lpcap

export CC
export CXX
export CFLAGS
export LDFLAGS
export LIBS

# enable use of netlink lib for native build
LIBNL3_HEADERS = -I/usr/include/libnl3
export LIBNL3_HEADERS
