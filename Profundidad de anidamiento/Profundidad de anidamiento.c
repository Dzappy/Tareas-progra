#include <stdio.h>
#include <string.h>

int esPareja(char abierto, char cierre) {
    if (abierto == '(' && cierre == ')') return 1;
    if (abierto == '[' && cierre == ']') return 1;
    if (abierto == '{' && cierre == '}') return 1;
    return 0;
}

int profundidad(char expr[]) {
    char pila[100];        /* pila simple */
    int top = -1;          /* índice tope */
    int i;
    for (i = 0; expr[i] != '\0'; i++) {
        char c = expr[i];
        if (c == '(' || c == '[' || c == '{') {
            if (top < 99) pila[++top] = c;   /* push */
            else return 0;                   /* pila llena (caso simple) */
        } else if (c == ')' || c == ']' || c == '}') {
            if (top == -1) return 0;         /* cierre sin apertura */
            char t = pila[top--];            /* pop */
            if (!esPareja(t, c)) return 0;   /* no coinciden */
        }
    }
    return (top == -1) ? 1 : 0; /* si pila vacía está balanceado */
}

int main() {
    char linea[200];
    printf("Escribe la expresion: ");
    if (fgets(linea, sizeof(linea), stdin) == NULL) return 0;
    /* quitar posible salto de linea */
    linea[strcspn(linea, "\n")] = '\0';

    if (profundidad(linea))
        printf("Balanceada (1)\n");
    else
        printf("No balanceada (0)\n");

    return 0;
}

