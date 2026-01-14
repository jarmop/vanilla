Generate the xdg-shell header file:

```sh
wayland-scanner client-header \
  /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml \
  xdg-shell-client-protocol.h
```

Generate a source file for linking:

```sh
wayland-scanner private-code \
  /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml \
  xdg-shell-protocol.c
```

Compile:

```sh
gcc wayland-example.c xdg-shell-protocol.c $(pkg-config --cflags --libs wayland-client)
```
