#include "set.h"
#include "word_validator.h"
#include <stdio.h>

int main() {
    printf("Hash testing:\n");
    char *test_words[5] = {
        "hello",
        "word",
        "blast",
        "chocolate",
        "orange",
    };

    for (int i = 0; i < 5; i++) {
        printf("%lu\n", djb2_hash(test_words[i]));
    }


    printf("Set testing:\n");
    struct set *s = set_create(100000);
    set_insert(s, "apple");
    set_insert(s, "banana");

    printf("Set size: %lu\n", set_get_size(s));

    if (set_contains(s, "apple")) {
        printf("Set contains apple\n");
    } else {
        printf("ERROR set does not contain apple");
    }

    if (set_contains(s, "banana")) {
        printf("Set contains banana\n");
    } else {
        printf("ERROR set does not contain banana");
    }

    if (set_contains(s, "orange")) {
        printf("ERROR set contains orange\n");
    } else {
        printf("set does not contain orange\n");
    }

    if (set_contains(s, "appll")) {
        printf("ERROR set contains appll\n");
    } else {
        printf("set does not contain appll\n");
    }

    printf("Word validator testing:\n");
    initialize_wordlist("wordlist.txt", 113809);
    printf("word: mountain, substr: in, isvaild: %d\n", word_is_valid("mountain", "in"));
    printf("word: thisisnotaword, substr: hello, isvaild: %d\n", word_is_valid("thisisnotaword", "hello"));
    printf("word: river, substr: no, isvaild: %d\n", word_is_valid("river", "no"));
    printf("word: river, substr: ri, isvaild: %d\n", word_is_valid("river", "ri"));

    return 0;
}
