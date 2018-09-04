
ECHO   := @echo
LINK   := ln -s
CP     := cp -av
MKDIR  := @mkdir -p
MV     := mv -v
MAKE   := make
PRINTF := @printf "\033[32;1m\t%s\033[0m\n"
RM     := /bin/rm

CROSS_COMPILE = arm-hisiv510-linux-
#CROSS_COMPILE =

GCC    := $(CROSS_COMPILE)gcc
CC     := $(CROSS_COMPILE)cc
CXX    := $(CROSS_COMPILE)g++
STRIP  := $(CROSS_COMPILE)strip
AR     := $(CROSS_COMPILE)ar
LD     := $(CROSS_COMPILE)ld
NM     := $(CROSS_COMPILE)nm
#CFLAGS := -g3 -Wall -Werror -O0
CFLAGS := -g3 -Wall -O0
LDFLAGS := 

TARGET     = search_dvr
LIB_TARGET = libnet_learn.a

#SRCS_TMP := $(wildcard *.c)
#SRCS := $(foreach n,$(SRCS_TMP),$(n))
SRCS := $(wildcard *.c)
LIB_SRCS := $(filter-out main.c, $(SRCS))
INCS := ./
LIB_INCS := $(wildcard *.h)
LIBS := net_learn
LIB_DIRS := ./

#OBJECTS = cross_sock.o main.o search_dvr.o slog.o
#OBJ_TMP := $(patsubst %.c,%.o,$(SRCS_TMP))
#OBJ_TMP := $(patsubst %.c,%.o,$(SRCS))
#OBJECTS := $(foreach n,$(OBJ_TMP),$(n))
OBJECTS := $(patsubst %.c,%.o,$(SRCS))
#OBJECTS := $(SRCS:%.c=%.o)

LIB_OBJECTS := $(LIB_SRCS:%.c=%.o)

.PHONY:all app_lib objects lib_objects lib strip
all:$(OBJECTS)
	$(GCC) $(CFLAGS) -o $(TARGET) $(OBJECTS) 

app_lib:lib
	$(GCC) $(CFLAGS) -o $(TARGET) main.c -I$(INCS) -L$(LIB_DIRS) -l$(LIBS)
	
objects:$(OBJECTS)

lib_objects:$(LIB_OBJECTS)

lib:lib_objects
	$(AR) -r $(LIB_TARGET) $(LIB_OBJECTS)

$(OBJECTS):%.o:%.c
	$(GCC) $(CFLAGS) -c $< -o $@
	
#$(LIB_OBJECTS):%.o:%.c
#	$(GCC) $(CFLAGS) -c $< -o $@	

strip:
	$(STRIP) $(TARGET) 

.PHONY: clean
clean:
	rm -rf $(OBJECTS) $(TARGET) $(LIB_TARGET)
