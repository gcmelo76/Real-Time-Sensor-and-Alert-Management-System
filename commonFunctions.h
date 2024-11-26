/*

Joao Pedro Morais Gaspar          nº2021221276
Guilherme Craveiro de Melo     nº 2021217138

*/
#ifndef COMMONFUNCTIONS_H
#define COMMONFUNCTIONS_H

#include <string.h>
#include <limits.h>
#include <stdlib.h>
#include <ctype.h>

int verify_alphanumeric(short int permit_underscore,const char* phrase);

int determine_integer(short int integer_type, const char* phrase);


#endif
