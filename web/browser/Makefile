default: run

prepare:
	mkdir -p bin

build: prepare
	gcc main.c $(shell pkg-config --cflags --libs gtk4) -o bin/main

run: build
	./bin/main

make clean:
	rm -rf bin