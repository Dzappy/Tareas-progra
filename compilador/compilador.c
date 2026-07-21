
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MEMSIZE 100
#define MAXLINE 512
#define MAXSYMS 2000
#define MAXTOK 256

typedef struct {
    char symbol[32]; 
    char type;
    int location; 
} TableEntry;

TableEntry symtab[MAXSYMS];
int symcount = 0;

int flags[MEMSIZE];   
int sml[MEMSIZE];     
int instr_count = 0; 
int data_ptr = MEMSIZE - 1; 

/* Prototipos */
int lookup_or_add(const char *s, char type, int add);
int make_instr(int op, int operand);
int precedence(const char *op);
int is_operator_token(const char *t);
char *infix_to_postfix(const char *expr);
int eval_postfix_generate(char *postfix);
void process_line_first(char *line);
void second_pass_and_write(const char *outname);

/* Buscar símbolo y si no existe y add==1 lo inserta
   Devuelve índice en symtab o -1 si no existe y add==0 */
int lookup_or_add(const char *s, char type, int add) {
    int i;
    for (i = 0; i < symcount; i++) {
        if (strcmp(symtab[i].symbol, s) == 0) return i;
    }
    if (!add) return -1;
    if (symcount >= MAXSYMS) {
        fprintf(stderr, "Error: tabla de símbolos llena\n");
        exit(1);
    }
    strcpy(symtab[symcount].symbol, s);
    symtab[symcount].type = type;
    if (type == 'L') {
        symtab[symcount].location = instr_count;
    } else {
        symtab[symcount].location = data_ptr;
        data_ptr--;
        if (data_ptr < 0) {
            fprintf(stderr, "Error: memoria de datos agotada\n");
            exit(1);
        }
    }
    symcount++;
    return symcount - 1;
}

/* Construye instrucción SML: op*100 + operand */
int make_instr(int op, int operand) {
    return op * 100 + (operand % 100);
}

/* Precedencia extendida */
int precedence(const char *op) {
    if (strcmp(op, "+") == 0 || strcmp(op, "-") == 0) return 1;
    if (strcmp(op, "*") == 0 || strcmp(op, "/") == 0) return 2;
    return 0;
}

/* Determina si token es operador aritmético */
int is_operator_token(const char *t) {
    return (!strcmp(t, "+") || !strcmp(t, "-") ||
            !strcmp(t, "*") || !strcmp(t, "/"));
}

/* Convierte infija a posfija usando Shunting-Yard
   Requiere que los tokens estén separados por espacios
   Devuelve cadena alocada (free() por el llamador) */
char *infix_to_postfix(const char *expr) {
    char *copy = NULL;
    char *token;
    char *out = (char *)malloc(MAXLINE);
    char *stack_ops[MAXTOK];
    int top = -1;
    int outpos = 0;
    int len;

    if (!out) { fprintf(stderr, "Memoria insuficiente\n"); exit(1); }
    out[0] = '\0';

    copy = (char *)malloc(strlen(expr) + 1);
    if (!copy) { fprintf(stderr, "Memoria insuficiente\n"); exit(1); }
    strcpy(copy, expr);

    token = strtok(copy, " \t\r\n");
    while (token != NULL) {
        if (isalpha(token[0]) || isdigit(token[0]) || (token[0]=='-' && isdigit(token[1]))) {
            /* operando: variable o constante */
            len = strlen(token);
            if (outpos + len + 2 >= MAXLINE) break;
            if (outpos > 0) { out[outpos++] = ' '; }
            strcpy(out + outpos, token);
            outpos += len;
        } else if (is_operator_token(token)) {
            while (top >= 0 && is_operator_token(stack_ops[top]) &&
                   precedence(stack_ops[top]) >= precedence(token)) {
                len = strlen(stack_ops[top]);
                if (outpos + len + 2 >= MAXLINE) break;
                if (outpos > 0) { out[outpos++] = ' '; }
                strcpy(out + outpos, stack_ops[top]);
                outpos += len;
                top--;
            }
            stack_ops[++top] = token;
        } else if (strcmp(token, "(") == 0) {
            stack_ops[++top] = token;
        } else if (strcmp(token, ")") == 0) {
            while (top >= 0 && strcmp(stack_ops[top], "(") != 0) {
                len = strlen(stack_ops[top]);
                if (outpos + len + 2 >= MAXLINE) break;
                if (outpos > 0) { out[outpos++] = ' '; }
                strcpy(out + outpos, stack_ops[top]);
                outpos += len;
                top--;
            }
            if (top >= 0 && strcmp(stack_ops[top], "(") == 0) top--;
        } else {
            /* token desconocido: ignorar */
        }
        token = strtok(NULL, " \t\r\n");
    }

    while (top >= 0) {
        if (strcmp(stack_ops[top], "(") != 0 && strcmp(stack_ops[top], ")") != 0) {
            len = strlen(stack_ops[top]);
            if (outpos + len + 2 >= MAXLINE) break;
            if (outpos > 0) { out[outpos++] = ' '; }
            strcpy(out + outpos, stack_ops[top]);
            outpos += len;
        }
        top--;
    }

    out[outpos] = '\0';
    free(copy);
    return out;
}

/* Evalúa expresión posfija pero en lugar de calcular produce instrucciones SML.
   Devuelve la posición de memoria que contiene el resultado (posición temporal). */
int eval_postfix_generate(char *postfix) {
    int stack_pos[256];
    int sp = 0;
    char *copy = NULL;
    char *tok;
    int idx_l, idx_r;
    int temp_loc;
    int i;

    copy = (char *)malloc(strlen(postfix) + 1);
    if (!copy) { fprintf(stderr, "Memoria insuficiente\n"); exit(1); }
    strcpy(copy, postfix);

    tok = strtok(copy, " ");
    while (tok != NULL) {
        if (isalpha(tok[0]) || isdigit(tok[0]) || (tok[0]=='-' && isdigit(tok[1]))) {
            char ttype;
            if (isalpha(tok[0])) ttype = 'v'; else ttype = 'c';
            idx_l = lookup_or_add(tok, ttype, 1);
            stack_pos[sp++] = symtab[idx_l].location;
        } else if (is_operator_token(tok)) {
            if (sp < 2) {
                fprintf(stderr, "Error: expresión posfija inválida\n");
                free(copy);
                exit(1);
            }
            idx_r = stack_pos[--sp];
            idx_l = stack_pos[--sp];
            if (instr_count + 3 >= MEMSIZE) {
                fprintf(stderr, "Error: memoria de instrucciones agotada\n");
                free(copy);
                exit(1);
            }
            /* cargar izquierdo */
            sml[instr_count++] = make_instr(20, idx_l); /* 20 = cargar */
            /* operar con derecho */
            if (strcmp(tok, "+") == 0) {
                sml[instr_count++] = make_instr(30, idx_r); /* 30 = sumar */
            } else if (strcmp(tok, "-") == 0) {
                sml[instr_count++] = make_instr(31, idx_r); /* 31 = restar */
            } else if (strcmp(tok, "*") == 0) {
                sml[instr_count++] = make_instr(33, idx_r); /* 33 = multiplicar */
            } else if (strcmp(tok, "/") == 0) {
                sml[instr_count++] = make_instr(32, idx_r); /* 32 = dividir */
            } else {
                fprintf(stderr, "Operador no soportado: %s\n", tok);
                free(copy);
                exit(1);
            }
            /* almacenar en temp */
            temp_loc = data_ptr;
            data_ptr--;
            if (data_ptr < 0) {
                fprintf(stderr, "Error: memoria de datos agotada (temporales)\n");
                free(copy);
                exit(1);
            }
            sml[instr_count++] = make_instr(21, temp_loc); /* 21 = almacenar */
            stack_pos[sp++] = temp_loc;
        } else {
            /* token desconocido: ignorar */
        }
        tok = strtok(NULL, " ");
    }

    if (sp != 1) {
        fprintf(stderr, "Error: evaluación posfija produjo pila con %d elementos\n", sp);
        free(copy);
        exit(1);
    }
    i = stack_pos[0];
    free(copy);
    return i;
}

/* Procesa una línea del programa Simple en la primera pasada */
void process_line_first(char *line) {
    char buffer[MAXLINE];
    char *save;
    char *num;
    char *cmd;
    char *rest;
    char *token;
    int idx, idx2;
    int destIdx;
    int destLine;
    int respos;
    char expr[MAXLINE];

    strncpy(buffer, line, MAXLINE - 1);
    buffer[MAXLINE - 1] = '\0';
    save = buffer;
    while (*save && (*save == ' ' || *save == '\t')) save++;
    num = strtok(save, " \t\r\n");
    if (!num) return;
    lookup_or_add(num, 'L', 1);

    cmd = strtok(NULL, " \t\r\n");
    if (!cmd) return;

    if (strcmp(cmd, "rem") == 0) {
        return;
    } else if (strcmp(cmd, "input") == 0) {
        token = strtok(NULL, " \t\r\n");
        if (!token) { fprintf(stderr, "Error: input sin variable\n"); return; }
        idx = lookup_or_add(token, 'v', 1);
        if (instr_count >= MEMSIZE) { fprintf(stderr, "Error: instrucciones agotadas\n"); return; }
        sml[instr_count++] = make_instr(10, symtab[idx].location); /* 10 = leer */
    } else if (strcmp(cmd, "print") == 0) {
        token = strtok(NULL, " \t\r\n");
        if (!token) { fprintf(stderr, "Error: print sin operando\n"); return; }
        idx = lookup_or_add(token, isalpha(token[0]) ? 'v' : 'c', 1);
        if (instr_count >= MEMSIZE) { fprintf(stderr, "Error: instrucciones agotadas\n"); return; }
        sml[instr_count++] = make_instr(11, symtab[idx].location); /* 11 = escribir */
    } else if (strcmp(cmd, "goto") == 0) {
        token = strtok(NULL, " \t\r\n");
        if (!token) { fprintf(stderr, "Error: goto sin destino\n"); return; }
        destIdx = lookup_or_add(token, 'L', 0);
        if (destIdx == -1) {
            if (instr_count >= MEMSIZE) { fprintf(stderr, "Error: instrucciones agotadas\n"); return; }
            destLine = atoi(token);
            flags[instr_count] = destLine;
            sml[instr_count++] = make_instr(40, 0); /* 40 = bifurcar incondicional */
        } else {
            if (instr_count >= MEMSIZE) { fprintf(stderr, "Error: instrucciones agotadas\n"); return; }
            sml[instr_count++] = make_instr(40, symtab[destIdx].location);
        }
    } else if (strcmp(cmd, "if") == 0) {
        /* formato: if a OP b goto N
           tokens separados por espacios: if a OP b goto N */
        char *a = strtok(NULL, " \t\r\n");
        char *op = strtok(NULL, " \t\r\n"); /* operator like ==, !=, <, >, <=, >= */
        char *b = strtok(NULL, " \t\r\n");
        char *goto_kw = strtok(NULL, " \t\r\n"); /* goto */
        char *dest = strtok(NULL, " \t\r\n");
        if (!a || !op || !b || !goto_kw || !dest) {
            fprintf(stderr, "Error: if mal formado\n");
            return;
        }
        idx = lookup_or_add(a, isalpha(a[0]) ? 'v' : 'c', 1);
        idx2 = lookup_or_add(b, isalpha(b[0]) ? 'v' : 'c', 1);

        /* Implementaciones por operador:
           - == : cargar a; restar b; bifurcar si cero dest
           - != : cargar a; restar b; bifurcar si cero skip; goto dest; skip:
           - <  : cargar a; restar b; bifurcar si negativo dest
           - >  : cargar b; restar a; bifurcar si negativo dest
           - <= : cargar a; restar b; bifurcar si negativo dest; bifurcar si cero dest
           - >= : cargar b; restar a; bifurcar si negativo dest; bifurcar si cero dest
        */

        if (instr_count + 5 >= MEMSIZE) { fprintf(stderr, "Error: instrucciones agotadas\n"); return; }

        if (strcmp(op, "==") == 0) {
            sml[instr_count++] = make_instr(20, symtab[idx].location); /* cargar a */
            sml[instr_count++] = make_instr(31, symtab[idx2].location); /* restar b */
            destIdx = lookup_or_add(dest, 'L', 0);
            if (destIdx == -1) {
                destLine = atoi(dest);
                flags[instr_count] = destLine;
                sml[instr_count++] = make_instr(42, 0); /* 42 = bifurcar si cero */
            } else {
                sml[instr_count++] = make_instr(42, symtab[destIdx].location);
            }
        } else if (strcmp(op, "!=") == 0) {
            /* cargar a; restar b; bifurcar si cero -> skip; goto dest; skip: (use known next index) */
            sml[instr_count++] = make_instr(20, symtab[idx].location);
            sml[instr_count++] = make_instr(31, symtab[idx2].location);
            /* bifurcar si cero a skip (skip = instr_count + 2) */
            {
                int skip_pos = instr_count + 2; /* after the unconditional goto we'll place skip */
                sml[instr_count] = make_instr(42, skip_pos); /* temporary operand; will be adjusted in second pass only if skip refers to a line number, but here skip_pos is an index so it's fine */
                /* We don't use flags[] for this internal skip because it's a direct numeric address */
                instr_count++;
            }
            /* unconditional goto dest */
            destIdx = lookup_or_add(dest, 'L', 0);
            if (destIdx == -1) {
                destLine = atoi(dest);
                flags[instr_count] = destLine;
                sml[instr_count++] = make_instr(40, 0);
            } else {
                sml[instr_count++] = make_instr(40, symtab[destIdx].location);
            }
            /* skip: no-op (could be a dummy instruction). We'll just continue; no explicit instruction needed */
        } else if (strcmp(op, "<") == 0) {
            sml[instr_count++] = make_instr(20, symtab[idx].location);
            sml[instr_count++] = make_instr(31, symtab[idx2].location);
            destIdx = lookup_or_add(dest, 'L', 0);
            if (destIdx == -1) {
                destLine = atoi(dest);
                flags[instr_count] = destLine;
                sml[instr_count++] = make_instr(41, 0); /* 41 = bifurcar si negativo */
            } else {
                sml[instr_count++] = make_instr(41, symtab[destIdx].location);
            }
        } else if (strcmp(op, ">") == 0) {
            /* a > b  <=>  b - a < 0 */
            sml[instr_count++] = make_instr(20, symtab[idx2].location); /* cargar b */
            sml[instr_count++] = make_instr(31, symtab[idx].location);  /* restar a */
            destIdx = lookup_or_add(dest, 'L', 0);
            if (destIdx == -1) {
                destLine = atoi(dest);
                flags[instr_count] = destLine;
                sml[instr_count++] = make_instr(41, 0);
            } else {
                sml[instr_count++] = make_instr(41, symtab[destIdx].location);
            }
        } else if (strcmp(op, "<=") == 0) {
            /* if a <= b then (a < b) or (a == b) */
            sml[instr_count++] = make_instr(20, symtab[idx].location);
            sml[instr_count++] = make_instr(31, symtab[idx2].location);
            /* bifurcar si negativo dest */
            destIdx = lookup_or_add(dest, 'L', 0);
            if (destIdx == -1) {
                destLine = atoi(dest);
                flags[instr_count] = destLine;
                sml[instr_count++] = make_instr(41, 0);
            } else {
                sml[instr_count++] = make_instr(41, symtab[destIdx].location);
            }
            /* bifurcar si cero dest */
            destIdx = lookup_or_add(dest, 'L', 0);
            if (destIdx == -1) {
                destLine = atoi(dest);
                flags[instr_count] = destLine;
                sml[instr_count++] = make_instr(42, 0);
            } else {
                sml[instr_count++] = make_instr(42, symtab[destIdx].location);
            }
        } else if (strcmp(op, ">=") == 0) {
            /* if a >= b then (a > b) or (a == b) */
            sml[instr_count++] = make_instr(20, symtab[idx2].location); /* cargar b */
            sml[instr_count++] = make_instr(31, symtab[idx].location);  /* restar a */
            /* bifurcar si negativo dest (b - a < 0) => a > b */
            destIdx = lookup_or_add(dest, 'L', 0);
            if (destIdx == -1) {
                destLine = atoi(dest);
                flags[instr_count] = destLine;
                sml[instr_count++] = make_instr(41, 0);
            } else {
                sml[instr_count++] = make_instr(41, symtab[destIdx].location);
            }
            /* bifurcar si cero dest (a == b) */
            destIdx = lookup_or_add(dest, 'L', 0);
            if (destIdx == -1) {
                destLine = atoi(dest);
                flags[instr_count] = destLine;
                sml[instr_count++] = make_instr(42, 0);
            } else {
                sml[instr_count++] = make_instr(42, symtab[destIdx].location);
            }
        } else {
            fprintf(stderr, "Operador relacional no soportado: %s\n", op);
            return;
        }
    } else if (strcmp(cmd, "let") == 0) {
        char *var = strtok(NULL, " \t\r\n");
        char *eq = strtok(NULL, " \t\r\n"); /* "=" */
        if (!var || !eq) { fprintf(stderr, "Error: let mal formado\n"); return; }
        rest = strtok(NULL, "\n");
        if (!rest) rest = "";
        while (*rest && (*rest == ' ' || *rest == '\t')) rest++;
        strncpy(expr, rest, MAXLINE - 1);
        expr[MAXLINE - 1] = '\0';
        idx = lookup_or_add(var, 'v', 1);
        {
            char *post = infix_to_postfix(expr);
            respos = eval_postfix_generate(post);
            free(post);
        }
        if (instr_count + 2 >= MEMSIZE) { fprintf(stderr, "Error: instrucciones agotadas\n"); return; }
        sml[instr_count++] = make_instr(20, respos); /* cargar temp */
        sml[instr_count++] = make_instr(21, symtab[idx].location); /* almacenar en var */
    } else if (strcmp(cmd, "end") == 0) {
        if (instr_count >= MEMSIZE) { fprintf(stderr, "Error: instrucciones agotadas\n"); return; }
        sml[instr_count++] = make_instr(43, 0); /* 43 = alto */
    } else {
        fprintf(stderr, "Advertencia: comando desconocido '%s'\n", cmd);
    }
}

/* Segunda pasada: resolver flags y escribir archivo SML */
void second_pass_and_write(const char *outname) {
    int i, s;
    FILE *f;
    char buf[32];

    for (i = 0; i < instr_count; i++) {
        if (flags[i] != -1) {
            sprintf(buf, "%d", flags[i]);
            for (s = 0; s < symcount; s++) {
                if (symtab[s].type == 'L' && strcmp(symtab[s].symbol, buf) == 0) {
                    int loc = symtab[s].location;
                    sml[i] = (sml[i] / 100) * 100 + loc;
                    break;
                }
            }
            /* si no se encontró, dejar operand 00 (o podríamos error) */
        }
    }

    f = fopen(outname, "w");
    if (!f) {
        fprintf(stderr, "Error: no se puede abrir %s para escritura\n", outname);
        return;
    }
    for (i = 0; i < instr_count; i++) {
        fprintf(f, "%+05d\n", sml[i]);
    }
    fclose(f);
}

/* Programa principal */
int main(int argc, char *argv[])
{
    FILE *in;
    char line[MAXLINE];
    char fuente[256];
    char salida[256];
    int i;

    if (argc >= 3) {
        strcpy(fuente, argv[1]);
        strcpy(salida, argv[2]);
    } else {
        printf("Archivo fuente: ");
        scanf("%255s", fuente);

        printf("Archivo salida: ");
        scanf("%255s", salida);
    }

    for (i = 0; i < MEMSIZE; i++)
        flags[i] = -1;

    printf("Intentando abrir: %s\n", fuente);
    in = fopen(fuente, "r");
    if (!in) {
        printf("No se pudo abrir %s\n", fuente);
        return 1;
    }

    while (fgets(line, sizeof(line), in) != NULL)
        process_line_first(line);

    fclose(in);

    second_pass_and_write(salida);

    printf("Compilacion completada.\n");

    return 0;
}

