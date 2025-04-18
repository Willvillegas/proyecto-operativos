#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <time.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/wait.h>
#include <sys/time.h>

#define DEBUG printf("Aqui\n");

typedef struct tree {
    unsigned char symbol;
    unsigned long int bits;
    char nBits;
    struct tree *left;
    struct tree *right;
} Node;

// Estructura para almacenar la información de cada libro
typedef struct {
    Node *tree;
    char title[256];  // Almacena el título del libro
    unsigned long totalCharacters;  // Cantidad total de caracteres del libro
    unsigned long compressedSize;   // Tamaño comprimido del contenido
    unsigned char *compressedData; // Datos comprimidos (arreglo dinámico)
} Book;

int size = 0;

void createTree(Node *current, Node *newNode, Node *tree, FILE *fi, int elements);
void decompressBook(Node *tree, const char *directory, const char *title, unsigned long totalCharacters, unsigned long compressedSize, unsigned char *compressedData);
Node *rebuildHuffmanTree(FILE *fi, int numElements);
void deleteTree(Node *n);
void decompress(const char *compressedFilePath, const char *outputDirectory);

// Función para eliminar el árbol de Huffman
void deleteTree(Node *n) {
    if (n->left) deleteTree(n->left);
    if (n->right) deleteTree(n->right);
    free(n);
}

// Función para descomprimir un libro individual
void decompressBook(Node *tree, const char *directory, const char *title, unsigned long totalCharacters, unsigned long compressedSize, unsigned char *compressedData) {
    // Crear el archivo de salida con el título
    char outputPath[1024];
    snprintf(outputPath, sizeof(outputPath), "%s/%s", directory, title);
    FILE *outputFile = fopen(outputPath, "w");
    if (!outputFile) {
        perror("Error creating the output file");
        return;
    }

    
    Node *current = tree;
    int cantBook = totalCharacters;

    for (unsigned long i = 0; i < compressedSize; ++i) {
        for (int bit = 7; bit >= 0; --bit) {
            unsigned char currentBit = (compressedData[i] >> bit) & 1;
            current = currentBit ? current->right : current->left;
            
            if (!current->left && !current->right) {
                // Es un nodo hoja, escribir el símbolo en el archivo de salida
                fputc(current->symbol, outputFile);
                cantBook--;
                current = tree; // Regresar al inicio del árbol
            }
            if (cantBook <= 0)
                break;
        }
        if (cantBook <= 0)
            break;
    }

    fclose(outputFile);
}

// Función principal que coordina la descompresión utilizando procesos hijos
void decompress(const char *compressedFilePath, const char *outputDirectory) {
    printf("%s\n%s\n", compressedFilePath, outputDirectory);
    FILE *fi = fopen(compressedFilePath, "rb");

    if (!fi) {
        perror("Error opening the compressed file");
        return;
    }

    fread(&size, sizeof(int), 1, fi);

    // Leer la cantidad total de caracteres en el archivo comprimido
    long int totalCharacters;
    fread(&totalCharacters, sizeof(long int), 1, fi);

    // Leer el número de elementos en la tabla de Huffman
    int numElements;
    fread(&numElements, sizeof(int), 1, fi);

    // Reconstruir el árbol de Huffman
    Node *huffmanTree = rebuildHuffmanTree(fi, numElements);

    // Crear el directorio de salida si no existe
    mkdir(outputDirectory, 0777);

    // Crear un segmento de memoria compartida para almacenar los libros
    int shmId = shmget(IPC_PRIVATE, size * sizeof(Book), IPC_CREAT | 0666);
    if (shmId < 0) {
        perror("Error creating shared memory");
        exit(1);
    }

    // Adjuntar el segmento de memoria compartida
    Book *books = (Book *)shmat(shmId, NULL, 0);

    for (int i = 0; i < size; i++) {
        unsigned int titleLength;
        fread(&titleLength, sizeof(unsigned int), 1, fi);

        fread(books[i].title, sizeof(char), titleLength, fi);
        books[i].title[titleLength] = '\0';  // Terminar la cadena

        fread(&books[i].totalCharacters, sizeof(unsigned long), 1, fi);
        fread(&books[i].compressedSize, sizeof(unsigned long), 1, fi);

        // Alocar memoria para los datos comprimidos de cada libro
        books[i].compressedData = (unsigned char *)malloc(books[i].compressedSize);
        fread(books[i].compressedData, sizeof(unsigned char), books[i].compressedSize, fi);

        // Copiar el árbol de Huffman a cada libro
        books[i].tree = huffmanTree;
    }

    // Crear los procesos hijos
    for (int i = 0; i < size; i++) {
        pid_t pid = fork();

        if (pid == 0) {
            // Proceso hijo: descomprime su parte del libro
            decompressBook(books[i].tree, outputDirectory, books[i].title, books[i].totalCharacters, books[i].compressedSize, books[i].compressedData);
            free(books[i].compressedData);  // Liberar memoria de los datos comprimidos
            exit(0);  // Finalizar el proceso hijo
        }
    }

    // Esperar a que todos los hijos terminen
    for (int i = 0; i < size; i++) {
        wait(NULL);
    }

    // Desconectar la memoria compartida
    shmdt(books);

    // Eliminar el segmento de memoria compartida
    shmctl(shmId, IPC_RMID, NULL);

    fclose(fi);
    deleteTree(huffmanTree); // Liberar el árbol de Huffman
}

// Función para reconstruir el árbol de Huffman desde el archivo comprimido
Node *rebuildHuffmanTree(FILE *fi, int numElements) {
    Node *tree = (Node *)malloc(sizeof(Node));
    memset(tree, 0, sizeof(Node)); // Inicializar el árbol

    for (int i = 0; i < numElements; i++) {
        unsigned char symbol;
        unsigned long int bits;
        char nBits;

        fread(&symbol, sizeof(unsigned char), 1, fi);
        fread(&bits, sizeof(unsigned long int), 1, fi);
        fread(&nBits, sizeof(char), 1, fi);

        // Reconstruir el nodo en el árbol de Huffman
        Node *current = tree;
        for (int bitPos = nBits - 1; bitPos >= 0; bitPos--) {
            unsigned char bit = (bits >> bitPos) & 1;

            if (bit) {
                if (!current->right) {
                    current->right = (Node *)malloc(sizeof(Node));
                    memset(current->right, 0, sizeof(Node));
                }
                current = current->right;
            } else {
                if (!current->left) {
                    current->left = (Node *)malloc(sizeof(Node));
                    memset(current->left, 0, sizeof(Node));
                }
                current = current->left;
            }
        }

        current->symbol = symbol;
    }

    return tree;
}

int main(int argc, char* argv[]) {
    char *fileName;
    char *directory;

    if (argc > 3) {
        printf("Expecting less arguments\n");
        printf("Correct Usage: ./decompress <Compressed File Name> <Directory Name>\n");
        return 1;
    }

    if (argc == 2) {
        printf("Directory argument not given using the default name: 'CompressedFile'\n");
        directory = "CompressedFile";
    } else {
        directory = argv[2];
    }
    if (argc <= 1) {
        printf("Not enough arguments passed\n");
        printf("Correct Usage: ./decompress <Compressed File Name> <Directory Name>\n");
        return 1;
    }
    fileName = argv[1];

    // Crear el directorio
    struct stat st = {0};
    if (stat(directory, &st) == -1) {
        mkdir(directory, 0700);
    }

    // Variables para medir tiempo
    struct timeval start, end;
    double elapsedTime;

    // Obtener el tiempo de inicio
    gettimeofday(&start, NULL);

    decompress(fileName, directory);

     // Obtener el tiempo de finalización
    gettimeofday(&end, NULL);

    // Calcular el tiempo transcurrido en segundos
    elapsedTime = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
    
    printf("Parallel Huffman decompression took: %f seconds\n", elapsedTime);
    return 0;
}
