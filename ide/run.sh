#!/bin/bash

# gcc *.c $(pkg-config --cflags --libs wayland-client)

gcc wayland-client.c xdg-shell-protocol.c $(pkg-config --cflags --libs wayland-client freetype2)

./a.out