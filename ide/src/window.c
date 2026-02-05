#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <stdint.h>
#include <errno.h>
#include <sys/timerfd.h>
#include <poll.h>
#include <string.h>
#include <time.h>
#include <linux/input-event-codes.h>

#include "xdg-shell-client-protocol.h"

#include "types.h"
#include "window.h"

static struct wl_compositor *compositor;
static struct wl_shm *shm;
static struct xdg_wm_base *wm_base;
static struct wl_surface *surface;
static struct wl_seat *seat;

// Buffer is apparently communicated between server and client via wl_buffer
// but the actual buffer data is inside our custom shm_buffer struct
static struct wl_buffer *wlbuffer;
struct bitmap bm;

// Configuration state
static int configured = 0;
const int initial_width = 800;
const int initial_height = 1000;

// Two variables for each dimension to compare on server event, so we 
// know not to redraw if the size has not actually changed.
static int width = initial_width;
static int height = initial_height;
static int new_width = initial_width;
static int new_height = initial_height;

static void (*draw_cb)(struct bitmap *buf);
static void (*key_cb)(xkb_keysym_t key);
static void (*mouse_cb)(uint32_t mouse_event, uint32_t x, uint32_t y, const struct scroll *scroll);

// ============================= BUFFER HANDLING ==============================

static void __buffer_destroy() {
    if (wlbuffer) wl_buffer_destroy(wlbuffer);
    if (bm.buffer && bm.size > 0) munmap(bm.buffer, (size_t)bm.size);
}

static int __buffer_create() {
    // Initialize the buffer with zeros
    memset(&bm, 0, sizeof(bm));
    bm.width = width; // in pixels
    bm.height = height; // in pixels
    bm.stride = width * 4; // Width in bytes (XRGB = 4 bytes)
    bm.size = bm.stride * height; // Total size in bytes

    // Create a file (descriptor) to map the buffer to
    int fd = memfd_create("shm-buffer", 0);
    if (fd < 0) {
        perror("memfd_create");
        return -1;
    }
    if (ftruncate(fd, bm.size) < 0) {
        perror("ftruncate");
        close(fd);
        return -1;
    }

    bm.buffer = mmap(NULL, (size_t)bm.size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (bm.buffer == MAP_FAILED) {
        perror("mmap");
        close(fd);
        bm.buffer = NULL;
        return -1;
    }

    struct wl_shm_pool *pool = wl_shm_create_pool(shm, fd, bm.size);
    // Wayland has what it needs from fd via the pool; safe to close now.
    close(fd);

    wlbuffer = wl_shm_pool_create_buffer(
        pool, 0, width, height, bm.stride, WL_SHM_FORMAT_XRGB8888
    );
    wl_shm_pool_destroy(pool);

    if (!wlbuffer) {
        fprintf(stderr, "wl_shm_pool_create_buffer failed\n");
        __buffer_destroy();
        return -1;
    }

    return 0;
}

static void recreate_buffer() {
    __buffer_destroy();
    if (__buffer_create() != 0) {
        fprintf(stderr, "Failed to create buffer\n");
        exit(1);
    }

    draw_cb(&bm);

    wl_surface_attach(surface, wlbuffer, 0, 0);
    wl_surface_damage_buffer(surface, 0, 0, width, height);
    wl_surface_commit(surface);
}

static void recreate_buffer_cb() {
    recreate_buffer();
}

// ============================ REGISTRY LISTENER ============================

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

static const uint8_t capability_pointer = 1 << 0;
static const uint8_t capability_keyboard = 1 << 1;
static const uint8_t capability_touch = 1 << 2;
static uint8_t capabilities = 0;

static void __seat_capabilities(
    void *data,
	struct wl_seat *wl_seat,
	uint32_t new_capabilities
) {
    // fprintf(stderr, "received seat capabilities: %d\n", new_capabilities);
    seat = wl_seat;
    capabilities = new_capabilities;
}

 static void __seat_name(
    void *data,
	struct wl_seat *wl_seat,
	const char *name
 ) {
    // fprintf(stderr, "received seat name: %s\n", name);
 }

static const struct wl_seat_listener seat_listener = {
    .capabilities = __seat_capabilities,
    .name = __seat_name,
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
    } else if (strcmp(interface, wl_seat_interface.name) == 0) {
        seat = wl_registry_bind(registry, id, &wl_seat_interface, 7);
        wl_seat_add_listener(seat, &seat_listener, NULL);
    }
}

static void __registry_remove(void *data, struct wl_registry *registry, uint32_t id) {}

static const struct wl_registry_listener registry_listener = {
    .global = __registry_handler,
    .global_remove = __registry_remove
};

// =========================== XDG SURFACE LISTENER ===========================

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
        width = new_width; 
        height = new_height;
        recreate_buffer();
    }
}

static const struct xdg_surface_listener xdg_surface_listener = {
    .configure = __xdg_surface_configure,
};

// ========================== XDG TOPLEVEL LISTENER ===========================

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

// ============================ KEYBOARD INPUT =============================

#define xkb_keycode_offset 8
#define ms_to_ns(ms) (ms * 1000 * 1000)

struct xkb_context *key_ctx;
struct xkb_keymap *key_keymap;
struct xkb_state *key_state;

int32_t repeat_rate = 0;
int32_t repeat_delay = 0;
xkb_keysym_t repeat_key = 0;
int repeat_fd;

static void set_key_repeat(uint32_t delay, uint32_t rate) {
    struct itimerspec its;
    memset(&its, 0, sizeof its);
    its.it_value.tv_nsec = ms_to_ns(delay);
    its.it_interval.tv_nsec = ms_to_ns(rate);
    if (timerfd_settime(repeat_fd, 0, &its, NULL) < 0) {
        printf("timerfd_settime: error\n");
        exit(1);
    }
}

static void start_key_repeat(xkb_keysym_t key) {
    repeat_key = key;
    set_key_repeat(repeat_delay, repeat_rate);
}

static void stop_key_repeat() {
    set_key_repeat(0, 0);
    repeat_key = 0;
}

static void __keyboard_keymap(
    void *data,
	struct wl_keyboard *keyboard,
	uint32_t format,
	int32_t fd,
	uint32_t size
) {
    // fprintf(stderr, "__keyboard_keymap\n");

    char *map_shm = mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);
    key_keymap = xkb_keymap_new_from_string(
        key_ctx, map_shm, XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS
    );
    key_state = xkb_state_new(key_keymap);
    munmap(map_shm, size);
    close(fd);
}

static void __keyboard_enter(
    void *data,
	struct wl_keyboard *wl_keyboard,
	uint32_t serial,
	struct wl_surface *surface,
	struct wl_array *keys
) {
    // fprintf(stderr, "__keyboard_enter\n");
}

static void __keyboard_leave(
    void *data,
	struct wl_keyboard *wl_keyboard,
	uint32_t serial,
	struct wl_surface *surface
) {
    // fprintf(stderr, "__keyboard_leave\n");
}

const int isModiferKey(xkb_keysym_t sym) {
    const xkb_keysym_t modifiers[] = {
        XKB_KEY_Shift_L,
        XKB_KEY_Shift_R,
        XKB_KEY_Alt_L,
        XKB_KEY_Alt_R,
        XKB_KEY_Control_L,
        XKB_KEY_Control_R
    };
    int n = sizeof(modifiers) / sizeof(*modifiers);
    for (int i = 0; i < n; i++) {
        if (sym == modifiers[i]) {
            return 1;
        }
    }
    return 0;
}

static void __keyboard_key(
    void *data,
	struct wl_keyboard *wl_keyboard,
	uint32_t serial,
	uint32_t time,
	uint32_t key,
	uint32_t state
) {
    if (state == WL_KEYBOARD_KEY_STATE_PRESSED) {
        uint32_t keycode = key + xkb_keycode_offset;    
        xkb_keysym_t sym = xkb_state_key_get_one_sym(key_state, keycode);
        // char buf[128];
        // xkb_keysym_get_name(sym, buf, sizeof(buf));
        // fprintf(stderr, " - %d\n", sym);
        if (!isModiferKey(sym)) {
            key_cb(sym);
            if (xkb_keymap_key_repeats(key_keymap, keycode)) {
                // fprintf(stderr, "repeat key: %c\n", sym);
                start_key_repeat(sym);
            }
        }        
    } else if (state == WL_KEYBOARD_KEY_STATE_RELEASED) {
        if (repeat_key) {
            stop_key_repeat();
        }
    }    
}

static void __keyboard_modifiers(
    void *data,
	struct wl_keyboard *wl_keyboard,
	uint32_t serial,
	uint32_t mods_depressed,
	uint32_t mods_latched,
	uint32_t mods_locked,
    uint32_t group
) {
    // fprintf(stderr, "__keyboard_modifiers\n");
    xkb_state_update_mask(
        key_state, mods_depressed, mods_latched, mods_locked, 0, 0, group
    );
}

static void __keyboard_repeat_info(
    void *data,
    struct wl_keyboard *wl_keyboard,
    int32_t rate,
    int32_t delay
) {
    // fprintf(stderr, "__keyboard_repeat_info - rate: %d, delay: %d\n", rate, delay);
    repeat_rate = rate;
    repeat_delay = delay;
}

static const struct wl_keyboard_listener keyboard_listener = {
    .keymap = __keyboard_keymap,
    .enter = __keyboard_enter,
    .leave = __keyboard_leave,
    .key = __keyboard_key,
    .modifiers = __keyboard_modifiers,
    .repeat_info = __keyboard_repeat_info,
};

// ============================ MOUSE INPUT =============================

#define px(fx) fx >> 8

static void __pointer_enter(
    void *data,
    struct wl_pointer *wl_pointer,
    uint32_t serial, 
    struct wl_surface *surface,
    wl_fixed_t surface_x, 
    wl_fixed_t surface_y
) {       
    // fprintf(stderr, "__pointer_enter: %d, %d\n", px(surface_x), px(surface_y));
}

static void __pointer_leave(
    void *data,
	struct wl_pointer *wl_pointer,
	uint32_t serial,
	struct wl_surface *surface
) {       
    // fprintf(stderr, "__pointer_leave\n");
}

static void __pointer_motion(
    void *data,
	struct wl_pointer *wl_pointer,
	uint32_t time,
	wl_fixed_t surface_x,
	wl_fixed_t surface_y
) {       
    // fprintf(stderr, "__pointer_motion\n");
}

static void __pointer_button(
    void *data,
	struct wl_pointer *wl_pointer,
	uint32_t serial,
	uint32_t time,
	uint32_t button,
	uint32_t state
) {
    // fprintf(stderr, "BTN_LEFT: %d\n", BTN_LEFT);

    // fprintf(stderr, "__pointer_button state %d\n", state);

    if (state == WL_POINTER_BUTTON_STATE_PRESSED) {
        const struct scroll scroll = {0, 0};
        mouse_cb(button, 0, 0, &scroll);
    }
}

static void __pointer_axis(
    void *data,
	struct wl_pointer *wl_pointer,
	uint32_t time,
	uint32_t axis,
	wl_fixed_t value
) {
    int x = 0;
    int y = 0;
    // fprintf(stderr, "__pointer_axis: %d\n", px(value));
    if (axis == WL_POINTER_AXIS_VERTICAL_SCROLL) {
        y = px(value);
        // fprintf(stderr, "VERTICAL_SCROLL\n");
    } else if (axis == WL_POINTER_AXIS_HORIZONTAL_SCROLL) {
        x = px(value);
        // fprintf(stderr, "HORIZONTAL_SCROLL\n");
    } else {
        fprintf(stderr, "Unknown axis %d\n", axis);
    }

    const struct scroll scroll = {axis, px(value)};

    mouse_cb(REL_WHEEL, x, y, &scroll);
}

static void __pointer_frame(
    void *data,
	struct wl_pointer *wl_pointer
) {       
    // fprintf(stderr, "__pointer_frame\n");
}

static void __pointer_axis_source(
    void *data,
	struct wl_pointer *wl_pointer,
	uint32_t axis_source
) {       
    // fprintf(stderr, "__pointer_axis_source\n");
    // if (axis_source == WL_POINTER_AXIS_SOURCE_WHEEL) {
    //     fprintf(stderr, "WL_POINTER_AXIS_SOURCE_WHEEL\n");
    // } else if (axis_source == WL_POINTER_AXIS_SOURCE_FINGER) {
    //     fprintf(stderr, "WL_POINTER_AXIS_SOURCE_FINGER\n");
    // } else if (axis_source == WL_POINTER_AXIS_SOURCE_CONTINUOUS) {
    //     fprintf(stderr, "WL_POINTER_AXIS_SOURCE_CONTINUOUS\n");
    // } else if (axis_source == WL_POINTER_AXIS_SOURCE_WHEEL_TILT) {
    //     fprintf(stderr, "WL_POINTER_AXIS_SOURCE_WHEEL_TILT\n");
    // } else {
    //     fprintf(stderr, "unknown axis %d\n", axis_source);

    // }
}

static void __pointer_axis_stop(
    void *data,
	struct wl_pointer *wl_pointer,
	uint32_t time,
	uint32_t axis
) {       
    fprintf(stderr, "__pointer_axis_stop\n");
}

static void __pointer_axis_discrete(
    void *data,
	struct wl_pointer *wl_pointer,
	uint32_t axis,
	int32_t discrete
) {       
    // fprintf(stderr, "__pointer_axis_discrete: %d, steps: %d\n", fx_to_px(axis), discrete);

}

static const struct wl_pointer_listener pointer_listener = {
       .enter = __pointer_enter,
       .leave = __pointer_leave,
       .motion = __pointer_motion,
       .button = __pointer_button,
       .axis = __pointer_axis,
       .frame = __pointer_frame,
       .axis_source = __pointer_axis_source,
       .axis_stop = __pointer_axis_stop,
       .axis_discrete = __pointer_axis_discrete,
};

// ================================== PUBLIC ===================================

struct wl_display *display;

recreate_buffer_cb_type initialize_window(
    void (*draw_cb_arg)(struct bitmap *buf),
    void (*key_cb_arg)(xkb_keysym_t key),
    void (*mouse_cb_arg)(uint32_t mouse_event, uint32_t x, uint32_t y, const struct scroll *scroll)
) {
    draw_cb = draw_cb_arg;
    key_cb = key_cb_arg;
    mouse_cb = mouse_cb_arg;
 
    key_ctx = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    if (!key_ctx) {
        fprintf(stderr, "no xkb\n");
    }

    /**
     * Get display from the compositor. Passing NULL as the display name, so
     * we get the default "wayland-0".
     */
    display = wl_display_connect(NULL);
    if (!display) {
        fprintf(stderr, "No Wayland display\n");
        exit(1);
    }

    /**
     * Initialize the global "compositor", "shm" and "wm_base" variables.
     * Registry_listener uses wl_compositor and wl_registry protocols.
     * The wl_display_roundtrip finally fills in the global variables.
     */ 
    struct wl_registry *registry = wl_display_get_registry(display);
    wl_registry_add_listener(registry, &registry_listener, NULL);
    wl_display_roundtrip(display);

    if (!compositor || !shm || !wm_base || !seat) {
        fprintf(stderr, "Wayland globals missing\n");
        exit(1);
    }

    surface = wl_compositor_create_surface(compositor);
    struct xdg_surface *xdg_surface = xdg_wm_base_get_xdg_surface(wm_base, surface);
    xdg_surface_add_listener(xdg_surface, &xdg_surface_listener, NULL);
    struct xdg_toplevel *toplevel = xdg_surface_get_toplevel(xdg_surface);
    xdg_toplevel_add_listener(toplevel, &xdg_toplevel_listener, NULL);
    xdg_toplevel_set_title(toplevel, "IDE");
    wl_surface_commit(surface);
    
    struct  wl_keyboard *keyboard = wl_seat_get_keyboard(seat);
    wl_keyboard_add_listener(keyboard, &keyboard_listener, NULL);

    struct wl_pointer *pointer = wl_seat_get_pointer(seat);
    wl_pointer_add_listener(pointer, &pointer_listener, NULL);

    while (!configured) {
        wl_display_dispatch(display);
    }

    recreate_buffer();

    return &recreate_buffer_cb;
}


int run_window() {
    repeat_fd = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC | TFD_NONBLOCK);
    if (repeat_fd < 0) {
        fprintf(stderr, "timerfd_create\n");
        exit(1);
    }
    
    int wl_fd = wl_display_get_fd(display);
    if (wl_fd < 0) {
        fprintf(stderr, "wl_display_get_fd\n");
        exit(1);
    }

    for (;;) {
        // 1) Dispatch anything already queued in libwayland
        wl_display_dispatch_pending(display);

        // 2) Send outgoing requests
        if (wl_display_flush(display) < 0) {
            // EPIPE means compositor disconnected
            fprintf(stderr, "wl_display_flush\n");
            exit(1);            
        }

        // 3) Prepare to read (required for correct poll integration)
        while (wl_display_prepare_read(display) != 0) {
            // If it fails, there were pending events; dispatch them and try again
            wl_display_dispatch_pending(display);
        }

        struct pollfd fds[2];
        fds[0].fd = wl_fd;
        fds[0].events = POLLIN;
        fds[1].fd = repeat_fd;
        fds[1].events = POLLIN;

        int ret = poll(fds, 2, -1);

        if (ret < 0) {
            wl_display_cancel_read(display);
            fprintf(stderr, "poll\n");
            exit(1);
        }

        // 4) If Wayland fd is readable, read events; otherwise cancel the read
        if (fds[0].revents & POLLIN) {
            if (wl_display_read_events(display) < 0) {
                fprintf(stderr, "wl_display_read_events\n");
                break;
            }
            // Now dispatch the newly read events (callbacks fire here)
            wl_display_dispatch_pending(display);
        } else {
            wl_display_cancel_read(display);
        }

        // 5) Handle repeat event
        if (fds[1].revents & POLLIN) {
            uint64_t expirations = 0;
            if (read(repeat_fd, &expirations, sizeof expirations) == sizeof expirations) {
                key_cb(repeat_key);
            }
        }
    }

    __buffer_destroy();

    return 0;
}
