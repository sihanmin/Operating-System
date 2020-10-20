CC = gcc
LFLAGS = -lmraa -lm -lssl -lcrypto -Wall -Wextra -g
TARGET = Makefile README lab4c_tcp.c lab4c_tls.c

.PHONY : dist default

default: lab4c

lab4c: lab4c_tcp lab4c_tls

lab4c_tcp:
ifeq ($(shell uname -r | grep 'edison'),)
	$(CC) $(LFLAGS) -o lab4c_tcp lab4c_tcp.c -DDUMMY
else
	$(CC) $(LFLAGS) -o lab4c_tcp lab4c_tcp.c
endif

lab4c_tls:
ifeq ($(shell uname -r | grep 'edison'),)
	$(CC) $(LFLAGS) -o lab4c_tls lab4c_tls.c -DDUMMY
else
	$(CC) $(LFLAGS) -o lab4c_tls lab4c_tls.c
endif

clean:
	rm -f *.o lab4c_tcp lab4c_tls *.log
	rm -f lab4c-504807176.tar.gz

dist: default
	@tar -czf lab4c-504807176.tar.gz $(TARGET)
