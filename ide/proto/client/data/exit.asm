BITS 64
ORG 0x400000

; ELF header
ehdr:
    db 0x7f,"ELF"
    db 2,1,1,0
    times 8 db 0
    dw 2              ; ET_EXEC
    dw 0x3e           ; x86-64
    dd 1
    dq _start         ; entry
    dq phdr - $$      ; program header offset
    dq 0
    dd 0
    dw ehdrsize
    dw phdrsize
    dw 1              ; one program header
    dw 0
    dw 0
    dw 0

ehdrsize equ $ - ehdr

; Program header describes a memory segment to load
phdr:
    dd 1              ; PT_LOAD
    dd 5              ; R + X
    dq 0
    dq $$             ; vaddr
    dq $$             ; paddr
    dq filesize
    dq filesize
    dq 0x1000

phdrsize equ $ - phdr

_start:
    mov eax, 60
    xor edi, edi
    syscall

filesize equ $ - $$
