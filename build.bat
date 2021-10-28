call gcc -std=c11 src\*.c src\types\*.c -I./src/ -Wall -Wextra -o iris -g -flto -DIRIS_COLLECT_MEMORY_METRICS -Wl,-Bstatic -static-libgcc -lpthread
