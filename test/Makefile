# Copyright (c) Konstantin Belyalov. All rights reserved.
# Licensed under the MIT license.
#
# This makefile is intended to be run only for CI purposes, do not use it to compile firmware!

BUILD_DIR_ARM = build_arm
BUILD_DIR_CROSS = build_cross

SOURCE_DIR := ..
TEST_DIR := .
PROTO_DIR := proto
NANOPB_DIR := nanopb

SOURCES = \
	$(SOURCE_DIR)/lora_sx1276.c \
	$(SOURCE_DIR)/debug.c \
	$(SOURCE_DIR)/static_alloc.c \
	$(SOURCE_DIR)/si7021.c \
	$(SOURCE_DIR)/ring_buffer.c \
	$(SOURCE_DIR)/ring_buffer_nanopb.c \
	$(SOURCE_DIR)/veml6030.c

HEADERS = \
	$(SOURCE_DIR)/debug.h \
	$(SOURCE_DIR)/lora_sx1276.h \
	$(SOURCE_DIR)/ring_buffer_fixed_size.h \
	$(SOURCE_DIR)/ring_buffer_nanopb.h \
	$(SOURCE_DIR)/si7021.h \
	$(SOURCE_DIR)/htons.h

TESTS = \
	$(TEST_DIR)/test_mocks.cpp \
	$(TEST_DIR)/test_debug.cpp \
	$(TEST_DIR)/test_si7021.cpp \
	$(TEST_DIR)/test_static_alloc.cpp \
	$(TEST_DIR)/test_ring.cpp \
	$(TEST_DIR)/test_ring_fixed_size.cpp \
	$(TEST_DIR)/test_ring_nanopb.cpp \
	$(TEST_DIR)/test_utils.cpp

PROTO = \
	$(PROTO_DIR)/sample.pb.c

NANOPB = \
	$(NANOPB_DIR)/pb_encode.c \
	$(NANOPB_DIR)/pb_decode.c \
	$(NANOPB_DIR)/pb_common.c


ARM_CC = arm-none-eabi-gcc
CROSS_CC = gcc
CROSS_CXX = g++
GTEST_LIBS = /usr/local/lib/libgtest_main.a /usr/local/lib/libgtest.a -lpthread

INCLUDES = -I../ -I. -Inanopb

ARM_CFLAGS = -mthumb -Wall -Werror $(INCLUDES)
CROSS_CFLAGS = -Wall $(INCLUDES) -I/usr/local/include -I/usr/include -Wno-missing-braces

OBJECTS_ARM := $(BUILD_DIR_ARM)/main_arm.o
OBJECTS_ARM += $(addprefix $(BUILD_DIR_ARM)/,$(notdir $(SOURCES:.c=.o)))

OBJECTS_CROSS = $(addprefix $(BUILD_DIR_CROSS)/,$(notdir $(SOURCES:.c=.o)))
OBJECTS_CROSS += $(addprefix $(BUILD_DIR_CROSS)/,$(notdir $(TESTS:.cpp=.o)))
OBJECTS_CROSS += $(addprefix $(BUILD_DIR_CROSS)/nanopb_,$(notdir $(NANOPB:.c=.o)))
OBJECTS_CROSS += $(addprefix $(BUILD_DIR_CROSS)/proto_,$(notdir $(PROTO:.c=.o)))

ARM_BINARY := $(BUILD_DIR_ARM)/utils.elf
TESTS_BINARY := $(BUILD_DIR_CROSS)/tests

all: $(ARM_BINARY) $(TESTS_BINARY) Makefile

dirs:
	mkdir -p $(BUILD_DIR_ARM) $(BUILD_DIR_CROSS)

# ARM native target
$(BUILD_DIR_ARM)/%.o: $(SOURCE_DIR)/%.c
	$(ARM_CC) $(ARM_CFLAGS) -c $^ -o $@

$(BUILD_DIR_ARM)/%.o: %.c
	$(ARM_CC) $(ARM_CFLAGS) -c $^ -o $@

$(ARM_BINARY): $(OBJECTS_ARM) dirs
	$(ARM_CC) --specs=nosys.specs $(OBJECTS_ARM) -o $@

# Cross compiled / tests
$(BUILD_DIR_CROSS)/%.o: $(TEST_DIR)/%.cpp
	$(CROSS_CXX) -std=c++11 $(CROSS_CFLAGS) -c $^ -o $@

$(BUILD_DIR_CROSS)/proto_%.o: $(PROTO_DIR)/%.c
	$(CROSS_CC) $(CROSS_CFLAGS) -c $^ -o $@

$(BUILD_DIR_CROSS)/nanopb_%.o: $(NANOPB_DIR)/%.c
	$(CROSS_CC) $(CROSS_CFLAGS) -c $^ -o $@

$(BUILD_DIR_CROSS)/%.o: $(SOURCE_DIR)/%.c
	$(CROSS_CC) $(CROSS_CFLAGS) -c $^ -o $@

$(TESTS_BINARY): $(OBJECTS_CROSS) $(HEADERS) Makefile dirs
	$(CROSS_CXX) $(GTEST_LIBS) $(OBJECTS_CROSS) -o $@

test: $(TESTS_BINARY)
	@$(BUILD_DIR_CROSS)/tests

tests: $(TESTS_BINARY)
	@$(BUILD_DIR_CROSS)/tests --gtest_filter=ring_buffer.*

clean:
	rm -f $(OBJECTS_ARM) $(OBJECTS_CROSS) $(ARM_BINARY) $(TESTS_BINARY)
