call gcc src\*.c src\types\*.c -I./src/ -Wall -Wextra -g -o iris -DIRIS_COLLECT_MEMORY_METRICS -static-libgcc
