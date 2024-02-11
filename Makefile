BIN := stopc

$(BIN): main.c Makefile
	gcc -flto=auto -O2 -march=sandybridge -o $(BIN) main.c -lpthread 
