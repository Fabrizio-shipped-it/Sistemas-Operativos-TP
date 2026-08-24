// cliente.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>



int main(int argc, char *argv[])
{
    int sock;       ///DESCRIPTOR del socket
    struct sockaddr_in servidor;    ///estructura que guarda IP y puerto del servidor al que se quiere conectar
    char mensaje[1024], respuesta[1024];    ///Define un BUFFER para enviar msj y otro para RECIBIR respuestas
    char ip[16];
    int puerto=0;

    /// Verificar que el usuario proporcione al menos la ip y opcional puerto
    if (argc < 2)
        {
            fprintf(stderr, "Uso: %s <IP> [PUERTO]\n", argv[0]);
            exit(1);
        }

    snprintf(ip, sizeof(ip), "%s", argv[1]);    ///Se copia la IP DESDE los argumentos recibidos HACIA la variable ip[16]

    if (argc == 3)  ///Si llego otro argumento entonces lo colocamos en la VARIABLE puerto
    {
        puerto = atoi(argv[2]);
    }

    sock = socket(AF_INET, SOCK_STREAM, 0); ///Genera el descriptor de archivos si esta todo correcto
    if (sock < 0) ///Si no funciona entonces salimos
        {
            perror("socket");

            return 1;
        }

    servidor.sin_family = AF_INET;  ///Configura la direccion del servidor al que se va a conectar, indicando que es una direccion IPV4
    servidor.sin_port = htons(puerto);  ///con un puerto 5000 enviado por consola por el cliente
                                        ///HTONS convierte la variable INT puerto a un FORMATO DE RED}

    if (inet_pton(AF_INET, ip, &servidor.sin_addr) <= 0)    ///Convierte la IP (string) a una estructura binaria
        {
            fprintf(stderr, "Valor de IP invalida\n");  ///Y si no funciona, entonces muestra un error por pantalla

            close(sock);
            return 1;
        }

    if (connect(sock, (struct sockaddr*)&servidor, sizeof(servidor)) < 0)
        {                                       ///Se intenta conectar con el socket a la direccion del servidor
            perror("connect");
                                                ///Si esto falla entonces cierra el socket
            close(sock);
            return 1;
        }

    int leido = read(sock, respuesta, sizeof(respuesta)-1); ///Aca espera primero a recibir un msj que envia el servidor
                                        ///Es el msj inicial que nos va a enviar el servidor
                                        ///Puede decir que rechazo la conexion o enviarnos las opciones disponibles

    if (leido > 0) ///SI HUBO RESPUESTA
        {
            respuesta[leido] = '\0';        ///Si el cliente no recibe nada que no sea lo esperado entonces muestra el msj
                if (!strncmp(respuesta, "Servidor pendiente de mensaje", 29))
                {
                    printf("[Cliente] Conectado al servidor.\n");   ///Diciendo que ya esta conectado y que ahora puede proceder
                }
                else
                    {
                        printf("[Servidor] %s\n", respuesta);   ///Si no coincide con lo esperado
                        close(sock);                            ///Depende de la respuesta que haya dado el servidor y cerramos el socket
                        return -1;
                    }
        }
        else        ///Aca es por si se cierra abruptamente el servidor entonces, desconecta al cliente
            {
                printf("[Cliente] No se pudo leer confirmación del servidor.\n");
                close(sock);
                return -1;
            }





/// /////////////////////////////////////////////////
    while (1)   ///Proceso comunicacion cliente - servidor
    {
            printf("> ");

                if (fgets(mensaje, sizeof(mensaje), stdin) == NULL) break;  ///Lee el msj escrito por consola por el cliente

                ///
// esta linea podria meterla dentro del if


            if (strncmp(mensaje, "SALIR", 5) == 0)  ///Si el cliente escribio SALIR entonces le enviamos SALIR al servidor
            {
                mensaje[strcspn(mensaje, "\n")] = 0;   ///Fgets pone un BARRA n al terminar de leer, con esa linea se saca caracter extra
                write(sock, "SALIR", 5);            ///El servidor esta preparado para interpretar este mensaje
                break;                              ///Luego rompemos el bucle
            }

        ///strcat(mensaje, "\n");
// y esta linea borrarla


        write(sock, mensaje, strlen(mensaje));  ///Le envia al servidor el msj escrito por el cliente


        leido = read(sock, respuesta, sizeof(respuesta)-1); ///Espera la respuesta del servidor para ser interpretada

        if (leido > 0)  ///SI LEE ALGO -> termina con  \ 0 para convertir a string
            {
                respuesta[leido] = '\0';
                printf("[Servidor] %s\n", respuesta);   ///Muestra por pantalla la respuesta procesada por el servidor
            }
        else    ///Si no leyo nada es porque el servidor cerro la conexion
            {
                printf("[Cliente] El servidor cerró la conexión. Saliendo...\n");
                break;  ///Entonces el cliente procede a cerrar su socket tambien
            }
    }

    close(sock);
    printf("[Cliente] Desconectado.\n");

    return 0;
}

