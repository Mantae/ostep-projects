#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h>

#define MAX_LINE 1024
#define MAX_ARGS 64
#define DELIMS " \t\r\n"
#define ERROR_MESSAGE "An error has occurred\n"
#define MAX_PATHS 64

//function prototypes
void print_error(void);
void set_path(char *args[]);
void execute_command(char *args[], int background, char *redirect_file);
void parse_and_execute(char *line);
int main(int argc, char *argv[]);
void trim(char *str);

// Global variable to track background processes
pid_t bg_processes[MAX_ARGS];
int bg_process_count = 0;//counter to keep track of background processes.

char *currentPath[MAX_PATHS] = {"/bin", NULL}; // Stored default path

// Function to wait for all background processes to finish.
void wait_for_background_processes() {
    for (int i = 0; i < bg_process_count; ++i) {
        if (bg_processes[i] > 0) {
            int status;
            waitpid(bg_processes[i], &status, 0);
        }
    }
    bg_process_count = 0; // Reset the counter after waiting for all background processes.
}

// Function to print an error message to standard error (stderr). This is used throughout the shell
// to maintain consistent error reporting.
void print_error(void) {
    // Define a constant message to be used for all error reporting.
    const char error_message[30] = "An error has occurred\n";
    // Write the error message directly to the standard error output.
    write(STDERR_FILENO, error_message, strlen(error_message));
}

// Function to trim leading and trailing whitespace from a string. This is useful for processing
// user input or file paths where extra spaces might cause issues.
void trim(char *str) {
    char *start, *end, *temp;

    // Iterate over the string to find the first non-space character.
    for (start = str; *start; start++) {
        if (!isspace((unsigned char)*start))
            break;
    }

    // If the entire string was spaces, set it to an empty string and return.
    if (*start == 0) {
        *str = 0;
        return;
    }

    // Find the last non-space character by iterating backward from the end of the string.
    end = start + strlen(start) - 1;
    while (end > start && isspace((unsigned char)*end))
        end--;
    // Mark the end of the trimmed string.
    *(end + 1) = 0;

    // Shift the trimmed string to the beginning of the original string buffer.
    temp = str;
    while (*start)
        *temp++ = *start++;
    // Null-terminate the shifted string.
    *temp = 0;
}

void set_path(char *args[]) {
    memset(currentPath, 0, sizeof(currentPath)); // Clear current path
    int i = 1; // Start from args[1] since args[0] is "path"
    int pathIndex = 0;
    while (args[i] != NULL && pathIndex < MAX_PATHS - 1) {
        currentPath[pathIndex++] = strdup(args[i++]);
    }
    currentPath[pathIndex] = NULL; // Ensure NULL-terminated
}

void execute_command(char *args[], int background, char *redirect_file) {
    if (args[0] == NULL) {
        // If no command is provided, simply return.
        return;
    }

    // Handle built-in commands: "exit", "cd", and "path" before forking.
    if (strcmp(args[0], "exit") == 0) {
        if (args[1] != NULL) {
            print_error(); // "exit" should not have any arguments.
        } else {
            exit(0);
        }
        return;
    } else if (strcmp(args[0], "cd") == 0) {
        if (args[1] == NULL || args[2] != NULL) {
            print_error(); // "cd" requires exactly one argument.
        } else if (chdir(args[1]) != 0) {
            print_error(); // Report error if changing directory fails.
        }
        return;
    } else if (strcmp(args[0], "path") == 0) {
        set_path(args);
        return;
    }

    pid_t pid = fork();
    if (pid == 0) { // Child process
        // Handle redirection if specified.
        if (redirect_file != NULL) {
                    // Use O_APPEND along with O_WRONLY and O_CREAT to append to the file
                    // This helps when multiple processes write to the same file
                    int fd_out = open(redirect_file, O_WRONLY | O_CREAT | O_APPEND, 0666);
                    if (fd_out < 0) {
                        print_error(); // Cannot open file for redirection.
                        exit(EXIT_FAILURE);
                    }
                    if (dup2(fd_out, STDOUT_FILENO) < 0 || dup2(fd_out, STDERR_FILENO) < 0) {
                        print_error(); // Error in duplicating file descriptor for stdout or stderr
                        close(fd_out);
                        exit(EXIT_FAILURE);
                    }
                    close(fd_out);
                }

        // Execute the command
        char cmd[MAX_LINE];
        int found = 0;
        for (int i = 0; currentPath[i] != NULL && !found; i++) {
            snprintf(cmd, sizeof(cmd), "%s/%s", currentPath[i], args[0]);
            if (access(cmd, X_OK) == 0) {
                execv(cmd, args);
                found = 1; // Command found and executed.
            }
        }
        if (pid > 0 && background) {
                // If this is a background process, add it to the list of background processes.
                bg_processes[bg_process_count++] = pid;
            } else if (pid > 0) {
                // If not in background, wait for the process immediately.
                int status;
                waitpid(pid, &status, 0);
            }

        if (!found) {
            print_error(); // Command not found.
        }
        exit(EXIT_FAILURE); // Ensure child process exits if execv fails.
    } else if (pid < 0) {
        // Forking failed.
        print_error();
    } else {
        // Parent process waits for the child if not in background.
        if (!background) {
            int status;
            waitpid(pid, &status, 0);
        }
    }
    
}

void parse_and_execute(char *line) {
    char *args[MAX_ARGS] = {NULL};
    char *redirect_file = NULL;
    int background_global = 0; // Renamed to avoid shadowing in the loop
    int arg_count = 0;
    int redirection_count = 0;
    int num_commands = 0;
    char* commands[MAX_ARGS]; // Assume MAX_ARGS is enough space for now

    // Split line into commands based on '&' delimiter
    char *command = strtok(line, "&");
    while (command != NULL && num_commands < MAX_ARGS - 1) {
        trim(command); // Trim whitespace from command
        if (strlen(command) > 0) { // Ignore empty commands
            commands[num_commands++] = command;
        }
        command = strtok(NULL, "&");
    }

    // Execute each command
    for (int i = 0; i < num_commands; i++) {
        int background = (i < num_commands - 1) ? 1 : 0; // All but the last command run in background
        char *rest = commands[i];
        char *token = strtok_r(rest, DELIMS, &rest);
        
        // Reset per-command variables
        arg_count = 0;
        redirection_count = 0;
        redirect_file = NULL;

        while (token != NULL) {
            if (strcmp(token, ">") == 0) {
                token = strtok_r(rest, DELIMS, &rest); // Get the next token (output file)
                if (!token) {
                    print_error();
                    return;
                }
                redirect_file = token;
                redirection_count++;
            } else if (strcmp(token, "&") == 0 && strtok_r(rest, DELIMS, &rest) == NULL) {
                background = 1; // Last token is '&', set background execution
            } else {
                args[arg_count++] = token;
            }
            token = strtok_r(NULL, DELIMS, &rest); // Get the next token
        }

        if (redirection_count > 1) {
            print_error();
            return;
        }

        if (arg_count == 0) continue; // If command was only '&', continue to the next command

        args[arg_count] = NULL; // Null-terminate the argument list

        // Reset background flag if not the last command or explicitly backgrounded
        if (i == num_commands - 1 && !background) {
            background_global = 0;
        } else {
            background_global = background;
        }

        // Execute the command
        if (strcmp(args[0], "cd") == 0) {
            if (arg_count != 2) print_error();
            else if (chdir(args[1]) != 0) print_error();
        } else if (strcmp(args[0], "path") == 0) {
            set_path(args);
        } else if (strcmp(args[0], "exit") == 0) {
            if (arg_count > 1) print_error();
            else exit(0);
        } else {
            execute_command(args, background_global, redirect_file);
        }
    }
}

/*
void validate_and_execute_command(char *line) {
    int redirect_count = 0;
    // Temporarily duplicate the line to avoid modifying the original with strtok
    char *temp_line = strdup(line);
    if (temp_line == NULL) {
        print_error();
        return;
    }
    
    // Use strtok to find '>' symbols
    char *token = strtok(temp_line, DELIMS);
    while (token != NULL) {
        if (strcmp(token, ">") == 0) {
            redirect_count++;
        }
        token = strtok(NULL, DELIMS);
    }

    free(temp_line); // Clean up the temporary line copy

    if (redirect_count > 1) {
        // More than one redirection found, print error and return
        print_error();
    } else {
        // Call parse_and_execute if the command is valid
        parse_and_execute(line);
    }
}
*/

bool isValidRedirection(char* line) {
    int redirectionCount = 0;
    char* tempLine = strdup(line); // Create a copy of the line to work with
    if (!tempLine) {
        print_error();
        return false;
    }

    char* token = strtok(tempLine, " "); // Tokenize by space to check each part of the command
    while (token != NULL) {
        if (strcmp(token, ">") == 0) {
            redirectionCount++;
            char* file = strtok(NULL, " "); // Get the next token to check if it's a filename
            if (!file || strtok(NULL, " ") != NULL) { // If there's no file specified or there are extra tokens after the file
                free(tempLine);
                return false; // This indicates multiple files for redirection or malformed command
            }
        }
        token = strtok(NULL, " ");
    }

    free(tempLine);
    return redirectionCount <= 1; // Valid if there's at most one redirection operator
}

void validate_and_execute_command(char *line) {
    if (!isValidRedirection(line)) {
        print_error();
    } else {
        // Your existing logic to call parse_and_execute if the line is valid
        parse_and_execute(line);
    }
}

int main(int argc, char *argv[]) {
    char line[MAX_LINE];
    int exitStatus = 0; // Default to success.

    if (argc > 1) { // Batch mode
        for (int i = 1; i < argc; i++) {
            FILE *file = fopen(argv[i], "r");
            if (!file) {
                print_error(); // Error opening file
                exitStatus = 1; // Set error status
                continue; // Proceed to next file, if any
            }

            // Check if the file is empty by attempting to read the first line
            if (fgets(line, MAX_LINE, file) != NULL) {
                // If the file is not empty, process the first line and subsequent lines
                do {
                    line[strcspn(line, "\n")] = '\0'; // Remove newline at end, if present
                    parse_and_execute(line);
                } while (fgets(line, MAX_LINE, file) != NULL);
            } else {
                // File is empty, so print error and set exit status
                print_error();
                exitStatus = 1; // Set error status
            }
            fclose(file); // Close the file after processing
        }
        return exitStatus; // Return the overall exit status
    } else {
        // Interactive mode
        printf("wish> ");
        while (fgets(line, MAX_LINE, stdin) != NULL) {
            if (line[strlen(line) - 1] == '\n') {
                line[strlen(line) - 1] = '\0'; // Remove newline at end
            }
            //parse_and_execute(line);
            validate_and_execute_command(line);
            printf("wish> ");
        }
    }
    wait_for_background_processes();
    return 0; // Normal exit
}


