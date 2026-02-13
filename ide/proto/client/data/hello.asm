; out.asm — minimal ELF64 x86-64 executable that prints one byte and exits

BITS 64
ORG 0x400000

; 64 bytes
ehdr:
    db 0x7f, "ELF"        ;4 e_ident[0..3]
    db 2                  ;5 EI_CLASS = ELFCLASS64
    db 1                  ;6 EI_DATA  = little endian
    db 1                  ;7 EI_VERSION = EV_CURRENT
    db 0                  ;8 EI_OSABI = System V

    db 0                  ;9 EI_ABIVERSION
    times 7 db 0          ;16 padding to 16 bytes

    dw 2                  ;2 e_type = ET_EXEC
    dw 62                 ;4 e_machine = EM_X86_64
    dd 1                  ;8 e_version = EV_CURRENT

    dq _start             ;16 e_entry
    
    dq phdr - $$          ;8 e_phoff
    dq 0                  ;16 e_shoff (no section headers)

    dd 0                  ;4 e_flags
    dw ehdrsize           ;6 e_ehsize
    dw phdrsize           ;8 e_phentsize

    dw 1                  ;10 e_phnum
    dw 0                  ;12 e_shentsize
    dw 0                  ;14 e_shnum
    dw 0                  ;16 e_shstrndx

ehdrsize equ $ - ehdr

phdr:
    ; row 5
    dd 1                  ;4 p_type = PT_LOAD
    dd 5                  ;8 p_flags = PF_R | PF_X
    dq 0                  ;16 p_offset
    
    ; row 6
    dq $$                 ;8 p_vaddr  (0x400000)
    dq $$                 ;16 p_paddr  (ignored)
    
    ; row 7
    dq filesize           ;8 p_filesz
    dq filesize           ;16 p_memsz
    
    ; row 8
    dq 0x1000             ;8 p_align

phdrsize equ $ - phdr

_start:
    ; write(1, msg, 1)
    mov eax, 1            ;5 --> 13 sys_write
    mov edi, 1            ;5 --> 18 --> 2 fd = stdout
    
    ; row 9, previous instruction overflows to the first 2 bytes
    lea rsi, [rel msg]    ;7 --> 9 [rel, msg] => NASM computes the distance from the current
                          ; instruction's address to the address of the 'X' (16 bytes)
    mov edx, 15           ;5 --> 14 ("Hello, World!\n" is 15 bytes)
    syscall               ;2 --> 16

    ; row 10
    ; exit(0)
    mov eax, 60           ;5 sys_exit
    xor edi, edi          ;2 --> 7 status = 0
    syscall               ;2 --> 9

msg:
    db 'Hello, world!', 13, 10  ;15 --> 24

filesize equ $ - $$
