#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include "set.h"
#include "word_validator.h"

static const int MAX_WORD_LEN = 1024;
static struct set *word_set;

static const int MAX_SUBSTR_LEN = 3;
static char *substrs;
static int substrs_len;

char *index_substrs(int index) {
    if (index >= substrs_len) {
        fprintf(stderr, "index (%d) out of bounds in index_substrs()\n", index);
        return NULL;
    }
    return substrs + (index * (MAX_SUBSTR_LEN + 1));
}

void initialize_substr_list(char *filepath) {
    int lines = 0;
    FILE *fptr = fopen(filepath, "r");
    if (fptr == NULL) {
        fprintf(stderr, "failed to open file at %s in initialize_substr_list() (%d)\n", filepath, errno);
        return;
    }
    while (!feof(fptr)) {
        char ch = fgetc(fptr);
        if (ch == '\n') {
            lines++;
        }
    }
    substrs_len = lines;

    substrs = malloc(lines * 4);
    rewind(fptr);
    char temp_buf[5];
    for(int i = 0; i < lines; i++) {
        fgets(temp_buf, 5, fptr);
        *strchr(temp_buf, '\n') = '\0';
        strcpy(index_substrs(i), temp_buf);
    }
}

// This function assumes srand() has already been called
char *get_random_required_substr() {
    return index_substrs(rand() % substrs_len);
}

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
