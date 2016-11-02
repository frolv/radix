# temporary file

DRIVER_OBJS := $(patsubst %.c,%.o,$(wildcard $(DRIVERDIR)/*/*/*.c))
