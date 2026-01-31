#include <sys/timerfd.h>
#include <stdio.h>
#include <poll.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

#define ms_to_ns(ms) (ms * 1000 * 1000)

int main() {
    // int repeat_fd = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC | TFD_NONBLOCK);
    int repeat_fd = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC);
    if (repeat_fd < 0) { 
        printf("timerfd_create: error\n");
        return 1; 
    }

    // Arm
    struct itimerspec its;
    memset(&its, 0, sizeof its);
    its.it_value.tv_sec = 1;
    // its.it_interval.tv_sec = 1;
    its.it_interval.tv_nsec = ms_to_ns(500);
    if (timerfd_settime(repeat_fd, 0, &its, NULL) < 0) {
        printf("timerfd_settime: error\n");
        return 1;
    }

    int number_of_fds = 1;
    int timeout = -1; // block until an event occurs
    // int timeout = 1000;

    struct pollfd fds[number_of_fds];
    fds[0].fd = repeat_fd;
    fds[0].events = POLLIN;

    for (;;) {
        fprintf(stderr, "start polling: %d\n", POLLIN);

        int ret = poll(fds, number_of_fds, timeout);

        // fprintf(stderr, "ret: %d\n", ret);


        if (ret < 0) {
            printf("poll: error\n");
            return 1;
        }


        if (fds[0].revents & POLLIN) {
            fprintf(stderr, "time: %ld\n", (long)time(NULL));

            uint64_t expirations = 0;
            if (read(repeat_fd, &expirations, sizeof expirations) == sizeof expirations) {
                // fprintf(stderr, "time: %ld (exp=%llu)\n",
                //         (long)time(NULL), (unsigned long long)expirations);
            }
        }
    }


    return 0;
}