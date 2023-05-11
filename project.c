#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <dirent.h>
#include <fts.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>


enum FileType {
    FILE_TYPE_UNKNOWN,
    FILE_TYPE_FILE,
    FILE_TYPE_DIRECTORY,
    FILE_TYPE_SYMBOLIC_LINK
};

typedef struct dirResult {
    const char *path;
    const char *name;
    off_t size;
    int access;
    int c_files;
} DirResult;

typedef struct symbolicResult {
    const char *path;
    const char *name;
    off_t size;
    off_t target_size;
    int access;
    int deleted;
} SymbolicResult;

typedef struct diroptions {
    const char *path;
    int name;
    int size;
    int perms;
    int c_files;
} DirOptions;

typedef struct symoptions {
    const char *path;
    int name;
    int size;
    int target_size;
    int perms;
    int delete;
} SymbolicOptions;

char* print_permissions(int);
off_t calculate_directory_size(char *);
char *get_folder_name(char *);
enum FileType getFileType(const char *);
int create_file(char *, char *);
long int calculate_symlink_target_size(char *);
int set_file_permissions(const char *, int);
DirOptions GetDirectoryOptions(char *);
void PrintDirInfo(char *, int *, DirOptions);
void PrintSymInfo(char *, int *, SymbolicOptions);
SymbolicOptions GetSymbolicOptions(char *);

int main(int argc, char *argv[]) {
    int *start = mmap ( NULL, sizeof(int),
                      PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0 );
    *start = 0;
    DirResult dirres;
    SymbolicResult symres;
    DirOptions dir_opts;
    SymbolicOptions sym_opts;
    int fd[2];
    pid_t p;
    pipe(fd);
    char *dirpath = (char *) malloc(sizeof(char));
    for (int i = 1; i < argc; i++) {
        switch (getFileType(argv[i])) {
            case FILE_TYPE_UNKNOWN:
            case FILE_TYPE_FILE:
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
                }
        }
    }
    *start = 1;
    int st;
    pid_t p2;
    for (int i = 1; i < argc; i++){
        p2 = wait(&st);
        printf("Process with PID %d exited with code %d\n", p2, st);
    }
}

SymbolicOptions GetSymbolicOptions(char *dirPath){
    char options[6] = {'n', 'l', 'd', 't', 'a', '\0'};
    char input[10];
    int invalidOption;
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

    // Get user input
    do {
        invalidOption = 0;
        printf("Enter options (-[n/d/a/l/t]): ");
        scanf("%s", input);
        if (input[0] != '-') {
            invalidOption = 1;
        } else {
            for (int i = 1; i < strlen(input); i++) {
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
    opts.name = nFlag;
    opts.size = dFlag;
    opts.target_size = tFlag;
    opts.perms = aFlag;
    opts.delete = lFlag;
    return opts;
}
SymbolicResult getSymInfo(SymbolicOptions opts){
    struct stat dirStat;
    int nFlag = opts.name;
    int aFlag = opts.perms;
    int dFlag = opts.size;
    int tFlag = opts.target_size;
    int lFlag = opts.delete;
    char *dirPath = strdup(opts.path);
    lstat(dirPath, &dirStat);
    SymbolicResult *res = (SymbolicResult *) malloc(sizeof(struct symbolicResult));
    res->path = strdup(dirPath);
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
void PrintSymInfo(char *path, int *start, SymbolicOptions opts){
    while (!*start);
    SymbolicResult symres;
    char *result = (char *) malloc(1000 * sizeof(char));
    symres = getSymInfo(opts);
    sprintf(result, "------------------------------------------\nDirectory Path:%s\n", path);
    if (symres.deleted) {
        sprintf(result, "%sSymbolic link deleted.\n", result);
        goto end;
    }
    if (symres.name)
        sprintf(result, "%sSymbolic link Name: %s\n", result, symres.name);
    if (symres.size >= 0)
        sprintf(result, "%sSymbolic link size: %ld\n", result, symres.size);
    if (symres.access)
        sprintf(result, "%sPermissions:\n%s", result, print_permissions(symres.access));
    if (symres.target_size >= 0)
        sprintf(result, "%sSymbolic link target size: %ld\n", result, symres.target_size);
    int st;
    pid_t p = fork();
    if (p > 0) {
        waitpid(p, &st, WUNTRACED);
        sprintf(result, "%s%s", result, !st ? "Changed permissions to 760\n" : "Error changing permissions.\n");
        sprintf(result, "%sProcess with PID %d exited with code %d\n",result, p, st);
    } else if (p == 0){
        set_file_permissions(symres.path, 760);
    }
    end:
    sprintf(result, "%s%s",result, "------------------------------------------\n");
    printf(result);
}

DirOptions GetDirectoryOptions(char *dirPath){
    DIR *dir = opendir(dirPath);
    char options[5] = {'n', 'd', 'a', 'c', '\0'};
    char input[10];
    int invalidOption;

    int nFlag = 0, dFlag = -1, aFlag = 0, cFlag = -1;
    DirOptions opts = {strdup(dirPath), nFlag, dFlag, aFlag, cFlag};
    if (dir == NULL) {
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

    // Get user input
    do {
        invalidOption = 0;
        printf("Enter options (-[n/d/a/c]): ");
        scanf("%s", input);
        if (input[0] != '-') {
            invalidOption = 1;
        } else {
            for (int i = 1; i < strlen(input); i++) {
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
    closedir(dir);
    opts.path = strdup(dirPath);
    opts.c_files = cFlag;
    opts.size = dFlag;
    opts.perms = aFlag;
    opts.name = nFlag;
    return opts;
}
DirResult getDirInfo(DirOptions opts){
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
        while ((entry = readdir(dir)) != NULL) {

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

    closedir(dir);
    return *res;
}
void PrintDirInfo(char *path, int *start, DirOptions opts) {
    while (!*start);
    DirResult dirres;
    char *result = (char *) malloc(1000 * sizeof(char));
    char *dirpath = (char *) malloc(sizeof(char));
    dirres = getDirInfo(opts);
    sprintf(result, "------------------------------------------\nDirectory Path:%s\n", path);
    if (dirres.name)
        sprintf(result, "%sDirectory Name: %s\n",result, get_folder_name(dirres.name));
    if (dirres.size >= 0)
        sprintf(result,"%sDirectory total size: %ld\n", result,dirres.size);
    if (dirres.access)
        sprintf(result, "%sPermissions:\n%s", result, print_permissions(dirres.access));
    if (dirres.c_files >= 0)
        sprintf(result,"%sTotal Number of c File: %d\n",result, dirres.c_files);
    sprintf(dirpath, "%s/%s_file.txt", path, get_folder_name(dirres.path));
    int st;
    pid_t p = fork();
    if (p > 0) {
        waitpid(p, &st, WUNTRACED);
        sprintf(result, "%s%s", result, !st ? "Successfully created file.\n" : "Error creating file.\n");
        sprintf(result, "%sProcess with PID %d exited with code %d\n",result, p, st);
    } else if (p == 0){
        create_file(dirpath, "");
    }
    sprintf(result, "%s%s",result, "------------------------------------------\n");
    printf(result);
}

char* print_permissions(int permissions) {
    // User permissions
    char *perms = (char *) malloc(1000 * sizeof(char));
    sprintf(perms, "User:\n");
    sprintf(perms, "%s\tRead - %s\n",perms, (permissions & 0400) ? "yes" : "no");
    sprintf(perms, "%s\tWrite - %s\n",perms, (permissions & 0200) ? "yes" : "no");
    sprintf(perms, "%s\tExec - %s\n",perms, (permissions & 0100) ? "yes" : "no");

    // Group permissions
    sprintf(perms, "%s\nGroup:\n", perms);
    sprintf(perms, "%s\tRead - %s\n", perms,(permissions & 040) ? "yes" : "no");
    sprintf(perms, "%s\tWrite - %s\n",perms, (permissions & 020) ? "yes" : "no");
    sprintf(perms, "%s\tExec - %s\n",perms, (permissions & 010) ? "yes" : "no");

    // Other permissions
    sprintf(perms,"%s\nOthers:\n", perms);
    sprintf(perms,"%s\tRead - %s\n", perms,(permissions & 04) ? "yes" : "no");
    sprintf(perms,"%s\tWrite - %s\n",perms, (permissions & 02) ? "yes" : "no");
    sprintf(perms,"%s\tExec - %s\n",perms, (permissions & 01) ? "yes" : "no");
    return perms;
}
off_t calculate_directory_size(char *path) {
    FTS *fts;
    FTSENT *entry;
    off_t total_size = 0;

    char *path_argv[] = {path, NULL};
    int fts_options = FTS_PHYSICAL | FTS_NOCHDIR;

    fts = fts_open(path_argv, fts_options, NULL);

    if (!fts) {
        perror("fts_open");
        return -1;
    }

    while ((entry = fts_read(fts)) != NULL) {
        if (entry->fts_info == FTS_F) {
            total_size += entry->fts_statp->st_size;
        }
    }

    fts_close(fts);

    return total_size;
}
char *get_folder_name(char *path) {
    char *folder_name = NULL;

    if (path) {
        folder_name = strrchr(path, '/');
        if (folder_name) {
            folder_name++;
        } else {
            folder_name = path;
        }
    }

    return folder_name;
}
int create_file(char* filename, char* content) {
    char command[100];
    sprintf(command, "echo '%s' > %s", content, filename);
    if (system(command) == -1) {
        exit(EXIT_FAILURE);
    } else {
        exit(EXIT_SUCCESS);
    }
}
long int calculate_symlink_target_size(char *symlink_path) {
    struct stat symlink_stat;
    int stat_res = lstat(symlink_path, &symlink_stat);

    if (stat_res == -1) {
        perror("Error getting symbolic link info");
        return -1;
    }

    char *target_path = (char *)malloc(symlink_stat.st_size + 1);
    if (target_path == NULL) {
        perror("Error allocating memory");
        return -1;
    }

    int read_res = readlink(symlink_path, target_path, symlink_stat.st_size + 1);
    if (read_res == -1) {
        perror("Error reading symbolic link");
        free(target_path);
        return -1;
    }

    target_path[read_res] = '\0';

    struct stat target_stat;
    int target_stat_res = stat(target_path, &target_stat);

    free(target_path);

    if (target_stat_res == -1) {
        perror("Error getting target file info");
        return -1;
    }

    return target_stat.st_size;
}
int set_file_permissions(const char* filename, int permissions) {
    if (chmod(filename, permissions) == 0){
        exit(EXIT_SUCCESS);
    } else {
        exit(EXIT_FAILURE);
    }
}
enum FileType getFileType(const char *path) {
    struct stat st;
    if (lstat(path, &st) == -1) {
        fprintf(stderr, "Error: failed to stat file %s\n", path);
        exit(EXIT_FAILURE);
    }

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
