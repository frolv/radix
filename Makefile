PROJECT_NAME = radix
KERNEL_NAME = k$(PROJECT_NAME)

DEFAULT_HOST := $(shell util/default-host)
BUILD_HOST ?= $(DEFAULT_HOST)
HOSTARCH := $(shell util/target-to-arch $(BUILD_HOST))

define \n


endef

$(info Building $(PROJECT_NAME) for target $(HOSTARCH)$(\n))

AR := $(BUILD_HOST)-ar
AS := $(BUILD_HOST)-as
CC := $(BUILD_HOST)-gcc

RM := rm -f
CTAGS := ctags

ASFLAGS ?=
CFLAGS ?= -O2
LDFLAGS ?=
LIBS ?=

CFLAGS := $(CFLAGS) -ffreestanding -Wall -Wextra -D__KERNEL__
ASFLAGS := $(ASFLAGS) -D__ASSEMBLY__
LIBS := $(LIBS) -nostdlib -lgcc

#
# Source directories.
#
KERNELDIR := kernel
ARCHDIR := arch/$(HOSTARCH)
DRIVERDIR := drivers
LIBDIR := lib
INCLUDEDIRS := include $(ARCHDIR)/include
CONFDIR := config

CONFIG_FILE := $(CONFDIR)/config
CONFIG_PARTIALS := $(wildcard $(CONFDIR)/.rconfig.*)
CONFIG_H := $(CONFDIR)/genconfig.h
CONF ?= $(CONFDIR)/default.$(HOSTARCH)
RCONFIG ?= util/rconfig/rconfig
CONFGEN := util/confgen

include $(ARCHDIR)/config.mk
include $(LIBDIR)/config.mk
include $(DRIVERDIR)/config.mk

CFLAGS := $(CFLAGS) $(KERNEL_ARCH_CFLAGS)
LDFLAGS := $(LDFLAGS) $(KERNEL_ARCH_LDFLAGS)
LIBS := $(LIBS) $(KERNEL_ARCH_LIBS)

KERNEL_OBJS := $(patsubst %.c,%.o,$(wildcard $(KERNELDIR)/*.c))
KERNEL_OBJS += $(patsubst %.c,%.o,$(wildcard $(KERNELDIR)/*/*.c))
KERNEL_OBJS += $(KERNEL_ARCH_OBJS)

_GLOBAL_INCLUDES := $(CONFIG_H)
_GLOBAL_INCLUDES := $(shell realpath $(_GLOBAL_INCLUDES))
GLOBAL_INCLUDES := $(patsubst %,-include %,$(_GLOBAL_INCLUDES))

_INCLUDE := $(shell realpath $(INCLUDEDIRS))
INCLUDE := $(patsubst %,-I%,$(_INCLUDE)) $(GLOBAL_INCLUDES)

.SUFFIXES: .o .c .S

all: kernel

kernel: $(KERNEL_NAME)

$(CONFIG_H): $(CONFIG_FILE)

$(CONFIG_FILE):
	@test -f $@ || (                                                  \
	echo >&2 "ERROR: no kernel configuration file found";             \
	echo >&2;                                                         \
	echo >&2 "Run";                                                   \
	echo >&2 "  \`make iconfig'";                                     \
	echo >&2 "     for an interactive setup";                         \
	echo >&2 "  \`make config'";                                      \
	echo >&2 "     to use the default configuration";                 \
	echo >&2 "  \`make config CONF=\$$CONFIGFILE'";                   \
	echo >&2 "     to use the configuration from file \$$CONFIGFILE"; \
	echo >&2;                                                         \
	echo >&2 "Aborting";                                              \
	false)

.PHONY: config
config:
	@test -f $(CONF) && (                                             \
	echo "Using configuration file $(CONF)";                          \
	cp $(CONF) $(CONFIG_FILE);                                        \
	$(CONFGEN) <$(CONFIG_FILE) >$(CONFIG_H)) || (                     \
	echo >&2 "ERROR: $(CONF): no such file";                          \
	echo >&2;                                                         \
	false)

.PHONY: iconfig
iconfig: rconfig
	@echo
	$(RCONFIG) --arch=$(HOSTARCH) -o $(CONFIG_FILE)
	$(CONFGEN) <$(CONFIG_FILE) >$(CONFIG_H)

.PHONY: rconfig
rconfig:
	@cd util/rconfig && make

.PHONY: rconfig-gen-default
rconfig-gen-default: rconfig
	$(RCONFIG) --arch=$(HOSTARCH) --default -o $(CONF)

.PHONY: rconfig-lint
rconfig-lint: rconfig
	$(RCONFIG) --arch=$(HOSTARCH) --lint

$(KERNEL_NAME): $(CONFIG_H) $(LIBK_OBJS) $(DRIVER_OBJS) $(KERNEL_OBJS) \
	$(ARCHDIR)/linker.ld
	$(CC) -T $(ARCHDIR)/linker.ld -o $@ $(CFLAGS) $(KERNEL_OBJS) \
		$(LIBK_OBJS) $(DRIVER_OBJS) $(LDFLAGS) $(LIBS)

$(ARCHDIR)/crtbegin.o $(ARCHDIR)/crtend.o:
	OBJ=`$(CC) -print-file-name=$(@F)` && cp "$$OBJ" $@

.PHONY: libk
libk: $(LIBK_OBJS)

# temp
.PHONY: drivers
drivers: $(DRIVER_OBJS)

.c.o:
	$(CC) -c $< -o $@ $(CFLAGS) $(INCLUDE)

.S.o:
	$(CC) -c $< -o $@ $(ASFLAGS) $(INCLUDE)

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
clean: clean-all-kernel

.PHONY: clean-all
clean-all: clean-all-kernel clean-all-config

.PHONY: clean-all-kernel
clean-all-kernel: clean-kernel clean-libk clean-drivers clean-iso

.PHONY: clean-config
clean-config: clean-all-config

.PHONY: clean-all-config
clean-all-config: clean-configfiles clean-rconfig

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

.PHONY: clean-configfiles
clean-configfiles:
	$(RM) $(CONFIG_FILE) $(CONFIG_PARTIALS) $(CONFIG_H)

.PHONY: clean-rconfig
clean-rconfig:
	@cd util/rconfig && make clean
