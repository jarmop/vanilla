#!/bin/bash

glslc shaders/vert.vert -o shaders/vert.spv
glslc shaders/frag.frag -o shaders/frag.spv

gcc main.c -lvulkan -lglfw

./a.out