#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>

/* --- Utilidades --- */
int esPareja(char a, char b) {
    return (a == '(' && b == ')') ||
           (a == '[' && b == ']') ||
           (a == '{' && b == '}');
}

int jerarquia(char op) {
    if (op == '^') return 4;
    if (op == '*' || op == '/') return 3;
    if (op == '+' || op == '-') return 2;
    return 0;
}

int prec(char op1, char op2) {
    int j1 = jerarquia(op1);
    int j2 = jerarquia(op2);
    if (j1 > j2) return 1;
    if (j1 < j2) return 0;
    /* j1 == j2 */
    if (op1 == '^') return 0;
    return 1; /* otros: asociativos a la izquierda */
}

/* --- Verifica balance de agrupadores --- */
int estaBalanceada(const char *s) {
    char pila[512];
    int top = -1;
    int i;
    char c;
    for (i = 0; s[i] != '\0'; i++) {
        c = s[i];
        if (c == '(' || c == '[' || c == '{') {
            if (top < (int)(sizeof(pila)/sizeof(pila[0]) - 1)) {
                top++;
                pila[top] = c;
            } else {
                return 0;
            }
        } else if (c == ')' || c == ']' || c == '}') {
            if (top == -1) return 0;
            if (!esPareja(pila[top], c)) return 0;
            top--;
        }
    }
    return (top == -1);
}

/* --- Convierte infijo a postfijo (tokens separados por espacios) */
void infijoAPostfijo(const char *expr, char *out, int outSize) {
    char pila[512];
    int top = -1;
    int i, j;
    char c;
    i = 0; j = 0;
    while (expr[i] != '\0' && j < outSize - 1) {
        c = expr[i];
        if (c == ' ' || c == '\t') {
            i++;
            continue;
        }
        /* número (posible multi-dígito y decimal) */
        if (isdigit((unsigned char)c)) {
            while (expr[i] != '\0' && (isdigit((unsigned char)expr[i]) || expr[i] == '.')) {
                if (j < outSize - 1) out[j++] = expr[i++];
                else break;
            }
            if (j < outSize - 1) out[j++] = ' ';
            continue;
        }
        /* variable (letra) */
        if (isalpha((unsigned char)c)) {
            if (j < outSize - 1) out[j++] = c;
            if (j < outSize - 1) out[j++] = ' ';
            i++;
            continue;
        }
        /* apertura de agrupador */
        if (c == '(' || c == '[' || c == '{') {
            top++;
            pila[top] = c;
            i++;
            continue;
        }
        /* cierre de agrupador: sacar hasta apertura correspondiente */
        if (c == ')' || c == ']' || c == '}') {
            while (top != -1 && !(pila[top] == '(' || pila[top] == '[' || pila[top] == '{')) {
                if (j < outSize - 1) out[j++] = pila[top];
                if (j < outSize - 1) out[j++] = ' ';
                top--;
            }
            if (top != -1) top--; /* quitar el agrupador de apertura */
            i++;
            continue;
        }
        /* operador */
        if (c == '+' || c == '-' || c == '*' || c == '/' || c == '^') {
            while (top != -1 &&
                   (pila[top] != '(' && pila[top] != '[' && pila[top] != '{') &&
                   prec(pila[top], c)) {
                if (j < outSize - 1) out[j++] = pila[top];
                if (j < outSize - 1) out[j++] = ' ';
                top--;
            }
            top++;
            pila[top] = c;
            i++;
            continue;
        }
        /* cualquier otro carácter lo ignoramos */
        i++;
    }
    /* vaciar pila */
    while (top != -1 && j < outSize - 1) {
        if (pila[top] != '(' && pila[top] != '[' && pila[top] != '{') {
            out[j++] = pila[top];
            out[j++] = ' ';
        }
        top--;
    }
    if (j > 0 && out[j-1] == ' ') j--; /* quitar espacio final */
    out[j] = '\0';
}


double evaluarPostfija(const char *postfijo) {
    double pila[512];
    int top = -1;
    char copia[1024];
    char *token;
    char *rest;
    double a, b, res;
    int len;
    len = (int)strlen(postfijo);
    if (len >= (int)sizeof(copia)) return 0.0;
    strcpy(copia, postfijo);
    rest = copia;
    token = strtok(rest, " ");
    while (token != NULL) {
        if (isdigit((unsigned char)token[0]) || 
            ((token[0] == '+' || token[0] == '-') && isdigit((unsigned char)token[1]))) {
            if (top < (int)(sizeof(pila)/sizeof(pila[0]) - 1)) {
                top++;
                pila[top] = atof(token);
            } else {
                return 0.0;
            }
        } else if (strlen(token) == 1 && (token[0] == '+' || token[0] == '-' ||
                                         token[0] == '*' || token[0] == '/' ||
                                         token[0] == '^')) {
            if (top < 1) return 0.0; /* no hay suficientes operandos */
            b = pila[top--];
            a = pila[top--];
            switch (token[0]) {
                case '+': res = a + b; break;
                case '-': res = a - b; break;
                case '*': res = a * b; break;
                case '/': res = (b == 0.0) ? 0.0 : a / b; break;
                case '^': res = pow(a, b); break;
                default: res = 0.0;
            }
            top++;
            pila[top] = res;
        } else {
            if (top < (int)(sizeof(pila)/sizeof(pila[0]) - 1)) {
                top++;
                pila[top] = 0.0;
            } else {
                return 0.0;
            }
        }
        token = strtok(NULL, " ");
    }
    if (top == -1) return 0.0;
    return pila[top];
}

/* --- Programa principal --- */
int main(void) {
    char linea[1024];
    char post[1024];
    double resultado;

    printf("Escribe la expresion infija (puedes usar numeros, variables y agrupadores):\n");
    if (!fgets(linea, sizeof(linea), stdin)) return 0;
    linea[strcspn(linea, "\n")] = '\0';

    if (!estaBalanceada(linea)) {
        printf("No balanceada (0)\n");
        return 0;
    }

    infijoAPostfijo(linea, post, sizeof(post));
    printf("Balanceada (1)\n");
    printf("Postfijo: %s\n", post);

    /* intentar evaluar: si la postfija contiene números, se evaluará */
    resultado = evaluarPostfija(post);
    printf("Evaluacion (si hay numeros): %.6g\n", resultado);

    return 0;
}

