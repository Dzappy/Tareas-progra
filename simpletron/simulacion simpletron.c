#include <stdio.h>
#include <stdlib.h>

#define MEM_SIZE 1000
#define MIN_WORD -9999
#define MAX_WORD  9998

int main(void)
{
    int memory[MEM_SIZE] = {0};
    int accumulator = 0;
    int instructionCounter = 0;
    int instructionRegister = 0;
    int operationCode = 0;
    int operand = 0;
    int input;
    int i;
    int instruccionesEjecutadas = 0;

    FILE *fp = fopen("programa.simp", "r");
    if (fp != NULL) {
        int pos = 0, val;
        while (pos < MEM_SIZE && fscanf(fp, "%d", &val) == 1) {
            if (val == 9999) break;
            if (val < MIN_WORD || val > MAX_WORD) {
                printf("Error: instruccion fuera de rango en archivo.\n");
                fclose(fp);
                return 1;
            }
            memory[pos++] = val;
        }
        fclose(fp);
        printf("Programa cargado desde archivo.\n");
    } else {
        printf("¡ Bienvenido a Simpletron!\n");
        printf("Introduzca su programa una instruccion (o palabra de datos) a la vez.\n");
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

    while (1) {
        if (instructionCounter < 0 || instructionCounter >= MEM_SIZE) {
            printf("Error: instructionCounter fuera de rango.\n");
            break;
        }

        instructionRegister = memory[instructionCounter];
        operationCode = instructionRegister / 100;
        operand = instructionRegister % 1000;

        if (operand < 0 || operand >= MEM_SIZE) {
            printf("Error: operando fuera de rango en instruccion %03d.\n", instructionCounter);
            break;
        }

        instruccionesEjecutadas++;

        switch (operationCode) {
            case 10: /* leer */
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
                break;

            case 11: /* escribir */
                printf("Salida: %d\n", memory[operand]);
                instructionCounter++;
                break;

            case 12: /* nueva linea */
                printf("\n");
                instructionCounter++;
                break;

            case 13: /* leer cadena */
            {
                char buffer[256];
                int len, j;
                printf("Entrada de cadena (max 255 chars): ");
                scanf(" %[^\n]", buffer);
                len = 0;
                while (buffer[len] != '\0' && len < 255) len++;

                memory[operand] = len;
                for (j = 0; j < len; j++) {
                    if (operand + 1 + j >= MEM_SIZE) {
                        printf("Error: cadena excede memoria.\n");
                        goto dump_and_exit;
                    }
                    memory[operand + 1 + j] = (int)buffer[j];
                }
                instructionCounter++;
                break;
            }

            case 14: /* escribir cadena */
            {
                int len = memory[operand];
                int j;
                if (len < 0 || operand + len >= MEM_SIZE) {
                    printf("Error: direccion o longitud invalida para cadena.\n");
                    goto dump_and_exit;
                }
                for (j = 0; j < len; j++) {
                    int asciiVal = memory[operand + 1 + j];
                    if (asciiVal < 0 || asciiVal > 127) {
                        printf("Error: valor ASCII invalido en cadena.\n");
                        goto dump_and_exit;
                    }
                    printf("%c", (char)asciiVal);
                }
                printf("\n");
                instructionCounter++;
                break;
            }

            case 20: accumulator = memory[operand]; instructionCounter++; break;
            case 21: memory[operand] = accumulator; instructionCounter++; break;

            case 30: accumulator += memory[operand];
                     if (accumulator < MIN_WORD || accumulator > 9999) {
                         printf("Error: desbordamiento del acumulador.\n");
                         goto dump_and_exit;
                     }
                     instructionCounter++;
                     break;

            case 31: accumulator -= memory[operand];
                     if (accumulator < MIN_WORD || accumulator > 9999) {
                         printf("Error: desbordamiento del acumulador.\n");
                         goto dump_and_exit;
                     }
                     instructionCounter++;
                     break;

            case 32: if (memory[operand] == 0) {
                         printf("** Intento de dividir entre cero **\n");
                         goto dump_and_exit;
                     }
                     accumulator /= memory[operand];
                     instructionCounter++;
                     break;

            case 33: accumulator *= memory[operand];
                     if (accumulator < MIN_WORD || accumulator > 9999) {
                         printf("Error: desbordamiento del acumulador.\n");
                         goto dump_and_exit;
                     }
                     instructionCounter++;
                     break;

            case 34: if (memory[operand] == 0) {
                         printf("** Intento de modulo con divisor cero **\n");
                         goto dump_and_exit;
                     }
                     accumulator %= memory[operand];
                     if (accumulator < MIN_WORD || accumulator > 9999) {
                         printf("Error: desbordamiento del acumulador.\n");
                         goto dump_and_exit;
                     }
                     instructionCounter++;
                     break;

            case 35:
            {
                int base = accumulator;
                int exp = memory[operand];
                int result = 1;
                int k;
                if (exp < 0) {
                    printf("Error: exponente negativo no soportado.\n");
                    goto dump_and_exit;
                }
                for (k = 0; k < exp; k++) {
                    result *= base;
                    if (result < MIN_WORD || result > 9999) {
                        printf("Error: desbordamiento en exponenciacion.\n");
                        goto dump_and_exit;
                    }
                }
                accumulator = result;
                instructionCounter++;
                break;
            }

            case 40: instructionCounter = operand; break;
            case 41: if (accumulator < 0) instructionCounter = operand; else instructionCounter++; break;
            case 42: if (accumulator == 0) instructionCounter = operand; else instructionCounter++; break;
            case 43: printf("Terminó la ejecucion de Simpletron\n"); goto dump_and_exit;

            default:
                printf("Error: codigo de operacion invalido (%d) en %03d.\n", operationCode, instructionCounter);
                goto dump_and_exit;
        }
    }

dump_and_exit:
    printf("\nREGISTROS:\n");
    printf("acumulador: %+05d\n", accumulator);
    printf("instructionCounter: %03d\n", instructionCounter);
    printf("instructionRegister: %+05d\n", instructionRegister);
    printf("operationCode: %02d\n", operationCode);
    printf("operand: %03d\n\n", operand);

    printf("MEMORIA:\n");
    for (i = 0; i < MEM_SIZE; i += 10) {
        int j;
        printf("%3d: ", i);
        for (j = 0; j < 10; ++j) {
            printf("%+05d ", memory[i + j]);
        }
        printf("\n");
    }

    printf("\nResumen:\n");
    printf("Instrucciones ejecutadas: %d\n", instruccionesEjecutadas);
    printf("Ultima instruccion: %+05d\n", instructionRegister);
    printf("Acumulador final: %+05d\n", accumulator);

    return 0;
}

