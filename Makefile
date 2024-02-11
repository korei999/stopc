BIN := stopc

$(BIN): main.c Makefile
	gcc -flto=auto -O2 -march=native -o $(BIN) main.c -lpthread 
