all:
	cc -g -o test test.c cbeacon.c -lbluetooth  -Wall -O
clean:
	rm -f test
