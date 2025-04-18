#ifndef TABLA_H
#define TABLA_H

#include "Nodos.h"
// Structure for the table
typedef struct Table
{
    unsigned char symbol;   // Guarda el caracter de la tabla
    unsigned char nBits;    // Numero de bits guardados
    unsigned long int bits; // Bit encoding
    struct Table *next;     // Puntero al siguiente nodo
} Table;

//global variable
Table *table;
void insertElement(unsigned char c, int nBits, int bits); // 👈 Prototipo

// Crear una tabla de forma recursiva
static inline void createTable(Node *node, int nBits, int bits){
    if (node->right)
        createTable(node->right, nBits + 1, (bits << 1) | 1);
    
        
    if (node->left)
        createTable(node->left, nBits + 1, bits << 1);
    
        
    if (!node->right && !node->left)
        insertElement(node->symbol, nBits, bits);
}

// Inserta un elemento en la tabla
void insertElement(unsigned char c, int nBits, int bits){
    Table *t, *p, *a;

    t = (Table *) malloc( sizeof(Table) );
    t->symbol = c;
    t->bits = bits;
    t->nBits = nBits;

    // If the table is empty, initialize it with the new element
    if (table == NULL){
        table = t;
        table->next = NULL;
    }else{
        p = table;
        a = NULL;
        // Traverse the table to find the insertion point
        while (p && p->symbol < t->symbol){
            a = p;
            p = p->next;
        }
        t->next = p;
        if (a){
            a->next = t;
        }else{
            table = t;
        }
    }
}

// Buscar un simbolo en la tabla
static inline Table* findSymbol(Table *table, unsigned char symbol){
    Table *t = table;
    while (t && t->symbol != symbol){
        t = t->next;
    }
    return t;
}

// Destruir la tabla
static inline void destroyTable(Table *table){
    Table *temp;
    while (table != NULL){
        temp = table;
        table = table->next;
        free(temp);
    }
}
static inline void printTable(Table *table){
    Table *temp;
    temp = table;
    while (temp != NULL){
        printf("Simbolo: %c, nBits: %i, bits: %li \n", temp->symbol, temp->nBits, temp->bits);
        temp = temp->next;
    }
}

#endif