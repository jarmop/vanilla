#!/bin/bash

rm -f a.out

# gcc -std=gnu2x src/*.c $(pkg-config --cflags --libs wayland-client freetype2 xkbcommon)
# clang -std=gnu2x src/*.c $(pkg-config --cflags --libs wayland-client freetype2 xkbcommon)
zig cc -std=gnu2x src/*.c $(pkg-config --cflags --libs wayland-client freetype2 xkbcommon)
# zig cc -std=gnu2x src/*.c src/*.zig $(pkg-config --cflags --libs wayland-client freetype2 xkbcommon)

./a.out