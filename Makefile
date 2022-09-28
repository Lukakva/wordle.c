main:
	gcc src/main.c src/words/words.c src/threads/threads.c -o main -O3 -lpthread