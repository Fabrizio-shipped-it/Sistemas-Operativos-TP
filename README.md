# Trabajos Prácticos - Sistemas Operativos

Dos aplicaciones en C para Linux desarrolladas para la materia **Sistemas Operativos**. Cada una pone en práctica un mecanismo distinto de comunicación e IPC (Inter-Process Communication) visto en la cursada: sockets con hilos en la primera, y memoria compartida con procesos y semáforos en la segunda.

## Contenido

- [App-Cliente-Servidor](#app-cliente-servidor) — arquitectura cliente/servidor con sockets TCP e hilos (`pthread`)
- [App-Memoria-Compartida](#app-memoria-compartida) — procesos (`fork`), memoria compartida (`shm`) y semáforos (`sem`)

## Requisitos

- Linux (o WSL en Windows)
- `gcc`
- `make` (opcional, para la app cliente-servidor)

---

## App-Cliente-Servidor

Aplicación de sockets TCP en la que un **servidor** multihilo atiende a varios **clientes** en simultáneo. Cada cliente que se conecta es manejado en un hilo propio (`pthread_create`), y el servidor usa un mutex para proteger de forma segura el contador de clientes activos.

### Funcionamiento

- El servidor escucha en `127.0.0.1:5000` y acepta hasta 4 clientes en simultáneo (`MAX_CLIENTES`). Si se supera el límite, rechaza la conexión.
- Cada cliente se comunica con el servidor enviando comandos de texto por consola; el servidor los procesa y responde.
- El servidor se apaga automáticamente cuando se desconecta el último cliente, o de forma manual con `Ctrl+C` (`SIGINT`), liberando los sockets correctamente.

### Comandos disponibles desde el cliente

| Comando | Descripción |
|---|---|
| `CORTAR:palabra` | Devuelve la palabra marcada como "cortada" |
| `COCINAR:palabra` | Devuelve la palabra marcada como "cocinada" |
| `EMPLATAR:palabra` | Devuelve la palabra marcada como "emplatada" |
| `SUMA:num1:num2` | Devuelve la suma de ambos números |
| `RESTA:num1:num2` | Devuelve la resta de ambos números |
| `INVERSO:palabra` | Devuelve la palabra invertida |
| `MAYUS:palabra` | Devuelve la palabra en mayúsculas |
| `RECETAS:` | Lista las recetas disponibles |
| `CHEFF:n` | Corre el proceso completo (cortar → cocinar → picar → emplatar) sobre un lote de datos de prueba, según la receta `n` |
| `SALIR` | Cierra la conexión del cliente |

### Compilación y ejecución

Con `make` (usando el `Makefile` incluido):

```bash
cd "1- App-Cliente-Servidor"
make clean
make
```

O manualmente:

```bash
gcc -Wall -pthread Servidor.c -o Servidor
gcc -Wall -pthread Cliente.c -o Cliente
```

Para probarlo, en una terminal levantar el servidor:

```bash
./Servidor
```

Y en otra (u otras, para simular varios clientes) conectar el cliente indicando IP y puerto del servidor:

```bash
./Cliente 127.0.0.1 5000
```

---

## App-Memoria-Compartida

Simulación de una cocina en la que un proceso "cheff" coordina a cuatro procesos hijos (cortar, cocinar, picar, emplatar) que se ejecutan en orden y se sincronizan mediante **semáforos**, compartiendo un mismo bloque de **memoria compartida** (`shmget`/`shmat`) donde se van modificando los datos del plato.

### Funcionamiento

El programa muestra un menú por consola para elegir uno de tres platos. Cada plato aplica la misma secuencia de operaciones sobre un lote de datos inicial `{30, 10, 60, 20, 8}`, cambiando únicamente el número por el que se opera:

1. **Cortar** → divide cada valor
2. **Cocinar** → suma cada valor
3. **Picar** → multiplica cada valor
4. **Emplatar** → ordena los valores de menor a mayor

Cada etapa corre en un proceso hijo separado (`fork`), se sincroniza con la anterior mediante un semáforo distinto (`semA → semB → semC → semD`) y simula demoras de 10 a 20 segundos, imprimiendo su PID para poder seguir la ejecución en la terminal. Al finalizar (opción 4 del menú), se muestra un informe con la cantidad de veces que se pidió cada plato y la cantidad de interacciones de cada etapa, y se liberan todos los recursos (memoria compartida y semáforos).

`Ctrl+C` (`SIGINT`) también dispara la liberación segura de los recursos del sistema (memoria compartida y semáforos), evitando que queden huérfanos.

### Platos y resultados esperados

Sobre el lote base `{30, 10, 60, 20, 8}` (nota: al dividir por un número impar, se redondea hacia abajo):

| Plato | Número usado | Resultado esperado |
|---|---|---|
| Pastel de papa | 2 | `{12, 14, 24, 34, 64}` |
| Guiso de lentejas | 5 | `{30, 35, 45, 55, 85}` |
| Locro | 8 | `{72, 72, 80, 88, 120}` |

### Compilación y ejecución

```bash
cd "2- App-Memoria-Compartida"
gcc -c Ej1.c
gcc Ej1.o -o cocina
./cocina
```

---

## Autor

Trabajos prácticos realizados para la cátedra de Sistemas Operativos.
