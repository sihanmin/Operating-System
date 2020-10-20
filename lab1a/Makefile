CC = gcc
CFLAGS = -Wall
TARGET = lab1a

$(TARGET): $(TARGET).c
	$(CC) $(CFLAGS) -o $(TARGET) $(TARGET).c

check:
	@./lab1a
	@./lab1a --shell

clean:
	@rm -rf $(TARGET)
	@rm -rf lab1a-504807176.tar.gz

dist: 
	@tar -czf lab1a-504807176.tar.gz $(TARGET).c README Makefile
