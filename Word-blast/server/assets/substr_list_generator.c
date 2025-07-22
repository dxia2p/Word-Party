#include <stdio.h>
#include <string.h>
#include "counting_set.h"

const int MAX_WORD_LEN = 512; 
const int OCCURENCES_THRESHOLD = 30;  // For a substring to be added to the list it must occur more than this amount of times
const char ALPHABET[] = "abcedfghijklmnopqrstuvwxyz";
const int ALPHABET_LEN = 26;

struct c_set *substr_set;

void get_substrs_from_word(char *word, int substr_len) {
    int word_len = strlen(word);
    if (word_len < substr_len) return;

    char temp[5];
    for (int i = 0; i < word_len - substr_len; i++) {
        for(int j = 0; j < substr_len; j++) {
            temp[j] = word[i + j];
        }
        temp[substr_len] = '\0';
        c_set_insert(substr_set, temp);
    }
}

int main() {
    substr_set = c_set_create(15000);
    
    FILE *wordlist_fptr = fopen("wordlist2.txt", "r");
    if (wordlist_fptr == NULL) {
        fprintf(stderr, "fopen() failed.\n");
        return 0;
    }

    char buf[MAX_WORD_LEN];
    while (fgets(buf, MAX_WORD_LEN, wordlist_fptr) != NULL) {
        int len = strlen(buf);
        if (len > 0 && buf[len - 1] == '\n'){
            buf[strlen(buf) - 1] = '\0';  // Remove the newline
        }
        len = strlen(buf);
        if(len > 0 && buf[len - 1] == '\r') {
            buf[strlen(buf) - 1] = '\0';  // Remove the carriage return 
        }
        get_substrs_from_word(buf, 2);
        get_substrs_from_word(buf, 3);
    }

    
    FILE *substr_list_fptr = fopen("substr_list.txt", "w");
    if (substr_list_fptr == NULL) {
        fprintf(stderr, "fopen() failed.\n");
        return 0;
    }

    // Length 2 substrings
    char temp[5];
    for(int i = 0; i < ALPHABET_LEN; i++) {
        for(int j = 0; j < ALPHABET_LEN; j++) {
            temp[0] = ALPHABET[i];
            temp[1] = ALPHABET[j];
            temp[2] = '\0';
            if (c_set_get_count(substr_set, temp) >= OCCURENCES_THRESHOLD) {
                temp[2] = '\n';
                temp[3] = '\0';
                fputs(temp, substr_list_fptr);
            }
        }
    }

    // Length 3 substrings
    for (int i = 0; i < ALPHABET_LEN; i++) {
        for (int j = 0; j < ALPHABET_LEN; j++) {
            for(int k = 0; k < ALPHABET_LEN; k++) {
                temp[0] = ALPHABET[i];
                temp[1] = ALPHABET[j];
                temp[2] = ALPHABET[k];
                temp[3] = '\0';
                if (c_set_get_count(substr_set, temp) >= OCCURENCES_THRESHOLD) {
                    temp[3] = '\n';
                    temp[4] = '\0';
                    fputs(temp, substr_list_fptr);
                }
            }
        }
    }




    fclose(substr_list_fptr);
    fclose(wordlist_fptr);

    /*
    const char VOWELS[] = "aeiou";
    const char CONSONANTS[] = "bcdfghjklmnpqrstvwxyz";
    const int VOWELS_LEN = 5;
    const int CONSONANTS_LEN = 21;
    FILE *fptr = fopen("substr_list2.txt", "w");

    // substrings with 1 consonant 1 vowel
    char temp[5];
    for (int i = 0; i < VOWELS_LEN; i++) {
        for(int j = 0; j < CONSONANTS_LEN; j++) {
            temp[0] = VOWELS[i];
            temp[1] = CONSONANTS[j];
            temp[2] = '\n';
            temp[3] = '\0';
            fputs(temp, fptr);
            temp[0] = CONSONANTS[j];
            temp[1] = VOWELS[i];
            fputs(temp, fptr);
        }
    }
    // substrings with 2 vowels
    for (int i = 0; i < VOWELS_LEN; i++) {
        for (int j = 0; j < VOWELS_LEN; j++) {
            if (VOWELS[i] == 'u' && VOWELS[j] == 'u') continue;
            if (VOWELS[i] == 'i' && VOWELS[j] == 'i') continue;

            temp[0] = VOWELS[i];
            temp[1] = VOWELS[j];
            temp[2] = '\n';
            temp[3] = '\0';
            fputs(temp, fptr);
        }
    }
    // substrings with 2 consonants 1 vowel
    for (int i = 0; i < VOWELS_LEN; i++) {
        for (int j = 0; j < CONSONANTS_LEN; j++) {
            for (int k = 0; k < CONSONANTS_LEN; k++) {
                if (CONSONANTS[j] == 'z' && CONSONANTS[k] == 'z') continue;
                if (CONSONANTS[j] == 'x' && CONSONANTS[k] == 'x') continue;
                if ((CONSONANTS[j] == 'x' && CONSONANTS[k] == 'z') || (CONSONANTS[j] == 'z' && CONSONANTS[k] == 'x')) continue;
                if ((CONSONANTS[j] == 'z' && CONSONANTS[k] == 'y') || (CONSONANTS[j] == 'y' && CONSONANTS[k] == 'z')) continue;
                temp[0] = VOWELS[i];
                temp[1] = CONSONANTS[j];
                temp[2] = CONSONANTS[k];
                temp[3] = '\n';
                temp[4] = '\0';
                fputs(temp, fptr);
                temp[0] = CONSONANTS[j];
                temp[1] = VOWELS[i];
                temp[2] = CONSONANTS[k];
                fputs(temp, fptr);
                temp[0] = CONSONANTS[j];
                temp[1] = CONSONANTS[k];
                temp[2] = VOWELS[i];
                fputs(temp, fptr);
            }
        }
    }
    // substrings with 1 consonant 2 vowel
    for (int i = 0; i < VOWELS_LEN; i++) {
        for (int j = 0; j < VOWELS_LEN; j++) {
            for (int k = 0; k < CONSONANTS_LEN; k++) {
                if (VOWELS[i] == 'u' && VOWELS[j] == 'u') continue;
                if (VOWELS[i] == 'i' && VOWELS[j] == 'i') continue;
                if (VOWELS[i] == 'a' && VOWELS[j] == 'a') continue;
                if (VOWELS[i] == 'u' && VOWELS[j] == 'o') continue;
                if (VOWELS[i] == 'i' && VOWELS[j] == 'u') continue;
                if (CONSONANTS[k] == 'z' || CONSONANTS[k] == 'x') continue;
                temp[0] = CONSONANTS[k];
                temp[1] = VOWELS[i];
                temp[2] = VOWELS[j];
                temp[3] = '\n';
                temp[4] = '\0';
                fputs(temp, fptr);
                temp[0] = VOWELS[i];
                temp[1] = CONSONANTS[k];
                temp[2] = VOWELS[j];
                fputs(temp, fptr);
                temp[0] = VOWELS[i];
                temp[1] = VOWELS[j];
                temp[2] = CONSONANTS[k];
                fputs(temp, fptr);
            }
        }
    }

    fclose(fptr);
    return 0;
    */
}
