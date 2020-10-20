CC=gcc
CFLAGS=-Wall -Wextra
UID=604838709
PROJECT=lab3a
ARCHIVE=$(PROJECT)-$(UID)

default:
	$(CC) $(CFLAGS) -g $(PROJECT).c -o $(PROJECT)

clean:
	rm -f *.o $(PROJECT) *.tar.gz 

dist:
	tar -zcf $(ARCHIVE).tar.gz $(PROJECT).c Makefile README ext2_fs.h

