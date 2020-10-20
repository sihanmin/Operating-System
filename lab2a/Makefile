#Name: Sihan Min
#ID: 504807176
#Email: sihanmin@yeah.net
#_*_MakeFile _*_

CC = gcc
SET = -Wall -Wextra -g -lpthread
TARGET = lab2_add.c lab2_list.c SortedList.c SortedList.h Makefile lab2_add.csv lab2_list.csv lab2_add-1.png lab2_add-2.png lab2_add-3.png lab2_add-4.png lab2_add-5.png lab2_list-1.png lab2_list-2.png lab2_list-3.png lab2_list-4.png lab2_add.gp lab2_list.gp lab2a_test.sh README

all: lab2_add lab2_list

lab2_add: lab2_add.c
	$(CC) $(SET) -o lab2_add lab2_add.c

lab2_list: lab2_list.c SortedList.c SortedList.h
	$(CC) $(SET) -o lab2_list lab2_list.c SortedList.c

tests:
	./lab2a_test.sh

graphs:
	gnuplot lab2_add.gp	
	gnuplot lab2_list.gp
clean:
	rm -rf *.o *.csv *.png lab2_add lab2_list
	rm lab2a-504807176.tar.gz

dist:
	@tar -czf lab2a-504807176.tar.gz $(TARGET)

