AM_CFLAGS = -Wall -g -O2 -fomit-frame-pointer \
	-D_GNU_SOURCE -D_FILE_OFFSET_BITS=64

bin_PROGRAMS = wrmsr rdmsr msr-cpuid rmwmsr

wrmsr_SOURCES =		\
	wrmsr.c		\
	version.h

wrmsr_CFLAGS = $(AM_CFLAGS)

rdmsr_SOURCES =		\
	rdmsr.c		\
	version.h

rdmsr_CFLAGS = $(AM_CFLAGS)

msr_cpuid_SOURCES =		\
	cpuid.c

msr_cpuid_CFLAGS = $(AM_CFLAGS)

rmwmsr_SOURCES =		\
	rmwmsr.c		\
	version.h

rmwmsr_CFLAGS = $(AM_CFLAGS)
