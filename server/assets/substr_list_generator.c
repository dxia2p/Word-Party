#include <stdio.h>

int main() {
    const char VOWELS[] = "aeiou";
    const char CONSONANTS[] = "bcdfghjklmnpqrstvwxyz";
    const int VOWELS_LEN = 5;
    const int CONSONANTS_LEN = 21;
    FILE *fptr = fopen("substr_list.txt", "w");

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
}
