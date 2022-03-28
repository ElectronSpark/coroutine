CURDIR = $(shell pwd)
ARCH := x86_64

CC := gcc
AS := as
AR := ar
LD := ld

ARCH_DIR = $(CURDIR)/cpu/$(ARCH)
ARCH_SRC = $(ARCH_DIR)/src
ARCH_INC = $(ARCH_DIR)/include

INC_DIR = $(CURDIR) $(CURDIR)/include $(CURDIR)/external/include $(ARCH_INC)
SRC_DIR = $(CURDIR)/src $(CURDIR)/external/src $(ARCH_DIR)/src

CFLAGS = $(foreach each,$(INC_DIR),-I$(each)) -g -O0
ASFLAGS = $(CFLAGS)

EXP_DIR = $(CURDIR)/example
EXP_SRC =	\
	$(EXP_DIR)/eventloop.c	\
	$(EXP_DIR)/channel.c	\
	$(EXP_DIR)/producer_customer.c	\
	$(EXP_DIR)/event.c
EXP_BIN = $(foreach each,$(EXP_SRC),$(subst .c,.elf,$(each)))

C_FILES =	\
	$(CURDIR)/src/coroutine.c	\
	$(CURDIR)/src/cr_channel.c	\
	$(CURDIR)/src/cr_pool.c	\
	$(CURDIR)/src/cr_task.c	\
	$(CURDIR)/src/cr_waitable.c	\
	$(CURDIR)/src/cr_semaphore.c	\
	$(CURDIR)/src/cr_event.c	\
	$(CURDIR)/external/src/rbtree.c
ASM_FILES =	\
	$(ARCH_DIR)/src/cpu.S

C_OBJS = $(foreach each,$(C_FILES),$(subst .c,.o,$(each)))
ASM_OBJS = $(foreach each,$(ASM_FILES),$(subst .S,.o,$(each)))
OBJS = $(ASM_OBJS) $(C_OBJS)
TARGET = libcoroutine.a

all: build example
	@echo all -- done

example: $(EXP_BIN)
	@echo examples -- done

$(EXP_BIN):%.elf:%.c build
	$(CC) $(CFLAGS) $< -o $@ $(TARGET)

build: $(TARGET)
	@echo build -- done

$(TARGET): $(OBJS)
	$(AR) cqs $@ $^

$(C_OBJS):%.o:%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(ASM_OBJS):%.o:%.S
	$(CC) $(ASFLAGS) -c $< -o $@

clean:
	rm $(OBJS) $(TARGET)
	rm $(EXP_BIN)

.PHONY = all library example clean
