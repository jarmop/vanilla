#!/bin/bash

rm -f a.out

glslc shaders/vert.vert -o shaders/vert.spv
glslc shaders/frag.frag -o shaders/frag.spv

gcc -Wall -Wextra main.c -lvulkan -lglfw

./a.out