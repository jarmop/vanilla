#!/bin/bash

rm -f a.out shaders/*.spv

gcc -Wall -Wextra main.c -lvulkan -lglfw

slangc shaders/triangle.slang -target spirv -o shaders/triangle.spv
# slangc shaders/minimal_triangle.slang -target spirv -o shaders/triangle.spv

# slangc shaders/triangle.slang -target spirv -profile \
#   spirv_1_4 -emit-spirv-directly -fvk-use-entrypoint-name \
#   -entry vertMain -entry fragMain -o shaders/triangle.spv

./a.out