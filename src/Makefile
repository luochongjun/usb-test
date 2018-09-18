CC=gcc
CFLAGS+=
LDFLAGS+= -lusb-1.0

all: usbtest
	@echo "compile done"

usbtest: usbtest.o
	$(CC) $^ -o $@ $(LDFLAGS)

clean:
	rm $(PROC) $(OBJS)

.PHONY: all clean
