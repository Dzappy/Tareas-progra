

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MEM_SIZE 1000
#define MIN_WORD -9999
#define MAX_WORD  9999

/* push/pop helpers: usan memoria y stackPointer */
static int push_to_stack(int memory[], int *sp, int value) {
    if (*sp <= 0) return -1; /* overflow */
    (*sp)--;
    memory[*sp] = value;
    return 0;
}
static int pop_from_stack(int memory[], int *sp, int *out) {
    if (*sp >= MEM_SIZE) return -1; /* underflow */
    *out = memory[*sp];
    (*sp)++;
    return 0;
}

int main(void)
{
    int memory[MEM_SIZE];
    int i;
    for (i = 0; i < MEM_SIZE; ++i) memory[i] = 0;

    int accumulator = 0;
    int instructionCounter = 0;
    int instructionRegister = 0;
    int operationCode = 0;
    int operand = 0;
    int input;
    int instruccionesEjecutadas = 0;

    /* Pila: crece hacia abajo desde MEM_SIZE-1; SP apunta a la próxima posición libre */
    int stackPointer = MEM_SIZE;

    /* Cargar programa desde archivo si existe */
    FILE *fp = fopen("programa.simp", "r");
    if (fp != NULL) {
        int pos = 0, val;
        while (pos < MEM_SIZE && fscanf(fp, "%d", &val) == 1) {
            if (val == 9999) break;
            if (val < MIN_WORD || val > MAX_WORD) {
                printf("Error: instruccion fuera de rango en archivo (pos %d).\n", pos);
                fclose(fp);
                return 1;
            }
            memory[pos++] = val;
        }
        fclose(fp);
        printf("Programa cargado desde archivo.\n");
    } else {
        printf(" Bienvenido a Simpletron!\n");
        printf("Introduzca su programa o una instruccion \n");
        printf("Formato instruccion: opcode*1000 + operand (ej: 13009 -> opcode 13, operand 009)\n");
        printf("Teclee 9999 para terminar la carga.\n\n");

        for (i = 0; i < MEM_SIZE; ++i) {
            while (1) {
                printf("%03d ? ", i);
                if (scanf("%d", &input) != 1) {
                    int c;
                    while ((c = getchar()) != '\n' && c != EOF) { }
                    printf("Entrada no valida. Intente de nuevo.\n");
                    continue;
                }
                if (input == 9999) {
                    printf("Se termino de cargar el programa\n");
                    goto start_execution;
                }
                if (input < MIN_WORD || input > MAX_WORD) {
                    printf("Valor fuera de rango (%d a %d). Intente de nuevo.\n", MIN_WORD, MAX_WORD);
                    continue;
                }
                memory[i] = input;
                break;
            }
        }
    }

start_execution:
    printf("** Comienza la ejecucion del programa **\n\n");
    /* limpiar entrada antes de usar fgets más adelante */
    {
        int c;
        while ((c = getchar()) != '\n' && c != EOF) { }
    }

    while (1) {
        if (instructionCounter < 0 || instructionCounter >= MEM_SIZE) {
            printf("Error: instructionCounter fuera de rango (%d).\n", instructionCounter);
            break;
        }

        instructionRegister = memory[instructionCounter];
        operationCode = instructionRegister / 1000;   /* opcode: 2 dígitos  */
        operand = instructionRegister % 1000;         /* operand: 0..999 */

        instruccionesEjecutadas++;

        switch (operationCode) {
            case 10: /* leer entero en memory[operand] */
                if (operand < 0 || operand >= MEM_SIZE) {
                    printf("Error: operando fuera de rango en leer (%03d).\n", operand);
                    goto dump_and_exit;
                }
                printf("Entrada para la posicion %03d: ", operand);
                if (scanf("%d", &input) != 1) {
                    int c;
                    while ((c = getchar()) != '\n' && c != EOF) { }
                    printf("Entrada no valida. Terminando.\n");
                    goto dump_and_exit;
                }
                if (input < MIN_WORD || input > MAX_WORD) {
                    printf("Valor fuera de rango. Terminando.\n");
                    goto dump_and_exit;
                }
                memory[operand] = input;
                instructionCounter++;
                /* limpiar resto de línea */
                {
                    int c;
                    while ((c = getchar()) != '\n' && c != EOF) { }
                }
                break;

            case 11: /* escribir entero desde memory[operand] */
                if (operand < 0 || operand >= MEM_SIZE) {
                    printf("Error: operando fuera de rango en escribir (%03d).\n", operand);
                    goto dump_and_exit;
                }
                printf("Salida: %d\n", memory[operand]);
                instructionCounter++;
                break;

            case 12: /* nueva linea */
                printf("\n");
                instructionCounter++;
                break;

            case 13: /* leer cadena, almacena longitud en memory[operand], luego ASCII en siguientes posiciones */
            {
                if (operand < 0 || operand >= MEM_SIZE) {
                    printf("Error: operando fuera de rango en leer cadena (%03d).\n", operand);
                    goto dump_and_exit;
                }
                char buffer[512];
                if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
                    buffer[0] = '\0';
                }
                /* quitar salto de linea final si existe */
                size_t len = strlen(buffer);
                if (len > 0 && buffer[len - 1] == '\n') {
                    buffer[len - 1] = '\0';
                    len--;
                }
              
                if (len > 255) {
                    len = 255;
                    buffer[len] = '\0';
                }
                /* comprobar espacio en memoria */
                if (operand + 1 + (int)len > MEM_SIZE) {
                    printf("Error: cadena excede memoria disponible.\n");
                    goto dump_and_exit;
                }
                memory[operand] = (int)len;
                for (i = 0; i < (int)len; ++i) {
                    memory[operand + 1 + i] = (int)(unsigned char)buffer[i];
                }
                instructionCounter++;
                break;
            }

            case 14: /* escribir cadena: lee longitud y caracteres ASCII */
            {
                if (operand < 0 || operand >= MEM_SIZE) {
                    printf("Error: operando fuera de rango en escribir cadena (%03d).\n", operand);
                    goto dump_and_exit;
                }
                int len = memory[operand];
                if (len < 0 || operand + 1 + len > MEM_SIZE) {
                    printf("Error: direccion o longitud invalida para cadena.\n");
                    goto dump_and_exit;
                }
                for (i = 0; i < len; ++i) {
                    int asciiVal = memory[operand + 1 + i];
                    if (asciiVal < 0 || asciiVal > 255) {
                        printf("Error: valor ASCII invalido en cadena.\n");
                        goto dump_and_exit;
                    }
                    putchar((char)asciiVal);
                }
                putchar('\n');
                instructionCounter++;
                break;
            }

            case 20: /* cargar */
                if (operand < 0 || operand >= MEM_SIZE) {
                    printf("Error: operando fuera de rango en cargar (%03d).\n", operand);
                    goto dump_and_exit;
                }
                accumulator = memory[operand];
                instructionCounter++;
                break;

            case 21: /* almacenar */
                if (operand < 0 || operand >= MEM_SIZE) {
                    printf("Error: operando fuera de rango en almacenar (%03d).\n", operand);
                    goto dump_and_exit;
                }
                memory[operand] = accumulator;
                instructionCounter++;
                break;

            case 30: /* sumar */
                if (operand < 0 || operand >= MEM_SIZE) {
                    printf("Error: operando fuera de rango en sumar (%03d).\n", operand);
                    goto dump_and_exit;
                }
                accumulator += memory[operand];
                if (accumulator < MIN_WORD || accumulator > MAX_WORD) {
                    printf("Error: desbordamiento del acumulador.\n");
                    goto dump_and_exit;
                }
                instructionCounter++;
                break;

            case 31: /* restar */
                if (operand < 0 || operand >= MEM_SIZE) {
                    printf("Error: operando fuera de rango en restar (%03d).\n", operand);
                    goto dump_and_exit;
                }
                accumulator -= memory[operand];
                if (accumulator < MIN_WORD || accumulator > MAX_WORD) {
                    printf("Error: desbordamiento del acumulador.\n");
                    goto dump_and_exit;
                }
                instructionCounter++;
                break;

            case 32: /* dividir */
                if (operand < 0 || operand >= MEM_SIZE) {
                    printf("Error: operando fuera de rango en dividir (%03d).\n", operand);
                    goto dump_and_exit;
                }
                if (memory[operand] == 0) {
                    printf("** Intento de dividir entre cero **\n");
                    goto dump_and_exit;
                }
                accumulator /= memory[operand];
                instructionCounter++;
                break;

            case 33: /* multiplicar */
                if (operand < 0 || operand >= MEM_SIZE) {
                    printf("Error: operando fuera de rango en multiplicar (%03d).\n", operand);
                    goto dump_and_exit;
                }
                accumulator *= memory[operand];
                if (accumulator < MIN_WORD || accumulator > MAX_WORD) {
                    printf("Error: desbordamiento del acumulador.\n");
                    goto dump_and_exit;
                }
                instructionCounter++;
                break;

            case 34: /* modulo */
                if (operand < 0 || operand >= MEM_SIZE) {
                    printf("Error: operando fuera de rango en modulo (%03d).\n", operand);
                    goto dump_and_exit;
                }
                if (memory[operand] == 0) {
                    printf("** Intento de modulo con divisor cero **\n");
                    goto dump_and_exit;
                }
                accumulator %= memory[operand];
                if (accumulator < MIN_WORD || accumulator > MAX_WORD) {
                    printf("Error: desbordamiento del acumulador.\n");
                    goto dump_and_exit;
                }
                instructionCounter++;
                break;

            case 35: /* exponenciacion (base = accumulator, exp = memory[operand]) */
            {
                if (operand < 0 || operand >= MEM_SIZE) {
                    printf("Error: operando fuera de rango en exponenciacion (%03d).\n", operand);
                    goto dump_and_exit;
                }
                int base = accumulator;
                int exp = memory[operand];
                int result = 1;
                int k;
                if (exp < 0) {
                    printf("Error: exponente negativo no soportado.\n");
                    goto dump_and_exit;
                }
                for (k = 0; k < exp; ++k) {
                    result *= base;
                    if (result < MIN_WORD || result > MAX_WORD) {
                        printf("Error: desbordamiento en exponenciacion.\n");
                        goto dump_and_exit;
                    }
                }
                accumulator = result;
                instructionCounter++;
                break;
            }

            case 40: /* bifurcar incondicional */
                if (operand < 0 || operand >= MEM_SIZE) {
                    printf("Error: operando fuera de rango en bifurcar (%03d).\n", operand);
                    goto dump_and_exit;
                }
                instructionCounter = operand;
                break;

            case 41: /* bifurcar si negativo */
                if (operand < 0 || operand >= MEM_SIZE) {
                    printf("Error: operando fuera de rango en bifurcar si negativo (%03d).\n", operand);
                    goto dump_and_exit;
                }
                if (accumulator < 0) instructionCounter = operand;
                else instructionCounter++;
                break;

            case 42: /* bifurcar si cero */
                if (operand < 0 || operand >= MEM_SIZE) {
                    printf("Error: operando fuera de rango en bifurcar si cero (%03d).\n", operand);
                    goto dump_and_exit;
                }
                if (accumulator == 0) instructionCounter = operand;
                else instructionCounter++;
                break;

            case 43: /* alto / terminar */
                printf("Terminó la ejecucion de Simpletron\n");
                goto dump_and_exit;

            /* --- Pila y subrutinas --- */
            case 50: /* CALL operand: push return address, jump to operand */
                if (operand < 0 || operand >= MEM_SIZE) {
                    printf("Error: operando fuera de rango en CALL (%03d).\n", operand);
                    goto dump_and_exit;
                }
                if (push_to_stack(memory, &stackPointer, instructionCounter + 1) != 0) {
                    printf("Error: overflow de pila en CALL.\n");
                    goto dump_and_exit;
                }
                instructionCounter = operand;
                break;

            case 51: /* RETURN: pop return address and jump */
            {
                int ret;
                if (pop_from_stack(memory, &stackPointer, &ret) != 0) {
                    printf("Error: underflow de pila en RETURN.\n");
                    goto dump_and_exit;
                }
                if (ret < 0 || ret >= MEM_SIZE) {
                    printf("Error: direccion de retorno invalida (%d).\n", ret);
                    goto dump_and_exit;
                }
                instructionCounter = ret;
                break;
            }

            case 52: /* PUSH operand: push memory[operand] */
                if (operand < 0 || operand >= MEM_SIZE) {
                    printf("Error: operando fuera de rango en PUSH (%03d).\n", operand);
                    goto dump_and_exit;
                }
                if (push_to_stack(memory, &stackPointer, memory[operand]) != 0) {
                    printf("Error: overflow de pila en PUSH.\n");
                    goto dump_and_exit;
                }
                instructionCounter++;
                break;

            case 53: /* POP operand: pop into memory[operand] */
                if (operand < 0 || operand >= MEM_SIZE) {
                    printf("Error: operando fuera de rango en POP (%03d).\n", operand);
                    goto dump_and_exit;
                }
                {
                    int val;
                    if (pop_from_stack(memory, &stackPointer, &val) != 0) {
                        printf("Error: underflow de pila en POP.\n");
                        goto dump_and_exit;
                    }
                    memory[operand] = val;
                }
                instructionCounter++;
                break;

            default:
                printf("Error: codigo de operacion invalido (%d) en %03d.\n", operationCode, instructionCounter);
                goto dump_and_exit;
        } /* switch */
    } /* while */

dump_and_exit:
    printf("\nREGISTROS:\n");
    printf("acumulador: %+05d\n", accumulator);
    printf("instructionCounter: %03d\n", instructionCounter);
    printf("instructionRegister: %+05d\n", instructionRegister);
    printf("operationCode: %02d\n", operationCode);
    printf("operand: %03d\n\n", operand);

    printf("STACK POINTER: %03d\n\n", stackPointer);

    printf("MEMORIA (primeras 200 posiciones o hasta MEM_SIZE):\n");
    {
        int limit = MEM_SIZE;
        for (i = 0; i < limit; i += 10) {
            int j;
            printf("%3d: ", i);
            for (j = 0; j < 10; ++j) {
                printf("%+05d ", memory[i + j]);
            }
            printf("\n");
        }
    }

    printf("\nResumen:\n");
    printf("Instrucciones ejecutadas: %d\n", instruccionesEjecutadas);
    printf("Ultima instruccion: %+05d\n", instructionRegister);
    printf("Acumulador final: %+05d\n", accumulator);

    return 0;
}

