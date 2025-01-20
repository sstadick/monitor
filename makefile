CC = gcc
CFLAGS = -Isrc -Wall -Werror -O3
# First, find all source files in src directory
SRCS = $(wildcard src/*.c)
# Create list of object files by replacing src/*.c with obj/*.o
OBJS = $(SRCS:src/%.c=obj/%.o)


# Final executable
monitor: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

# Create object directory if it doesn't exist and compile source files
obj/%.o: src/%.c
	@mkdir -p obj
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f obj/*.o monitor
	rmdir obj
