call gcc -std=c11 src\*.c src\types\*.c -I./src/ -Wall -Wextra -g -o iris -DIRIS_COLLECT_MEMORY_METRICS -Wl,-Bstatic -static-libgcc -lpthread
