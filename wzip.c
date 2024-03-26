/*
  CSC 139
  Spring 2024
  Ian Ichwara
 */
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("wzip: file1 [file2 ...]\n");
        exit(1);
    }

    char current, previous = '\0';
    int count = 0;

    for (int i = 1; i < argc; i++) {
        FILE *fp = fopen(argv[i], "r");
        if (fp == NULL) {
            printf("wzip: cannot open file\n");
            exit(1);
        }

        while ((current = fgetc(fp)) != EOF) {
            if (current == previous || previous == '\0') {
                count++;
            } else {
                fwrite(&count, sizeof(int), 1, stdout);
                printf("%c", previous);
                count = 1;
            }
            previous = current;
        }

        fclose(fp);
    }

    // Output the final sequence
    if (count > 0) {
        fwrite(&count, sizeof(int), 1, stdout);
        printf("%c", previous);
    }

    return 0;
}

