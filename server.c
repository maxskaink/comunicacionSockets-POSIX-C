/**
 * @file
 * @brief Programa servidor
 * @author Miguel Angel Calambas Vivas <mangelcvivas@unicauca.edu.co>
 * @copyright MIT License
 */



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <signal.h>
#include <pthread.h>

#include "protocol.h"

pthread_mutex_t mutex;
/**
 * @brief Va a manejar el ingreso de signals
 */
void handle_sigint(int sig);

/**
 * @brief Va a cerrar las conexione existentes en el programa
 */
void terminate();

/**
 * @brief Intenta crear una nueva conexion
 * y si es eixotoso, crea un hilo para la comunicacion\
 * @param client_socket is the socket to conect
 * @return -1 if is any error
 */
int createConexion(int client_socket);

/**
 * @brief es el hilo que va a manejar cada conexion
 * @param user_socket es el socket con el que se va a comnicar
 */
void *hiloLecturaUsuario(void *args);

/**
 * @brief escribe el ultimo mensaje a todos los usuarios conectados en el servidor
 * @param message El mensaje que sera transmitido a todos lo usuarios activos
 */
void *hiloEscrituraUsuarios(void *args);


/**
 * @brief Elimina un cliente de la estructura myServer
 * @param clientSocket es el socket del cliente a eliminar
 */
void eliminarCliente(int clientSocket);


struct Server
{
    int numeroHilos;
    int *hilosUsuarios;
};

struct HiloEscrituraArgs{
    int clientSocket;
    char message[256];
};

struct Server *myServer = NULL;
int s; // socket del servidor

int main(int argc, char *argv[]){
    
    if(argc < 2){
        printf("Ingrese el puerto del servidor\n");
        exit(EXIT_FAILURE);
    }
    //Asignamos los manejadores de seniales
    signal(SIGINT, handle_sigint);
    signal(SIGTERM, handle_sigint);

    //Obtener un conector s = socket(...)

    myServer = malloc(sizeof(struct Server));
    myServer->hilosUsuarios = malloc(100*sizeof(int));
    myServer->numeroHilos = 0;
    pthread_mutex_init(&mutex, NULL);

    struct sockaddr_in addr;

    s = socket(AF_INET, SOCK_STREAM, 0);

    //Asociar una direccion al socket (ip, puerto) bind
    int PORT = atoi(argv[1]);

    if(PORT ==0){
        printf("Ingrese un puerto valido\n");
        exit(EXIT_FAILURE);
    }

    memset(&addr, 0, sizeof(struct sockaddr_in));//rellenamos la estructura de ceros
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT); // TODO colocarlo por linea de comndos
    addr.sin_addr.s_addr = INADDR_ANY; //0.0.0.0

    if (bind(s, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) == -1) {
        perror("bind");
        exit(EXIT_FAILURE);
    }    


    if (listen(s, 100) == -1) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    
    //Poner el socket disponible (escuchar)
    printf("Servidor escuchando\n");
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        //Se bloquea esperando conexion de algun usuario
        int client_socket = accept(s, (struct sockaddr *)&client_addr, &client_len);
        if (client_socket == -1) {
            perror("accept");
            continue;
        }

        printf("Cliente [%d] conectado\n", client_socket);

        if(createConexion(client_socket) == -1){
            printf("Error al intentar conetarse\n");
            continue;
        }
        pthread_mutex_lock(&mutex);
        myServer->hilosUsuarios[myServer->numeroHilos] = client_socket;
        myServer->numeroHilos++;
        pthread_mutex_unlock(&mutex);
    }
    close(s);
    terminate();
}

void eliminarCliente(int clientSocket) {
    pthread_mutex_lock(&mutex);
    int index = -1;
    for (int i = 0; i < myServer->numeroHilos; i++) {
        if (myServer->hilosUsuarios[i] == clientSocket) {
            index = i;
            break;
        }
    }

    if (index == -1) {
        printf("Cliente no encontrado\n");
        pthread_mutex_unlock(&mutex);
        return;
    }

    for (int i = index; i < myServer->numeroHilos - 1; i++) {
        myServer->hilosUsuarios[i] = myServer->hilosUsuarios[i + 1];
    }

    myServer->numeroHilos--;
    pthread_mutex_unlock(&mutex);
}

int createConexion(int client_socket){
    if(receive_greeting(client_socket) == -1){
        return -1;
    }
    int *client_socket_ptr = malloc(sizeof(int));
    *client_socket_ptr = client_socket;

    pthread_t thread_id;
    if(pthread_create(&thread_id, NULL, hiloLecturaUsuario, (void *)client_socket_ptr) != 0){
        free(client_socket_ptr);
        return -1;
    }

    pthread_detach(thread_id);
    return 0;
}
void *hiloLecturaUsuario(void *args){
    int clientSocket = *(int *) args;

    size_t bufferSize = 256;
    char buf[256];

    while(1){
        size_t bytes_read = read(clientSocket, buf, bufferSize-1);
        if( bytes_read == -1)
            break;
        else if( bytes_read ==0){
            printf("Un cliente se ha desconectado\n\n");
            break;
        }
        buf[bytes_read] = '\0';
        pthread_t thread_id;

        struct HiloEscrituraArgs *hiloArgs = malloc(sizeof(struct HiloEscrituraArgs));
        hiloArgs->clientSocket = clientSocket;
        strncpy(hiloArgs->message, buf, sizeof(hiloArgs->message) - 1);
        hiloArgs->message[sizeof(hiloArgs->message) - 1] = '\0';

        pthread_create(&thread_id, NULL, hiloEscrituraUsuarios, (void *)hiloArgs);
        pthread_detach(thread_id);
    }
    eliminarCliente(clientSocket);
    close(clientSocket);
}

void *hiloEscrituraUsuarios(void *args){
    struct HiloEscrituraArgs *hiloArgs = (struct HiloEscrituraArgs *)args;
    int remitent = hiloArgs->clientSocket;
    char *message = hiloArgs->message;

    size_t bufferSize = 256;
    char buf[bufferSize];
    pthread_mutex_lock(&mutex);
    for(size_t i = 0; i < myServer->numeroHilos; i++){
        int clientSocket = myServer->hilosUsuarios[i];
        if(clientSocket == remitent)
            continue; 
        char messageToSend[256];
        snprintf(messageToSend, 256, "User(%d) -> %s", remitent, message);
        if( write(clientSocket, messageToSend, strlen(messageToSend)) == -1){
            printf("No se pudo enviar el mensaje al usuario (%d)", clientSocket);
        }
    }
    pthread_mutex_unlock(&mutex);
}


void handle_sigint(int sig){
    printf("Se ha solicitado terminar el servidor\n");
    terminate();
}

void terminate(){
    printf("Proceso terminado \n");

    for(size_t i = 0; i < myServer->numeroHilos; i++)
        close(myServer->hilosUsuarios[i]);

    free(myServer->hilosUsuarios);
    free(myServer);    
    pthread_mutex_destroy(&mutex);
    close(s);
    exit(EXIT_SUCCESS);
}

