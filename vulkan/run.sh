#!/bin/bash

rm -f build/bin/VanillaVulkan

time cmake -B build
time cmake --build build

vendored/slang/bin/slangc assets/shader.slang -target spirv -o assets/shader.spv

build/bin/VanillaVulkan 