#ifndef H_UTIL
#define H_UTIL

#include <stdio.h>

void printArray(int* arr, int len) {
  for (int i = 0; i < len; i++) {
    fprintf(stderr, "%d\n", arr[i]);
  }
}

void printArrayF(float* arr, int len) {
  for (int i = 0; i < len; i++) {
    fprintf(stderr, "%f\n", arr[i]);
  }
}

#endif