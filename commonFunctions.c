/*

Joao Pedro Morais Gaspar          nº2021221276
Guilherme Craveiro de Melo     nº 2021217138

*/
#include "commonFunctions.h"

int verify_alphanumeric(short int permit_underscore, const char* phrase) {
    int phrase_size = strlen(phrase);

    if (phrase_size < 3 || phrase_size > 32) return 0;
    
    int idx = 0;
    while(phrase[idx]) {
        char c = phrase[idx++];
        if (!isalnum(c) && (!permit_underscore || c != '_')) return 0;
    }

    return 1;
}

int determine_integer(short int integer_type, const char* phrase) {
    int phrase_size = strlen(phrase);

    if (phrase_size < 1) return 0;

    long int value = atol(phrase);

    if ((integer_type > 0 && (value < 0 || value > UINT_MAX)) || 
        (integer_type == 0 && (value < INT_MIN || value > INT_MAX)) || 
        (integer_type < 0 && value < 1)) return 0;

    int idx = 0;
    while(phrase[idx]) {
        char c = phrase[idx++];
        if (!isdigit(c) && !(idx == 0 && phrase_size > 1 && (c == '-' || c == '+'))) return 0;
    }
    
    return 1;
}
