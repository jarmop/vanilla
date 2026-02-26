#define _GNU_SOURCE
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>


int wl_fd;

/**
 * Wl_display is always present singleton object so it gets the id 1.
 * All the other objects are created by the server when needed.
 */
uint32_t wl_display_id = 1;
uint32_t registry_id = 0;
uint32_t wl_compositor_id = 0;
char *wl_compositor_iname = "wl_compositor";
uint32_t wl_shm_id = 0;
char *wl_shm_iname = "wl_shm";
uint32_t wl_surface_id = 0;
uint32_t wl_shm_pool_id = 0;

const uint32_t buffer_width = 800;
const uint32_t buffer_height = 1000;
const uint32_t pixel_size = 4;
uint32_t buffer_size = buffer_width * buffer_height * pixel_size;
uint32_t *buffer;

int new_id = 1;
int get_new_id() {
    new_id++;
    return new_id;
}

/**
 * Wl_display defines two requests (in /usr/share/wayland.xml), and get_registry 
 * is the second, so it gets opcode 1. (The first gets opcode 0.)
 */
int wl_display_get_registry_opcode = 1;

/**
 * Header structure for the message that is sent between the wayland client and 
 * the server.
 * https://wayland.freedesktop.org/docs/html/ch04.html#sect-Protocol-Wire-Format
 * 
 * __attribute__((__packed__)) tells gcc to place the struct members in a way 
 * that minimizes the memory usage. In other words, align the members based on 
 * the smallest member, 2 bytes in this case.
 */
struct __attribute__((__packed__)) message_header {
    uint32_t object_id;
    uint16_t opcode;
    uint16_t size;
};

/**
 * Round up value in fours (1 --> 4, 6 --> 8, 4 --> 4, etc.)
 * Adding 3u turns the lowest two bits into 1, and carries whatever value there 
 * was forward. Bitwise AND and NOT with 3u turns the lowest two bits (that are 
 * now 1) to zero. These two together effectively round up the value by four.
 * u = 32-bit unsigned integer
 * ~ = bitwise NOT
 * 3u = 00000000 00000000 00000000 00000011
 */ 
static size_t align4(size_t n) { return (n + 3u) & ~3u; }

void print_msg(uint8_t *msg, int msg_len) {
    for (int i = 0; i < msg_len; i++) {
        fprintf(stderr, "%x ", msg[i]);
    }
    fprintf(stderr, "\n");
}

/**
 * Create a socket and connect it to the wayland server socket on a path 
 * defined by the environment variables XDG_RUNTIME_DIR and WAYLAND_DISPLAY. 
 * WAYLAND_DISPLAY can also specify an absolute path in which case the 
 * XDG_RUNTIME_DIR should of course be ignored.
 */
static int connect_server(void) {
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
    strncpy(addr.sun_path, path, sizeof(addr.sun_path));

    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("connect");
        close(fd);
        return -1;
    }

    return fd;
}

/**
 * Request registry by sending "get_registry" msg to the wl_display. Usually 
 * done with send (wrapper to sendto) syscall, but can be done with the generic 
 * write syscall as well.
 */
static int get_registry() {
    long int sizeof_wl_header = sizeof(struct message_header);
    long int sizeof_id = sizeof(registry_id);
    long int sizeof_msg = sizeof_wl_header + sizeof_id; // 8 + 4
    struct message_header h;
    h.object_id = wl_display_id;
    h.opcode = 1; // opcode 1 = get_registry
    h.size = sizeof_msg;

    uint8_t msg[sizeof_msg];
    memcpy(msg, &h, sizeof_wl_header);
    memcpy(msg + sizeof_wl_header, &registry_id, sizeof_id);

    ssize_t n = write(wl_fd, msg, sizeof_msg);
    if (n != (ssize_t)sizeof_msg) {
        perror("get_registry");
        return -1;
    }
    return 0;
}

// Read server response from the socket
static int read_message(int fd, uint8_t *buf, size_t n) {
    size_t got = 0;
    while (got < n) {
        // ssize_t r = recv(fd, buf + got, n - got, 0);
        ssize_t r = read(fd, buf + got, n - got);
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

static const char *parse_payload(
    const uint8_t *payload, 
    size_t payload_len, 
    size_t *offset
) {
    size_t off = *offset;
    if (off + 4 > payload_len) return NULL;

    uint32_t len;
    memcpy(&len, payload + off, 4);
    off += 4;

    if (len == 0) {
        // empty string is encoded as len=0 (rare) or len=1 with NUL; 
        // treat 0 as invalid here
        return NULL;
    }
    if (off + len > payload_len) return NULL;

    const char *s = (const char *)(payload + off); // includes NUL terminator
    off += align4((size_t)len);

    if (off > payload_len) return NULL;
    *offset = off;
    return s;
}

static int registry_bind(u_int32_t name, const char *interface_name, uint32_t interface_version, uint32_t id) {
    long int sizeof_header = sizeof(struct message_header);

    // Include the null byte in length
    uint32_t interface_name_len = strlen(interface_name) + 1;
    long int str_len = align4(sizeof(uint32_t) + interface_name_len);
    uint8_t str[str_len];
    memset(str, 0, str_len);
    memcpy(str, &interface_name_len, 4);
    memcpy(str + 4, interface_name, interface_name_len);

    long int sizeof_payload = 4 + str_len + 4 + 4;
    uint8_t p[sizeof_payload];
    memcpy(p, &name, 4);
    memcpy(p + 4, str, str_len);
    memcpy(p + 4 + str_len, &interface_version, 4);
    memcpy(p + 4 + str_len + 4 , &id, 4);

    long int sizeof_msg = sizeof_header + sizeof_payload;
    struct message_header h;
    h.object_id = registry_id;
    h.opcode = 0; // opcode 1 = get_registry
    h.size = sizeof_msg;

    uint8_t msg[sizeof_msg];
    memcpy(msg, &h, sizeof_header);
    memcpy(msg + sizeof_header, &p, sizeof_payload);

    ssize_t n = write(wl_fd, msg, sizeof_msg);
    if (n != (ssize_t)sizeof_msg) {
        perror("send(registry_bind)");
        return -1;
    }

    // fprintf(stderr, "Bind %s: wrote %ld bytes \n", interface_name, n);
    // print_msg(msg, sizeof_msg);

    return 0;
}

static int wl_compositor_create_surface() {
    wl_surface_id = get_new_id();
    long int sizeof_header = sizeof(struct message_header);
    long int sizeof_id = sizeof(wl_surface_id);
    long int sizeof_msg = sizeof_header + sizeof_id;
    struct message_header h;
    h.object_id = wl_compositor_id;
    h.opcode = 0;
    h.size = sizeof_msg;

    uint8_t msg[sizeof_msg];
    memcpy(msg, &h, sizeof_header);
    memcpy(msg + sizeof_header, &wl_surface_id, sizeof_id);

    ssize_t n = write(wl_fd, msg, sizeof_msg);
    if (n != (ssize_t)sizeof_msg) {
        perror("wl_compositor_create_surface");
        return -1;
    }

    // fprintf(stderr, "Create surface: wrote %ld bytes \n", n);
    // print_msg(msg, sizeof_msg);

    return 0;
}

/**
 * Send fd as ancillary data.
 */
static int send_with_fd(int sock, const void *wl_msg, size_t len, int fd_to_send) {
    struct iovec iov = {
        .iov_base = (void*)wl_msg,
        .iov_len = len,
    };

    size_t fd_size = sizeof(int);

    /**
     * Union with a member of type cmsghdr makes sure that the cmsgbuf is 
     * aligned suitably for the struct cmsghdr (though this is rarely, if ever, 
     * an issue on x86-64).
     */
    union {
        unsigned char buf[CMSG_SPACE(fd_size)]; // size of the fd_to_send
        struct cmsghdr _;
    } cmsgbuf;
    memset(&cmsgbuf, 0, sizeof(cmsgbuf));

    struct msghdr msg;
    memset(&msg, 0, sizeof(msg));
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    msg.msg_control = cmsgbuf.buf;
    msg.msg_controllen = sizeof(cmsgbuf.buf);

    struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type  = SCM_RIGHTS;
    cmsg->cmsg_len   = CMSG_LEN(fd_size);

    memcpy(CMSG_DATA(cmsg), &fd_to_send, fd_size);

    // Some kernels want msg_controllen to match exactly the used length:
    // msg.msg_controllen = cmsg->cmsg_len;

    ssize_t n = sendmsg(sock, &msg, MSG_NOSIGNAL);
    if (n < 0) return -1;
    if ((size_t)n != len) return -2;

    // fprintf(stderr, "send_with_fd: wrote %ld bytes \n", n);

    return 0;
}

static int wl_shm_create_pool() {
    wl_shm_pool_id = get_new_id();
    int shm_fd = memfd_create("shm-buffer", 0);
    buffer = mmap(NULL, (size_t)buffer_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    int sizeof_payload = 12;
    uint8_t payload[sizeof_payload];
    memcpy(payload, (uint8_t *)&wl_shm_pool_id, 4);
    memcpy(payload + 4, (uint8_t *)&shm_fd, 4);
    memcpy(payload + 8, (uint8_t *)&buffer_size, 4);

    long int sizeof_header = sizeof(struct message_header);
    long int sizeof_msg = sizeof_header + sizeof_payload;
    struct message_header h;
    h.object_id = wl_shm_id;
    h.opcode = 0;
    h.size = sizeof_msg;

    uint8_t msg[sizeof_msg];
    memcpy(msg, &h, sizeof_header);
    memcpy(msg + sizeof_header, payload, sizeof_payload);

    // print_msg(msg, sizeof_msg);

    int failed = send_with_fd(wl_fd, msg, sizeof_msg, shm_fd);
    if (failed) {
        perror("wl_shm_create_pool");
        return -1;
    }
    return 0;
}

int main(void) {

    wl_fd = connect_server();
    if (wl_fd < 0) return 1;

    registry_id = get_new_id();
    if (get_registry(wl_fd, registry_id) < 0) {
        close(wl_fd);
        return 1;
    }

    // for (int i=0; i<1; i++) {
    for (;;) {
        // First read the header to get the size of the payload 
        struct message_header h;
        int bytes_read = read_message(wl_fd, (uint8_t *)&h, sizeof(h));
        if (bytes_read == 0) {
            fprintf(stderr, "EOF from compositor\n");
            break;
        }
        if (bytes_read < 0) break;

        // fprintf(stderr, "----------\n");
        // fprintf(stderr, "header: ");
        // print_msg((uint8_t *)&h, sizeof(h));

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

        // Read the payload
        if (payload_len) {
            bytes_read = read_message(wl_fd, payload, payload_len);
            if (bytes_read <= 0) break;
        }

        // fprintf(stderr, "payload_len: %ld\n", payload_len);
        // fprintf(stderr, "payload:\n");
        // print_msg(payload, payload_len);

        // Decode registry globals
        if (h.object_id == registry_id) {
            if (h.opcode == 0) {
                // wl_registry.global(uint32 name, string interface, uint32 version)
                size_t off = 0;
                if (off + 4 > payload_len) continue;
                uint32_t name;
                memcpy(&name, payload + off, 4);
                off += 4;

                const char *interface = parse_payload(payload, payload_len, &off);
                if (!interface) continue;

                if (off + 4 > payload_len) continue;
                uint32_t version;
                memcpy(&version, payload + off, 4);
                off += 4;

                printf("%u\tv%u\t%s\n", name, version, interface);

                if (strcmp(interface, wl_compositor_iname) == 0) {
                    wl_compositor_id = get_new_id();
                    registry_bind(name, interface, version, wl_compositor_id);
                    wl_compositor_create_surface();
                } else if (strcmp(interface, wl_shm_iname) == 0) {
                    wl_shm_id = get_new_id();
                    registry_bind(name, interface, version, wl_shm_id);
                    wl_shm_create_pool();
                }
                
            } else if (h.opcode == wl_display_id) {
                // wl_registry.global_remove(uint32 name)
                if (payload_len >= 4) {
                    uint32_t name;
                    memcpy(&name, payload, 4);
                    printf("global_remove: name=%u\n", name);
                }
            }
        } else if (h.object_id == 1 && h.opcode == 0) {
            int error_object_id = *(uint32_t *)payload;
            int error_code = *(uint32_t *)(payload + 4);
            int error_msg_len = *(uint32_t *)(payload + 8);
            char *error_msg = malloc(error_msg_len);
            strcpy(error_msg, (char *)(payload + 12));

            fprintf(
                stderr, 
                "Error – object_id: %d, error_code: %d, error_msg: %s \n", 
                error_object_id, error_code, error_msg
            );
            free(error_msg);
        }
    }

    close(wl_fd);
    return 0;
}
