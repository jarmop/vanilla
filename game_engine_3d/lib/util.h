#include <stdio.h>
#include <cglm/cglm.h>

void print_vec(float *arr, int len) {
    for (int i = 0; i < len - 1; i++) {
        fprintf(stderr, "%f, ", arr[i]);
    }
    fprintf(stderr, "%f\n", arr[len - 1]);
}

void print_vec3(vec3 v) {
    print_vec(v, 3);
}