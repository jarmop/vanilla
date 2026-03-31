#!/bin/bash

gcc -Wall -Wextra src/camera.c glad/glad.c lib/stb_image.c -lglfw -lm -lcglm

./a.out