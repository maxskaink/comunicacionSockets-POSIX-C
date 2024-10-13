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
#include <pthread.h>

#include "protocol.h"

#define BUFFER_SIZE 256

/**
 * @brief Va a manejar el cerrar la aplicacion de manera adeucada
 * @param sig la senial asociada
 */
void terminate(int sig);

/**
 * @brief Va a leer por consola cadenas para enviarlas por el scoket
 * @param clientSocker socket para enviar mensajes
 */
void *hiloLecturaEntrada(void *args);

/**
 * @brief Va a esperar que el servidor envie mesnajes para mostrarlos por consola
 * @param clientSocket socket para esucchar mensajes
 */
void *hiloEscritura(void *args);
int client_socket;
pthread_t thread_id_escritura;
pthread_t thread_id_lectura;

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
    int *clientSocketPtr = &client_socket;
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

    pthread_create(&thread_id_escritura, NULL, hiloEscritura, (void *)clientSocketPtr);
    pthread_create(&thread_id_lectura, NULL, hiloLecturaEntrada, (void *)clientSocketPtr);

    pthread_join(thread_id_escritura, NULL);
    pthread_join(thread_id_lectura, NULL);


    close(client_socket);
    return 0;
}

void *hiloLecturaEntrada(void *args){
    int clientSocket = *(int * )args;

    char buff[BUFFER_SIZE];

    while(1){
        if( fgets(buff, BUFFER_SIZE, stdin) != NULL ){
            size_t len = strlen(buff);
            if (len > 0 && buff[len - 1] == '\n') {
                buff[len - 1] = '\0';
            }

            if (write(clientSocket, buff, strlen(buff)) == -1) {
                perror("Error sendin message");
                break;
            }
        }
    }
    close(client_socket);
    return NULL;
}

/**
 * @brief Va a esperar que el servidor envie mesnajes para mostrarlos por consola
 * @param clientSocket socket para esucchar mensajes
 */
void *hiloEscritura(void *args){
    int clientSocket = *(int *)args;

    char buff[BUFFER_SIZE];
    while(1){
        size_t bytes_read = read(client_socket, buff, BUFFER_SIZE-1);
        if(bytes_read == -1){
            perror("error reading message");
            break;
        }else if(bytes_read == 0){
            printf("El servidor se ha desconectado\n");
            terminate(1);
            break;
        }
        buff[bytes_read] = '\0';
        printf("%s\n", buff);
    }
    close(client_socket);
}

void terminate(int sig){
    printf("El cliente se ha cerrado\n");
    pthread_cancel(thread_id_escritura);
    pthread_cancel(thread_id_lectura);
    close(client_socket);
}