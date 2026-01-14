#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>

#include "xdg-shell-client-protocol.h"

static struct wl_compositor *compositor; // For wl_surface
// For allocating Shared Memory Buffers (shared between the server and the client)
static struct wl_shm *shm; 
static struct xdg_wm_base *wm_base; // For creating xdg-shell surfaces

static void __xdg_wm_base_ping(
    void *data, 
    struct xdg_wm_base *xdg_wm_base, 
    uint32_t serial
) {
    xdg_wm_base_pong(xdg_wm_base, serial);
}

static const struct xdg_wm_base_listener xdg_wm_base_listener = {
    .ping = __xdg_wm_base_ping,
};

// Configuration state
static int configured = 0;
static int win_width = 400;
static int win_height = 300;
static uint32_t configure_serial;

static void __registry_handler(
    void *data,
    struct wl_registry *registry,
    uint32_t id, 
    const char *interface,
    uint32_t version
) {
    if (strcmp(interface, wl_compositor_interface.name) == 0) {
        compositor = wl_registry_bind(registry, id, &wl_compositor_interface, 4);
    } else if (strcmp(interface, wl_shm_interface.name) == 0) {
        shm = wl_registry_bind(registry, id, &wl_shm_interface, 1);
    } else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
        wm_base = wl_registry_bind(registry, id, &xdg_wm_base_interface, 1);
        xdg_wm_base_add_listener(wm_base, &xdg_wm_base_listener, data);
    }
}

static void __registry_remove(void *data, struct wl_registry *registry, uint32_t id) {}

static const struct wl_registry_listener registry_listener = {
    .global = __registry_handler,
    .global_remove = __registry_remove
};

static void __xdg_surface_configure(
    void *data, 
    struct xdg_surface *xdg_surface, 
    uint32_t serial
) {
    configure_serial = serial;
    xdg_surface_ack_configure(xdg_surface, serial);
    configured = 1;
}

static const struct xdg_surface_listener xdg_surface_listener = {
    .configure = __xdg_surface_configure,
};

static void __xdg_toplevel_configure(
    void *data, 
    struct xdg_toplevel *toplevel, 
    int32_t width, 
    int32_t height, 
    struct wl_array *states
) {
    if (width > 0)
        win_width = width;
    if (height > 0)
        win_height = height;
}

static const struct xdg_toplevel_listener xdg_toplevel_listener = {
    .configure = __xdg_toplevel_configure,
    .close = NULL,
};

static struct wl_buffer *__create_buffer(int width, int height) {
    int stride = width * 4;
    int size = stride * height;

    int fd = memfd_create("shm-buffer", 0);
    ftruncate(fd, size);

    void *data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    // A pool of pixel memory
    struct wl_shm_pool *pool = wl_shm_create_pool(shm, fd, size);
    // A single image inside the pool
    struct wl_buffer *buffer = wl_shm_pool_create_buffer(
        pool, 0, width, height, stride, WL_SHM_FORMAT_XRGB8888
    );
    // The buffer is still shared by the compositor when the shared pool is destroyed
    wl_shm_pool_destroy(pool);

    // Create the image
    uint32_t *p = data;
    for (int i = 0; i < width * height; i++) {
        p[i] = 0xff404040;  // gray
    }

    return buffer;
}

int main() {
    /**
     * Get display from the compositor. Passing NULL as the display name, so
     * we get the default "wayland-0".
     */
    struct wl_display *display = wl_display_connect(NULL);
    if (!display) {
        fprintf(stderr, "No Wayland display\n");
        return 1;
    }

    /**
     * Initialize the global "compositor", "shm" and "wm_base" variables.
     * Registry_listener uses wl_compositor and wl_registry protocols.
     * The wl_display_roundtrip finally fills in the global variables.
     */ 
    struct wl_registry *registry = wl_display_get_registry(display);
    wl_registry_add_listener(registry, &registry_listener, NULL);
    wl_display_roundtrip(display);
    if (!compositor || !shm || !wm_base) {
        fprintf(stderr, "Wayland globals missing\n");
        return 1;
    }

    // Create surface
    struct wl_surface *surface = wl_compositor_create_surface(compositor);
    struct xdg_surface *xdg_surface = xdg_wm_base_get_xdg_surface(wm_base, surface);
    xdg_surface_add_listener(xdg_surface, &xdg_surface_listener, NULL);
    struct xdg_toplevel *toplevel = xdg_surface_get_toplevel(xdg_surface);
    xdg_toplevel_add_listener(toplevel, &xdg_toplevel_listener, NULL);
    xdg_toplevel_set_title(toplevel, "Wayland Hello");
    wl_surface_commit(surface);

    while (!configured) {
        wl_display_dispatch(display);
    }

    // Create the pixel buffer and attach it to the surface
    struct wl_buffer *buffer = __create_buffer(400, 300);
    wl_surface_attach(surface, buffer, 0, 0);
    wl_surface_commit(surface);

    while (wl_display_dispatch(display)) {
    }

    return 0;
}
