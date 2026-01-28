#include "libc.h"

// int main(int argc, char **argv, char **envp) {
//     (void)argc; (void)argv; (void)envp;
int main() {
    const char msg[] = "hello from no-libc!";
    println(msg);

    printfmt("num: %d, yay", 7);
    
    return 0;
}
