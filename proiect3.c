#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_PATH_LENGTH 1024

// Funcție pentru crearea snapshot-ului pentru un director dat
void createSnapshot(const char *dirPath, const char *outputDir) {
    // Implementăm aici logica pentru crearea snapshot-ului
    // Folosim funcția fork() pentru a crea un proces copil
    pid_t pid = fork();

    if (pid == -1) {
        // Eroare la crearea procesului copil
        perror("Error creating child process");
        exit(1);
    } else if (pid == 0) {
        // Suntem în procesul copil
        char snapshotCommand[MAX_PATH_LENGTH];
        snprintf(snapshotCommand, sizeof(snapshotCommand), "snapshot_creator %s %s", dirPath, outputDir);
        system(snapshotCommand);
        exit(0);
    }
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

    // Parcurgem argumentele pentru directoarele de monitorizat și creăm un proces copil pentru fiecare
    for (int i = 3; i < argc; i++) {
        createSnapshot(argv[i], outputDir);
    }

    // Suntem în procesul părinte
    // Așteptăm ca fiecare proces copil să se încheie și afișăm un mesaj corespunzător
    for (int i = 3; i < argc; i++) {
        int status;
        pid_t child_pid = waitpid(-1, &status, 0);
        if (child_pid != -1) {
            if (WIFEXITED(status)) {
                printf("Child Process %d terminated with PID %d and exit code %d.\n", i-2, child_pid, WEXITSTATUS(status));
            } else {
                printf("Child Process %d terminated abnormally.\n", i-2);
            }
        } else {
            perror("Error waiting for child process");
        }
    }

    return 0;
}
