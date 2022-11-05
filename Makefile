CC := gcc
CCFLAGS := -Wall -O3

wordle: main words threads
	$(CC) $(CCFLAGS) main.o words.o threads.o -lpthread -o wordle

words:
	$(CC) $(CCFLAGS) -c src/words/words.c -o words.o

threads:
	$(CC) $(CCFLAGS) -c src/threads/threads.c -o threads.o

main:
	$(CC) $(CCFLAGS) -c src/main.c -o main.o

clean:
	rm -f *.o wordle