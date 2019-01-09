# This makefile is intended to be run only for CI purposes, do not use it to compile firmware!

BUILD_DIR_ARM = build_arm
BUILD_DIR_CROSS = build_cross

ARM_CC = arm-none-eabi-gcc
CC = gcc

INCLUDES = -I./src -I./mock
ARM_C_DEFS = -mthumb

CFLAGS = -Wall -Werror $(INCLUDES)

all: $(BUILD_DIR_ARM)/lora_sx1276.o $(BUILD_DIR_CROSS)/lora_sx1276.o Makefile

$(BUILD_DIR_ARM)/lora_sx1276.o: src/lora_sx1276.c
	$(ARM_CC) $(ARM_C_DEFS) -c $(CFLAGS) $< -o $@

$(BUILD_DIR_CROSS)/lora_sx1276.o: src/lora_sx1276.c
	$(CC) -c $(CFLAGS) $< -o $@
