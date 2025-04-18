#ifndef NODOS_H
#define NODOS_H

typedef struct Node {
    struct Node *next;
    struct Node *left;
    struct Node *right;
    unsigned char symbol;
    int count;
}Node;

//void insertSymbol(Node *List, Node *element, Node *head, Node *aux);
/*
    Function that insert a symbol in a list
    input
    -list: Node * type, is the actual position in the list
    -element: Node * type, is the element we want to insert
    -head: Node * type, is the head of the list
    -aux: Node * type, is the next node in the list
    output
    none
*/
static inline void insertSymbol(Node *list, Node *element, Node *head, Node *aux){    
    element->next = aux;
    if(!list) head = element;
    else list->next = element;
}
/*
    Function that inserts a new entry in a list
    input
    -actual: Node * type, this is the actual position in the list
    -next: Node * type, this is the next position in the list
    -List" Node * type, this is the head of the list
    -c: unsigned char, this is the symbol that we are going to insert
    output
    none
*/
static inline void insertNewSymbol(Node *actual, Node *next, Node *List, unsigned char c){
    Node *newNode = (Node *)malloc(sizeof(Node));
    newNode->symbol = c;
    newNode->left = newNode->right = NULL;
    newNode-> count = 1;
    insertSymbol(actual, newNode, List, next);

    
}
// void sortList(Node **head);
static inline void sortList(Node **head){
    Node *listAux;
    Node *aux;
    listAux = *head;
    *head = NULL;
    
    while(listAux){
        aux = listAux;
        listAux = aux->next;
        
        insertInOrder(head, aux);
    }
}
// void insertInOrder(Node **head, Node *element);
/*
    inserInOrder: Función que inserta un elemento en una lista ordenada
    Entradas:
        -head = Node** : La cabeza de la lista
        -element: Node*: El elemento a insertar en la lista
    Salidas:
        - void
*/
static inline void insertInOrder(Node **head, Node *element){
    Node *aux = NULL;
    Node *auxNext = NULL;
    
    if(!*head){
        *head = element;
        (*head)->next = NULL;
    }else{
        aux = *head;
        auxNext = NULL;
        while(aux && aux->count < element->count){
            auxNext = aux;
            aux = aux->next;
            
        }
        element->next = aux;
        if(auxNext) auxNext->next = element;
        else *head = element;
    }
}
// void freeNode(Node *head);
/*
    freeNode: Función que libera la memoria de la lista
    Entradas:
        -head = Node* : Es la cabeza de la lista
    Salidas:
        -void 
    
*/
static inline void freeNode(Node *head){
    if(head->left) freeNode(head->left);
    if(head->right) freeNode(head->right);
    free(head);
}
// void printNode(Node **head);
/*
    printNode: Función que imprime la lista
    Entradas:
        -head = Node* : Es la cabeza de la lista
    Salidas:
        -void
*/
static inline void printNode(Node **head){
    Node *aux;
    aux = *head;
    while(aux){
        printf("Simbolo: %c cuenta: %i\n", aux->symbol, aux->count);
        if(aux->left)
            printf("izquierda Simbolo: %c cuenta: %i\n", aux->left->symbol, aux->left->count);
        if(aux->right)
            printf("derecha Simbolo: %c cuenta: %i\n", aux->right->symbol, aux->right->count);
        aux = aux->next;
    }
}

#endif