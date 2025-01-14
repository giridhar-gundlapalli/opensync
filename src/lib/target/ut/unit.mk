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

##############################################################################
#
# TARGET specific layer library
#
##############################################################################
UNIT_NAME := test_target_api

# Template type:
UNIT_TYPE := TEST_BIN

# List of source files
UNIT_SRC := unit_test_main.c
UNIT_SRC += target_unit_test.c

UNIT_CFLAGS +=-Isrc/nm2/inc
UNIT_CFLAGS +=-Isrc/cm2/src
UNIT_CTAGS  +=-Isrc/lib/target/inc/
UNIT_CFLAGS +=-Isrc/sm/src/
UNIT_CFLAGS +=-Ivendor/plume/src/blem/src

# Other units that this unit may depend on
UNIT_DEPS := src/lib/target
UNIT_DEPS += src/lib/log
UNIT_DEPS += src/lib/ds
UNIT_DEPS += src/lib/datapipeline/
UNIT_DEPS += src/lib/ovsdb
UNIT_DEPS += src/lib/synclist
UNIT_DEPS += src/lib/reflink
UNIT_DEPS += src/lib/osn
UNIT_DEPS += src/lib/inet
UNIT_DEPS += src/lib/unity
UNIT_DEPS += src/lib/unit_test_utils
