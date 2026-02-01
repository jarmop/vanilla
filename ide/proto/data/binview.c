#include <stdio.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Give filename\n");
        return 1;
    }

    char *filename = argv[1];
    FILE *file = fopen(filename,"rb");

    size_t n;
    unsigned char buffer[16];
    while ((n = fread(buffer, sizeof(*buffer), sizeof(buffer), file)) > 0) {
        for(size_t i=0; i<n; i++) {
            printf("%02x", buffer[i]);
        }
    }
    
    return 0;
}