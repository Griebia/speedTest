SOURCES = example.c
OBJS = $(SOURCES:.c=.o)
TARGET = exampleapp
RM = rm -f

CFLAGS = -Wall

all:
	$(CC) $(SOURCES) $(CFLAGS) -o $(TARGET)

clean:
	$(RM) $(TARGET)