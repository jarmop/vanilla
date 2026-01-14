#!/bin/bash

gcc *.c $(pkg-config --cflags --libs wayland-client)
./a.out