#!/bin/bash

rm -f a.out shaders/*.spv

gcc -Wall -Wextra main.c -lm $(pkg-config --cflags --libs glfw3 vulkan cglm)

# slangc shaders/triangle.slang -target spirv -o shaders/out.spv
# slangc shaders/dynamic.slang -target spirv -o shaders/out.spv
slangc shaders/uniform.slang -target spirv -o shaders/out.spv

# slangc shaders/triangle.slang -target spirv -profile \
#   spirv_1_4 -emit-spirv-directly -fvk-use-entrypoint-name \
#   -entry vertMain -entry fragMain -o shaders/out.spv

./a.out