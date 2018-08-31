CROSS_COMPILE = arm-hisiv510-linux-
#CROSS_COMPILE = 
GCC = $(CROSS_COMPILE)gcc
CC  = $(CROSS_COMPILE)gcc
STRIP = $(CROSS_COMPILE)strip
AR = $(CROSS_COMPILE)ar
LD = $(CROSS_COMPILE)ld
NM = $(CROSS_COMPILE)nm
#CFLAGS = -g3 -Wall -Werror
CFLAGS = -g3 -Wall -O0

#SRCS_TMP := $(wildcard *.c)
#SRCS := $(foreach n,$(SRCS_TMP),$(n))
SRCS := $(wildcard *.c)
INCS :=
LIBS :=

TARGET = search_dvr
#OBJECTS = cross_sock.o main.o search_dvr.o slog.o
#OBJ_TMP := $(patsubst %.c,%.o,$(SRCS_TMP))
#OBJ_TMP := $(patsubst %.c,%.o,$(SRCS))
#OBJECTS := $(foreach n,$(OBJ_TMP),$(n))
OBJECTS := $(patsubst %.c,%.o,$(SRCS))
#OBJECTS := $(SRCS:%.c=%.o)

.PHONY:all
all:$(OBJECTS)
	$(GCC) $(CFLAGS) -o $(TARGET) $(OBJECTS) 


strip:
	$(STRIP) $(TARGET) 
.PHONY: clean
clean:
	rm -rf $(OBJECTS) $(TARGET)
