#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>

#define MAX_PATH_LENGTH 1024
#define MAX_METADATA_LENGTH 1024
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
void createSnapshot(const char *dirPath, EntryMetadata *entries, int numEntries, const char *outputDir) {
    char snapshotPath[MAX_PATH_LENGTH];
    snprintf(snapshotPath, sizeof(snapshotPath), "%s/Snapshot.txt", outputDir);

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

// Funcție pentru citirea și compararea snapshot-urilor anterioare și actuale
void compareSnapshots(const char *dirPath, EntryMetadata *entries, int numEntries, const char *outputDir) {
    char oldSnapshotPath[MAX_PATH_LENGTH];
    snprintf(oldSnapshotPath, sizeof(oldSnapshotPath), "%s/Snapshot.txt", outputDir);

    char newSnapshotPath[MAX_PATH_LENGTH];
    snprintf(newSnapshotPath, sizeof(newSnapshotPath), "%s/Snapshot_new.txt", outputDir);

    // Citim snapshot-ul vechi
    FILE *oldSnapshotFile = fopen(oldSnapshotPath, "r");
    if (oldSnapshotFile == NULL) {
        perror("Error opening old snapshot file");
        return;
    }

    // Cream snapshot-ul nou
    FILE *newSnapshotFile = fopen(newSnapshotPath, "w");
    if (newSnapshotFile == NULL) {
        perror("Error creating new snapshot file");
        fclose(oldSnapshotFile);
        return;
    }

    char line[MAX_METADATA_LENGTH];
    char *entrySnapshot = malloc(MAX_METADATA_LENGTH * 2 * sizeof(char)); // Allocați un buffer dinamic pentru metadate
    if (entrySnapshot == NULL) {
        perror("Error allocating memory");
        fclose(oldSnapshotFile);
        fclose(newSnapshotFile);
        return;
    }

    // Copiem snapshot-ul vechi în cel nou
    while (fgets(line, sizeof(line), oldSnapshotFile) != NULL) {
        fprintf(newSnapshotFile, "%s", line);
    }

    // Verificăm fiecare intrare și actualizăm snapshot-ul dacă există modificări
    for (int i = 0; i < numEntries; i++) {
        snprintf(entrySnapshot, MAX_METADATA_LENGTH * 2, "%s: %s\n", entries[i].name, entries[i].metadata);

        // Căutăm intrarea în snapshot-ul vechi
        fseek(oldSnapshotFile, 0, SEEK_SET);
        int found = 0;
        while (fgets(line, sizeof(line), oldSnapshotFile) != NULL) {
            if (strcmp(line, entrySnapshot) == 0) {
                found = 1;
                break;
            }
        }

        // Dacă nu am găsit intrarea, adăugăm intrarea nouă în snapshot-ul nou
        if (!found) {
            fprintf(newSnapshotFile, "%s", entrySnapshot);
        }
    }

    fclose(oldSnapshotFile);
    fclose(newSnapshotFile);
    free(entrySnapshot); // Eliberăm memoria alocată pentru buffer-ul dinamic

    // Suprascriem snapshot-ul vechi cu cel nou
    remove(oldSnapshotPath);
    rename(newSnapshotPath, oldSnapshotPath);
}

int main(int argc, char *argv[]) {
    // Verificăm dacă numărul de argumente este valid
    if (argc < 4 || argc > 12) {
        printf("Usage: %s -o output_directory dir1 dir2 ... (up to 10 directories)\n", argv[0]);
        return 1;
    }

    // Verificăm dacă primul argument este "-o"
    if (strcmp(argv[1], "-o") != 0) {
        printf("Error: First argument must be '-o' followed by the output directory\n");
        return 1;
    }

    char *outputDir = argv[2];

    // Inițializăm un vector pentru a stoca metadatele fiecărei intrări din toate directoarele
    EntryMetadata entries[MAX_ENTRIES];
    int numEntries = 0;

    // Listăm fișierele și directoarele pentru fiecare director specificat și adăugăm metadatele în vectorul de intrări
    for (int i = 3; i < argc; i++) {
        if (!listFiles(argv[i], entries, &numEntries)) {
            return 1;
        }
    }

    // Creăm sau actualizăm snapshot-urile pentru toate directoarele specificate
    for (int i = 3; i < argc; i++) {
        compareSnapshots(argv[i], entries, numEntries, outputDir);
    }

    printf("Snapshots updated successfully.\n");

    return 0;
}
