KERNEL_ARCH_CFLAGS =
KERNEL_ARCH_LDFLAGS =
KERNEL_ARCH_LIBS =

KERNEL_ARCH_OBJS := $(patsubst %.c,%.o,$(wildcard $(ARCHDIR)/*.c))
KERNEL_ARCH_OBJS += $(patsubst %.S,%.o,$(wildcard $(ARCHDIR)/*.S))
KERNEL_ARCH_OBJS += $(patsubst %.c,%.o,$(wildcard $(ARCHDIR)/*/*.c))
KERNEL_ARCH_OBJS += $(patsubst %.S,%.o,$(wildcard $(ARCHDIR)/*/*.S))
KERNEL_ARCH_OBJS += $(ARCHDIR)/crtbegin.o $(ARCHDIR)/crtend.o
