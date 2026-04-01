#!/bin/bash

# -lm is for libm, the standard C math library
# -lcglm is for cglm, a C version of the C++ math library, glm
gcc -Wall -Wextra src/camera.c glad/glad.c lib/stb_image.c -lglfw -lm -lcglm

./a.out