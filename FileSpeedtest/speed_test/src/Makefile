SOURCES = speedtest.c
OBJS = $(SOURCES:.c=.so)
TARGET = libspeedtest.so
RM = rm -f

CFLAGS = -shared -Wall -lcurl
TFLAGS = -fPIC

all:
	$(CC) $(SOURCES) $(CFLAGS) -o $(TARGET) $(TFLAGS) -I/usr/include/lua5.1 -llua

clean:
	$(RM) $(TARGET)
