call gcc src\main.c src\memory.c src\reader.c src\utils.c src\types\types.c src\types\list.c src\types\dict.c src\types\string.c -flto -g -DIRIS_COLLECT_MEMORY_METRICS -I./src/
