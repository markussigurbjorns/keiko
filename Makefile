# Compiler and flags
CC = gcc
CFLAGS = -Wall -std=c11 -g  -fsanitize=address
LDFLAGS = -lportaudio -lm 

# Output executable
TARGET = keiko

# Source files
SRCS = main.c audioInterface.c 

# Header files
HEADERS = audioInterface.h 

# Object files (replace .c with .o)
OBJS = $(SRCS:.c=.o)

# Default target
all: $(TARGET)

# Link object files to create the executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)

# Compile source files into object files
%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up build artifacts
clean:
	rm -f $(OBJS) $(TARGET)

# Rebuild everything
rebuild: clean all
