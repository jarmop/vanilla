#!/bin/bash

rm -f triangle *.spv

slangc triangle_minimal.slang -target spirv -o out.spv

odin run .