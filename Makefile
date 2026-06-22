# Makefile for the C Shell (Part A)
#
# Layout:
#   include/  — header files (.h)
#   src/      — source files (.c)
#   obj/      — compiled objects (.o)
#   shell.out — final binary (in this directory)
#
# Usage:
#   make all   — compile everything → shell.out
#   make clean — remove built artefacts

CC      = gcc
CFLAGS  = -std=c99 \
          -D_POSIX_C_SOURCE=200809L \
          -D_XOPEN_SOURCE=700 \
          -Wall -Wextra -Werror \
          -Wno-unused-parameter \
          -fno-asm \
          -I include

SRCDIR  = src
OBJDIR  = obj
INCDIR  = include

SRCS    = $(SRCDIR)/main.c \
          $(SRCDIR)/prompt.c \
          $(SRCDIR)/input.c \
          $(SRCDIR)/parser.c

OBJS    = $(OBJDIR)/main.o \
          $(OBJDIR)/prompt.o \
          $(OBJDIR)/input.o \
          $(OBJDIR)/parser.o

TARGET  = shell.out

.PHONY: all clean

all: $(OBJDIR) $(TARGET)

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET)

# Header dependencies
$(OBJDIR)/main.o:   $(SRCDIR)/main.c   $(INCDIR)/types.h $(INCDIR)/prompt.h $(INCDIR)/input.h $(INCDIR)/parser.h
$(OBJDIR)/prompt.o: $(SRCDIR)/prompt.c $(INCDIR)/types.h $(INCDIR)/prompt.h
$(OBJDIR)/input.o:  $(SRCDIR)/input.c  $(INCDIR)/input.h
$(OBJDIR)/parser.o: $(SRCDIR)/parser.c $(INCDIR)/types.h $(INCDIR)/parser.h
