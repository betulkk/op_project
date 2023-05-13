#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <dirent.h>
#include <fts.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <time.h>

//End of String. Used with sprintf to get the pointer of string buffer.
#define eos(s) ((s)+strlen(s))

// File types
enum FileType {
    FILE_TYPE_UNKNOWN,
    FILE_TYPE_FILE,
    FILE_TYPE_DIRECTORY,
    FILE_TYPE_SYMBOLIC_LINK
};

// Holds Directory information
typedef struct dirResult {
    const char *path;
    const char *name;
    off_t size;
    int access;
    int c_files;
} DirResult;

// Holds Symbolic link information
typedef struct symbolicResult {
    const char *path;
    const char *name;
    off_t size;
    off_t target_size;
    int access;
    int deleted;
} SymbolicResult;

// Holds the options entered by user for Directory
typedef struct diroptions {
    const char *path;
    int name;
    int size;
    int perms;
    int c_files;
} DirOptions;

// Holds the options entered by user for Symbolic link
typedef struct symoptions {
    const char *path;
    int name;
    int size;
    int target_size;
    int perms;
    int delete;
} SymbolicOptions;

// Holds the options entered by user for regular file
typedef struct fileoptions{
    const char *path;
    int name;
    int size;
    int hard_link;
    int last_modification;
    int perms;
    int symbolic;
    const char *symbolic_name;
} FileOptions;

// Holds regular file information
typedef struct fileresult{
    const char *path;
    const char *name;
    int hard_link;
    int access;
    off_t size;
    int symbolic;
    time_t mtime;
} FileResult;

char* print_permissions(int);
off_t calculate_directory_size(char *);
char *get_folder_name(const char *);
enum FileType getFileType(const char *);
int create_file(char *, char *);
long int calculate_symlink_target_size(char *);
int set_file_permissions(const char *, int);
DirOptions GetDirectoryOptions(char *);
void PrintDirInfo(char *, int *, DirOptions);
void PrintSymInfo(char *, int *, SymbolicOptions);
SymbolicOptions GetSymbolicOptions(char *);
FileOptions GetFileOptions(char *);
void PrintFileInfo(char *, int *, FileOptions);
int check_file_extension (const char *, const char *);
int count_lines_in_file(const char *);
double compile_file_in_child(char *);

int main(int argc, char *argv[]) {
    // Allocate a shared memory to control child process's start time.
    int *start = mmap ( NULL, sizeof(int),
                        PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0 );
    // Set start to 0 until the options for all paths are entered
    *start = 0;
    DirOptions dir_opts;
    SymbolicOptions sym_opts;
    FileOptions file_opts;
    int fd[2];
    pid_t p;
    pipe(fd);
    // loop through all args and create a child process according to the file type
    for (int i = 1; i < argc; i++) {
        switch (getFileType(argv[i])) {
            case FILE_TYPE_UNKNOWN:
                break;
            case FILE_TYPE_FILE:
                file_opts = GetFileOptions(argv[i]);
                p = fork();
                if (p > 0){
                    continue;
                } else if (p == 0){
                    PrintFileInfo(argv[i], start, file_opts);
                    exit(0);
                } else {
                    printf("Error");
                    exit(0);
                }
            case FILE_TYPE_SYMBOLIC_LINK:
                sym_opts = GetSymbolicOptions(argv[i]);
                p = fork();
                if (p > 0){
                    continue;
                } else if (p == 0){
                    PrintSymInfo(argv[i], start, sym_opts);
                    exit(0);
                } else {
                    printf("Error");
                    exit(0);
                }
            case FILE_TYPE_DIRECTORY:
                dir_opts = GetDirectoryOptions(argv[i]);
                p = fork();
                if (p > 0){
                    continue;
                } else if (p == 0){
                    PrintDirInfo(argv[i], start, dir_opts);
                    exit(0);
                } else {
                    printf("Error");
                    exit(0);
                }
        }
    }
    // set start to 1 to start all the child process at the same time
    *start = 1;
    int st;
    pid_t p2;
    // Wait for all child process to end
    for (int i = 1; i < argc; i++){
        p2 = wait(&st);
        printf("Process with PID %d exited with code %d\n", p2, st);
    }
}

// Get options for symbolic link and return a SymbolicOptions structure
SymbolicOptions GetSymbolicOptions(char *dirPath){
    char options[6] = {'n', 'l', 'd', 't', 'a', '\0'};
    char input[10];
    int invalidOption;
    // Initialize a flag for each option and save it to SymbolicOptions structure
    int nFlag = 0, dFlag = -1, aFlag = 0, lFlag = 0, tFlag = 0;
    SymbolicOptions opts = {strdup(dirPath), nFlag,dFlag, tFlag, aFlag, lFlag};

    // Print directory name and message
    printf("Symbolic link path: %s\n", dirPath);
    printf("This is a Symbolic link\n");

    // Print options string
    printf("Options:\n");
    printf("-n: Name\n");
    printf("-d: Size of link\n");
    printf("-a: Access rights\n");
    printf("-l: Delete\n");
    printf("-t: Size of target file\n");

    // Get the user input and update the flags according to the entered options
    do {
        invalidOption = 0;
        printf("Enter options (-[n/d/a/l/t]): ");
        scanf("%s", input);
        if (input[0] != '-') {
            invalidOption = 1;
        } else {
            for (int i = 1; i < (int)strlen(input); i++) {
                if (strchr(options, input[i]) == NULL) {
                    invalidOption = 1;
                    break;
                } else {
                    switch (input[i]) {
                        case 'n':
                            nFlag = 1;
                            break;
                        case 'd':
                            dFlag = 0;
                            break;
                        case 'a':
                            aFlag = 1;
                            break;
                        case 'l':
                            lFlag = 1;
                            break;
                        case 't':
                            tFlag = 1;
                    }
                }
            }
        }
        if (invalidOption) {
            printf("Error: Invalid option\n");
        }
    } while (invalidOption);
    // Update the SymbolicOptions structure with the updated flags and return it.
    opts.name = nFlag;
    opts.size = dFlag;
    opts.target_size = tFlag;
    opts.perms = aFlag;
    opts.delete = lFlag;
    return opts;
}
// Get Symbolic link information and return a SymbolicResult structure
SymbolicResult getSymInfo(SymbolicOptions opts){
    // Using stat struct to hold the link info
    struct stat dirStat;
    int nFlag = opts.name;
    int aFlag = opts.perms;
    int dFlag = opts.size;
    int tFlag = opts.target_size;
    int lFlag = opts.delete;
    char *dirPath = strdup(opts.path);
    // Using lstat to get the data of the symbolic link
    lstat(dirPath, &dirStat);
    SymbolicResult *res = (SymbolicResult *) malloc(sizeof(struct symbolicResult));
    res->path = strdup(dirPath);
    // Get the required link info according to the saved options and return a SymbolicResult structure
    if (lFlag){
        unlink(dirPath);
        res->deleted = 1;
        return *res;
    }
    if (nFlag) {
        res->name = strdup(get_folder_name(dirPath));
    }
    if (dFlag == 0) {
        res->size = dirStat.st_size;
    } else {
        res->size = -1;
    }
    if (aFlag) {
        res->access = dirStat.st_mode & 0777;
    }
    if (tFlag) {
        res->target_size = calculate_symlink_target_size(dirPath);
    }
    return *res;
}
// Print Symbolic link information
void PrintSymInfo(char *path, int *start, SymbolicOptions opts){
    // Wait for all options to get entered and start variable set to 1
    while (!*start);
    SymbolicResult symres;
    char *result = (char *) malloc(1000 * sizeof(char));
    symres = getSymInfo(opts);
    sprintf(result, "------------------------------------------\nDirectory Path:%s\n", path);
    if (opts.delete) {
        sprintf(eos(result), "Symbolic link deleted.\n");
        goto end;
    }
    if (opts.name)
        sprintf(eos(result), "Symbolic link Name: %s\n", symres.name);
    if (opts.size >= 0)
        sprintf(eos(result), "Symbolic link size: %ld\n", symres.size);
    if (opts.perms)
        sprintf(eos(result), "Permissions:\n%s", print_permissions(symres.access));
    if (opts.target_size >= 0)
        sprintf(eos(result), "Symbolic link target size: %ld\n", symres.target_size);
    int st;
    // Create a child process to change the link permissions
    pid_t p = fork();
    if (p > 0) { // Main process
        waitpid(p, &st, WUNTRACED);
        sprintf(eos(result), "%s", !st ? "Changed permissions to 760\n" : "Error changing permissions.\n");
        sprintf(eos(result), "Process with PID %d exited with code %d\n", p, st);
    } else if (p == 0){ // Child process
        set_file_permissions(symres.path, 760);
    }
    end:
    sprintf(eos(result), "%s", "------------------------------------------\n");
    printf("%s",result);
}
// Get options for Directory and return a DirOptions structure
DirOptions GetDirectoryOptions(char *dirPath){
    // Open the directory
    DIR *dir = opendir(dirPath);
    char options[5] = {'n', 'd', 'a', 'c', '\0'};
    char input[10];
    int invalidOption;
    // Initialize a flag for each option and save it to DirOptions structure
    int nFlag = 0, dFlag = -1, aFlag = 0, cFlag = -1;
    DirOptions opts = {strdup(dirPath), nFlag, dFlag, aFlag, cFlag};
    if (dir == NULL) { // Exit if couldn't open the directory
        printf("Error: Failed to open directory %s\n", dirPath);
        exit(-1);
    }

    // Print directory name and message
    printf("Directory: %s\n", dirPath);
    printf("This is a directory\n");

    // Print options string
    printf("Options:\n");
    printf("-n: Name\n");
    printf("-d: Size of directory\n");
    printf("-a: Access rights\n");
    printf("-c: Total number of files with the .c extension\n");

    // Get the user input and update the flags according to the entered options
    do {
        invalidOption = 0;
        printf("Enter options (-[n/d/a/c]): ");
        scanf("%s", input);
        if (input[0] != '-') {
            invalidOption = 1;
        } else {
            for (int i = 1; i < (int)strlen(input); i++) {
                if (strchr(options, input[i]) == NULL) {
                    invalidOption = 1;
                    break;
                } else {
                    switch (input[i]) {
                        case 'n':
                            nFlag = 1;
                            break;
                        case 'd':
                            dFlag = 0;
                            break;
                        case 'a':
                            aFlag = 1;
                            break;
                        case 'c':
                            cFlag = 0;
                            break;
                    }
                }
            }
        }
        if (invalidOption) {
            printf("Error: Invalid option\n");
        }
    } while (invalidOption);
    // Close the dir
    closedir(dir);
    // Update the DirOptions structure with the updated flags and return it.
    opts.path = strdup(dirPath);
    opts.c_files = cFlag;
    opts.size = dFlag;
    opts.perms = aFlag;
    opts.name = nFlag;
    return opts;
}
// Get directory information and return a DirResult structure
DirResult getDirInfo(DirOptions opts){
    // directory entry
    struct dirent *entry;
    struct stat dirStat;
    int cFlag = opts.c_files;
    int aFlag = opts.perms;
    int dFlag = opts.size;
    int nFlag = opts.name;
    char *dirPath = strdup(opts.path);
    DIR *dir = opendir(dirPath);
    stat(dirPath, &dirStat);
    DirResult *res = (DirResult *) malloc(sizeof(struct dirResult));
    res->path = strdup(dirPath);
    // Get the required directory info according to the saved options and return a DirResult structure
    if (nFlag) {
        res->name = strdup(opts.path);
    }
    if (dFlag == 0) {
        res->size = calculate_directory_size(dirPath);
    } else {
        res->size = -1;
    }
    if (aFlag) {
        res->access = dirStat.st_mode & 0777;
    }
    if (cFlag == 0) {
        while ((entry = readdir(dir)) != NULL) { // Loop  through files and check its extension

            if (entry->d_type == DT_REG && strstr(entry->d_name, ".c") != NULL) {
                cFlag++;
            }
        }
    } else {
        res->c_files = -1;
    }

    if (cFlag >= 0) {
        res->c_files = cFlag;
    }
    // Close the directory and return DirResult
    closedir(dir);
    return *res;
}
// Print directory information
void PrintDirInfo(char *path, int *start, DirOptions opts) {
    // Wait for all options to get entered and start variable set to 1
    while (!*start);
    DirResult dirres;
    char *result = (char *) malloc(1000 * sizeof(char));
    char *dirpath = (char *) malloc(sizeof(char));
    dirres = getDirInfo(opts);
    sprintf(result, "------------------------------------------\nDirectory Path:%s\n", path);
    if (opts.name)
        sprintf(eos(result), "Directory Name: %s\n", get_folder_name(dirres.name));
    if (opts.size >= 0)
        sprintf(eos(result), "Directory total size: %ld\n",dirres.size);
    if (opts.perms)
        sprintf(eos(result), "Permissions:\n%s", print_permissions(dirres.access));
    if (opts.c_files >= 0)
        sprintf(eos(result), "Total Number of c File: %d\n", dirres.c_files);
    sprintf(dirpath, "%s/%s_file.txt", path, get_folder_name(dirres.path));
    // Create a child process to create a new file
    int st;
    pid_t p = fork();
    if (p > 0) { // Main process
        waitpid(p, &st, WUNTRACED);
        sprintf(eos(result), "%s", !st ? "Successfully created file.\n" : "Error creating file.\n");
        sprintf(eos(result), "Process with PID %d exited with code %d\n", p, st);
    } else if (p == 0){ // Child process
        create_file(dirpath, "");
        exit(0);
    }
    sprintf(eos(result), "%s", "------------------------------------------\n");
    printf("%s",result);
}
// Get options for regular file and return a FileOptions structure
FileOptions GetFileOptions(char *dirPath){
    char options[7] = {'n', 'd', 'a', 'h', 'm', 'l', '\0'};
    char input[10];
    int invalidOption;
    char symlinkname[20] = {'\0'};
    // Initialize a flag for each option and save it to FileOptions structure
    int nFlag = 0, dFlag = -1, aFlag = 0, hFlag = -1, lFlag = 0, mFlag = 0;
    FileOptions opts = {strdup(dirPath), nFlag, dFlag, hFlag, mFlag, aFlag, lFlag, strdup("")};
    struct stat filestat;
    if (stat(dirPath, &filestat) != 0) {
        perror("stat");
        exit(0);
    }

    // Print directory name and message
    printf("File: %s\n", dirPath);
    printf("This is a regular file\n");

    // Print options string
    printf("Options:\n");
    printf("-n: Name\n");
    printf("-d: Size\n");
    printf("-a: Access rights\n");
    printf("-m: Last modification time\n");
    printf("-l: Create a symbolic link\n");
    printf("-h: Hard link count\n");

    // Get the user input and update the flags according to the entered options
    do {
        invalidOption = 0;
        printf("Enter options (-[n/d/a/m/l/h]): ");
        scanf("%s", input);
        if (input[0] != '-') {
            invalidOption = 1;
        } else {
            for (int i = 1; i < (int)strlen(input); i++) {
                if (strchr(options, input[i]) == NULL) {
                    invalidOption = 1;
                    break;
                } else {
                    switch (input[i]) {
                        case 'n':
                            nFlag = 1;
                            break;
                        case 'd':
                            dFlag = 0;
                            break;
                        case 'a':
                            aFlag = 1;
                            break;
                        case 'm':
                            mFlag = 1;
                            break;
                        case 'h':
                            hFlag = 0;
                            break;
                        case 'l':
                            lFlag = 1;
                            while (!strlen(symlinkname)) { // Get sym link name in case of l option is entered
                                printf("Enter symbolic link name: ");
                                scanf("%s", symlinkname);
                            }
                            break;
                    }
                }
            }
        }
        if (invalidOption) {
            printf("Error: Invalid option\n");
        }
    } while (invalidOption);
    // Update the DirOptions structure with the updated flags and return it.
    opts.path = strdup(dirPath);
    opts.hard_link = hFlag;
    opts.last_modification = mFlag;
    opts.symbolic = lFlag;
    if (lFlag){
        opts.symbolic_name = strdup(symlinkname);
    }
    opts.size = dFlag;
    opts.perms = aFlag;
    opts.name = nFlag;
    return opts;
}
// Get file information and return a FileResult structure
FileResult getFileInfo(FileOptions opts){
    struct stat dirStat;
    int nFlag = opts.name;
    int aFlag = opts.perms;
    int dFlag = opts.size;
    int hFlag = opts.hard_link;
    int lFlag = opts.symbolic;
    int mFlag = opts.last_modification;
    char *dirPath = strdup(opts.path);
    lstat(dirPath, &dirStat);
    FileResult *res = (FileResult *) malloc(sizeof(struct fileresult));
    res->path = strdup(dirPath);
    // Get the required file info according to the saved options and return a FileResult structure
    if (lFlag){
        symlink(dirPath, opts.symbolic_name);
        res->symbolic = 1;
    }
    if (nFlag) {
        res->name = strdup(get_folder_name(dirPath));
    }
    if (dFlag == 0) {
        res->size = dirStat.st_size;
    } else {
        res->size = -1;
    }
    if (aFlag) {
        res->access = dirStat.st_mode & 0777;
    }
    if (hFlag == 0) {
        res->hard_link = dirStat.st_nlink;
    } else {
        res->hard_link = -1;
    }
    if (mFlag){
        res->mtime = dirStat.st_mtime;
    }
    return *res;
}
// Print file information
void PrintFileInfo(char *path, int *start, FileOptions opts){
    // Wait for all options to get entered and start variable set to 1
    while (!*start);
    FileResult symres;
    char *result = (char *) malloc(1000 * sizeof(char));
    symres = getFileInfo(opts);
    sprintf(result, "------------------------------------------\nDirectory Path:%s\n", path);
    if (opts.name)
        sprintf(eos(result), "File Name: %s\n", symres.name);
    if (opts.size >= 0)
        sprintf(eos(result), "File size: %ld\n", symres.size);
    if (opts.perms)
        sprintf(eos(result), "Permissions:\n%s", print_permissions(symres.access));
    if (opts.hard_link >= 0)
        sprintf(eos(result), "Hard link count: %d\n", symres.hard_link);
    if (opts.last_modification) {
        sprintf(eos(result), "Last modification time: %s\n", ctime(&symres.mtime));
    }
    if (opts.symbolic) {
        sprintf(eos(result), "Symbolic link created: %s\n", opts.symbolic_name);
    }
    // Check file extension
    if (check_file_extension(path, ".c")) { // If c file, compile_file_in_child function will be called to compile the file in child process
        double score = compile_file_in_child((char *)symres.path);
        sprintf(eos(result), "Score: %lf\n", score);
        // Append the score to grades.txt file
        char filecontent[100];
        sprintf(filecontent, "%s:%lf\n", get_folder_name(symres.path), score);
        create_file("grades.txt", filecontent);
    } else { // If a regular file, the line count will be printed
        sprintf(eos(result), "Line Count: %d\n", count_lines_in_file(symres.path));
    }
    sprintf(eos(result), "%s", "------------------------------------------\n");
    printf("%s",result);
}

char* print_permissions(int permissions) {
    // The function uses bitwise AND operation to check if the corresponding bit is set for each permission, and prints "yes" or "no" accordingly.
    // User permissions
    char *perms = (char *) malloc(1000 * sizeof(char));
    sprintf(perms, "User:\n");
    sprintf(eos(perms), "\tRead - %s\n", (permissions & 0400) ? "yes" : "no");
    sprintf(eos(perms), "\tWrite - %s\n", (permissions & 0200) ? "yes" : "no");
    sprintf(eos(perms), "\tExec - %s\n", (permissions & 0100) ? "yes" : "no");

    // Group permissions
    sprintf(eos(perms), "\nGroup:\n");
    sprintf(eos(perms), "\tRead - %s\n",(permissions & 040) ? "yes" : "no");
    sprintf(eos(perms), "\tWrite - %s\n", (permissions & 020) ? "yes" : "no");
    sprintf(eos(perms), "\tExec - %s\n", (permissions & 010) ? "yes" : "no");

    // Other permissions
    sprintf(eos(perms), "\nOthers:\n");
    sprintf(eos(perms), "\tRead - %s\n",(permissions & 04) ? "yes" : "no");
    sprintf(eos(perms), "\tWrite - %s\n", (permissions & 02) ? "yes" : "no");
    sprintf(eos(perms), "\tExec - %s\n", (permissions & 01) ? "yes" : "no");
    return perms;
}
// Calculates the actual data size inside a directory
off_t calculate_directory_size(char *path) {
    // The fts functions are provided for traversing file hierarchies
    FTS *fts;
    FTSENT *entry;
    off_t total_size = 0;

    char *path_argv[] = {path, NULL};
    int fts_options = FTS_PHYSICAL | FTS_NOCHDIR;
    // Open the directory
    fts = fts_open(path_argv, fts_options, NULL);

    if (!fts) {
        perror("fts_open");
        return -1;
    }
    // Loop through all files and add its size to total_size variable
    while ((entry = fts_read(fts)) != NULL) {
        if (entry->fts_info == FTS_F) {
            total_size += entry->fts_statp->st_size;
        }
    }
    // Close the directory and return total size
    fts_close(fts);

    return total_size;
}
// Get the folder or file name from path.
char *get_folder_name(const char *path) {
    char *folder_name = NULL;
    // Right split path string with '/' and return the folder name
    if (path) {
        folder_name = strrchr(path, '/');
        if (folder_name) {
            folder_name++;
        } else {
            folder_name = (char *)path;
        }
    }

    return folder_name;
}
// Opens a file and write a content. Creates the file if it doesn't exist
int create_file(char* filename, char* new_line) {
    // Open the file in append mode
    FILE* fp = fopen(filename, "a");
    if (fp == NULL) {
        // Handle error opening file
        printf("Error opening file %s\n", filename);
        return 0;
    }

    // Write the new line to the file
    fprintf(fp, "%s", new_line);

    // Close the file
    fclose(fp);
    return 0;
}
// Calculates sym link's target size
long int calculate_symlink_target_size(char *symlink_path) {
    // get sym link info
    struct stat symlink_stat;
    int stat_res = lstat(symlink_path, &symlink_stat);
    // return in case of error
    if (stat_res == -1) {
        perror("Error getting symbolic link info");
        return -1;
    }
    // get sym link's target
    char *target_path = (char *)malloc(symlink_stat.st_size + 1);
    if (target_path == NULL) {
        perror("Error allocating memory");
        return -1;
    }
    // read the content of sym link (target path)
    int read_res = readlink(symlink_path, target_path, symlink_stat.st_size + 1);
    if (read_res == -1) {
        perror("Error reading symbolic link");
        free(target_path);
        return -1;
    }

    target_path[read_res] = '\0';
    // get target's info
    struct stat target_stat;
    int target_stat_res = stat(target_path, &target_stat);

    free(target_path);

    if (target_stat_res == -1) {
        perror("Error getting target file info");
        return -1;
    }
    //return target's size
    return target_stat.st_size;
}
// sets file permission using chmod
int set_file_permissions(const char* filename, int permissions) {
    if (chmod(filename, permissions) == 0){
        exit(EXIT_SUCCESS);
    } else {
        exit(EXIT_FAILURE);
    }
}

// Checks the file type using lstat
enum FileType getFileType(const char *path) {
    struct stat st;
    if (lstat(path, &st) == -1) {
        fprintf(stderr, "Error: failed to stat file %s\n", path);
        exit(EXIT_FAILURE);
    }
	//check file type
    if (S_ISREG(st.st_mode)) {
        return FILE_TYPE_FILE;
    } else if (S_ISDIR(st.st_mode)) {
        return FILE_TYPE_DIRECTORY;
    } else if (S_ISLNK(st.st_mode)) {
        return FILE_TYPE_SYMBOLIC_LINK;
    } else {
        return FILE_TYPE_UNKNOWN;
    }
}
// Checks file extension
int check_file_extension(const char *path, const char *extension) {
    int len = strlen(extension);
    if ((int)strlen(path) > len && strcmp(path + strlen(path) - len, extension) == 0) {
        // the file has the given extension
        return 1;
    } else {
        // the file does not have the given extension
        return 0;
    }
}
// Opens a file and count its lines
int count_lines_in_file(const char *filename) {
    // Open the file or return in case of error
    FILE *file = fopen(filename, "r");
    if (!file) {
        printf("Error opening file\n");
        return -1;
    }
    int line_count = 0;
    int ch = 0;
    // get a char till the end of the file and check if the char equals to '\n'
    while ((ch = fgetc(file)) != EOF) {
        if (ch == '\n') {
            line_count++;
        }
    }
    fclose(file);
    return line_count;
}
// Calculate the score according to the given formula
double calculateScore(int errors, int warnings) {
    double score;
    if (errors == 0 && warnings == 0) {
        score = 10;
    } else if (errors >= 1) {
        score = 1;
    } else if (warnings > 10) {
        score = 2;
    } else {
        score = 2 + 8 * (10 - warnings) / 10;
    }
    return score;
}
// Function to compile c file in child process
double compile_file_in_child(char *argv){
    // Create pipes and exit in case of error
    int pipe1[2], pipe2[2];
    if(pipe(pipe1) < 0)
    {
        perror("Error :  Creating first pipe\n");
        exit(2);
    }

    if(pipe(pipe2) < 0)
    {
        perror("Error : Creating second pipe\n");
        exit(3);
    }

    int pid1,pid2;

    if((pid1=fork())<0)
    {
        perror("Error \n");
        exit(4);
    }

    //code of child 1
    if(pid1==0)
    {
        if(close(pipe1[0]) < 0)
        {
            perror("Error :  Closing the read end of pipe1 in first child\n");
            exit(5);
        }

        if(close(pipe2[0]) < 0)
        {
            perror("Error : Closing the read end of pipe2 in first child \n");
            exit(6);
        }

        if(close(pipe2[1]) < 0)
        {
            perror("Error :  Closing the write end of pipe1 in first child\n");
            exit(7);
        }

        dup2(pipe1[1], 2);

        execlp("gcc", "gcc", "-Wall", "-o", "prog", argv, NULL);


        exit(-1);
    }

    if((pid2=fork())<0)
    {
        perror("Error\n");
        exit(4);
    }

    //code of child2
    if(pid2==0)
    {
        if(close(pipe1[1]) < 0)
        {
            perror("Error :  Closing the write end of pipe1 in second child\n");
            exit(8);
        }

        if(close(pipe2[0]) < 0)
        {
            perror("Error : Closing the read end of pipe2 in second child\n");
            exit(9);
        }

        dup2(pipe1[0], 0);

        dup2(pipe2[1], 1);

        execlp("grep", "grep", "(error|warning)", "-E", NULL);

        exit(-2);
    }

    //code of parent
    if(close(pipe1[0]) < 0)
    {
        perror("Error : Closing read end of pipe1 in parent\n");
        exit(10);
    }

    if(close(pipe1[1]) < 0)
    {
        perror("Error : Closing write end of pipe1 in parent\n");
        exit(11);
    }

    if(close(pipe2[1]) < 0)
    {
        perror("Error : Closing write end of pipe2 in parent\n");
        exit(12);
    }

    int iserror=0;
    int war=0;

    int rparent;
    char ch;
    char text_buffer[40960]="";
    int len=0;

    while((rparent=read(pipe2[0], &ch, sizeof(char))) != 0)
    {
        if (rparent < 0)
        {
            perror("Error\n");
            exit(13);
        }

        text_buffer[len]=ch;
        len++;
    }

    if(close(pipe2[0]) < 0)
    {
        perror("Error : Closing read end of pipe2 in parent\n");
        exit(14);
    }

    if(strstr(text_buffer, "error"))
        iserror=1;

    char sep[]=" \r\n,.!?";

    char *p;

    p=strtok(text_buffer, sep);

    while(p)//counting warnings
    {
        if(strcmp(p, "warning") == 0)
            war++;
        p=strtok(NULL, sep);
    }

    double result = calculateScore(iserror, war);

    char buff[20480];
    int status, wait_pid;
    int k;
    for(k=0;k<2;k++)
    {
        if((wait_pid=wait(&status)) < 0)
        {
            perror("Error\n");
            exit(15);
        }

        if(WIFEXITED(status))
            continue;
        else
        {
            sprintf(buff, "Child process with PID %d exited with error code %d", wait_pid, WEXITSTATUS(status));
            if(write(2, buff, strlen(buff)) < 0)
            {
                perror("Error\n");
                exit(-3);
            }
        }
    }
    return result;
}
