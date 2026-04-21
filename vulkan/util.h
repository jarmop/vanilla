#ifndef H_UTIL
#define H_UTIL

#include <stdio.h>

void printArray(int* arr, int len) {
  for (int i = 0; i < len - 1; i++) {
    fprintf(stderr, "%d\n", arr[i]);
  }
}

#endif