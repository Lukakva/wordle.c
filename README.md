# About

A **multi-threaded C-based** solution to Matt Parker's Wordle problem. Solves the problem in less than 2 seconds (On an M1 Pro).

```
$ ./wordle -t 10 -w 8
Loading words done...
Encountered 218 hashmap collisions while filtering out anagrams.
Left with 4387 usable words out of the 12947 total words in the file.

Starting processing: max_threads = 10, words_per_thread = 8

thread #039   chunk[0312-0320]: bemix clunk grypt vozhd waqfs
thread #055   chunk[0440-0448]: blunk cimex grypt vozhd waqfs
thread #051   chunk[0408-0416]: bling jumpy treck vozhd waqfs
thread #068   chunk[0544-0552]: brick glent jumpy vozhd waqfs
thread #071   chunk[0568-0576]: brung cylix kempt vozhd waqfs
thread #103   chunk[0824-0832]: chunk fjord gymps vibex waltz
thread #110   chunk[0880-0888]: clipt jumby kreng vozhd waqfs
thread #202   chunk[1616-1624]: fjord gucks nymph vibex waltz
thread #241   chunk[1928-1936]: glent jumby prick vozhd waqfs
thread #306   chunk[2448-2456]: jumby pling treck vozhd waqfs

Finished after 11,844,572,947 iterations in 1672.35 milliseconds
```

# Problem

https://www.youtube.com/watch?v=_-AfhLQfb6w

Brief: Given a `wordle` database, containing ~13,000 five-letter words,
find all combinations of 5 five-letter words which have a total of 25 unique characters.
In other words, 5 words, 25 unique letters in total. Therefore no duplicate letters.

# Solution

Since there 26 letters in the alphabet, we can use a 32bit bit-field to represent
what characters each word contains.

For example:
`abcde` = 1st, 2nd, 3rd, 4th, and 5th bits are set to 1 (from `lsb` to `msb`).
`hello` = 8th, 5th, 12th, and 14th bits are set to 1. Only 4 bits are set for hello, which will be handy later.

This allows us to very efficiently filter out words that:

**1. Contain duplicate characters.**
Words that have a bitcount of `< 5` are filtered out.

**2. Contain more than 2 vowels (y included).**
A simple bitmask `0b00000001000100000100000100010001` represents all of the vowels.
If a word contains more than 3 vowels, there are no vowels left for
the other remaining 4 words, therefore this word could never end up in our final solution.

**3. Are anagrams.**
`scald = clads, nails = snail`. Anagrams achieve the same result and are interchangeable in the solution, however trimming extra words drastically reduces necessary iterations.
Anagrams have the same numeric representation, therefore are easy to spot.

## Numeric representation

Having numeric representations makes comparisons extremely fast and easy.

```c
uint32_t fjord = 0b00000000000000100100001000101000;
uint32_t clunk = 0b00000000000100000010110000000100;
uint32_t cards = 0b00000000000001100000000000001101;

assert((fjord & bemix) == 0); // 0 letter overlap
assert((clunk & cards) == 4); // 2^2, 3rd letter, C
```

This allows us to select a second word. Third word needs to be collision-free with both 1st and the 2nd words, and that comparison is just as easy.

```c
uint32_t fjord = 0b00000000000000100100001000101000;
uint32_t clunk = 0b00000000000100000010110000000100;
uint32_t fjord_clunk = fjord | clunk;

// Both words together, now we can use fjord_clunk & word
assert((fjord_clunk) == 0b00000000000100100110111000101100);
```

## Optimization

In order to better eliminate combinations and skip over some iterations, every word `a` gets an array of all the possible other words that don't share letters with `a`. This way, if a certain word `a` is chosen to be in a first position, only the words that don't overlap with `a` are checked.
For every such word `b`, another list is used which enables us to find a word `c` which has no overlap with `b`. Therefore, the only thing that is needed to be checked is wether `c` overlaps with `a`, and so on. For every word at position `n`, the word definitely doesn't overlap with the word at `n-1`, however it may overlap with words at `1..n-2`.

# Multithreading

Initially I divided the search into `n` pieces and give them to `n` threads. Some threads finished much earlier, since some regions contain words that are much easier to determine as useless, and can be skipped.

Now every thread gets a small fixed range to go through. Once a thread finishes its chunk, a new thread is created to start working on the next unexplored chunk. Number of threads is always kept at maximum.

The program accepts arguments to play around with the number of threads and how many combinations a single thread should check. The search is divided by telling a thread how many words it should check as a first-word. Meaning, if a thread should only check some arbitrary `8` words, it will check all 5 word combinations where the first word is either the 1st, 2nd, 3rd, ..., or 8th word given to the thread.

# Running

### Compiling

```
git clone https://github.com/Lukakva/wordle.c
cd wordle.c && make
```

### Usage

```
$ ./wordle -h
usage: ./wordle [-t thread] [-w words_per_thread] [-s] [-h]

-h help

-s silent mode, only print the results

-t threads
    max number of threads running at a time

-w words_per_thread
    number words each thread should check.
    -w 8 would tell thread to pick 8 words and try
    all combinations where either of these 8 words are the word #1
```
