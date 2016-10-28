PROJECT_NAME = untitled

DEFAULT_HOST != util/default-host
HOST ?= $(DEFAULT_HOST)
HOSTARCH != util/target-to-arch $(HOST)

AR =  $(HOST)-ar
AS =  $(HOST)-as
CC =  $(HOST)-gcc

RM := rm -f

ASFLAGS ?=
CFLAGS ?= -O2 -g
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
KERNEL_OBJS += $(KERNEL_ARCH_OBJS)

LIBDIR := lib

LIBK_OBJS :=

include $(LIBDIR)/config.mk

INCLUDE := -I./include -I./$(ARCHDIR)/include

.SUFFIXES: .o .c .S

all: libk $(PROJECT_NAME)-kernel

.PHONY: $(PROJECT_NAME)-kernel
$(PROJECT_NAME)-kernel: $(KERNEL_OBJS) $(ARCHDIR)/linker.ld
	$(CC) -T $(ARCHDIR)/linker.ld -o $(PROJECT_NAME) $(CFLAGS) \
		$(KERNEL_OBJS) $(LIBK_OBJS) $(LDFLAGS) $(LIBS)

$(ARCHDIR)/crtbegin.o $(ARCHDIR)/crtend.o:
	OBJ=`$(CC) $(CFLAGS) $(LDFLAGS) -print-file-name=$(@F)` && cp "$$OBJ" $@

.PHONY: libk
libk: $(LIBK_OBJS)

.c.o:
	$(CC) -c $< -o $@ $(CFLAGS) $(INCLUDE)

.S.o:
	$(AS) $< -o $@ $(ASFLAGS)

ISODIR := isodir
ISONAME := $(PROJECT_NAME).iso

.PHONY: iso
iso: $(PROJECT_NAME)
	mkdir -p $(ISODIR)/boot/grub
	cp $(PROJECT_NAME) $(ISODIR)/boot
	util/mkgrubconfig > $(ISODIR)/boot/grub/grub.cfg
	grub-mkrescue -o $(ISONAME) $(ISODIR)

.PHONY: clean-all
clean-all: clean-kernel clean-libk clean-iso

.PHONY: clean-kernel
clean-kernel:
	$(RM) $(KERNEL_OBJS) $(PROJECT_NAME)

.PHONY: clean-libk
clean-libk:
	$(RM) $(LIBK_OBJS)

.PHONY: clean-iso
clean-iso:
	$(RM) -r $(ISODIR) $(ISONAME)
