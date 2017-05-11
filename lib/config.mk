LIBK_OBJS := $(patsubst %.c,%.o,$(wildcard $(LIBDIR)/*/*.c))
LIBK_OBJS += $(patsubst %.c,%.o,$(wildcard $(LIBDIR)/*/*/*.c))
