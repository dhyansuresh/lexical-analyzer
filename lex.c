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
    int index;
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

// identifier entry
typedef struct {
    char *name;
    int line;
    int column;
} Identifier;

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

const char *symbols[20] = {
  [3] = "+",
  [4] = "-",
  [5] = "*",
  [6] = "/",
  [7] = "==",
  [8] = "!=",
  [9] = "<",
  [10] = "<=",
  [11] = ">",
  [12] = ">=",
  [13] = "(",
  [14] = ")",
  [15] = ",",
  [16] = ";",
  [17] = ".",
  [18] = "=",
  [19] = ":="
};

// prototypes
int get_ident(char *buffer, const unsigned char *bytes, int offset, int capacity);
int reserved_cmp (char* buffer, const char* reserved_words[], int words_count);
void print_error ( const LexError *error);
int name_lookup_or_insert(Identifier **names, size_t *name_count,
  size_t *name_capacity,
  const char *name, 
  int line, 
  int column);
int scan_source(const unsigned char *source,
    size_t source_length,
    Token **tokens,
    size_t *token_count,
    LexError *error);
void free_all(Token *tokens, size_t token_count, LexError *error, InputFile *file);

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

    // source program print
    printf("Source Program:\n");
    fwrite(file.bytes, file.size, 1, stdout);

    Token *tokens = NULL;
    size_t token_count = 0;
    LexError error = {0};

    int c = scan_source(file.bytes, file.size, &tokens, &token_count, &error);

    if (c == -1) {
        for (size_t i = 0; i < token_count; i++) {
            free(tokens[i].name);
        }
        free(tokens);
        free(error.lexeme);
        free(file.bytes);
        return 1;
    }
    printf("\nLexeme Table:\n\n");
    printf("%-15s%s\n", "lexeme", "token");

    for (int i = 0; i < token_count; i++) {
        printf("%-15s%d\n", tokens[i].name, tokens[i].code);
    }
    printf("\n");

    printf("Name Table:\n");

    printf("Token List:\n\n");
    for (int i = 0; i < token_count; i++) {
        printf("%d ", tokens[i].code);
    }


    if (c == 1) {
        printf("\n");
       print_error(&error);
    }
}

// scans each byte and stores into the Token struct
int scan_source(
    const unsigned char *source,
    size_t source_length,
    Token **tokens,
    size_t *token_count,
    LexError *error
) {
    int capacity = 0;
    int start = 0;
    int length = 0;
    int line = 1;
    int column = 1;
    int start_line = 1;
    int start_column = 1;
    int error_number = 0;

    *tokens = NULL;
    *token_count = 0;
    *error = (LexError){0};

    for (size_t i = 0; i < source_length; i++) {
        unsigned char current = source[i];
        int c = 0;

        start = i;
        start_line = line;
        start_column = column;
        length = 0;

        // checks for error 10
        if ((current < 0x20 || current > 0x7e) &&
            current != '\n' &&
            current != '\r' &&
            current != '\t') {
            error_number = 10;
            length = 1;
            goto lexical_error;
        }
        // ignores blanks spaces
        if (isspace(current)) {
            if (current == '\n') {
                line++;
                column = 1;
            } else if (current != '\r') {
                column++;
            }
            continue;
        }

        // START: COMMENT CHECKING
        // ignores '/*....*/'
        if (current == '*' &&
            source_length - i >= 2 && // if there is '*' and also a next bye
            source[i + 1] == '/') { // and if that byte is '/' error 8
            length = 2;
            error_number = 8;
            goto lexical_error;
        }

        // see if ignore comments
        if (current == '/' &&
            source_length - i >= 2 &&
            source[i + 1] == '*') {
            int closed = 0;

            i += 2;
            column += 2;

            for (; i < source_length; i++) {
                current = source[i];

                if (source_length - i >= 2 &&
                    current == '/' &&
                    source[i + 1] == '*') {
                    start = i;
                    start_line = line;
                    start_column = column;
                    length = 2;
                    error_number = 9;
                    goto lexical_error;
                }

                if (source_length - i >= 2 &&
                    current == '*' &&
                    source[i + 1] == '/') {
                    i++;
                    column += 2;
                    closed = 1;
                    break;
                }

                if (current == '\n') {
                    line++;
                    column = 1;
                } else if (current != '\r') {
                    column++;
                }
            }

            if (!closed) {
                length = 2;
                error_number = 7;
                goto lexical_error;
            }
            continue;
        }
        // END: COMMENT CHECKING

        // checks for letter and if it is a res word
        if (isalpha(current)) {
            char buffer[MAX_IDENTIFIER_LENGTH + 1];
            int word_length = get_ident(buffer,source,i,source_length);

            if (word_length == -1) {
                int end = i;

                for (; end < source_length &&
                       isalnum(source[end]); end++) {
                }

                length = end - start;
                error_number = 2;
                goto lexical_error;
            }
            length = word_length;

            // check for res words
            c = reserved_cmp(buffer, reserved_words, RESERVED_WORDS_COUNT);

            i += length - 1;
            column += word_length;
        }
        // if is number check for length
        else if (isdigit(current)) {
            int end = i;
            // just iterating end untill it hits a letter
            for (; end < source_length && isdigit(source[end]); end++) {
            }

            int digit_count = end - start;

            if (end < source_length && isalpha(source[end])) {
                for (; end < source_length && isalnum(source[end]); end++) { // picks up where end found a letter
                }

                length = end - start; // gives the whole set of number followed by letters
                error_number = 6;
                goto lexical_error;
            }

            length = digit_count;
            // longer than 6 digits
            if (length > NUMBER_LENGTH) {
                error_number = 3;
                goto lexical_error;
            }

            c = 2;
            i += length - 1;
            column += length;
        }
        else { // handles symbols
            for (int j = 0; j < 20; j++) {
                if (symbols[j] == NULL) // 0,1,2 are empty
                    continue;

                int symbol_length = strlen(symbols[j]);
                // loop throught the symbols array and try to find a match
                if (symbol_length > length && symbol_length <= source_length - i && memcmp(source + i, symbols[j], symbol_length) == 0) {
                    c = j;
                    length = symbol_length;
                }
            }
            // symbol is not part of the array so error
            if (c == 0) {
                if (current == ':')
                    error_number = 4;
                else if (current == '!')
                    error_number = 5;
                else
                    error_number = 1;
                length = 1;
                goto lexical_error;
            }
            i += length - 1;
            column += length;
        }

        if (*token_count == capacity) {
            size_t max_capacity = (size_t)-1 / sizeof(**tokens);
            size_t new_capacity;

            if (capacity == max_capacity)
                return -1;

            if (capacity == 0) {
                new_capacity = max_capacity < 64 ? max_capacity : 64;
            } else {
                new_capacity = capacity > max_capacity / 2 ? max_capacity : capacity * 2;
            }

            Token *new_list = realloc(*tokens, new_capacity * sizeof(**tokens));

            if (new_list == NULL)
                return -1;

            *tokens = new_list;
            capacity = new_capacity;
        }

        char *text = malloc(length + 1);

        if (text == NULL)
            return -1;

        memcpy(text, source + start, length);
        text[length] = '\0';

        (*tokens)[*token_count] = (Token){
            .name = text,
            .code = c,
            .line = start_line,
            .column = start_column
        };

        (*token_count)++;
    }
    // empty input or just comments inside
    if (*token_count == 0) {
        error->number = 11;
        error->line = 1;
        error->column = 1;
        return 1;
    }
    return 0;

lexical_error:
    error->number = error_number;
    error->line = start_line;
    error->column = start_column;
    error->byte = source[start];
    error->character = (char)source[start];
    error->lexeme = malloc(length + 1);

    if (error->lexeme == NULL)
        return -1;

    memcpy(error->lexeme, source + start, length);
    error->lexeme[length] = '\0';

    return 1;
}

// errors list
void print_error(const LexError *error) {

    printf("Error %d at line %d, column %d: ", error->number, error->line, error->column);
    switch (error->number) {
        case 1:
            printf("invalid character '%c'", error->character);
            break;
        case 2:
            printf("identifier too long '%s'", error->lexeme);
            break;
        case 3:
            printf("number too long '%s'", error->lexeme);
            break;
        case 4:
            printf("':' must be followed by '='");
            break;
        case 5:
            printf("'!' must be followed by '='");
            break;
        case 6:
            printf("number followed by a letter '%s'", error->lexeme);
            break;
        case 7:
            printf("comment is not closed before end of file");
            break;
        case 8:
            printf("'*/' without a matching '/*'");
            break;
        case 9:
            printf("'/*' inside a comment");
            break;
        case 10:
            printf(
                "byte 0x%02X is not part of this language",
                (unsigned int)error->byte
            );
            break;
        case 11:
            printf("no tokens in the source program");
            break;
    }
    printf("\n");
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

int reserved_cmp (char* buffer, const char* reserved_words[], int words_count) {
    for (int i = 0; i < words_count; i++ ){
        if (strcmp(buffer, reserved_words[i]) == 0) {
            return 20 + i;
        }
    }
    return 1;
}

void free_all(Token *tokens, size_t token_count, LexError *error, InputFile *file) {
    for (size_t i = 0; i < token_count; i++)
        free(tokens[i].name);

    free(tokens);
    free(error->lexeme);
    free(file->bytes);
}


int name_lookup_or_insert(Identifier **names, size_t *name_count, size_t *name_capacity, const char *name, int line, int column) {
    size_t i = 0;

    for (; i < *name_count; i++) {
        if (strcmp(name, (*names)[i].name) == 0) {
            return (int)i;
        }
    }

    if (*name_count == *name_capacity) {
        size_t new_capacity = (*name_capacity == 0) ? 64 : *name_capacity * 2;

        Identifier *tmp = realloc(*names, new_capacity * sizeof(Identifier));
        if (tmp == NULL) {
            return -1;
        }
        *names = tmp;
        *name_capacity = new_capacity;
    }

    (*names)[i].name = malloc(strlen(name) + 1);
    if ((*names)[i].name == NULL) {
        return -1;
    }

    strcpy((*names)[i].name, name);
    (*names)[i].line = line;
    (*names)[i].column = column;

    (*name_count)++;

    return (int)i;
}

