#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <hashmap.h>

#define BUFFER_SIZE 1024 * 128
#define MAX_EXTENSIONS 50
#define MAX_EXTENSION_LEN 20

// Function prototype for the callback
typedef void (*file_callback)(const char* filepath, void* user_data);

bool endswith(const char *str, const char *suffix) {
    if (str == NULL || suffix == NULL) return false;
    size_t lenstr = strlen(str);
    size_t lensuffix = strlen(suffix);

    if (lensuffix > lenstr) return false;
    return strncmp(str + lenstr - lensuffix, suffix, lensuffix) == 0;
}

int walk_directory(const char* dir_path, char** valid_exts, file_callback callback, void* user_data) {
    DIR* dir;
    struct dirent* entry;
    // Buffer size for paths - 4096 is a common filesystem path length limit
    #define PATH_MAX 4096
    char full_path[PATH_MAX];
    struct stat path_stat;

    // open dir
    dir = opendir(dir_path);
    if (dir == NULL) {
        fprintf(stderr, "Failed to open directory '%s': %s\n", dir_path, strerror(errno));
        return -1;
    }

    // Read dir contents
    while ((entry = readdir(dir)) != NULL) {
        // Skip "." and ".."
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        // Get full path
        snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);

        // Get the file status
        if (stat(full_path, &path_stat) == -1) {
            fprintf(stderr, "Failed to get status for '%s': %s\n", full_path, strerror(errno));
            continue;
        }
        
        if (S_ISDIR(path_stat.st_mode)) {
            // Recurse into it
            walk_directory(full_path, valid_exts, callback, user_data);
        } else if (S_ISREG(path_stat.st_mode)) {
            bool ext_matches = false;
            for (int i = 0; i < MAX_EXTENSIONS; i++) {
                char* ext = valid_exts[i];
                if (ext == NULL) break;
                if (endswith(entry->d_name, ext)) {
                    ext_matches = true;
                    break;
                }
            }
            if (!ext_matches) continue;
            // call callback
            if (callback) {
                callback(full_path, user_data);
            }
        }

    }
    closedir(dir);
    return 0;
}

void destroy_value(void* value) {
    free(value);
}

// Simple checksum that just sums all the ascii bytes.
uint64_t simple_checksum(const char* filepath) {
    FILE* fh = fopen(filepath, "rb");
    char buffer[BUFFER_SIZE];
    size_t bytes = 0;

    uint64_t sum = 0;

    while ((bytes = fread(buffer, 1, BUFFER_SIZE, fh)) > 0) {
        for (int b=0; b < bytes; b++) {
            sum += (uint64_t)buffer[b];
        }
    }
    return sum;
}

void check_file(const char* filepath, void* user_data) {
    HashMap* map = (HashMap*)user_data;

    // TODO: check file extension

    // TODO: get a checksum
    uint64_t new_checksum = simple_checksum(filepath); 

    uint64_t* checksum = hm_get(map, filepath);
    bool change_detected = false;
    if (checksum == NULL) {
        // Set it and run command
        checksum = malloc(sizeof(uint64_t));
        if (checksum == NULL) return;
        fprintf(stderr, "////////////////////////////////////////////////////////////////////////////////\n");
        fprintf(stderr, "New file found: %s\n", filepath);
        *checksum = new_checksum;
        hm_put(map, filepath, checksum);
        change_detected = true;
    } else {
        if (*checksum == new_checksum) return;
        fprintf(stderr, "////////////////////////////////////////////////////////////////////////////////\n");
        fprintf(stderr, "Change detected in: %s\n", filepath);
        // Update and run command
        *checksum = new_checksum;
        change_detected = true;
    }
    if (change_detected)  {
        system("make");
    }
}


char** parse_extensions(char* exts) {
    char** parsed = calloc(MAX_EXTENSIONS, sizeof(char*));
    int count = 0;
    if (parsed == NULL) return NULL;

    char *token = strtok(exts, ",");
    while (token != NULL) {
        parsed[count] = strdup(token);
        token = strtok(NULL, ",");
        count++;
    }
    return parsed;
}

void print_help() {
    fprintf(stderr, "monitor <DIR> <EXTS>\n");
    fprintf(stderr, "\t<EXTS> are a comma-delimited list like '.c,.py'\n");
    fprintf(stderr, "\tA very limited file watcher and command runner.\n");
    fprintf(stderr, "\tThis will run 'make' when a file is changed, for each file changed.\n");
    fprintf(stderr, "\tThis will changdir to the passe dir and then scan the files ever tenth of a second\n");
    fprintf(stderr, "\tusing a naive sum of the file bytes as a checksum.\n");
}

// A very limited file watcher and command runner.
// This will run 'make' when a file is changed, for each file changed.
// This will changdir to the passe dir and then scan the files ever tenth of a second
// using a naive sum of the file bytes as a checksum.
int main(int argc, char** args) {

    if (argc < 3) {
        print_help();
        // TODO: proper error code
        return -1;
    }
    // Change dir to the dir in question
    chdir(args[1]);

    char** exts = parse_extensions(args[2]);
    for (int i = 0; i < MAX_EXTENSIONS; i++) {
        char* ext = exts[i];
        if (ext == NULL) break;
        fprintf(stderr, "Found Ext: '%s'\n", ext);
    }
    fprintf(stderr, "Monitoring %s\n", args[1]);
    HashMap* map = hm_create(16, destroy_value);

    for (;;) {
        walk_directory(args[1], exts, check_file, map);
        usleep(100000);
    }

    hm_destroy(map);
    return 0;
}
