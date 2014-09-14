all:
        cc -g -o test test.c cbeacon.c -lbluetooth -Wall -O2
clean:
        rm -f test
