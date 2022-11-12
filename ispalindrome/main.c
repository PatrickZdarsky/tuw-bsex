/**
 * @file main.c
 * @author Patrick ZDARSKY (12123697)
 * @brief Program to check if strings are palindromes. Can read the values from stdin or input files and writes the reult to stdout or an output file
 * @version 0.1
 * @date 2022-11-01
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#include <ctype.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <getopt.h>
#include <stdlib.h>
#include <errno.h>

/**
 * @brief Checks whether a given string is a palindrom
 * 
 * @param value The string to be checked
 * @param caseInsensitive If true, check case insensitive
 * @return true If the string is a palindrom
 * @return false If the string is not a palindrom
 */
static bool isPalindrom(char* value, bool caseInsensitive) {
    size_t length = strlen(value);
    if (length == 0)
        return false;

    for (int i = 0; i < length/2; i++) {
        char c1 = value[i];
        char c2 = value[length-1-i];

        if (caseInsensitive) {
            c1 = toupper(c1);
            c2 = toupper(c2);
        }

        if (c1 != c2)
            return false;
    }
    return true;
}

/**
 * @brief Removes trailing newline characters from the given string
 * 
 * @param value The string to remove trailing newline characters from, it must be editable
 */
static void removeStringTrailingNewline(char *value) {
  if (value == NULL)
    return;
  int length = strlen(value);
  if (value[length-1] == '\n')
    value[length-1]  = '\0';
}

/**
 * @brief Removes whitespace caracters from the given string
 * 
 * @param value The string to remove whitespace characters from, it must be editable
 */
static void removeWhitespace(char* value) {
    char* d = value;
    do {
        // Advance untill we passed all the whitespaces
        while (*d == ' ') {
            ++d;
        }
    // Write the non whitespace character in our original string and 
    // advance only if we haven't encountered the NULL character
    } while ((*value++ = *d++));
}

/**
 * @brief Checks a given value if it is a palindrome and writes the result to the output file
 * 
 * @param value The value to check
 * @param caseInsensitive If true, check case insensitive
 * @param ignoreWhitespace  If true, ignores whitespace characters
 * @param output The output file to where to write the result
 */
static void processValue(char* value, bool caseInsensitive, bool ignoreWhitespace, FILE *output) {
    char *toCheck = strdup(value);

    if (ignoreWhitespace) {
        removeWhitespace(toCheck);
    }

    bool palindrom = isPalindrom(toCheck, caseInsensitive);

    if (palindrom) {
        fprintf(output, "%s is a palindrom\n", value);
    } else {
        fprintf(output, "%s is not a palindrom\n", value);
    }

    free(toCheck);
}

/**
 * @brief Checks a given file for palindromes and writes the result in the given output file
 * 
 * @param input The input File from where to read the input values
 * @param caseInsensitive If true, check case insensitive
 * @param ignoreWhitespace  If true, ignores whitespace characters
 * @param output The output file to where to write the result
 */
static void checkFile(FILE *input, bool caseInsensitive, bool ignoreWhitespace, FILE *output) {
    char *line = NULL;
    size_t len = 0;
    ssize_t read;

    while ((read = getline(&line, &len, input)) != -1) {
        if (feof(input) != 0) { //Check for eof
            break;
        }

        removeStringTrailingNewline(line);

        processValue(line, caseInsensitive, ignoreWhitespace, output);
    }

    free(line);
}

int main(int argc, char *argv[]) {
    char *outputFile = NULL;
    bool caseInsensitive = false;
    bool ignoreWhitespace = false;
    int c;

    while ( (c = getopt(argc, argv, "sio:")) != -1 ){
        switch ( c ) {
            case 's': ignoreWhitespace = true;
                break;
            case 'i': caseInsensitive = true;
                break;
            case 'o': outputFile = optarg;
                break;
            default:
                printf("SYNOPSIS:\n     %s [-s] [-i] [-o outfile] [file...]\n", argv[0]);
                return EXIT_FAILURE;
        }
    }

    //Initialize output stream
    FILE *output = stdout;
    if (outputFile != NULL) {
        output = fopen(outputFile, "w");

        //Check if we can open the output file
        if (output == NULL) {
            fprintf(stderr, "%s: fopen failed of output file: %s\n", argv[0], strerror(errno));
            return EXIT_FAILURE;
        }
    }

    //Check if there are input files defined
    if ((argc - optind) > 0) {
        //Read from inputfiles
        for (int i = optind; i<argc; i++) {
            FILE *inputF = fopen(argv[i], "r");
            //Check if file exists and we can read it
            if (inputF == NULL) {
                fprintf(stderr, "%s:fopen failed of input file %s: %s\n", argv[0], argv[i], strerror(errno));
                return EXIT_FAILURE;
            }


            checkFile(inputF, caseInsensitive, ignoreWhitespace, output);

            if (fclose(inputF) == EOF) {
                fprintf(stderr, "%s:fclose failed of input file %s: %s\n", argv[0], argv[i], strerror(errno));
            }
        }
    } else {
        //Read from stdin
        while(true) {
            checkFile(stdin, caseInsensitive, ignoreWhitespace, output);
        }
    }

    //If we didn't write to stdout, close the new file
    if (outputFile != NULL) {
        if (fclose(output)== EOF) {
            fprintf(stderr, "%s:fclose failed of output file %s: %s\n", argv[0], outputFile, strerror(errno));
        }
    }

    return EXIT_SUCCESS;
}


