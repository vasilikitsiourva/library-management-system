CC = gcc

CFLAGS = -Wall -Wextra -g

TARGET = library_system

SRCS = main.c

HEADERS = library.h

OBJS = $(SRCS:.c=.o)
 
all: $(TARGET)
 
$(TARGET): $(OBJS)

	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)
 
%.o: %.c $(HEADERS)

	$(CC) $(CFLAGS) -c $< -o $@
 
clean:

	rm -f $(OBJS) $(TARGET)
 
run: $(TARGET)

	./$(TARGET)

 
 