#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>



void display_file_info(const char* filePath);
void handle_regular_file(const char* filePath, const char* options);
void handle_symbolic_link(const char* filePath, const char* options);
void handle_directory(const char* filePath, const char* options);
void handle_file_with_extension(const char* filePath, const char* options);
void handle_directory_creation(const char* directoryPath);
void handle_symbolic_link_permission(const char* symbolicLinkPath);
void print_process_exit(pid_t pid, int exitCode);


int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("ERROR: Usage: %s filename options\n", argv[0]);
        return 1;
    }

    for (int i = 1; i < argc; i++) {
        const char *path = argv[i];
        const char *options = argv[++i];  // Move to the next argument for options

        struct stat fileStat;
        if (lstat(path, &fileStat) < 0) {
            perror("lstat");
            continue;
        }

        if (S_ISREG(fileStat.st_mode)) {
            handle_regular_file(path, options);
        } else if (S_ISLNK(fileStat.st_mode)) {
            handle_symbolic_link(path, options);
        } else if (S_ISDIR(fileStat.st_mode)) {
            handle_directory(path, options);
        } else {
            printf("Unknown file type: %s\n", path);
        }
    }

    return 0;
}


void display_file_info(const char* path) {
    struct stat fileStat;

    // Get file information
    if (lstat(path, &fileStat) == -1) {
        perror("Error accessing file information");
        return;
    }

    // Display file name and type
    printf("File: %s\n", path);

    if (S_ISREG(fileStat.st_mode)) {
        printf("Type: Regular File\n");
        // Display interactive menu for regular file options
        printf("Options:\n");
        printf("-n : Display file name\n");
        printf("-d : Display file size\n");
        printf("-h : Display hard link count\n");
        printf("-m : Display time of last modification\n");
        printf("-a : Display access rights\n");
        printf("-l : Create symbolic link (provide link name)\n");
    } else if (S_ISLNK(fileStat.st_mode)) {
        printf("Type: Symbolic Link\n");
        // Display interactive menu for symbolic link options
        printf("Options:\n");
        printf("-n : Display link name\n");
        printf("-l : Delete symbolic link\n");
        printf("-d : Display size of symbolic link\n");
        printf("-t : Display size of target file\n");
        printf("-a : Display access rights\n");
    } else if (S_ISDIR(fileStat.st_mode)) {
        printf("Type: Directory\n");
        // Display interactive menu for directory options
        printf("Options:\n");
        printf("-n : Display directory name\n");
        printf("-d : Display directory size\n");
        printf("-a : Display access rights\n");
        printf("-c : Display total number of .c files\n");
    } else {
        printf("Type: Unknown\n");
        return;
    }
}



void handle_regular_file(const char *path, const char *options) {
    struct stat fileStat;
    if (stat(path, &fileStat) < 0) {
        perror("stat");
        return;
    }

    printf("Regular File: %s\n", path);

    while (*options) {
        switch (*options) {
            case 'n':
                printf("File Name: %s\n", path);
                break;
            case 'd':
                printf("File Size: %lld bytes\n", (long long)fileStat.st_size);
                break;
            case 'h':
                printf("Hard Link Count: %ld\n", (long)fileStat.st_nlink);
                break;
            case 'm':
                printf("Time of Last Modification: %s", ctime(&fileStat.st_mtime));
                break;
            case 'a':
                printf("Access Rights:\n");
                printf("User:\n");
                printf("Read - %s\n", (fileStat.st_mode & S_IRUSR) ? "yes" : "no");
                printf("Write - %s\n", (fileStat.st_mode & S_IWUSR) ? "yes" : "no");
                printf("Exec - %s\n", (fileStat.st_mode & S_IXUSR) ? "yes" : "no");
                printf("Group:\n");
                printf("Read - %s\n", (fileStat.st_mode & S_IRGRP) ? "yes" : "no");
                printf("Write - %s\n", (fileStat.st_mode & S_IWGRP) ? "yes" : "no");
                printf("Exec - %s\n", (fileStat.st_mode & S_IXGRP) ? "yes" : "no");
                printf("Others:\n");
                printf("Read - %s\n", (fileStat.st_mode & S_IROTH) ? "yes" : "no");
                printf("Write - %s\n", (fileStat.st_mode & S_IWOTH) ? "yes" : "no");
                printf("Exec - %s\n", (fileStat.st_mode & S_IXOTH) ? "yes" : "no");
                break;
            case 'l':
                printf("Create Symbolic Link:\n");
                char linkName[256];
                printf("Enter the name for the symbolic link: ");
                scanf("%s", linkName);
                symlink(path, linkName);
                break;
            default:
                printf("Invalid option: %c\n", *options);
                break;
        }

        options++;
    }
}




void handle_symbolic_link(const char *path, const char *options) {
    struct stat fileStat;
    if (lstat(path, &fileStat) < 0) {
        perror("lstat");
        return;
    }

    printf("Symbolic Link: %s\n", path);

    while (*options) {
        switch (*options) {
            case 'n':
                printf("Link Name: %s\n", path);
                break;
            case 'l':
                printf("Delete Symbolic Link: %s\n", path);
                unlink(path);
                break;
            case 'd':
                printf("Size of Symbolic Link: %lld bytes\n", (long long)fileStat.st_size);
                break;
            case 't':
                if (S_ISLNK(fileStat.st_mode)) {
                    char targetPath[256];
                    ssize_t len = readlink(path, targetPath, sizeof(targetPath) - 1);
                    if (len != -1) {
                        targetPath[len] = '\0';
                        struct stat targetStat;
                        if (stat(targetPath, &targetStat) == 0) {
                            printf("Size of Target File: %lld bytes\n", (long long)targetStat.st_size);
                        } else {
                            perror("stat");
                        }
                    } else {
                        perror("readlink");
                    }
                }
                break;
            case 'a':
                printf("Access Rights:\n");
                printf("User:\n");
                printf("Read - %s\n", (fileStat.st_mode & S_IRUSR) ? "yes" : "no");
                printf("Write - %s\n", (fileStat.st_mode & S_IWUSR) ? "yes" : "no");
                printf("Exec - %s\n", (fileStat.st_mode & S_IXUSR) ? "yes" : "no");
                printf("Group:\n");
                printf("Read - %s\n", (fileStat.st_mode & S_IRGRP) ? "yes" : "no");
                printf("Write - %s\n", (fileStat.st_mode & S_IWGRP) ? "yes" : "no");
                printf("Exec - %s\n", (fileStat.st_mode & S_IXGRP) ? "yes" : "no");
                printf("Others:\n");
                printf("Read - %s\n", (fileStat.st_mode & S_IROTH) ? "yes" : "no");
                printf("Write - %s\n", (fileStat.st_mode & S_IWOTH) ? "yes" : "no");
                printf("Exec - %s\n", (fileStat.st_mode & S_IXOTH) ? "yes" : "no");
                break;
            default:
                printf("Invalid option: %c\n", *options);
                break;
        }

        options++;
    }
}


void handle_directory(const char *path, const char *options) {
    struct stat fileStat;
    if (stat(path, &fileStat) < 0) {
        perror("stat");
        return;
    }

    printf("Directory: %s\n", path);

    while (*options) {
        switch (*options) {
            case 'n':
                printf("Directory Name: %s\n", path);
                break;
            case 'd':
                printf("Directory Size: %lld bytes\n", (long long)fileStat.st_size);
                break;
            case 'a':
                printf("Access Rights:\n");
                printf("User:\n");
                printf("Read - %s\n", (fileStat.st_mode & S_IRUSR) ? "yes" : "no");
                printf("Write - %s\n", (fileStat.st_mode & S_IWUSR) ? "yes" : "no");
                printf("Exec - %s\n", (fileStat.st_mode & S_IXUSR) ? "yes" : "no");
                printf("Group:\n");
                printf("Read - %s\n", (fileStat.st_mode & S_IRGRP) ? "yes" : "no");
                printf("Write - %s\n", (fileStat.st_mode & S_IWGRP) ? "yes" : "no");
                printf("Exec - %s\n", (fileStat.st_mode & S_IXGRP) ? "yes" : "no");
                printf("Others:\n");
                printf("Read - %s\n", (fileStat.st_mode & S_IROTH) ? "yes" : "no");
                printf("Write - %s\n", (fileStat.st_mode & S_IWOTH) ? "yes" : "no");
                printf("Exec - %s\n", (fileStat.st_mode & S_IXOTH) ? "yes" : "no");
                break;
            case 'c':
                count_c_files(path);
                break;
            default:
                printf("Invalid option: %c\n", *options);
                break;
        }

        options++;
    }
}


void count_c_files(const char* path) {
    DIR* dir;
    struct dirent* entry;
    int count = 0;

    // Open directory
    dir = opendir(path);
    if (dir == NULL) {
        perror("Error opening directory");
        return;
    }

    // Count files with .c extension
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG && has_c_extension(entry->d_name)) {
            count++;
        }
    }

    closedir(dir);

    printf("Total number of files with .c extension: %d\n", count);
}

int has_c_extension(const char* filename) {
    const char* extension = strrchr(filename, '.');
    if (extension != NULL && strcmp(extension, ".c") == 0) {
        return 1;
    }
    return 0;
}


void execute_script(const char* filePath, const char* fileName) {
    pid_t pid = fork();

    if (pid < 0) {
        perror("Error forking process");
        return;
    }

    if (pid == 0) {
        // Child process
        char* argv[3];
        argv[0] = strdup("sh");
        argv[1] = strdup(filePath);
        argv[2] = NULL;

        execvp(argv[0], argv);
        perror("Error executing script");
        exit(1);
    } else {
        // Parent process
        int status;
        waitpid(pid, &status, 0);

        if (WIFEXITED(status)) {
            int exitCode = WEXITSTATUS(status);
            printf("The process with PID %d has ended with the exit code %d\n", pid, exitCode);

            // skor hesaplama- Compute score based on exit code
            int score;
            if (exitCode == 0) {
                // No errors or warnings
                score = 10;
            } else if (exitCode == 1) {
                // At least one error
                score = 1;
            } else {
                // No errors, but warnings
                int numWarnings = (exitCode > 10) ? 10 : exitCode;
                score = 2 + 8 * (10 - numWarnings) / 10;
            }

            // grade.txt dosyası alıp skoru yazdır -Write score to grades.txt
            FILE* file = fopen("grades.txt", "a");
            if (file != NULL) {
                fprintf(file, "%s: %d\n", fileName, score);
                fclose(file);
            } else {
                perror("Error opening grades.txt");
            }
        } else {
            printf("The process with PID %d did not terminate normally.\n", pid);
        }
    }
}



int compute_score(int numErrors, int numWarnings) {
    if (numErrors == 0 && numWarnings == 0) {
        return 10;
    } else if (numErrors > 0) {
        return 1;
    } else {
        int maxWarnings = 10;
        int score = 2 + (8 * (maxWarnings - numWarnings)) / maxWarnings;
        return score;
    }
}


void write_to_file(const char* fileName, const char* message) {
    FILE* file = fopen(fileName, "w");
    if (file != NULL) {
        fprintf(file, "%s\n", message);
        fclose(file);
        printf("Successfully wrote to file: %s\n", fileName);
    } else {
        perror("Error opening file");
    }
}


void handle_file_with_extension(const char* filePath, const char* options) {
    const char* extension = strrchr(filePath, '.');
    if (extension != NULL && strcmp(extension, ".c") == 0) {
        // Execute script
        const char* scriptPath = "/path/to/script.sh";
        const char* fileName = strrchr(filePath, '/');
        if (fileName != NULL) {
            fileName++; // Skip the '/'
        } else {
            fileName = filePath;
        }
        execute_script(scriptPath, fileName);

        // Compute score (dummy values for numErrors and numWarnings)
        int numErrors = 0;
        int numWarnings = 3; // Assume there are 3 warnings
        int score = compute_score(numErrors, numWarnings);

        // Write score to file
        const char* resultFile = "grades.txt";
        char scoreString[10];
        sprintf(scoreString, "%d", score);
        char message[256];
        strcpy(message, fileName);
        strcat(message, ": ");
        strcat(message, scoreString);
        write_to_file(resultFile, message);
    } else {
        printf("File is not a regular file with the .c extension.\n");
    }
}


void handle_directory_creation(const char* directoryPath) {
    // Create a text file with a specific name inside the directory
    char fileName[256];
    snprintf(fileName, sizeof(fileName), "%s/%s_file.txt", directoryPath, directoryPath);

    FILE* file = fopen(fileName, "w");
    if (file != NULL) {
        fclose(file);
        printf("Successfully created file: %s\n", fileName);
    } else {
        perror("Error creating file");
    }
}


void handle_symbolic_link_permission(const char* symbolicLinkPath) {
    // Change permissions of the symbolic link
    if (chmod(symbolicLinkPath, S_IRUSR | S_IWUSR | S_IRGRP) == 0) {
        printf("Successfully changed permissions of symbolic link: %s\n", symbolicLinkPath);
    } else {
        perror("Error changing permissions of symbolic link");
    }
}


void print_process_exit(pid_t pid, int exitCode) {
    if (WIFEXITED(exitCode)) {
        printf("The process with PID %d has ended with the exit code %d\n", pid, WEXITSTATUS(exitCode));
    } else if (WIFSIGNALED(exitCode)) {
        printf("The process with PID %d was terminated by a signal\n", pid);
    } else {
        printf("The process with PID %d has ended\n", pid);
    }
}




