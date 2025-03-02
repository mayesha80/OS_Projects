CC=/bin/gcc

all: matmul matgen

matmul: matmul.c matmul.h matmul_core.c  matmul_multi_proc.c matmul_multi_thread.c
	$(CC) matmul.c matmul_core.c  matmul_multi_proc.c  matmul_multi_thread.c -g -o matmul -lpthread

matgen: matgen.c
	$(CC) matgen.c -g -o matgen

clean:
	rm -f wc matmul matgen

