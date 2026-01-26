#!/bin/bash

rm -f a.out

gcc src/*.c $(pkg-config --cflags --libs wayland-client freetype2 xkbcommon)

./a.out