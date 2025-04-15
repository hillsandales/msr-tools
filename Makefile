# CFLAGS += -Wall -Wextra -Wpedantic -Og -std=c11
# program: cpuid.o rdmsr.o rmwmsr.o wrmsr.o


# Define the C compiler and flags
CC = gcc
CFLAGS = -Wall -g

# Find all .c files in the current directory
SOURCES = $(wildcard *.c)

# Generate the names of the executable files by removing the .c extension
PROGRAMS = $(SOURCES:.c=)

# Default target: compile all programs
all: $(PROGRAMS)

# Rule to compile each program
%: %.c
	$(CC) $(CFLAGS) -o $@ $<

# Clean rule to remove all executables
clean:
	rm -f $(PROGRAMS)
