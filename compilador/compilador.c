
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MEM_SIZE 100
#define LINE_MAX 256

/* Códigos de operación */
enum {
    OP_READ = 10,
    OP_WRITE = 11,
    OP_LOAD = 20,
    OP_STORE = 21,
    OP_ADD = 30,
    OP_SUB = 31,
    OP_BRANCH = 40,
    OP_BRANCHNEG = 41,
    OP_BRANCHZERO = 42,
    OP_HALT = 43
};

/* Entrada de la tabla de símbolos */
typedef struct tableEntry {
    int symbol;             /* valor numérico para línea o ASCII para variable */
    char type;              /* 'L' línea, 'V' variable, 'C' constante */
    int location;           /* ubicación en memoria 0..99 */
    struct tableEntry *next;
} tableEntry;

tableEntry *symbolTable = NULL;

/* Memoria Simpletron y flags */
int memory[MEM_SIZE];
int flags[MEM_SIZE]; /* -1 significa resuelto, si no, almacena número de línea destino */
int instr_ptr = 0;   /* siguiente índice de instrucción (crece desde 0) */
int data_ptr = MEM_SIZE - 1; /* siguiente celda de datos (asignada hacia atrás) */

/* Buscar símbolo (opcionalmente filtrar por tipo) */
tableEntry *find_symbol(int sym, char type_filter) {
    tableEntry *p = symbolTable;
    while (p) {
        if (p->symbol == sym) {
            if (type_filter == 0 || p->type == type_filter) return p;
        }
        p = p->next;
    }
    return NULL;
}

/* Insertar símbolo en la tabla (si ya existe devuelve el existente) */
tableEntry *insert_symbol(int sym, char type) {
    tableEntry *existing = find_symbol(sym, 0);
    if (existing) {
        if (existing->location == -1 && (type == 'V' || type == 'C' || type == 'L')) {
            existing->type = type;
        }
        return existing;
    }
    tableEntry *e = malloc(sizeof(tableEntry));
    if (!e) { perror("malloc"); exit(EXIT_FAILURE); }
    e->symbol = sym;
    e->type = type;
    e->location = -1;
    e->next = symbolTable;
    symbolTable = e;
    return e;
}

/* Asignar una celda de datos (variables/constantes/temporales) */
int allocate_data_slot() {
    if (data_ptr <= instr_ptr) {
        fprintf(stderr, "Error: memoria agotada (colisión código/datos)\n");
        exit(EXIT_FAILURE);
    }
    return data_ptr--;
}

/* Emitir instrucción en memoria */
void emit(int opcode, int operand) {
    if (instr_ptr < 0 || instr_ptr >= MEM_SIZE) {
        fprintf(stderr, "Error: puntero de instrucción fuera de rango\n");
        exit(EXIT_FAILURE);
    }
    int instr = opcode * 100 + operand;
    memory[instr_ptr] = instr;
    instr_ptr++;
}

/* Emitir bifurcación con destino de línea posiblemente no resuelto */
void emit_branch_with_line(int opcode, int targetLine) {
    int idx = instr_ptr;
    emit(opcode, 0); /* operando 00 temporal */
    flags[idx] = targetLine; /* se resolverá en la segunda pasada */
}

/* Recortar espacios al inicio y final */
char *trim(char *s) {
    while (isspace((unsigned char)*s)) s++;
    if (*s == 0) return s;
    char *end = s + strlen(s) - 1;
    while (end > s && isspace((unsigned char)*end)) *end-- = '\0';
    return s;
}

/* Precedencia de operadores */
int prec(const char *op) {
    if (strcmp(op, "+") == 0 || strcmp(op, "-") == 0) return 1;
    if (strcmp(op, "*") == 0 || strcmp(op, "/") == 0) return 2;
    return 0;
}

/* Es operador */
int is_op(const char *t) {
    return (strcmp(t, "+") == 0 || strcmp(t, "-") == 0 ||
            strcmp(t, "*") == 0 || strcmp(t, "/") == 0);
}

/* Convertir infijo (tokens separados por espacios) a posfijo.
   Devuelve arreglo dinámico de cadenas terminado en NULL. */
char **infix_to_postfix(const char *expr) {
    char *copy = strdup(expr);
    if (!copy) { perror("strdup"); exit(EXIT_FAILURE); }
    char *tok;
    char *stack[128];
    int top = -1;
    char *out[256];
    int outi = 0;

    tok = strtok(copy, " ");
    while (tok) {
        if (is_op(tok)) {
            while (top >= 0 && is_op(stack[top]) && prec(stack[top]) >= prec(tok)) {
                out[outi++] = strdup(stack[top--]);
            }
            stack[++top] = strdup(tok);
        } else if (strcmp(tok, "(") == 0) {
            stack[++top] = strdup(tok);
        } else if (strcmp(tok, ")") == 0) {
            while (top >= 0 && strcmp(stack[top], "(") != 0) {
                out[outi++] = strdup(stack[top--]);
            }
            if (top >= 0 && strcmp(stack[top], "(") == 0) {
                free(stack[top--]);
            }
        } else {
            out[outi++] = strdup(tok);
        }
        tok = strtok(NULL, " ");
    }
    while (top >= 0) {
        out[outi++] = strdup(stack[top--]);
    }
    out[outi] = NULL;
    free(copy);
    char **ret = malloc((outi + 1) * sizeof(char *));
    if (!ret) { perror("malloc"); exit(EXIT_FAILURE); }
    {
        int i;
        for (i = 0; i <= outi; ++i) ret[i] = out[i];
    }
    return ret;
}

/* Liberar arreglo posfijo */
void free_postfix(char **pf) {
    {
        int i;
        for (i = 0; pf[i]; ++i) free(pf[i]);
    }
    free(pf);
}

/* Evaluar posfijo pero generando instrucciones SML (gancho).
   Devuelve la dirección de memoria que contiene el resultado. */
int eval_postfix_generate_sml(char **postfix) {
    int addr_stack[256];
    int sp = -1;
    {
        int i;
        for (i = 0; postfix[i]; ++i) {
            char *t = postfix[i];
            if (is_op(t)) {
                if (sp < 1) {
                    fprintf(stderr, "Error: expresión mal formada (operandos insuficientes)\n");
                    exit(EXIT_FAILURE);
                }
                int right = addr_stack[sp--];
                int left = addr_stack[sp--];

                /* Generar secuencia: LOAD left; ADD/SUB right; STORE temp; push temp */
                emit(OP_LOAD, left);
                if (strcmp(t, "+") == 0) emit(OP_ADD, right);
                else if (strcmp(t, "-") == 0) emit(OP_SUB, right);
                else if (strcmp(t, "*") == 0 || strcmp(t, "/") == 0) {
                    /* Multiplicación/división no implementadas en SML básico;
                       aquí se podría simular con sumas repetidas o llamadas a rutina.
                       Por ahora, reportamos error si se usan. */
                    fprintf(stderr, "Error: operador %s no implementado en generación SML\n", t);
                    exit(EXIT_FAILURE);
                } else {
                    fprintf(stderr, "Error: operador desconocido %s\n", t);
                    exit(EXIT_FAILURE);
                }
                int temp = allocate_data_slot();
                emit(OP_STORE, temp);
                addr_stack[++sp] = temp;
            } else {
                /* Operando: constante entera o variable (nombre de una letra) */
                char *endptr;
                long val = strtol(t, &endptr, 10);
                if (*endptr == '\0') {
                    /* constante */
                    int sym = (int)val;
                    tableEntry *e = find_symbol(sym, 'C');
                    if (!e) {
                        e = insert_symbol(sym, 'C');
                        int loc = allocate_data_slot();
                        e->location = loc;
                        memory[loc] = (int)val;
                    }
                    addr_stack[++sp] = e->location;
                } else {
                    /* variable: asumimos una sola letra */
                    if (strlen(t) != 1 || !isalpha((unsigned char)t[0])) {
                        fprintf(stderr, "Error: nombre de variable inválido: %s\n", t);
                        exit(EXIT_FAILURE);
                    }
                    int sym = (int)t[0];
                    tableEntry *e = find_symbol(sym, 'V');
                    if (!e) {
                        e = insert_symbol(sym, 'V');
                        int loc = allocate_data_slot();
                        e->location = loc;
                        e->type = 'V';
                        memory[loc] = 0;
                    }
                    addr_stack[++sp] = e->location;
                }
            }
        }
    }
    if (sp != 0) {
        fprintf(stderr, "Error: evaluación posfija terminó con pila de tamaño %d (se esperaba 1)\n", sp+1);
        exit(EXIT_FAILURE);
    }
    return addr_stack[sp];
}

/* Procesar una línea en la primera pasada */
void process_line_first_pass(char *line) {
    char *copy = strdup(line);
    if (!copy) { perror("strdup"); exit(EXIT_FAILURE); }
    char *token = strtok(copy, " ");
    if (!token) { free(copy); return; }
    int lineno = atoi(token);

    /* Insertar número de línea en tabla como tipo 'L' */
    tableEntry *lineEntry = find_symbol(lineno, 'L');
    if (!lineEntry) {
        lineEntry = insert_symbol(lineno, 'L');
        lineEntry->location = instr_ptr;
    } else if (lineEntry->location == -1) {
        lineEntry->location = instr_ptr;
    }

    char *cmd = strtok(NULL, " ");
    if (!cmd) { free(copy); return; }

    if (strcmp(cmd, "rem") == 0) {
        free(copy);
        return;
    } else if (strcmp(cmd, "input") == 0) {
        char *var = strtok(NULL, " ");
        if (!var) { fprintf(stderr, "Error: input sin variable\n"); exit(EXIT_FAILURE); }
        int sym = (int)var[0];
        tableEntry *v = find_symbol(sym, 'V');
        if (!v) {
            v = insert_symbol(sym, 'V');
            int loc = allocate_data_slot();
            v->location = loc;
            v->type = 'V';
            memory[loc] = 0;
        }
        emit(OP_READ, v->location);
    } else if (strcmp(cmd, "print") == 0) {
        char *var = strtok(NULL, " ");
        if (!var) { fprintf(stderr, "Error: print sin operando\n"); exit(EXIT_FAILURE); }
        if (isdigit((unsigned char)var[0]) || (var[0]=='-' && isdigit((unsigned char)var[1]))) {
            int val = atoi(var);
            tableEntry *c = find_symbol(val, 'C');
            if (!c) {
                c = insert_symbol(val, 'C');
                int loc = allocate_data_slot();
                c->location = loc;
                c->type = 'C';
                memory[loc] = val;
            }
            emit(OP_WRITE, c->location);
        } else {
            int sym = (int)var[0];
            tableEntry *v = find_symbol(sym, 'V');
            if (!v) {
                v = insert_symbol(sym, 'V');
                int loc = allocate_data_slot();
                v->location = loc;
                v->type = 'V';
                memory[loc] = 0;
            }
            emit(OP_WRITE, v->location);
        }
    } else if (strcmp(cmd, "goto") == 0) {
        char *target = strtok(NULL, " ");
        int targetLine = atoi(target);
        tableEntry *t = find_symbol(targetLine, 'L');
        if (t && t->location != -1) {
            emit(OP_BRANCH, t->location);
        } else {
            emit_branch_with_line(OP_BRANCH, targetLine);
        }
    } else if (strcmp(cmd, "if") == 0) {
        /* if <left> <relop> <right> goto <line> */
        char *left = strtok(NULL, " ");
        char *relop = strtok(NULL, " ");
        char *right = strtok(NULL, " ");
        char *goto_kw = strtok(NULL, " ");
        char *target = strtok(NULL, " ");
        if (!left || !relop || !right || !goto_kw || !target) {
            fprintf(stderr, "Error: if mal formado\n"); exit(EXIT_FAILURE);
        }
        int left_addr, right_addr;
        if (isdigit((unsigned char)left[0]) || (left[0]=='-' && isdigit((unsigned char)left[1]))) {
            int val = atoi(left);
            tableEntry *c = find_symbol(val, 'C');
            if (!c) {
                c = insert_symbol(val, 'C');
                int loc = allocate_data_slot();
                c->location = loc;
                c->type = 'C';
                memory[loc] = val;
            }
            left_addr = c->location;
        } else {
            int sym = (int)left[0];
            tableEntry *v = find_symbol(sym, 'V');
            if (!v) {
                v = insert_symbol(sym, 'V');
                int loc = allocate_data_slot();
                v->location = loc;
                v->type = 'V';
                memory[loc] = 0;
            }
            left_addr = v->location;
        }
        if (isdigit((unsigned char)right[0]) || (right[0]=='-' && isdigit((unsigned char)right[1]))) {
            int val = atoi(right);
            tableEntry *c = find_symbol(val, 'C');
            if (!c) {
                c = insert_symbol(val, 'C');
                int loc = allocate_data_slot();
                c->location = loc;
                c->type = 'C';
                memory[loc] = val;
            }
            right_addr = c->location;
        } else {
            int sym = (int)right[0];
            tableEntry *v = find_symbol(sym, 'V');
            if (!v) {
                v = insert_symbol(sym, 'V');
                int loc = allocate_data_slot();
                v->location = loc;
                v->type = 'V';
                memory[loc] = 0;
            }
            right_addr = v->location;
        }
        /* Generar: LOAD left; SUB right; BRANCHZERO target (para ==) */
        emit(OP_LOAD, left_addr);
        emit(OP_SUB, right_addr);
        {
            int targetLine = atoi(target);
            tableEntry *t = find_symbol(targetLine, 'L');
            if (t && t->location != -1) {
                emit(OP_BRANCHZERO, t->location);
            } else {
                emit_branch_with_line(OP_BRANCHZERO, targetLine);
            }
        }
    } else if (strcmp(cmd, "let") == 0) {
        /* let <var> = <expression...> */
        char *var = strtok(NULL, " ");
        char *eq = strtok(NULL, " ");
        char *rest = strtok(NULL, "\n");
        if (!var || !eq || !rest) { fprintf(stderr, "Error: let mal formado\n"); exit(EXIT_FAILURE); }
        char *expr = trim(rest);
        int sym = (int)var[0];
        tableEntry *lhs = find_symbol(sym, 'V');
        if (!lhs) {
            lhs = insert_symbol(sym, 'V');
            int loc = allocate_data_slot();
            lhs->location = loc;
            lhs->type = 'V';
            memory[loc] = 0;
        }
        char **postfix = infix_to_postfix(expr);
        int result_addr = eval_postfix_generate_sml(postfix);
        emit(OP_LOAD, result_addr);
        emit(OP_STORE, lhs->location);
        free_postfix(postfix);
    } else if (strcmp(cmd, "end") == 0) {
        emit(OP_HALT, 0);
    } else {
        fprintf(stderr, "Error: comando desconocido: %s\n", cmd);
        exit(EXIT_FAILURE);
    }

    free(copy);
}

/* Primera pasada: leer archivo de entrada y procesar línea por línea */
void first_pass(const char *infile) {
    FILE *f = fopen(infile, "r");
    if (!f) { perror("Error al abrir archivo de entrada"); exit(EXIT_FAILURE); }
    char line[LINE_MAX];
    while (fgets(line, sizeof(line), f)) {
        char *t = trim(line);
        if (strlen(t) == 0) continue;
        process_line_first_pass(t);
    }
    fclose(f);
}

/* Segunda pasada: resolver flags y escribir archivo SML */
void second_pass_and_write(const char *outfile) {
    {
        int i;
        for (i = 0; i < MEM_SIZE; ++i) {
            if (flags[i] != -1) {
                int targetLine = flags[i];
                tableEntry *t = find_symbol(targetLine, 'L');
                if (!t || t->location == -1) {
                    fprintf(stderr, "Error: referencia a línea indefinida: %d\n", targetLine);
                    exit(EXIT_FAILURE);
                }
                int opcode = memory[i] / 100;
                memory[i] = opcode * 100 + t->location;
                flags[i] = -1;
            }
        }
    }
    FILE *out = fopen(outfile, "w");
    if (!out) { perror("Error al abrir archivo de salida"); exit(EXIT_FAILURE); }
    {
        int i;
        for (i = 0; i < instr_ptr; ++i) {
            fprintf(out, "%+05d\n", memory[i]);
        }
    }
    fclose(out);
}

/* Inicializar estado */
void init_state() {
    {
        int i;
        for (i = 0; i < MEM_SIZE; ++i) {
            memory[i] = 0;
            flags[i] = -1;
        }
    }
    instr_ptr = 0;
    data_ptr = MEM_SIZE - 1;
}

/* Imprimir tabla de símbolos (para verificación) */
void print_symbol_table() {
    printf("Tabla de símbolos:\n");
    tableEntry *p = symbolTable;
    while (p) {
        if (p->type == 'L') printf("Línea %d -> loc %02d\n", p->symbol, p->location);
        else if (p->type == 'V') printf("Var '%c' -> loc %02d\n", (char)p->symbol, p->location);
        else if (p->type == 'C') printf("Const %d -> loc %02d\n", p->symbol, p->location);
        p = p->next;
    }
}

/* Programa principal */
int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "Uso: %s archivo_entrada.simple archivo_salida.sml\n", argv[0]);
        return 1;
    }
    const char *infile = argv[1];
    const char *outfile = argv[2];

    init_state();
    first_pass(infile);
    second_pass_and_write(outfile);

    print_symbol_table();

    printf("Compilación finalizada. Archivo SML escrito en %s\n", outfile);
    return 0;
}

