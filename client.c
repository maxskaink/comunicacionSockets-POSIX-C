/**
 * @file
 * @brief Programa cliente
 * @author Miguel Angel Calambas Vivas <mangelcvivas@unicauca.edu.co>
 * @copyright MIT License
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "protocol.h"

#define BUFFER_SIZE 256

/**
 * @brief Va a manejar el cerrar la aplicacion de manera adeucada
 * @param sig la senial asociada
 */
void terminate(int sig);

int client_socket;

int main(int argc, char *argv[]) {
    signal(SIGINT, terminate);
    signal(SIGTERM, terminate);
    //Validamos que tenga todos loa argumentos necesarios
    if (argc < 3) {
        printf("Uso: %s <IP del servidor> <Puerto>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    //Extraemos la ip y puerto
    const char *server_ip = argv[1];
    int server_port = atoi(argv[2]);

    //Creamos el socket cn la configuracion establecida
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        perror("Error al crear elsocket");
        exit(EXIT_FAILURE);
    }

    //Creamos las estructuras para crear la conexion
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);

    //Convertimos la ip en el formato necesario
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        perror("direccion ip invalida");
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    //Creamos la conexion con el servidor
    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Error al intentar conectarse con el servidor");
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    printf("Conectado al servidor %s en el puerto %d\n", server_ip, server_port);

    //Hacemos el saludo siguiendo el protocolo
    send_greeting(client_socket);

    char message[BUFFER_SIZE];
    int cont = 0;
    while(1){
        snprintf(message, BUFFER_SIZE, "mensaje %d", cont);
        if(write(client_socket, message, strlen(message)) == -1)
            break;
        sleep(1);
        cont++;
    }

    close(client_socket);
    return 0;
}

void terminate(int sig){
    printf("El cliente se ha cerrado\n");
    close(client_socket);
}