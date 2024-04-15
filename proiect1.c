#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>

#define MAX_PATH_LENGTH 1024
#define MAX_METADATA_LENGTH 512
#define MAX_ENTRIES 1000

// Structura pentru stocarea metadatelor fiecărei intrări
typedef struct {
    char name[MAX_PATH_LENGTH];
    char metadata[MAX_METADATA_LENGTH];
} EntryMetadata;

// Funcție pentru listarea fișierelor și directoarelor și adăugarea metadatelor în structura EntryMetadata
int listFiles(const char *dirPath, EntryMetadata *entries, int *numEntries) {
    DIR *dir;
    struct dirent *entry;
    struct stat statbuf;

    if ((dir = opendir(dirPath)) == NULL) {
        perror("Error opening directory");
        return 0;
    }

    while ((entry = readdir(dir)) != NULL) {
        char path[MAX_PATH_LENGTH];
        snprintf(path, sizeof(path), "%s/%s", dirPath, entry->d_name);

        if (stat(path, &statbuf) == -1) {
            perror("Error getting file status");
            continue;
        }

        if (S_ISDIR(statbuf.st_mode)) {
            // este director
            if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                // ignorăm directorul curent și directorul părinte
                strcpy(entries[*numEntries].name, entry->d_name);
                strcpy(entries[*numEntries].metadata, "Directory");
                (*numEntries)++;
            }
        } else {
            // este fișier
            strcpy(entries[*numEntries].name, entry->d_name);
            sprintf(entries[*numEntries].metadata, "File, Size: %ld bytes, Last modified: %s", statbuf.st_size, ctime(&statbuf.st_mtime));
            (*numEntries)++;
        }
    }

    closedir(dir);
    return 1;
}

// Funcție pentru crearea sau actualizarea snapshot-ului
void createSnapshot(const char *dirPath, EntryMetadata *entries, int numEntries) {
    char snapshotPath[MAX_PATH_LENGTH];
    snprintf(snapshotPath, sizeof(snapshotPath), "%s/Snapshot.txt", dirPath);

    FILE *snapshotFile = fopen(snapshotPath, "w");
    if (snapshotFile == NULL) {
        perror("Error creating snapshot file");
        return;
    }

    fprintf(snapshotFile, "Snapshot of directory: %s\n", dirPath);
    fprintf(snapshotFile, "--------------------------------------------\n");

    for (int i = 0; i < numEntries; i++) {
        fprintf(snapshotFile, "%s: %s\n", entries[i].name, entries[i].metadata);
    }

    fclose(snapshotFile);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <directory_path>\n", argv[0]);
        return 1;
    }

    char *dirPath = argv[1];

    // Inițializăm un vector pentru a stoca metadatele fiecărei intrări din director
    EntryMetadata entries[MAX_ENTRIES];
    int numEntries = 0;

    // Listăm fișierele și directoarele din directorul specificat și adăugăm metadatele în vectorul de intrări
    if (!listFiles(dirPath, entries, &numEntries)) {
        return 1;
    }

    // Creăm sau actualizăm snapshot-ul
    createSnapshot(dirPath, entries, numEntries);

    printf("Snapshot created successfully.\n");

    return 0;
}

