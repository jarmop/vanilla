#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <stdint.h>
#include <errno.h>

#include "xdg-shell-client-protocol.h"

#include "ide.h"
#include "draw.h"

static struct wl_compositor *compositor;
static struct wl_shm *shm;
static struct xdg_wm_base *wm_base;
struct wl_surface *surface;
struct shm_buffer buf;

// Configuration state
static int configured = 0;
const int initial_width = 800;
const int initial_height = 1000;
static int width = initial_width;
static int height = initial_height;
static int new_width = initial_width;
static int new_height = initial_height;

static void shm_buffer_destroy(struct shm_buffer *b) {
    if (!b) return;
    if (b->buffer) wl_buffer_destroy(b->buffer);
    if (b->data && b->size > 0) munmap(b->data, (size_t)b->size);
    memset(b, 0, sizeof(*b));
}

static int shm_buffer_create(struct shm_buffer *buf, int width, int height) {
    memset(buf, 0, sizeof(*buf));
    buf->width = width;
    buf->height = height;
    buf->stride = width * 4;
    buf->size = buf->stride * height;

    int fd = memfd_create("shm-buffer", 0);
    if (fd < 0) {
        perror("memfd_create");
        return -1;
    }
    if (ftruncate(fd, buf->size) < 0) {
        perror("ftruncate");
        close(fd);
        return -1;
    }

    buf->data = mmap(NULL, (size_t)buf->size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (buf->data == MAP_FAILED) {
        perror("mmap");
        close(fd);
        buf->data = NULL;
        return -1;
    }

    struct wl_shm_pool *pool = wl_shm_create_pool(shm, fd, buf->size);
    // Wayland has what it needs from fd via the pool; safe to close now.
    close(fd);

    buf->buffer = wl_shm_pool_create_buffer(
        pool, 0, width, height, buf->stride, WL_SHM_FORMAT_XRGB8888
    );
    wl_shm_pool_destroy(pool);

    if (!buf->buffer) {
        fprintf(stderr, "wl_shm_pool_create_buffer failed\n");
        munmap(buf->data, (size_t)buf->size);
        buf->data = NULL;
        return -1;
    }

    return 0;
}

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
        xdg_wm_base_add_listener(wm_base, &xdg_wm_base_listener, NULL);
    }
}

static void __registry_remove(void *data, struct wl_registry *registry, uint32_t id) {}

static const struct wl_registry_listener registry_listener = {
    .global = __registry_handler,
    .global_remove = __registry_remove
};

static void resize_window(int width, int height) {
    shm_buffer_destroy(&buf);
    shm_buffer_create(&buf, width, height);

    draw(&buf);

    wl_surface_attach(surface, buf.buffer, 0, 0);
    wl_surface_damage_buffer(surface, 0, 0, width, height);
    wl_surface_commit(surface);
}

static void __xdg_surface_configure(
    void *data, 
    struct xdg_surface *xdg_surface, 
    uint32_t serial
) {
    xdg_surface_ack_configure(xdg_surface, serial);
    configured = 1;
    if (
        new_width > 0
        && new_height > 0
        && (new_width != width || new_height != height)
    ) {
        resize_window(new_width, new_height);
        width = new_width; height = new_height;
    }
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
    if (width > 0 && height > 0) {
        new_width = width;
        new_height = height;
    }
}

static const struct xdg_toplevel_listener xdg_toplevel_listener = {
    .configure = __xdg_toplevel_configure,
    .close = NULL,
};

int main(int argc, char **argv) {
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

    surface = wl_compositor_create_surface(compositor);
    struct xdg_surface *xdg_surface = xdg_wm_base_get_xdg_surface(wm_base, surface);
    xdg_surface_add_listener(xdg_surface, &xdg_surface_listener, NULL);
    struct xdg_toplevel *toplevel = xdg_surface_get_toplevel(xdg_surface);
    xdg_toplevel_add_listener(toplevel, &xdg_toplevel_listener, NULL);
    xdg_toplevel_set_title(toplevel, "Wayland + FreeType (ASCII)");
    wl_surface_commit(surface);

    while (!configured) {
        wl_display_dispatch(display);
    }

    // ---------- Create shm buffer sized from configure ----------
    if (shm_buffer_create(&buf, width, height) != 0) {
        return 1;
    }

    draw(&buf);

    wl_surface_attach(surface, buf.buffer, 0, 0);
    wl_surface_damage_buffer(surface, 0, 0, buf.width, buf.height);
    wl_surface_commit(surface);

    // Simple event loop (static image)
    while (wl_display_dispatch(display)) {
    }

    // Cleanup (won't be reached in this trivial loop unless display disconnects)
    shm_buffer_destroy(&buf);

    return 0;
}
