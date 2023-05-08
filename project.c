#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#define MAX_OPTIONS 10

typedef struct {
    char name[100];
    char type;
    struct stat filestat;
} file_info;

// function to display the options menu for regular files
void display_regular_file_options() {
    printf("\nOptions:\n");
    printf("-n: display file name\n");
    printf("-d: display file size\n");
    printf("-h: display hard link count\n");
    printf("-m: display time of last modification\n");
    printf("-a: display access rights\n");
    printf("-l: create symbolic link (enter link name)\n");
}

// function to display the options menu for symbolic links
void display_symbolic_link_options() {
    printf("\nOptions:\n");
    printf("-n: display link name\n");
    printf("-l: delete symbolic link\n");
    printf("-d: display size of symbolic link\n");
    printf("-t: display size of target file\n");
    printf("-a: display access rights\n");
}

// function to display the options menu for directories
void display_directory_options() {
    printf("\nOptions:\n");
    printf("-n: display directory name\n");
    printf("-d: display directory size\n");
    printf("-a: display access rights\n");
    printf("-c: display total number of .c files\n");
}

// function to print access rights in human-readable format
void print_access_rights(mode_t mode) {
    printf("User:\n");
    printf("Read - %s\n", (mode & S_IRUSR) ? "yes" : "no");
    printf("Write - %s\n", (mode & S_IWUSR) ? "yes" : "no");
    printf("Exec - %s\n", (mode & S_IXUSR) ? "yes" : "no");

    printf("\nGroup:\n");
    printf("Read - %s\n", (mode & S_IRGRP) ? "yes" : "no");
    printf("Write - %s\n", (mode & S_IWGRP) ? "yes" : "no");
    printf("Exec - %s\n", (mode & S_IXGRP) ? "yes" : "no");

    printf("\nOthers:\n");
    printf("Read - %s\n", (mode & S_IROTH) ? "yes" : "no");
    printf("Write - %s\n", (mode & S_IWOTH) ? "yes" : "no");
    printf("Exec - %s\n", (mode & S_IXOTH) ? "yes" : "no");
}


int execute_script(char* filename) {
    int num_warnings = 0, num_errors = 0;

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {  // child process
        // redirect standard error to a pipe
        int pipefd[2];
        if (pipe(pipefd) == -1) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }
        dup2(pipefd[1], STDERR_FILENO);
        close(pipefd[0]);
        close(pipefd[1]);

        // execute script with given filename
        char* args[] = { "bash", "-c", NULL, NULL };
        args[2] = malloc(strlen(filename) + 11);
        sprintf(args[2], "gcc -o /dev/null -Wall %s", filename);
        execvp(args[0], args);
        perror("execvp");
        exit(EXIT_FAILURE);
    } else {  // parent process
        int status;
        if (waitpid(pid, &status, 0) == -1) {
            perror("waitpid");
            exit(EXIT_FAILURE);
        }
        if (WIFEXITED(status)) {
            int exit_code = WEXITSTATUS(status);
            if (exit_code == 0) {
                printf("Compilation successful\n");
            } else {
                printf("Compilation failed\n");
                // read error messages from pipe
                int pipefd[2];
                if (pipe(pipefd) == -1) {
                    perror("pipe");
                    exit(EXIT_FAILURE);
                }
                char buffer[1024];
                while (read(pipefd[0], buffer, sizeof(buffer)) != 0) {
                    if (strstr(buffer, "warning") != NULL) {
                        num_warnings++;
                    } else if (strstr(buffer, "error") != NULL) {
                        num_errors++;
                    }
                }
                close(pipefd[0]);
                close(pipefd[1]);
            }
        } else {
            printf("Compilation failed\n");
        }
    }

    // calculate score based on number of warnings and errors
    if (num_errors == 0 && num_warnings == 0) {
        return 10;
    } else if (num_errors > 0) {
        return 1;
    } else {
        int score = 2 + 8 * (10 - num_warnings) / 10;
        return score;
    }
}



int count_lines(char *filename) {
    FILE *fp;
    int count = 0;
    char c;

    fp = fopen(filename, "r");
    if (fp == NULL) {
        printf("Error opening file %s\n", filename);
        return -1;
    }

    while ((c = fgetc(fp)) != EOF) {
        if (c == '\n') {
            count++;
        }
    }

    fclose(fp);
    return count;
}



void create_txt_file(char *dirname) {
    // Generate filename
    char filename[256];
    snprintf(filename, sizeof(filename), "%s/%s_file.txt", dirname, dirname);

    // Open file for writing
    FILE *fp = fopen(filename, "w");
    if (fp == NULL) {
        perror("Failed to create file");
        return;
    }

    // Write some content to the file
    fprintf(fp, "This is a text file created by the program.\n");
    fprintf(fp, "It is located in the '%s' directory.\n", dirname);

    // Close file
    fclose(fp);
}


void change_permissions(char *filename) {
    struct stat file_stat;
    int res = lstat(filename, &file_stat);
    if (res < 0) {
        perror("lstat error");
        return;
    }
    if (!S_ISLNK(file_stat.st_mode)) {
        printf("Error: '%s' is not a symbolic link.\n", filename);
        return;
    }

    if (chmod(filename, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP) < 0) {
        perror("chmod error");
        return;
    }
    printf("Permissions for '%s' changed successfully.\n", filename);
}


int parse_options(char *options) {
    int result = 0;
    char *option = strtok(options, "-");
    while (option != NULL) {
        if (strlen(option) == 0) {
            option = strtok(NULL, "-");
            continue;
        }
        switch (tolower(option[0])) {
            case 'n':
                result |= 1; // set first bit to 1
                break;
            case 'd':
                result |= 2; // set second bit to 1
                break;
            case 'h':
                result |= 4; // set third bit to 1
                break;
            case 'm':
                result |= 8; // set fourth bit to 1
                break;
            case 'a':
                result |= 16; // set fifth bit to 1
                break;
            case 'l':
                result |= 32; // set sixth bit to 1
                break;
            case 'c':
                result |= 64; // set seventh bit to 1
                break;
            default:
                fprintf(stderr, "Error: invalid option '%c'\n", option[0]);
                return -1; // indicate error
        }
        option = strtok(NULL, "-");
    }
    return result;
}


void print_error(char *msg) {
    fprintf(stderr, "ERROR: %s\n", msg);
}


void print_message(char *msg) {
    printf("%s\n", msg);
}



void write_to_file(char *filename, char *msg) {
    FILE *fp = fopen(filename, "a");
    if (fp == NULL) {
        printf("Error opening file %s\n", filename);
        return;
    }
    fprintf(fp, "%s\n", msg);
    fclose(fp);
}


void handle_child_process(int fd, char *filename, int (*func)(char *)) {
    pid_t pid = fork();

    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        // child process
        close(fd); // close unused read end of the pipe
        int result = func(filename);
        write(fd, &result, sizeof(int)); // write result to the parent process
        exit(EXIT_SUCCESS);
    } else {
        // parent process
        close(fd); // close unused write end of the pipe
        int status, result;
        waitpid(pid, &status, 0); // wait for child process to finish
        if (WIFEXITED(status) && WEXITSTATUS(status) == EXIT_SUCCESS) {
            // read result from child process
            read(fd, &result, sizeof(int));
            printf("Child process with PID %d has finished with result %d\n", pid, result);
        } else {
            printf("Child process with PID %d has failed\n", pid);
        }
    }
}



int compute_score(int errors, int warnings) {
    int score = 100;
    
    // Subtract 10 points for each error
    score -= errors * 10;
    
    // Subtract 5 points for each warning
    score -= warnings * 5;
    
    // Score cannot be negative
    if (score < 0) {
        score = 0;
    }
    
    return score;
}


int main(int argc, char **argv) {
    // Check for correct number of arguments
    if (argc != 3) {
        print_error("Usage: ./project filename options");
        return 1;
    }
    
    char *filename = argv[1];
    char *options = argv[2];
    
    // Parse options
    int opt = parse_options(options);
    if (opt == -1) {
        print_error("Invalid options specified");
        return 1;
    }
    
    // Get file/directory type and access rights
    struct stat filestat;
    if (stat(filename, &filestat) == -1) {
        print_error("Failed to get file/directory information");
        return 1;
    }
    if (!S_ISREG(filestat.st_mode) && !S_ISDIR(filestat.st_mode)) {
        print_error("Specified file/directory does not exist");
        return 1;
    }
    
    // Print access rights for file/directory
    print_access_rights(filestat.st_mode);
    

    
    return 0;
}

