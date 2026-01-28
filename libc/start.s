.global _start
.type _start, @function

_start:
    xor %rbp, %rbp            # conventional: clear frame pointer

    mov (%rsp), %rdi          # rdi = argc
    lea 8(%rsp), %rsi         # rsi = argv = &argv[0]

    # envp = &argv[argc+1]
    lea 16(%rsp,%rdi,8), %rdx # rdx = envp

    call main                 # main(argc, argv, envp)

    # exit(main_return_value)
    mov %eax, %edi            # status
    mov $60, %eax             # SYS_exit
    syscall
.size _start, .-_start

# tell linker that an excutable stack is not needed
.section .note.GNU-stack,"",@progbits
