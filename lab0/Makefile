# variables
CC = gcc
TARGET = lab0

#all: $(TARGET)

$(TARGET): $(TARGET).c
	$(CC) -Wall -Wextra -g -o $(TARGET) $(TARGET).c

check: 
#Testing --input
	@echo "helloworld" > input.txt
	@echo "Testing input flag..."
	@./lab0 --input=input.txt > output.txt ;\
	if [ $$? -ne 0 ] ;\
	then \
		echo "Error at input flag." ;\
        else \
		echo "Input test passed.";\
	fi
	@rm output.txt

#Testing --output
	@echo "Testing output flag..."
	@./lab0 --input=input.txt --output=output.txt
	@diff input.txt output.txt ;\
	if [ $$? -ne 0 ] ;\
	then \
	echo "Error at output flag." ;\
	else \
	echo "Output test passed." ;\
	fi
	@rm output.txt
	@rm input.txt

#Testing missed argument
	@echo "Testing missed argument..."
	@./lab0 --input= ;\
	if [ $$? -ne 2 ] ;\
	then \
	echo "Error at missed argument." ;\
	else \
	echo "Missed argument test passed." ;\
	fi

#Testing undefined flag
	@echo "Testing undefined flag..."
	@./lab0 --whatever ;\
	if [ $$? -ne 1 ] ;\
	then \
	echo "Error at undefined flag." ;\
	else \
	echo "Undefined flag test passed." ;\
	fi

clean:
	$(RM) $(TARGET) *~
	@rm -rf lab0-504807176.tar.gz

dist:
	@tar -czf lab0-504807176.tar.gz README Makefile *.c backtrace.png breakpoint.png
