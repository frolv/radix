PROJECT_NAME = radix
KERNEL_NAME = k$(PROJECT_NAME)

DEFAULT_HOST := $(shell util/default-host)
HOST ?= $(DEFAULT_HOST)
HOSTARCH := $(shell util/target-to-arch $(HOST))

AR = $(HOST)-ar
AS = $(HOST)-as
CC = $(HOST)-gcc

RM := rm -f

ASFLAGS ?=
CFLAGS ?= -O2
LDFLAGS ?=
LIBS ?=

CFLAGS := $(CFLAGS) -ffreestanding -Wall -Wextra
LDFLAGS := $(LDFLAGS)
LIBS := $(LIBS) -nostdlib -lgcc

ARCHDIR := arch/$(HOSTARCH)

include $(ARCHDIR)/config.mk

CFLAGS := $(CFLAGS) $(KERNEL_ARCH_CFLAGS)
LDFLAGS := $(LDFLAGS) $(KERNEL_ARCH_LDFLAGS)
LIBS := $(LIBS) $(KERNEL_ARCH_LIBS)

KERNEL_OBJS := $(patsubst %.c,%.o,$(wildcard kernel/*.c))
KERNEL_OBJS += $(patsubst %.c,%.o,$(wildcard kernel/*/*.c))
KERNEL_OBJS += $(KERNEL_ARCH_OBJS)

LIBDIR := lib

LIBK_OBJS :=

include $(LIBDIR)/config.mk

# until module support is added
DRIVERDIR = drivers
include $(DRIVERDIR)/config.mk

INCLUDE := -I./include -I./$(ARCHDIR)/include

.SUFFIXES: .o .c .S

all: libk drivers $(KERNEL_NAME)

.PHONY: $(KERNEL_NAME)
$(KERNEL_NAME): $(KERNEL_OBJS) $(ARCHDIR)/linker.ld
	$(CC) -T $(ARCHDIR)/linker.ld -o $@ $(CFLAGS) $(KERNEL_OBJS) \
		$(LIBK_OBJS) $(DRIVER_OBJS) $(LDFLAGS) $(LIBS)

$(ARCHDIR)/crtbegin.o $(ARCHDIR)/crtend.o:
	OBJ=`$(CC) $(CFLAGS) $(LDFLAGS) -print-file-name=$(@F)` && cp "$$OBJ" $@

.PHONY: libk
libk: $(LIBK_OBJS)

# temp
.PHONY: drivers
drivers: $(DRIVER_OBJS)

.c.o:
	$(CC) -c $< -o $@ $(CFLAGS) $(INCLUDE)

.S.o:
	$(AS) $< -o $@ $(ASFLAGS)

ISODIR := iso
ISONAME := $(PROJECT_NAME)-$(HOSTARCH).iso

.PHONY: iso
iso: $(KERNEL_NAME)
	mkdir -p $(ISODIR)/boot/grub
	cp $(KERNEL_NAME) $(ISODIR)/boot
	util/mkgrubconfig > $(ISODIR)/boot/grub/grub.cfg
	grub-mkrescue -o $(ISONAME) $(ISODIR)

.PHONY: clean
clean: clean-all

.PHONY: clean-all
clean-all: clean-kernel clean-libk clean-drivers clean-iso

.PHONY: clean-kernel
clean-kernel:
	$(RM) $(KERNEL_OBJS) $(KERNEL_NAME)

.PHONY: clean-libk
clean-libk:
	$(RM) $(LIBK_OBJS)

# temp
.PHONY: clean-drivers
clean-drivers:
	$(RM) $(DRIVER_OBJS)

.PHONY: clean-iso
clean-iso:
	$(RM) -r $(ISODIR) $(ISONAME)
