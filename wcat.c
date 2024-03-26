/*                                                                                                                 
  CSC 139                                                                                                          
  Spring 2024                                                                                                     
  Ian Ichwara                                                                                               
 */
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
  //Code
  FILE *fp;
  char buffer[1024]; // Adjust buffer size as needed

  for (int i = 1; i < argc; i++) {
    fp = fopen(argv[i], "r");
    if (fp == NULL) {
        printf("wcat: cannot open file\n");
        exit(1);
    }
    while (fgets(buffer, sizeof(buffer), fp)) {
        printf("%s", buffer);
    }

    fclose(fp);
}

  return 0;
}


