#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "Nodos.h"
#include "Tabla.h"


#define DEBUG printf("Aqui\n");

void CountCharacter(Node **list, unsigned char character);
typedef struct {
    char* name;
    int count;
} CharactersCount;

extern Table *table;
long int fileLength = 0;
int size = 0;

void processFile(const char *filePath, Node **list) {
  FILE *file = fopen(filePath, "r");
  if (!file) {
    printf("Error opening the file %s\n", filePath);
    return;
  }

  unsigned char character;
  unsigned int cant = 0;
  do{
    character = fgetc(file);
    if(feof(file))
      break;
    fileLength++; // Incrementa la longitud por cada carácter leído
    cant++;
    CountCharacter(list, character);
  }while (1);
  fclose(file);
}

void processDirectory(const char *directoryPath, Node** list) {
    struct dirent *entry;
    DIR *dp = opendir(directoryPath);

    if (dp == NULL) {
        perror("The directory can not be opened");
        return;
    }

    

    while ((entry = readdir(dp))) {
        // Ignorar las entradas "." y ".."
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        char filePath[1024];
        snprintf(filePath, sizeof(filePath), "%s/%s", directoryPath, entry->d_name);

        // Llama a processFile con la ruta completa
        processFile(filePath, list);
        size++;
        //printf("Process: %s\n", entry->d_name);
    }

    closedir(dp);
}

void CountCharacter(Node **list, unsigned char character) {
  Node *current, *previous, *newNode;
  if (!*list){ // If the list is empty, create a new node as the head 
    *list = (Node *)malloc(sizeof(Node)); // Create a new node
    (*list)->symbol = character;                  // Assign the character
    (*list)->count = 1; // Initialize the count to 1
    (*list)->next = (*list)->left = (*list)->right = NULL; // Initialize pointers
  } else {
    // Find the correct position in the list for the character
    current = *list;
    previous = NULL;
    while (current && current->symbol < character) {
      previous = current;      // Keep reference to the previous node
      current = current->next; // Move to the next node
    }

    // Check if the character already exists in the list
    if (current && current->symbol == character) {
      current->count++; // If it exists, increment its count
    } else {
      newNode = (Node *)malloc(sizeof(Node));
      newNode->symbol = character;
      newNode->left = newNode->right = NULL;
      newNode-> count = 1;
      newNode->next = current;
      if(previous) previous->next = newNode;
      else *list = newNode;
    }
  }
}

void compressFile(const char *filePath, const char *fileName, Node **list, int fileIndex) {
    FILE *fe = fopen(filePath, "r");
    if (!fe) {
        printf("Error opening the file %s\n", filePath);
        return;
    }

    // Crear un archivo temporal único para cada proceso hijo
    char tempFileName[256];
    snprintf(tempFileName, sizeof(tempFileName), "temp_compressed_%d.bin", fileIndex);
    FILE *tempFile = fopen(tempFileName, "wb");
    if (!tempFile) {
        printf("Error creating temporary file for %s\n", fileName);
        fclose(fe);
        return;
    }

    unsigned char byte = 0;
    int nBits = 0;
    int c;
    long totalCharacters = 0;  // Total de caracteres del archivo original
    long compressedBytes = 0;  // Contador de bytes comprimidos

    // Usar un buffer para almacenar el contenido comprimido
    unsigned char *compressedData = (unsigned char *)malloc(1024 * sizeof(unsigned char));
    size_t bufferSize = 1024;
    size_t bufferIndex = 0;

    // 1. Longitud del nombre del archivo
    int nameLength = strlen(fileName);
    fwrite(&nameLength, sizeof(int), 1, tempFile);

    // 2. Escribir el nombre del archivo
    fwrite(fileName, sizeof(char), nameLength, tempFile);

    // 3. Escribir el total de caracteres, pero primero contar los caracteres
    while ((c = fgetc(fe)) != EOF) {
        totalCharacters++;
    }

    // Escribir el total de caracteres en el archivo temporal
    fwrite(&totalCharacters, sizeof(long), 1, tempFile);

    // Reiniciar el puntero del archivo para comprimir
    rewind(fe);

    // 4. Comprimir el archivo y almacenar los datos en el buffer
    while ((c = fgetc(fe)) != EOF) {
        Table *node = findSymbol(table, (unsigned char)c);
        if (!node) {
            fprintf(stderr, "Symbol not found in the table: %c\n", c);
            continue;
        }

        for (int i = node->nBits - 1; i >= 0; i--) {
            unsigned char bit = (node->bits >> i) & 1;
            byte = (byte << 1) | bit;
            nBits++;

            if (nBits == 8) {
                if (bufferIndex >= bufferSize) {
                    bufferSize *= 2;
                    compressedData = (unsigned char *)realloc(compressedData, bufferSize);
                }
                compressedData[bufferIndex++] = byte;
                compressedBytes++;  // Incrementar contador de bytes comprimidos
                byte = 0;
                nBits = 0;
            }
        }
    }

    // Escribir el último byte si no está lleno
    if (nBits > 0) {
        byte <<= (8 - nBits);
        if (bufferIndex >= bufferSize) {
            bufferSize *= 2;
            compressedData = (unsigned char *)realloc(compressedData, bufferSize);
        }
        compressedData[bufferIndex++] = byte;
        compressedBytes++;  // Incrementar el contador de bytes comprimidos
    }

    // 5. Escribir la cantidad de bytes comprimidos
    fwrite(&compressedBytes, sizeof(long), 1, tempFile);

    // 6. Escribir el contenido comprimido en el archivo temporal
    fwrite(compressedData, sizeof(unsigned char), compressedBytes, tempFile);

    // Liberar el buffer temporal
    free(compressedData);

    fclose(fe);
    fclose(tempFile);  // Cerrar el archivo temporal
}



void compress(const char *directoryPath) {
    Node *list = NULL;
    struct dirent *entry;
    DIR *dp = opendir(directoryPath);
    if (dp == NULL) {
        perror("The directory cannot be opened");
        return;
    }

    int fileIndex = 0;
    pid_t *childPids = NULL;
    int numChilds = 0;

    while ((entry = readdir(dp))) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char filePath[1024];
        snprintf(filePath, sizeof(filePath), "%s/%s", directoryPath, entry->d_name);
        
        // crea el fork 
        pid_t pid = fork();
        if (pid == 0) {
            // Proceso hijo: comprime y escribe en un archivo temporal
            compressFile(filePath, entry->d_name, &list, fileIndex);
            exit(0);
        } else if (pid > 0) {
            // // Proceso padre: esperar a que el proceso hijo termine
            // wait(NULL);
            childPids = (pid_t *)realloc(childPids, (numChilds + 1) * sizeof(pid_t));
            if (childPids == NULL) {
                perror("Error allocating memory for child PIDs");
                closedir(dp);
                exit(EXIT_FAILURE);
            }
            // Almacenar el PID del proceso hijo
            childPids[numChilds++] = pid;
            fileIndex++;
        } else {
            perror("Error al crear el proceso hijo");
            closedir(dp);
            free(childPids);  
            return;
        }
        //fileIndex++;
    }

    closedir(dp);

    // Esperar a que todos los procesos hijos terminen
    for (int i = 0; i < numChilds; i++) {
        waitpid(childPids[i], NULL, 0);
    }
    free(childPids);  // Liberar la memoria de los PIDs de los hijos
}

void appendTemporaryFilesToMain(int numFiles, FILE *outputFile) {
    for (int i = 0; i < numFiles; i++) {
        // Abrir el archivo temporal
        char tempFileName[256];
        snprintf(tempFileName, sizeof(tempFileName), "temp_compressed_%d.bin", i);
        FILE *tempFile = fopen(tempFileName, "rb");
        if (!tempFile) {
            printf("Error opening temporary file %s\n", tempFileName);
            continue;
        }

        // Leer el contenido del archivo temporal y escribirlo en el archivo de salida
        unsigned char buffer[1024];
        size_t bytesRead;
        while ((bytesRead = fread(buffer, sizeof(unsigned char), sizeof(buffer), tempFile)) > 0) {
            fwrite(buffer, sizeof(unsigned char), bytesRead, outputFile);
        }

        fclose(tempFile);

        // Eliminar el archivo temporal después de usarlo
        remove(tempFileName);
    }
}


int main(int argc, char *argv[]){
    Node *List = NULL;
    Node *Tree = NULL;
    char *fileName;
    char *directory;

    if(argc > 3){
        printf("Expecting less arguments\n");
        printf("Correct Usage: ./compress <Directory Path> <Compressed File Name>\n");
        return 1;
    }

    if(argc == 2){
        printf("Compressed File Name argument not given using the default name: 'CompressedFile.bin'\n");
        fileName = "CompressedFile.bin";
    }else{
        fileName = argv[2];
    }
    if(argc <= 1){
        printf("Not enought arguments passed\n");
        printf("Correct Usage: ./compress <Directory Path> <Compressed File Name>\n");
        return 1;
    }
    directory = argv[1];

    // Variables para medir tiempo
    struct timeval start, end;
    double elapsedTime;

    // Obtener el tiempo de inicio
    gettimeofday(&start, NULL);

    processDirectory(directory, &List);
    sortList(&List);

    // Crear el árbol de Huffman
    Tree = List;
    while(Tree && Tree->next){
        Node *newNode = (Node*)malloc(sizeof(Node));
        newNode->symbol = ';';
        newNode->right = Tree;
        Tree = Tree->next;
        newNode->left = Tree;
        Tree = Tree->next;
        newNode->count = newNode->left->count + newNode->right->count;
        insertInOrder(&Tree, newNode);
    }

    createTable(Tree, 0, 0);

    // Abrir el archivo comprimido
    FILE *compressFile = fopen(fileName, "wb");
    if (!compressFile) {
        perror("Error creating compressed file");
        return 1;
    }

    // Realizar la compresión (incluyendo los forks y procesos hijos)
    compress(directory);

    fwrite(&size, sizeof(int), 1, compressFile);

    // Escribir información de compresión en el archivo
    fwrite(&fileLength, sizeof(long int), 1, compressFile);
    int countElements = 0;
    Table *t = table;  
    while (t) {
        countElements++;
        t = t->next;
    }

    fwrite(&countElements, sizeof(int), 1, compressFile);
    t = table;
    while (t) {
        fwrite(&t->symbol, sizeof(char), 1, compressFile);
        fwrite(&t->bits, sizeof(unsigned long int), 1, compressFile);
        fwrite(&t->nBits, sizeof(char), 1, compressFile);
        t = t->next;
    }

    appendTemporaryFilesToMain(size, compressFile);

    // Cerrar el archivo comprimido
    fclose(compressFile);

    // Liberar memoria
    freeNode(Tree);
    destroyTable(table);

    // Obtener el tiempo de finalización
    gettimeofday(&end, NULL);

    // Calcular el tiempo transcurrido en segundos
    elapsedTime = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;

    // Mostrar el tiempo transcurrido
    printf("Parallel Huffman compression took: %f seconds\n", elapsedTime);

    return 0;
}
