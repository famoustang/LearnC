CROSS_COMPILE = arm-hisiv500-linux-
GCC = $(CROSS_COMPILE)gcc
CC  = $(CROSS_COMPILE)cc
STRIPC = $(CROSS_COMPILE)strip
AR = $(CROSS_COMPILE)ar
LD = $(CROSS_COMPILE)ld
NM = $(CROSS_COMPILE)nm

OBJECT = cross_sock.o main.o search_dvr.o slog.o

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
all:$(OBJECT)
	$(GCC) -o search_dvr $(OBJECT)


.PHONY: clean
clean:
	rm -rf $(OBJECT)
