typedef unsigned long usize;

#define stdin 0
#define stdout 1
#define stderr 2

char* strcpy(char *d, const char *s) {
    for(; (*d=*s); d++, s++);    
    return d;
}

usize strlen(const char *str) {
    int i = 0;
    while (str[i] != '\0' && i < 100000) {
        i++;
    }
    return i;
}

int fprint(int fd, const char *str) {
    usize len = strlen(str);
    long ret;
    register long rax asm("rax") = 1; // syscall number goes to rax
    register long rdi asm("rdi") = fd; // rdi = arg 1
    register long rsi asm("rsi") = (long)str; // rsi = arg 2
    register long rdx asm("rdx") = (long)len; // rdx = arg 3
    asm volatile ("syscall"
        : "=a"(ret)
        : "a"(rax), "D"(rdi), "S"(rsi), "d"(rdx)
        : "rcx", "r11", "memory");
    return ret;
}

int print(const char *buf) {
    fprint(stdout, buf);
}

int println(const char *str) {
    char strn[strlen(str) + 1];
    int i = 0;
    for (; str[i] != '\0' && i < 100000; i++) {
        strn[i] = str[i];
    }
    strn[i] = '\n';
    strn[i + 1] = '\0';
    return print(strn);

    // print(str);
    // print("\n");
}

// TYPEDEF __builtin_va_list va_list;
// TYPEDEF __builtin_va_list __isoc_va_list;

typedef __builtin_va_list va_list;
typedef __builtin_va_list __isoc_va_list;


#define va_start(v,l)   __builtin_va_start(v,l)
#define va_end(v)       __builtin_va_end(v)
#define va_arg(v,l)     __builtin_va_arg(v,l)
#define va_copy(d,s)    __builtin_va_copy(d,s)
#define max_int_len 10

/**
 * TODO
 * - handle negative numbers
 */
int num_to_string(int n, char* buf) {
    int i = 0;
    char tmp[max_int_len];
    do {
        int remainder = n%10;
        n = n/10;
        tmp[i] = '0' + remainder;
        i++;
    } while (n > 0);

    int size = i;
    for (int j = 0; j < size; j++) {
        buf[j] = tmp[size - j - 1];
    }

    return size;
}

/**
 * TODO
 * - handle multiple arguments
 * - handle all types of arguments
 */
int printfmt(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);

    char buf[1024];
    char *b = buf;
    for (char *p = fmt; *p; p++) {
        if (*p != '%') {
            *b = *p;
            b++;
            continue;
            // Add to the string
        }
        p++;
        switch (*p) {
            case 'd':
                b += num_to_string(va_arg(ap, int), b);
                break;
            
            default:
                b += va_arg(ap, int);
                break;
            }
    }

    println(buf);
}