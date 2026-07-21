# Programación Avanzada

Repositorio que reúne las prácticas y proyectos desarrollados durante la materia de **Programación Avanzada**. El objetivo principal del repositorio es mostrar la evolución de los distintos ejercicios hasta llegar al desarrollo de un compilador para el lenguaje **Simple**, generando código **SML** ejecutable en el simulador de **Simpletron**.

---

# Contenido del repositorio separado por carpetas 

## 1. Evaluación de expresiones

Esta carpeta contiene la primera implementación de los algoritmos para el manejo de expresiones aritméticas.

> **Nota:** Su contenido es prácticamente el mismo que la carpeta **Profundidad de anidamiento**. Se conserva únicamente la carpeta **Profundidad de anidamiento** porque fue subida en un commit diferente en la cual se pueden ver las fechas de los commits de las mejoras. Las dos  versiones son iguales pero si se requiere ver las fechas de las mejoras se  recomienda revisar **Profundidad de anidamiento**, ya que refleja mejor el historial y las modificaciones realizadas durante el desarrollo.

---

## 2. Profundidad de anidamiento

Implementación de los algoritmos que posteriormente fueron utilizados como base del compilador.

Incluye:

- Validación de profundidad de anidamiento.
- Verificación del balance de símbolos de agrupación.
- Conversión de expresiones infijas a postfijas.
- Evaluación de expresiones postfijas.
- Manejo de precedencia y asociatividad de operadores:
  - Paréntesis.
  - Potencia.
  - Multiplicación y división.
  - Suma y resta.
- Soporte para operandos de más de un dígito.
- Base para el procesamiento de expresiones del compilador.

---

## 3. Compilador (Proyecto principal)


Proyecto desarrollado en lenguaje C que implementa un compilador para el lenguaje **Simple**, convirtiendo programas fuente en código **SML (Simpletron Machine Language)** mediante un proceso de compilación de dos pasadas. ## Características El compilador es capaz de: - Leer un programa fuente escrito en Simple. - Realizar una primera pasada para: - Construir la tabla de símbolos. - Registrar números de línea, variables y constantes. - Generar instrucciones SML preliminares. - Marcar referencias hacia adelante. - Realizar una segunda pasada para: - Resolver referencias pendientes. - Completar instrucciones incompletas. - Generar el archivo final en formato SML. - Compilar correctamente las instrucciones: - `input` - `print` - `goto` - `if/goto` - `let` - `end` - Convertir expresiones de notación infija a posfija para las instrucciones `let`. - Generar código SML compatible con el simulador de Simpletron. --- # Requisitos - Compilador de C compatible con C89 (Dev-C++, GCC, MinGW, etc.). --- 

El programa puede ejecutarse de dos maneras. 

### Opción 1: Con argumentos 

```bash 
compilador.exe programa.simple.txt salida.sml 
``` 

### Opción 2: Sin argumentos 

el programa solicitará los nombres de los archivos: 

```text 
Archivo fuente: programa.simple.txt 
Archivo salida: salida.sml 
``` 

Al finalizar correctamente mostrará un mensaje similar a: 

```text 
Compilación completada. 
``` 

--- 

# Archivos 

- `compilador.c` → Código fuente del compilador. 
- `programa.simple.txt` → Programa fuente en Simple. 
- `salida.sml` → Código generado para Simpletron. 

--- 

### Historial de versiones

#### Versión 1.0

- Primera implementación del compilador.
- Presentaba múltiples errores durante la compilación.
- Existían problemas al localizar y abrir los archivos fuente, lo que impedía su correcto funcionamiento en distintos equipos.

#### Versión 2.0 (Actual)

- Corrección del manejo de archivos.
- Posibilidad de ejecutar el programa con argumentos o solicitando los nombres de los archivos al usuario.
- Corrección de errores de compilación.
- Mejor estabilidad general.
- Generación correcta del archivo SML listo para ejecutarse en el simulador.

---

## 4. Simpletron

Implementación del simulador de la computadora **Simpletron**.

Características principales:

- Carga de instrucciones SML en memoria.
- Inicialización de registros especiales.
- Ejecución del ciclo de instrucción:
  - Traer instrucción.
  - Decodificar.
  - Ejecutar.
  - Actualizar el contador de instrucciones.
- Implementación de los registros:
  - `accumulator`
  - `instructionCounter`
  - `instructionRegister`
  - `operationCode`
  - `operand`
- Generación del *memory dump*.
- Detección de errores:
  - División entre cero.
  - Opcode inválido.
  - Desbordamiento del acumulador.
  - Direcciones inválidas.
  - Valores fuera del rango permitido.

### Mejoras implementadas

- Carga de programas desde archivo.
- Soporte para memoria extendida (cuando aplica).
- Nuevos opcodes SML:
  - Residuo (módulo).
  - Exponenciación.
  - Salto de línea.
  - Entrada y salida de cadenas.
- Soporte básico para punto flotante.
- Definición de nuevos códigos de operación.

---

## 5. Tarea Git Remote

Carpeta correspondiente a una actividad solicitada durante el curso. Contiene únicamente la evidencia requerida (captura de pantalla e identificación del alumno).

---



**Diego V.**

Proyecto desarrollado como parte de la materia de **Programación Avanzada**.
