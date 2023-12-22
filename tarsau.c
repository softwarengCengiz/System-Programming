#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>

#define MAX_FILES 32
#define MAX_TOTAL_SIZE (200 * 1024 * 1024)

typedef struct {
    char name[256];
    char permissions[10];
    size_t size;
} FileInfo;

void createArchive(int argc, char *argv[]);
size_t getFileSize(const char *filename);

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: tarsau -b files... -o arsiv.sau OR tarsau -a arsiv.sau directory\n");
        return 1;
    }

    if (strcmp(argv[1], "-b") == 0) {
        createArchive(argc, argv);
    } else {
        printf("Invalid transaction. Use -b to create an archive or -a to extract it.");
        return 1;
    }

    return 0;
}

void createArchive(int argc, char *argv[]) {
    const char *outputFile = "a.sau"; // default
    for (int i = 0; i < argc; ++i) {
        if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            outputFile = argv[i + 1];
            break;
        }
    }

    size_t totalSize = 0;
    FileInfo files[MAX_FILES];
    int fileCount = 0;

    for (int i = 2; i < argc; ++i) {
        if (strcmp(argv[i], "-o") == 0) {
            i++;
            continue;
        }

        if (fileCount >= MAX_FILES) {
            printf("Too many files. Maximum allowed is 32.\n");
            exit(1);
        }

        const char *filename = argv[i];
        size_t fileSize = getFileSize(filename);

        if (fileSize == 0 || totalSize + fileSize > MAX_TOTAL_SIZE) {
            printf("Invalid file or total size limit exceeded.\n");
            exit(1);
        }

        strcpy(files[fileCount].name, filename);
        files[fileCount].size = fileSize;
        strcpy(files[fileCount].permissions, "rw-r--r--"); // Basitleştirme: izinler 644 olarak kabul ediliyor

        totalSize += fileSize;
        fileCount++;
    }

    FILE *archive = fopen(outputFile, "wb");
    if (!archive) {
        printf("An error occurred while creating the archive file.\n");
        exit(1);
    }

    fprintf(archive, "%010zu", totalSize);
    for (int i = 0; i < fileCount; ++i) {
        fprintf(archive, "|%s,%s,%zu|", files[i].name, files[i].permissions, files[i].size);
    }

    for (int i = 0; i < fileCount; ++i) {
        FILE *file = fopen(files[i].name, "rb");
        if (!file) {
            printf("The file could not be opened during the reading operation.\n");
            exit(1);
        }

        char buffer[1024];
        size_t bytesRead;

        while ((bytesRead = fread(buffer, 1, sizeof(buffer), file)) > 0) {
            fwrite(buffer, 1, bytesRead, archive);
        }

        fclose(file);
    }

    fclose(archive);
    printf("The files have been merged.\n");
}

size_t getFileSize(const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        printf("The file could not be opened during the size programming process.\n");
        exit(1);
    }

    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    fclose(file);

    return size;
}
