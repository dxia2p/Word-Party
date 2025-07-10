#ifndef WORD_VALIDATOR_H
#define WORD_VALIDATOR_H
#include <stdbool.h>

void initialize_wordlist(char *filepath, unsigned long lines_in_wordlist);
bool word_is_valid(char *word, char *substr);

#endif
