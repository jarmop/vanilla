void _start(void) {
    asm volatile(
        "mov $60, %rax\n"
        "xor %rdi, %rdi\n"
        "syscall\n"
    );
}