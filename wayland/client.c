#define _GNU_SOURCE
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

struct wl_header {
    uint32_t object_id;
    uint16_t opcode;
    uint16_t size;
} __attribute__((packed));

static size_t align4(size_t n) { return (n + 3u) & ~3u; }

static int connect_wayland_socket(void) {
    const char *runtime = getenv("XDG_RUNTIME_DIR");
    const char *display = getenv("WAYLAND_DISPLAY");
    if (!runtime) {
        fprintf(stderr, "XDG_RUNTIME_DIR not set\n");
        return -1;
    }
    if (!display) display = "wayland-0";

    char path[108];
    // sun_path max is typically 108 including NUL
    if (snprintf(path, sizeof(path), "%s/%s", runtime, display) >= (int)sizeof(path)) {
        fprintf(stderr, "Wayland socket path too long\n");
        return -1;
    }

    int fd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if (fd < 0) {
        perror("socket");
        return -1;
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);

    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("connect");
        close(fd);
        return -1;
    }

    return fd;
}

static int send_get_registry(int fd, uint32_t new_id) {
    // wl_display (object id = 1), opcode 1 = get_registry
    // payload: new_id (uint32)
    uint8_t msg[12];
    struct wl_header h;
    h.object_id = 1;
    h.opcode = 1;
    h.size = sizeof(msg); // 8 + 4

    memcpy(msg, &h, sizeof(h));
    memcpy(msg + sizeof(h), &new_id, sizeof(new_id));

    ssize_t n = send(fd, msg, sizeof(msg), 0);
    if (n != (ssize_t)sizeof(msg)) {
        perror("send(get_registry)");
        return -1;
    }
    return 0;
}

// Read exactly n bytes unless EOF/error
static int read_full(int fd, void *buf, size_t n) {
    uint8_t *p = (uint8_t *)buf;
    size_t got = 0;
    while (got < n) {
        ssize_t r = recv(fd, p + got, n - got, 0);
        if (r == 0) return 0; // EOF
        if (r < 0) {
            if (errno == EINTR) continue;
            perror("recv");
            return -1;
        }
        got += (size_t)r;
    }
    return 1;
}

static const char *parse_string(const uint8_t *payload, size_t payload_len, size_t *offset_io) {
    size_t off = *offset_io;
    if (off + 4 > payload_len) return NULL;

    uint32_t len;
    memcpy(&len, payload + off, 4);
    off += 4;

    if (len == 0) {
        // empty string is encoded as len=0 (rare) or len=1 with NUL; treat 0 as invalid here
        return NULL;
    }
    if (off + len > payload_len) return NULL;

    const char *s = (const char *)(payload + off); // includes NUL terminator
    off += align4((size_t)len);

    if (off > payload_len) return NULL;
    *offset_io = off;
    return s;
}

int main(void) {
    int fd = connect_wayland_socket();
    if (fd < 0) return 1;

    uint32_t registry_id = 2;
    if (send_get_registry(fd, registry_id) < 0) {
        close(fd);
        return 1;
    }

    // Now: receive messages. We’ll decode:
    // - wl_registry.global (object_id == registry_id, opcode 0)
    // - wl_registry.global_remove (object_id == registry_id, opcode 1)
    //
    // We stop after printing some globals (or on EOF).
    int globals_printed = 0;

    for (;;) {
        struct wl_header h;
        int rr = read_full(fd, &h, sizeof(h));
        if (rr == 0) {
            fprintf(stderr, "EOF from compositor\n");
            break;
        }
        if (rr < 0) break;

        if (h.size < sizeof(h) || (h.size % 4) != 0) {
            fprintf(stderr, "Bad message size: %u\n", h.size);
            break;
        }

        size_t payload_len = (size_t)h.size - sizeof(h);
        uint8_t payload[4096];
        if (payload_len > sizeof(payload)) {
            fprintf(stderr, "Payload too big for demo buffer: %zu\n", payload_len);
            // You’d dynamically allocate here in real code.
            break;
        }

        if (payload_len) {
            rr = read_full(fd, payload, payload_len);
            if (rr <= 0) break;
        }

        // Decode registry globals
        if (h.object_id == registry_id) {
            if (h.opcode == 0) {
                // wl_registry.global(uint32 name, string interface, uint32 version)
                size_t off = 0;
                if (off + 4 > payload_len) continue;
                uint32_t name;
                memcpy(&name, payload + off, 4);
                off += 4;

                const char *iface = parse_string(payload, payload_len, &off);
                if (!iface) continue;

                if (off + 4 > payload_len) continue;
                uint32_t version;
                memcpy(&version, payload + off, 4);
                off += 4;

                printf("global: name=%u interface=%s version=%u\n", name, iface, version);
                globals_printed++;

                // For a first milestone, bail after “enough” output.
                if (globals_printed >= 15) break;
            } else if (h.opcode == 1) {
                // wl_registry.global_remove(uint32 name)
                if (payload_len >= 4) {
                    uint32_t name;
                    memcpy(&name, payload, 4);
                    printf("global_remove: name=%u\n", name);
                }
            }
        } else if (h.object_id == 1 && h.opcode == 0) {
            // wl_display.error(object_id, code, message)
            // We’re not parsing it fully here, but it’s useful to know.
            fprintf(stderr, "wl_display.error received (not decoded in demo)\n");
        }
    }

    close(fd);
    return 0;
}
