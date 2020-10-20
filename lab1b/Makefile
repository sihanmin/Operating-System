#NAME: Sihan Min
#EMAIL: sihanmin@yeah.net
#ID: 504807176

# macros
CC = gcc
TARGET = lab1b-client.c lab1b-server.c README Makefile my.key

all: lab1b-client lab1b-server

lab1b-client: lab1b-client.c
	$(CC) -lmcrypt -g -o $@ lab1b-client.c

lab1b-server: lab1b-server.c
	$(CC) -lmcrypt -g -o $@ lab1b-server.c

dist: 
	@tar -czf lab1b-504807176.tar.gz $(TARGET)

clean: 
	@rm -rf lab1b-client lab1b-server *.txt
	@rm -rf lab1b-504807176.tar.gz

