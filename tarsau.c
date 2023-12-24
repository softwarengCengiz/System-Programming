#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#define MAX_FILES 32
#define MAX_TOTAL_SIZE (200 * 1024 * 1024)

typedef struct {
    char name[256];
    mode_t permissions;
    size_t size;
} FileInfo;

void createArchive(int argc, char *argv[]);
size_t getFileSize(const char *filename);


void extractArchive(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Usage: tarsau -a archive.sau directory\n");
        exit(1);
    }

    const char *archiveFile = argv[2]; // a.sau
    const char *outputDirectory = argv[3]; // s1

    // Kontrol edilecek dizinin var olup olmadığını kontrol et
    struct stat st;
    if (stat(outputDirectory, &st) != 0) {
        // Dizin yoksa, oluştur
        if (mkdir(outputDirectory, 0777) != 0) {
            perror("Error creating output directory");
            exit(1);
        }
    } else if (!S_ISDIR(st.st_mode)) {
        // Varolan bir dosya varsa hata ver
        fprintf(stderr, "Error: %s is not a directory\n", outputDirectory);
        exit(1);
    }

    // Dosyayı aç
    FILE *file = fopen(archiveFile, "r");
    if (file == NULL) {
        perror("Error opening archive file");
        exit(1);
    }

    // Dosyanın boyutunu al
    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    rewind(file);

    // Dosya içeriğini bir tampona oku
    char *buffer = (char *)malloc(fileSize * sizeof(char));
    if (buffer == NULL) {
        perror("Error allocating memory for file content");
        fclose(file);
        exit(1);
    }

    fread(buffer, sizeof(char), fileSize, file);

    // Dosya isimlerini ayrıştır ve consola yazdır
    char *start = buffer;

    // | işaretinin son konumu
    char *lastSeperatorLocation = strrchr(start, '|');
    char *startSeperateText = lastSeperatorLocation + 1;

    while (1) {
        char *separator = strchr(start, '|');
        if (separator == NULL) {
            break;
        }

        char *filenameStart = separator + 1;
        char *filenameEnd = strchr(filenameStart, ',');
        if (filenameEnd == NULL) {
            break;
        }

        *filenameEnd = '\0';

        // Dosya boyutunu al
        int boyut;
        if (sscanf(filenameEnd + 1, "%*d,%d", &boyut) == 1) {
            // Dosya Adi: %s, Boyut: %d
            // printf("Dosya Adi: %s, Boyut: %d\n", filenameStart, boyut);

            // Dosyanın içeriğini dosyaya yaz
            char filePath[256];
            snprintf(filePath, sizeof(filePath), "%s/%s", outputDirectory, filenameStart);

            FILE *outputFile = fopen(filePath, "w");
            if (outputFile == NULL) {
                perror("Error creating output file");
                fclose(file);
                free(buffer);
                exit(1);
            }

            // Dosyanın içeriğini yaz
            fprintf(outputFile, "%.*s", boyut, startSeperateText);

            // Konsol ekranına da yaz
            //printf("Dosya Icerigi:\n%.*s\n", boyut, startSeperateText);

            // Dosyayı kapat
            fclose(outputFile);

            // İlerleme
            startSeperateText += boyut;
        }

        start = filenameEnd + 1;
    }

    // Bellek ve dosya kaynaklarını temizle
    free(buffer);
    fclose(file);
}



int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: tarsau -b files... -o arsiv.sau OR tarsau -a arsiv.sau directory\n");
        return 1;
    }

    if (strcmp(argv[1], "-b") == 0) {
        createArchive(argc, argv);
    } else if (strcmp(argv[1], "-a") == 0) {
        extractArchive(argc, argv);
    } else {
        printf("Invalid transaction. Use -b to create an archive or -a to extract it.");
        return 1;
    }

    return 0;
}

// CREATE ARCHIVE
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

        // Orijinal dosyanın izinlerini kullan
        struct stat st;
        if (stat(filename, &st) != 0) {
            perror("stat");
            exit(1);
        }
        files[fileCount].permissions = st.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO);

        totalSize += fileSize;
        fileCount++;
    }

    FILE *archive = fopen(outputFile, "wb");
    if (!archive) {
        printf("An error occurred while creating the archive file.\n");
        exit(1);
    }

    fprintf(archive, "%010zu|", totalSize);
    for (int i = 0; i < fileCount; ++i) {
        fprintf(archive, "%s,%o,%zu|", files[i].name, files[i].permissions, files[i].size);
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


//GET FILE SIZE
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
