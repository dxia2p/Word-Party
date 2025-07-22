#ifndef WORD_VALIDATOR_H
#define WORD_VALIDATOR_H
#include <stdbool.h>

char *index_substrs(int index);
bool initialize_substr_list(char *filepath);
char *get_random_required_substr();
bool initialize_wordlist(char *filepath);
bool word_is_valid(char *word, char *substr);

#endif
