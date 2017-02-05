# Add object files for kernel drivers to main build process.

DRIVER_OBJS := $(patsubst %.c,%.o,$(wildcard $(DRIVERDIR)/*/*.c))
DRIVER_OBJS += $(patsubst %.c,%.o,$(wildcard $(DRIVERDIR)/*/*/*.c))
