# gcc -nostdlib -static -no-pie -Wl,-e,_start start.s main.c

#        -c   Compile and assemble but do not link
# -nostdlib   Don’t link crt files or libc (ld complains if gcc is run without this)
#   -static   Don’t depend on dynamic loader / shared libs
#   -no-pie   Avoids position-independent executable defaults on Ubuntu
gcc -c -nostdlib -static -no-pie start.s main.c

# -e _start   Set "_start" section as the entry point
ld -e _start main.o start.o

./a.out
echo $?
