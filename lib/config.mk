LIBK_OBJS := $(patsubst %.c,%.o,$(wildcard lib/*.c))
LIBK_OBJS += $(patsubst %.c,%.o,$(wildcard lib/*/*.c))
