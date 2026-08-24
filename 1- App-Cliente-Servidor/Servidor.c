// servidor.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <ctype.h>
#include <signal.h>

#define PUERTO 5000
#define MAX_CLIENTES 4

int clientes_activos = 0;
int servidor_activo = 1;
int servidor_fd;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

void cortarIngredientes(int ingredientes[], int cantidadIngredientes, int num);
void picarIngredientes(int ingredientes[], int cantidadIngredientes, int num);
void cocinarIngredientes(int ingredientes[], int cantidadIngredientes, int num);
void emplatarIngredientes(int ingredientes[], int cantidadIngredientes);



/// ///////////////////// Manejador de señal Ctrl+C
void manejar_senal(int senal)
{
    if (senal == SIGINT) {
        printf("\n[Servidor] Señal de interrupción recibida. Cerrando servidor...\n");
        servidor_activo = 0;
        shutdown(servidor_fd, SHUT_RDWR);
        close(servidor_fd);
    }
}
/// /////////////////////

void procesar_mensaje(char* buffer, char* respuesta) {
    char arg1[64], arg2[64];
    int datosIniciales[] = {30, 10, 60, 20, 8};
    int num, numReceta;

    if (strncmp(buffer, "RECETAS:", 8) == 0)
        sprintf(respuesta, "Recetas:\n1- Pastel de Papas.\n2- Guiso de lentejas.\n3- Locro.\n");
    else if (strncmp(buffer, "CHEFF:", 6) == 0) {
        numReceta = atoi(buffer + 6);
        if (numReceta == 1) num = 2;
        else if (numReceta == 2) num = 5;
        else if (numReceta == 3) num = 8;
        else {
            sprintf(respuesta, "No se conoce ese plato.");
            return;
        }
        cortarIngredientes(datosIniciales, 5, num);
        cocinarIngredientes(datosIniciales, 5, num);
        picarIngredientes(datosIniciales, 5, num);
        emplatarIngredientes(datosIniciales, 5);
        sprintf(respuesta, "Plato final:");
        for(int i = 0; i < 5; i++) {
            sprintf(arg1, " %d", datosIniciales[i]);
            strcat(respuesta, arg1);
        }
    }
    else if (strncmp(buffer, "CORTAR:", 7) == 0) {
        sscanf(buffer + 7, "%s", arg1);
        sprintf(respuesta, "Resultado: %s cortada\n", arg1);
    } else if (strncmp(buffer, "COCINAR:", 8) == 0) {
        sscanf(buffer + 8, "%s", arg1);
        sprintf(respuesta, "Resultado: %s cocinada\n", arg1);
    } else if (strncmp(buffer, "EMPLATAR:", 9) == 0) {
        sscanf(buffer + 9, "%s", arg1);
        sprintf(respuesta, "Resultado: %s emplatada\n", arg1);
    } else if (strncmp(buffer, "SUMA:", 5) == 0) {
        sscanf(buffer + 5, "%[^:]:%s", arg1, arg2);
        int a = atoi(arg1), b = atoi(arg2);
        sprintf(respuesta, "Resultado: %d\n", a + b);
    } else if (strncmp(buffer, "RESTA:", 6) == 0) {
        sscanf(buffer + 6, "%[^:]:%s", arg1, arg2);
        int a = atoi(arg1), b = atoi(arg2);
        sprintf(respuesta, "Resultado: %d\n", a - b);
    } else if (strncmp(buffer, "INVERSO:", 8) == 0) {
        sscanf(buffer + 8, "%s", arg1);
        int len = strlen(arg1);
        for (int i = 0; i < len; i++)
            respuesta[i] = arg1[len - 1 - i];
        respuesta[len] = '\0';
        strcat(respuesta, "\n");
    } else if (strncmp(buffer, "MAYUS:", 6) == 0) {
        sscanf(buffer + 6, "%s", arg1);
        for (int i = 0; arg1[i]; i++)
            respuesta[i] = toupper(arg1[i]);
        respuesta[strlen(arg1)] = '\0';
        strcat(respuesta, "\n");
    } else {
        strcpy(respuesta, "Comando desconocido.\n");
    }
}

void* manejar_cliente(void* arg) {
    int cliente_fd = *(int*)arg;    ///Este es el socket del cliente recibido como VOID, pasa de void a int para recibir el DESCRIPTOR
    free(arg);              ///Free del argumento ya que no lo necesitamos, este ARG fue creado en el MAIN
                            ///Normal, ante un malloc se hace un free

    char buffer[1024], respuesta[1024]; ///Dos BUFFER uno para recibir msj del cliente y otro para RESPONDERLE msj al cliente
    int leido;

    while ((leido = read(cliente_fd, buffer, sizeof(buffer)-1)) > 0)    ///Lee el msj del socket y lo escribe en BUFFER
        {                                     ///Este bucle se va a repetir mientras el cliente siga enviando msj
            buffer[leido] = '\0';       ///Aseguramiento de cadenas

            if (strncmp(buffer, "SALIR", 5) == 0) break;    ///Si el cliente escribe SALIR se termina el bucle y SE CIERRA EL SOCKET

            printf("[Servidor] Recibido: %s", buffer);  ///Mensaje de control para el servidor

            memset(respuesta, 0, sizeof(respuesta));    ///LIMPIEZA DE BUFFER antes de llenarlo con una nueva respuesta
                                                    ///Esto sirve para las siguientes iteraciones

            procesar_mensaje(buffer, respuesta);    ///Analiza la respuesta de la cadena recibida por el CLIENTE y la procesa

            write(cliente_fd, respuesta, strlen(respuesta));    ///Envia la respuesta procesada al cliente a traves de su socket
        }
/// ///////////////
    close(cliente_fd);      ///En caso de que se rompa el bucle con SALIR, se cierra correctamente el socket del cliente en si
                    ///Esto afecta unicamente al socket del cliente que se guardo el HILO al ser creado y al haber ejecutado esta funcion
                    ///No afecta al main


    printf("[Servidor] Cliente desconectado.\n");   ///MENSAJE opcional para el servidor



    pthread_mutex_lock(&lock); ///Entra en zona crítica: acceso exclusivo al contador global clientes_activos.

    clientes_activos--;     ///Se resta la cantida de clientes activos ya que estamos cerrando uno



        if (clientes_activos == 0 && servidor_activo)   ///Si no hay clientes activos -> el servidor cierra
        {
            printf("[Servidor] Todos los clientes se desconectaron. Cerrando servidor...\n");

            servidor_activo = 0;    ///Pone en 0 para cambiar la condicion de BUCLE DEL MAIN
            shutdown(servidor_fd, SHUT_RDWR); ///interrumpe nuevas lecturas y escrituras en el socket del servidor.

            close(servidor_fd);     ///Cierra completamente el SOCKET DEL SERVIDOR
        }

    pthread_mutex_unlock(&lock);

    return NULL;
}






int main()
{
    struct sockaddr_in servidor_addr, cliente_addr; ///sockaddr_in es una estructura de IPV4 ip + puerto
    socklen_t cliente_len = sizeof(cliente_addr); ///tamaño de la estructura del cliente


    signal(SIGINT, manejar_senal);  //Dispara señal SIGINT Ctrl+C = apagado seguro
                    ///Al recibir SIGINT, en lugar de cerrar automaticamente, ejecutar la funcion enviada por parametro

    servidor_fd = socket(AF_INET, SOCK_STREAM, 0); /// AF_INET es ipv4 SOCK_STREAM protocolo stream 0 es protocolo por defecto TCP
                                                    ///SERVIDOR_FD guarda toda la informacion del socket
    ///servidor_fd ES INT
    servidor_addr.sin_family = AF_INET; ///Familia AF_INET o sea ipv4
    servidor_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); ///Define la IP para el servidor
    servidor_addr.sin_port = htons(PUERTO);         ///Define el puerto para el servidor


    bind(servidor_fd, (struct sockaddr*)&servidor_addr, sizeof(servidor_addr));
                                ///Asocia el socket del servidor con la dirección IP y puerto definidos.


    listen(servidor_fd, MAX_CLIENTES); ///Pone al socket en modo de escucha pasiva
                                       ///Con MAX_CLIENTES como conexiones maximas en cola esperando a ser aceptadas.

    printf("[Servidor] Escuchando en puerto %d...\n", PUERTO);


    /// ////////////

    while (servidor_activo) ///Se pone en 0 si recibe un CTRL + C o kill a algun proceso
        {
        int nuevo_fd = accept(servidor_fd, (struct sockaddr*)&cliente_addr, &cliente_len);
                                        ///Se queda en bucle hasta que recibe una conexion del cliente con su IP y su PUERTO
                                        ///Luego de esto, se crea un nuevo socket NUEVO_FD para comunicarse por esa via con el cliente
        if (nuevo_fd == -1)
            {
                if (!servidor_activo) break;    ///Si el servidor se esta apagando, salir
                perror("accept");
                continue;                       ///Intenta aceptar de nuevo en caso de error
            }

        pthread_mutex_lock(&lock);
        if (clientes_activos >= MAX_CLIENTES)       ///Si supera el max clientes, entonces cerramos esa conexion
            {
                pthread_mutex_unlock(&lock);
                printf("[Servidor] Limite alcanzado. Cliente rechazado.\n");
                write(nuevo_fd, "Capacidad maxima alcanzada", 26); ///26 letras
                close(nuevo_fd);
                continue;
            }


        clientes_activos++;             ///Si no supera el max, lo aceptamos e incrementamos
        pthread_mutex_unlock(&lock);

    /// ////////////

        int* nuevo_socket = malloc(sizeof(int));
        *nuevo_socket = nuevo_fd;       ///Guarda el descriptor del socket en el puntero
                                        ///La idea era usar este puntero para que el hilo se comunique con el cliente

        write(nuevo_fd, "Servidor pendiente de mensaje", 29);   ///Le manda msj al cliente al iniciar la conexion (era opcional esto)

        pthread_t hilo;     ///Define un nuevo hilo que voy a usar para manejar POR CADA cliente
        pthread_create(&hilo, NULL, manejar_cliente, nuevo_socket); ///crea el hilo que va a ejecutar "manejar_cliente"
        pthread_detach(hilo);   ///Cuando el hilo termina, muere y el sistema libera sus recursos, es NO BLOQUEANTE
    }

    close(servidor_fd);     ///Aca cierra el socket del servidor, hace un close del file descriptor del servidor_fd
                            ///Se ejecutará si recibe un CTRL + C o si le tiramos un KILL a un proceso hijo y este recibe la SIGINT
    printf("[Servidor] Finalizado correctamente.\n");
    return 0;
}

// funciones auxiliares

void cortarIngredientes(int ingredientes[], int cantidadIngredientes, int num) {
    for (int i = 0; i < cantidadIngredientes; i++)
        ingredientes[i] /= num;
}

void picarIngredientes(int ingredientes[], int cantidadIngredientes, int num) {
    for (int i = 0; i < cantidadIngredientes; i++)
        ingredientes[i] *= num;
}

void cocinarIngredientes(int ingredientes[], int cantidadIngredientes, int num) {
    for (int i = 0; i < cantidadIngredientes; i++)
        ingredientes[i] += num;
}

void emplatarIngredientes(int ingredientes[], int cantidadIngredientes) {
    for (int i = 0; i < cantidadIngredientes - 1; i++)
        for (int j = 0; j < cantidadIngredientes - i - 1; j++)
            if (ingredientes[j] > ingredientes[j + 1]) {
                int tmp = ingredientes[j];
                ingredientes[j] = ingredientes[j + 1];
                ingredientes[j + 1] = tmp;
            }
}
