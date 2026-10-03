/*
Homework:
lex - HW2 PL/0 lexical analyzer

Author(s): Dhyan Suresh, Osmany Leyva Aldana

Language: C only

To Compile:
    gcc -Wall -Wextra -std=c11 -O2 lex.c -o lex

To Execute (on Eustis):
    ./lex <input_file>

where:
    <input_file> is the path to a text file holding a PL/0 source program

Notes:
    - Implements the lexical analyzer described in the homework
      instructions.
    - Prints four sections to standard output: Source Program, Lexeme
      Table, Name Table and Token List.
    - Writes two files into the working directory: tokens.txt and
      nametable.txt.
    - Stops at the first lexical error, prints everything scanned before
      it, reports the error with its line and column, and exits with a
      non-zero status.
    - Exits with status 0 when the whole program scans without an error.
    - Tested on Eustis.

Class: COP 3402 - Systems Software

Instructor: Jie Lin, Ph.D.

Due Date: See Webcourses
*/


#include <ctype.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RESERVED_WORDS_COUNT 17
#define MAX_IDENTIFIER_LENGTH 12
#define NUMBER_LENGTH 6

// token storage
typedef struct {
    char *name;
    int code;
    int line;
    int column;
} Token;

// use to store the errors the scanner finds
typedef struct {
    int number;
    int line;
    int column;

    char *lexeme;
    unsigned char byte;
    char character;
} LexError;

// file holds no token stuct
typedef struct {
    unsigned char *bytes;
    int size;
    int capacity;
} InputFile;

const char *reserved_words[RESERVED_WORDS_COUNT] = {
  "begin",
  "end", 
  "if",
  "fi", 
  "then",
  "while",
  "elihw", 
  "do",
  "od", 
  "odd", 
  "call", 
  "const", 
  "var", 
  "procedure", 
  "write", 
  "read", 
  "else"
};




int get_ident(char *buffer, const unsigned char *bytes, int offset, int capacity);

int get_number(char *buffer, const unsigned char *bytes, int offset, int capacity);

int reserved_cmp (char* buffer, const char* reserved_words[], int words_count);

// use for token reading
int scan_source(
    const unsigned char *source,
    size_t source_length,
    Token **tokens,
    size_t *token_count,
    LexError *error
);

int main(int argc, char *argv[]) {
    // read files and check arguments
    if (argc != 2) {
        fprintf(stderr, "Usage: ./lex <input file>\n");
        return 1;
    }

    // file can't be opened
    FILE *input = fopen(argv[1], "rb");
    if (input == NULL) {
        printf("Error: unable to open input file %s\n", argv[1]);
        return 1;
    }

    // read the input file
    InputFile file = {NULL, 0, 0};
    int byte;

    while ((byte = fgetc(input)) != EOF) {
        if (file.size == file.capacity) {
            int new_capacity = file.capacity == 0 ? 1024 : file.capacity * 2;

            if (new_capacity <= file.capacity) {
                free(file.bytes);
                fclose(input);
                return 1;
            }

            unsigned char *new_bytes = realloc(file.bytes, new_capacity);
            if (new_bytes == NULL) {
                free(file.bytes);
                fclose(input);
                return 1;
            }

            file.bytes = new_bytes;
            file.capacity = new_capacity;
        }
        file.bytes[file.size++] = byte;
        }
    if (ferror(input)) {
        free(file.bytes);
        fclose(input);
        return 1;
    }
    fclose(input);

    // Error 11
    if (file.bytes == NULL && file.capacity == 0) {
        fprintf(stderr, "no tokens in the source program.\n");
        return 1;
    }

    // printf("%s", file.bytes);

    char **identifiers_table = NULL;
    int column = 0;


    for (int i = 0; i < file.size; i++){
      char buffer[15] = { 0 };
      char c = file.bytes[i];
      char next = (i == file.capacity)? -1: file.bytes[i + 1];

      if (isalpha(c)){
        int length = get_ident(buffer, file.bytes, i, file.capacity);

        if (length == -1){
          // print error and scape
        }

        // TODO: optimize this skiping this step if the 
        // identifier is longer than any word, have uppercase
        // or have any number
        int reserved = reserved_cmp (buffer, reserved_words, RESERVED_WORDS_COUNT);

        if (reserved == 0){
          // identifier
        } else {
          // reserved word
        }

        i += length;
        continue;
      }

      if (isdigit(c)){
        int length = get_number(buffer, file.bytes, i, file.capacity);

        if (length == -1){
          // print error and scape
        }

        int number = atoi(buffer);
      }

      switch (c) {
         case '+':
           continue;
         case '-':
           continue;
         case '*':
           continue;
         case '/':
           continue;

         case '=': 
           if (next == '='){
           } else {
             i++;
           }
           continue;
         case '!':
            i++;
            continue;
         case '>':
            if (!(next == '=')){
            } else {
              i++;

            }
            continue;
         case '<':
            if (next == '='){
              i++;
            } else {
            }
            continue;

         case '(':
            continue;
         case ')':
            continue;
         case ',':
            continue;
         case ';':
            continue;
         case '.':
            continue;
         case ':':
            i++;
            continue;
         case '\n':
              column++;
          continue;
      }

      if (isspace(c)) continue;

    }

}

int get_ident(char *buffer, const unsigned char *bytes, int offset, int capacity) {
  if (offset >= capacity || !isalpha(bytes[offset]))
    return 0;

  int length = 0;

  while (offset + length < capacity && isalnum(bytes[offset + length])) {
    if (length == MAX_IDENTIFIER_LENGTH)
      return -1; // error 2

    buffer[length] = bytes[offset + length];
    length++;
  }

  buffer[length] = '\0';
  return length;
}

int get_number(char *buffer, const unsigned char *bytes, int offset, int capacity) {
  int length = 0;

  while (offset + length < capacity && isdigit(bytes[offset + length])) {
    if (length == NUMBER_LENGTH )
      return -1; // error 3

    buffer[length] = bytes[offset + length];
    length++;
  }

  buffer[length] = '\0';
  return length;
}


int reserved_cmp (char* buffer, const char* reserved_words[], int words_count){
  int cmp = 0;

  for ( int i = 0; i < words_count; i++ ){
       cmp = strcmp(buffer, reserved_words[i]);

      if (cmp != 0)
        return cmp;
  }

  return cmp;
}



