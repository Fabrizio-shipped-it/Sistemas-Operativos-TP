#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/prctl.h>
#include <signal.h>

#define N 5
#define HIJOS 4
#define MAX_CHEFFS 100

typedef struct {
    int datos[N];
    int interacciones[HIJOS];
    int finalizar;
} Mesa;

// Semáforos
struct sembuf wait_op = {0, -1, 0}; /// wait_op: operación de espera (P) → decrementa el valor del semáforo (bloquea si llega a 0).

struct sembuf signal_op = {0, 1, 0}; /// signal_op: operación de liberación (V) → incrementa el valor del semáforo (libera).


Mesa* mesa;
int shmid;
int semA, semB, semC, semD;

pid_t cheffsActivos[MAX_CHEFFS];
int cheffsCount = 0;

// ----------Handlers------------

void manejar_salida_hijo(int sig) {
    printf("\n[Proceso %d] Finalizado abruptamente por SIGTERM\n", getpid());
    kill(0, SIGTERM);
    exit(0);
}

void instalar_handler_hijo(){
    struct sigaction sa = {0};

    sa.sa_handler = manejar_salida_hijo;
    sigaction(SIGTERM, &sa, NULL);
}

void liberarRecursos(){
    if (semA > 0) semctl(semA, 0, IPC_RMID);
    if (semB > 0) semctl(semB, 0, IPC_RMID);
    if (semC > 0) semctl(semC, 0, IPC_RMID);
    if (semD > 0) semctl(semD, 0, IPC_RMID);
    if (mesa) {
        shmdt(mesa);
        shmctl(shmid, IPC_RMID, NULL);
    }
}

void manejar_salida_padre(int sig) {
    printf("\n[PADRE %d] Finalizando por SIGTERM\n", getpid());
    // Limpia antes de salir
    if (semA > 0) semctl(semA, 0, IPC_RMID);
    if (semB > 0) semctl(semB, 0, IPC_RMID);
    if (semC > 0) semctl(semC, 0, IPC_RMID);
    if (semD > 0) semctl(semD, 0, IPC_RMID);
    if (shmid > 0)
    {
        shmdt(mesa);
        shmctl(shmid, IPC_RMID, NULL);
    }
    exit(0);
}

/// ------------------------------ FUNCIONES HIJOS ----------------------------------------

void cortarIngredientes(int cantidad, int num, Mesa* mesa, int semEspera, int semSignal) {
    instalar_handler_hijo();
    prctl(PR_SET_PDEATHSIG, SIGTERM);
    semop(semEspera, &wait_op, 1);
    printf("\n[Cortar] PID: %d\n", getpid());

    for (int i = 0; i < cantidad; i++) {
        mesa->datos[i] /= num;
        printf(" Corta %d", mesa->datos[i]);
    }
    printf("\n");

    mesa->interacciones[0]++;
    sleep(10);

    semop(semSignal, &signal_op, 1);
    exit(0);
}

void picarIngredientes(int cantidad, int num, Mesa* mesa, int semEspera, int semSignal) {
    instalar_handler_hijo();
    prctl(PR_SET_PDEATHSIG, SIGTERM);
    semop(semEspera, &wait_op, 1);
    printf("\n[Picar] PID: %d\n", getpid());

    for (int i = 0; i < cantidad; i++) {
        mesa->datos[i] *= num;
        printf(" Pica %d", mesa->datos[i]);
    }
    printf("\n");

    mesa->interacciones[1]++;
    sleep(15);

    semop(semSignal, &signal_op, 1);
    exit(0);
}

void cocinarIngredientes(int cantidad, int num, Mesa* mesa, int semEspera, int semSignal) {
    instalar_handler_hijo();
    prctl(PR_SET_PDEATHSIG, SIGTERM);
    semop(semEspera, &wait_op, 1);  ///Bajo el valor del semaforo en 1
    printf("\n[Cocinar] PID: %d\n", getpid());

    for (int i = 0; i < cantidad; i++) {
        mesa->datos[i] += num;
        printf(" Cocina %d", mesa->datos[i]);
    }
    printf("\n");

    mesa->interacciones[2]++;
    sleep(10);

    semop(semSignal, &signal_op, 1);///Subo el valor del semaforo en 1 para que el sig semaforo pueda avanzar
    exit(0);
}

void emplatarIngredientes(int cantidad, Mesa* mesa, int semEspera, int semSignal) {
    instalar_handler_hijo();
    prctl(PR_SET_PDEATHSIG, SIGTERM);
    semop(semEspera, &wait_op, 1);
    printf("\n[Emplatar] PID: %d\n", getpid());

    for (int i = 0; i < cantidad - 1; i++) {
        for (int j = 0; j < cantidad - i - 1; j++) {
            if (mesa->datos[j] > mesa->datos[j + 1]) {
                int tmp = mesa->datos[j];
                mesa->datos[j] = mesa->datos[j + 1];
                mesa->datos[j + 1] = tmp;
            }
        }
    }

    printf("\nEmplatado: ");
    for (int i = 0; i < cantidad; i++) {
        printf("%d ", mesa->datos[i]);
    }
    printf("\n");

    mesa->interacciones[3]++;
    sleep(20);

    semop(semSignal, &signal_op, 1);
    exit(0);
}
/// ------------------------------ FUNCIONES HIJOS ---------------------------------------- ///


int main()
{
    signal(SIGINT, manejar_salida_padre);
    signal(SIGTERM, manejar_salida_padre);  ///Asegura que si usamos CTRL + C en hijo o padre, se libere todo completamente

    instalar_handler_hijo();

    key_t key = ftok("/tmp", 65);       ///Genera una clave a partir de un archivo y un numero
    shmid = shmget(key, sizeof(Mesa), 0666 | IPC_CREAT);    ///CREA la region de memoria compartida

    if (shmid == -1)    ///Si no se crea la memoria compartida entonces salir
    {
        perror("shmget");
        exit(1);
    }

    mesa = (Mesa*)shmat(shmid, NULL, 0);    ///shmat mapea la memoria compartida al espacio de direcciones del proceso, devolviendo un puntero.

                        ///Desde este momento, podés usar mesa->datos, mesa->interacciones, etc
                        ///como si fueran variables normales, pero compartidas entre procesos


    if (mesa == (void*)-1) ///Si no se logra conectar la memoria compartida al espacio del proceso entonces SALIR
    {                   ///Ya que SHMAT devuelve (void*)-1 si es que falla
        perror("shmat");
        exit(1);
    }

    int datos_iniciales[N] = {30, 10, 60, 20, 8};       ///Son datos harcodeados de prueba para inicializar la variable compartida "mesa"

    memcpy(mesa->datos, datos_iniciales, sizeof(datos_iniciales));

    memset(mesa->interacciones, 0, sizeof(mesa->interacciones));

    mesa->finalizar = 0;
/// //////////////////////////////////////////////////////////////////////


    /// crear semáforos
    semA = semget(IPC_PRIVATE, 1, IPC_CREAT | 0666);    ///SEM GET crea semaforos en la memoria del sistema
    semB = semget(IPC_PRIVATE, 1, IPC_CREAT | 0666);    ///IPC_PRIVATE grupo privado de semaforos accesible para procesos emparentados
    semC = semget(IPC_PRIVATE, 1, IPC_CREAT | 0666);    ///1 semaforo
    semD = semget(IPC_PRIVATE, 1, IPC_CREAT | 0666);    ///IPC_CREAT lo crea si no existe o le da permisos RDWR para todos los usuarios

    if (semA == -1 ||
        semB == -1 ||
        semC == -1 ||
        semD == -1) {
        perror("semget");
        exit(1);            ///Si en la creacion de alguno de los 4 falla, entonces SALIR
    }

    semctl(semA, 0, SETVAL, 1); ///inicia el semaforo A luego B espera para iniciar Luego C Luego D
    semctl(semB, 0, SETVAL, 0); ///sem A -> sem B -> sem C -> sem D
    semctl(semC, 0, SETVAL, 0); ///se setea valor a un semaforo con SETVAL
    semctl(semD, 0, SETVAL, 0); ///semctl() es usado para configurar o consultar semáforos.


/// /////////////////////////////////////////

    int opc = 0;    ///Opcion elegida por el usuario
    int platos[3] = {0, 0, 0};

    int num;

    while (opc != 4)    ///Mientras el usuario no haya elegido la opcion 4 - SALIR
        {
            if (mesa->finalizar) break; ///Si se fuerza la finalizacion entonces SALIR

            printf("\n\tMenu:\n1- Pastel de Papas.\n2- Guiso de lentejas.\n3- Locro.\n4- Salir.\n");

            scanf("%d", &opc);

            if (opc > 0 && opc < 4)
        {
                int num = (opc == 1) ? 2 : (opc == 2) ? 5 : 8;

                pid_t cheff = fork();   ///Proceso hijo CHEFF

                if (cheff == 0) ///Se instala un handler por si muere abruptamente
                    {
                        instalar_handler_hijo();
                        prctl(PR_SET_PDEATHSIG, SIGTERM); /// asegura que si el padre muere, el hijo también lo haga
                        if (setpgid(0, 0) == -1)   ///pone al proceso en su propio grupo para poder matarlos en grupo si fuera necesario
                    {
                        perror("setpgid");
                        exit(1);
                    }

                        printf("\n[Cheff] PID: %d\n", getpid());    ///Imprime el PID del cheff

                        if (fork() == 0) cortarIngredientes(N, num, mesa, semA, semB);  ///Crea 4 procesos hijos para cada ETAPA
                        if (fork() == 0) cocinarIngredientes(N, num, mesa, semB, semC);
                        if (fork() == 0) picarIngredientes(N, num, mesa, semC, semD);
                        if (fork() == 0) emplatarIngredientes(N, mesa, semD, semA);

                        for (int i = 0; i < HIJOS; i++)     ///Espera a que cada hijo termine para poder continuar
                            wait(NULL);

                        exit(0);
                    }

            // En el padre
            if (setpgid(cheff, cheff) == -1)    ///Guarda al PID del cheff
                {
                    perror("setpgid padre");
                }
            cheffsActivos[cheffsCount++] = cheff;

            waitpid(cheff, NULL, 0);    ///Espera a que el proceso CHEFF termine

            memcpy(mesa->datos, datos_iniciales, sizeof(datos_iniciales));  ///Restaura datos iniciales para el siguiente plato

            platos[opc - 1]++;
        }
        else
            if (opc == 4)   ///Si se elige la opcion SALIR entonces se muestra el informe
            {
                printf("\n--- Informe final ---\n");

                for (int i = 0; i < 3; i++)
                {
                    printf("Se pidieron en total del plato %d: %d\n", i + 1, platos[i]);
                }

            printf("\nInteracciones por cocinero:\n");

            printf("Cortar: %d\n", mesa->interacciones[0]);
            printf("Picar: %d\n", mesa->interacciones[1]);
            printf("Cocinar: %d\n", mesa->interacciones[2]);
            printf("Emplatar: %d\n", mesa->interacciones[3]);

            liberarRecursos();      ///Finalmente se liberan los recursos
        }
        else        ///Si se ingreso una opcion por fuera del 1 al 4
            {
                printf("Opción inválida. Ingrese nuevamente...\n");

            }
    }

    return 0;
}
