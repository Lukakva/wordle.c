wordle: main.o words.o threads.o
	gcc -Wall main.o words.o threads.o -lpthread -o wordle -O3

words.o:
	gcc -c src/words/words.c -o words.o -O3 

threads.o:
	gcc -c src/threads/threads.c -o threads.o -O3 

main.o:
	gcc -c src/main.c -o main.o -O3 

clean:
	rm *.o && rm wordle