CC = gcc
LFLAGS = -Wall -Wextra -lmraa -lm -g
TARGET = Makefile README lab4b.c

.PHONY : dist default tests

default: lab4b

lab4b: lab4b.c
	$(CC) $(LFLAGS) -o lab4b lab4b.c

check:
	echo "TEST unrecognized argument"
	./lab4b --nothing 2> STDERR
	if [ $? -ne 1 ]
	then
		echo "ERROR. Expected RC=1."
	else
		echo "PASSED"
	rm STDERR
	fi

clean:
	rm -f *.o lab4b
	rm -f lab4b-504807176.tar.gz

dist: default tests
	@tar -czf lab4b-504807176.tar.gz $(TARGET)
