#!/bin/bash

# Generate the xdg-shell header file:
wayland-scanner client-header \
  /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml \
  src/xdg-shell-client-protocol.h

# Generate a source file for linking:
wayland-scanner private-code \
  /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml \
  src/xdg-shell-protocol.c