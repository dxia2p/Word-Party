#include <stdio.h>
#include <string.h>
#include "set.h"
#include "word_validator.h"

static const int MAX_WORD_LEN = 1024;
static struct set *word_set;

// Words are separated by newline
void initialize_wordlist(char *filepath, unsigned long lines_in_wordlist) {
    
    FILE *fptr = fopen(filepath, "r");
    if (fptr == NULL) {
        fprintf(stderr, "fopen() in initialize_wordlist() failed.\n");
        return;
    }
    word_set = set_create((unsigned long)(lines_in_wordlist * 1.3));
    char buf[MAX_WORD_LEN];

    while (fgets(buf, MAX_WORD_LEN, fptr) != NULL) {
        int len = strlen(buf);
        if (len > 0 && buf[len - 1] == '\n'){
            buf[strlen(buf) - 1] = '\0';  // Remove the newline
        }
        len = strlen(buf);
        if(len > 0 && buf[len - 1] == '\r') {
            buf[strlen(buf) - 1] = '\0';  // Remove the carriage return 
        }
        set_insert(word_set, buf);
    }

    printf("Initialized wordlist. (size: %lu)\n", set_get_size(word_set));
}

bool word_is_valid(char *word, char *substr) {
    
    if (strstr(word, substr) == NULL) {  // The word must contain the substring specified
        return false;
    }
    

    if (set_contains(word_set, word)) {
        return true;
    }
    return false;
}
