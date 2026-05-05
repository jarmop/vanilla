#!/bin/bash

rm -f a.out

gcc -Wall -Wextra main.c $(pkg-config --cflags --libs glfw3 vulkan)

./a.out