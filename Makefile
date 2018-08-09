CROSS_COMPILE = arm-hisiv510-linux-
GCC = $(CROSS_COMPILE)gcc
CC  = $(CROSS_COMPILE)cc
STRIP = $(CROSS_COMPILE)strip
AR = $(CROSS_COMPILE)ar
LD = $(CROSS_COMPILE)ld
NM = $(CROSS_COMPILE)nm
CFLAGS = -g3 -Wall -Werror

SRCS :=
INCS :=
LIBS :=

TARGET = search_dvr
OBJECTS = cross_sock.o main.o search_dvr.o slog.o
#OBJECTS := $(wildcard *.o)

.PHONY:all
all:$(OBJECTS)
	$(GCC) $(CFLAGS) -o $(TARGET) $(OBJECTS) 

cross_sock.o:cross_sock.c
	$(GCC) -c cross_sock.c
main.o:main.c
	$(GCC) -c main.c
search_dvr.o:search_dvr.c search_dvr.h
	$(GCC) -c search_dvr.c
slog.o:slog.c
	$(GCC) -c slog.c

a.out:cross_sock.o main.o search_dvr.o slog.o
	$(GCC) -o a.out $(OBJECT) 

all_objects:$(OBJECTS)
$(OBJECTS):%.o:%.c
	$(GCC) -c $< -o  $@

strip:
	$(STRIP) $(TARGET) 
.PHONY: clean
clean:
	rm -rf $(OBJECTS)
