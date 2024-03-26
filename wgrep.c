/*                                                                                                                 
  CSC 139                                                                                                          
  Spring 2024                                                                                                      
  Ian Ichwara                                                                                                     
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
    char buffer[BUFFER_SIZE];

    // Check if at least a search term is provided
    if (argc < 2) {
        printf("wgrep: searchterm [file ...]\n");
        exit(1);
    }

    char *searchTerm = argv[1];

    if (argc == 2) {
        // Read from standard input if no file is provided
        while (fgets(buffer, BUFFER_SIZE, stdin)) {
            if (strstr(buffer, searchTerm) != NULL) {
                printf("%s", buffer);
            }
        }
    } else {
        // Process each file provided as an argument
        for (int i = 2; i < argc; i++) {
            FILE *fp = fopen(argv[i], "r");
            if (fp == NULL) {
                printf("wgrep: cannot open file\n");
                exit(1);
            }

            while (fgets(buffer, BUFFER_SIZE, fp)) {
                if (strstr(buffer, searchTerm) != NULL) {
                    printf("%s", buffer);
                }
            }

            fclose(fp);
        }
    }

    return 0;
}

