#include <stdio.h>
#include <string.h>

/* Comprueba si dos agrupadores son pareja */
int esPareja(char a, char b) {
    return (a == '(' && b == ')') ||
           (a == '[' && b == ']') ||
           (a == '{' && b == '}');
}

/* --- Verifica balance de agrupadores --- */
int estaBalanceada(const char *s) {
    char pila[200];
    int top = -1;
    int i;            /* declarar fuera del for para compatibilidad C89 */
    char c;
    for (i = 0; s[i] != '\0'; i++) {
        c = s[i];
        if (c == '(' || c == '[' || c == '{') {
            if (top < 199) {
                top++;
                pila[top] = c;
            } else {
                return 0; /* pila llena (caso simple) */
            }
        } else if (c == ')' || c == ']' || c == '}') {
            if (top == -1) return 0; /* cierre sin apertura */
            if (!esPareja(pila[top], c)) return 0; /* no coinciden */
            top--;
        }
    }
    return (top == -1);
}

/* --- Precedencia: mayor número = mayor precedencia --- */
int jerarquia(char op) {
    if (op == '^') return 3;
    if (op == '*' || op == '/') return 2;
    if (op == '+' || op == '-') return 1;
    return 0;
}

/* Devuelve 1 si op1 tiene mayor precedencia que op2,
   o si tienen igual precedencia y son asociativos a la izquierda.
   Para '^' (asociatividad derecha) no se debe sacar cuando op1 == op2. */
int prec(char op1, char op2) {
    int j1 = jerarquia(op1);
    int j2 = jerarquia(op2);
    if (j1 > j2) return 1;
    if (j1 < j2) return 0;
    /* j1 == j2 */
    if (op1 == '^') return 0; /* '^' es asociativo a la derecha */
    return 1; /* otros operadores asociativos a la izquierda */
}

int esOperando(char c) {
    /* Considera letras y dígitos como operandos */
    if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) return 1;
    if (c >= '0' && c <= '9') return 1;
    return 0;
}

/* --- Convierte infijo a postfijo; asume que la expresión está balanceada --- */
void infijoAPostfijo(const char *expr, char *out) {
    char pila[200];
    int top = -1;
    int i;            /* declarar fuera de los for para C89 */
    int j;
    char c;
    for (i = 0, j = 0; expr[i] != '\0'; i++) {
        c = expr[i];
        if (c == ' ' || c == '\t') continue; /* ignorar espacios */
        if (esOperando(c)) {
            out[j++] = c;
        } else if (c == '(' || c == '[' || c == '{') {
            top++;
            pila[top] = c;
        } else if (c == ')' || c == ']' || c == '}') {
            /* sacar hasta el agrupador de apertura correspondiente */
            while (top != -1 && !(pila[top] == '(' || pila[top] == '[' || pila[top] == '{')) {
                out[j++] = pila[top];
                top--;
            }
            if (top != -1) {
                /* quitar el agrupador de apertura y descartarlo */
                top--;
            }
        } else {
            /* c es operador */
            while (top != -1 &&
                   (pila[top] != '(' && pila[top] != '[' && pila[top] != '{') &&
                   prec(pila[top], c)) {
                out[j++] = pila[top];
                top--;
            }
            top++;
            pila[top] = c;
        }
    }
    while (top != -1) {
        /* descartar agrupadores sobrantes por seguridad */
        if (pila[top] != '(' && pila[top] != '[' && pila[top] != '{') {
            out[j++] = pila[top];
        }
        top--;
    }
    out[j] = '\0';
}

int main(void) {
    char linea[300];
    char salida[400];

    printf("Escribe la expresion: ");
    if (!fgets(linea, sizeof(linea), stdin)) return 0;
    /* quitar posible salto de linea */
    linea[strcspn(linea, "\n")] = '\0';

    if (!estaBalanceada(linea)) {
        printf("No balanceada (0)\n");
        return 0;
    }

    infijoAPostfijo(linea, salida);
    printf("Balanceada (1)\n");
    printf("Postfijo: %s\n", salida);
    return 0;
}

