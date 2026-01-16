#!/bin/bash

gcc src/*.c $(pkg-config --cflags --libs wayland-client freetype2)

./a.out