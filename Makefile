PROJECT_NAME = radix
KERNEL_NAME = k$(PROJECT_NAME)

DEFAULT_HOST := $(shell util/default-host)
BUILD_HOST ?= $(DEFAULT_HOST)
HOSTARCH := $(shell util/target-to-arch $(BUILD_HOST))

AR := $(BUILD_HOST)-ar
AS := $(BUILD_HOST)-as
CC := $(BUILD_HOST)-gcc

RM := rm -f
CTAGS := ctags

ASFLAGS ?=
CFLAGS ?= -O2
LDFLAGS ?=
LIBS ?=

CFLAGS := $(CFLAGS) -ffreestanding -Wall -Wextra
LDFLAGS := $(LDFLAGS)
LIBS := $(LIBS) -nostdlib -lgcc

#
# Source directories.
#
KERNELDIR := kernel
ARCHDIR := arch/$(HOSTARCH)
DRIVERDIR := drivers
LIBDIR := lib
INCLUDEDIRS := include $(ARCHDIR)/include

include $(ARCHDIR)/config.mk
include $(LIBDIR)/config.mk
include $(DRIVERDIR)/config.mk

CFLAGS := $(CFLAGS) $(KERNEL_ARCH_CFLAGS)
LDFLAGS := $(LDFLAGS) $(KERNEL_ARCH_LDFLAGS)
LIBS := $(LIBS) $(KERNEL_ARCH_LIBS)

KERNEL_OBJS := $(patsubst %.c,%.o,$(wildcard $(KERNELDIR)/*.c))
KERNEL_OBJS += $(patsubst %.c,%.o,$(wildcard $(KERNELDIR)/*/*.c))
KERNEL_OBJS += $(KERNEL_ARCH_OBJS)

_INCLUDE := $(shell realpath $(INCLUDEDIRS))
INCLUDE := $(patsubst %,-I%,$(_INCLUDE))

.SUFFIXES: .o .c .S

all: kernel

kernel: libk drivers $(KERNEL_NAME)

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

#
# Build kernel ISO image using GRUB.
#
.PHONY: iso
iso: kernel
	mkdir -p $(ISODIR)/boot/grub
	cp $(KERNEL_NAME) $(ISODIR)/boot
	util/mkgrubconfig $(PROJECT_NAME) $(KERNEL_NAME) \
		> $(ISODIR)/boot/grub/grub.cfg
	grub-mkrescue -o $(ISONAME) $(ISODIR)

.PHONY: ctags
ctags:
	$(CTAGS) -R $(KERNELDIR) $(ARCHDIR) $(LIBDIR) $(DRIVERDIR) \
		$(INCLUDEDIRS)

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
