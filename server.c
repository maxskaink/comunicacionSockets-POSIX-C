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

#define BUFFER_SIZE 256
#define MAX_USERS 1000
pthread_mutex_t mutex;
/**
 * @brief Maneja el ingreso de signals
 */
void handle_sigint(int sig);

/**
 * @brief Cirrar los sockets y libera memoria
 */
void terminate();

/**
 * @brief Intenta crear una nueva conexion
 * si es eixotoso, crea un hilo para la comunicacion
 * @param client_socket is the socket to conect
 * @return -1 if is any error
 */
int createConexion(int client_socket);

/**
 * @brief maneja un hilo que escucha mensajes del usuario
 * si llega un mensaje llama a la funcion hiloEscrituraUsuarios
 * @param user_socket es el socket en el cual escucha 
 */
void *hiloLecturaUsuario(void *args);

/**
 * @brief Escribe un mensaje a todos los usuarios actuales en el arreglo
 * de myServer
 * @param HiloEscrituraArgs estructura con mesnaje y el socket que escribio el mensaje
 */
void *hiloEscrituraUsuarios(void *args);

/**
 * @brief Elimina un cliente de la estructura myServer
 * @param clientSocket es el socket del cliente a eliminar
 */
void eliminarCliente(int clientSocket);

/**
 * @brief Estructura que almacena los usuarios del servidor
 */
struct Server
{
    int numeroHilos;
    int *hilosUsuarios;
};

/**
 * @brief estructura de los argumentos a pasar a la funcion HiloEscrituraUsuarios
 */
struct HiloEscrituraArgs{
    int clientSocket;
    char message[256];
};

struct Server *myServer = NULL; //Definicion unica de la estructura Server
int s; // socket del servidor

int main(int argc, char *argv[]){
    
    if(argc < 2){
        perror("Ingrese el puerto del servidor");
        exit(EXIT_FAILURE);
    }
    //Asignamos los manejadores de seniales
    signal(SIGINT, handle_sigint);
    signal(SIGTERM, handle_sigint);

    //Asignamos memoria y damos valores iniciales a myServer
    myServer = malloc(sizeof(struct Server));
    myServer->hilosUsuarios = malloc(MAX_USERS*sizeof(int));
    myServer->numeroHilos = 0;
    pthread_mutex_init(&mutex, NULL);

    //Obtener un conector s = socket(...)
    s = socket(AF_INET, SOCK_STREAM, 0);

    //Asociar una direccion al socket (ip, puerto) bind
    int PORT = atoi(argv[1]);
    if(PORT ==0){
        perror("Ingrese un puerto valido");
        exit(EXIT_FAILURE);
    }
    
    struct sockaddr_in addr;    
    memset(&addr, 0, sizeof(struct sockaddr_in));//rellenamos la estructura de ceros
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT); // TODO colocarlo por linea de comndos
    addr.sin_addr.s_addr = INADDR_ANY; //0.0.0.0

    if (bind(s, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) == -1) {
        perror("No se pudo hacer el bind al socket");
        exit(EXIT_FAILURE);
    }    

    if (listen(s, MAX_USERS) == -1) {
        perror("Error al empezar a escuchar en el socket");
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
            perror("Error al intentar conectar un usuario");
            continue;
        }

        printf("Cliente [%d] conectado\n", client_socket);

        //Iniciamos el protocolo para conexion
        if(createConexion(client_socket) == -1){
            printf("Error con el protocolo inicial\n");
            continue;
        }
        //Agregamos el usuario a nuestra estructura de manera segura
        pthread_mutex_lock(&mutex);
        myServer->hilosUsuarios[myServer->numeroHilos] = client_socket;
        myServer->numeroHilos++;
        pthread_mutex_unlock(&mutex);
    }
    //Se cierra el socket y se llama al procedimiento para finalizar la app
    close(s);
    terminate();
}

void eliminarCliente(int clientSocket) {
    //Bloqueamos con mutex por ser seccion critica
    pthread_mutex_lock(&mutex);
    //Iteramos hasta encontrar el clientSocket a eliminar
    int index = -1;
    for (int i = 0; i < myServer->numeroHilos; i++) {
        if (myServer->hilosUsuarios[i] == clientSocket) {
            index = i;
            break;
        }
    }
    //retornamos si no se encunetra
    if (index == -1) {
        printf("Cliente no encontrado\n");
        pthread_mutex_unlock(&mutex);
        return;
    }
    //LLenamos el arreglo desde el index (saltamos el clietnSocket) hasta la longitud -1
    for (int i = index; i < myServer->numeroHilos - 1; i++) {
        myServer->hilosUsuarios[i] = myServer->hilosUsuarios[i + 1];
    }

    myServer->numeroHilos--;
    pthread_mutex_unlock(&mutex);
}

int createConexion(int client_socket){
    //Recibimos el saludo que es parte del protocolo
    if(receive_greeting(client_socket) == -1){
        return -1;
    }
    //Creamos un puntero al socketClient
    int *client_socket_ptr = malloc(sizeof(int));
    *client_socket_ptr = client_socket;

    //Comenzamos el hilo que escucha los mensajes del susuario
    pthread_t thread_id;
    if(pthread_create(&thread_id, NULL, hiloLecturaUsuario, (void *)client_socket_ptr) != 0){
        free(client_socket_ptr);
        return -1;
    }

    pthread_detach(thread_id);
    return 0;
}

void *hiloLecturaUsuario(void *args){
    //Recibimos el socket a escuchar
    int clientSocket = *(int *) args;

    //Buffer que recibe el mensaje entrante
    char buf[BUFFER_SIZE];

    while(1){
        //Leemos el mensaje 
        size_t bytes_read = read(clientSocket, buf, BUFFER_SIZE-1);
        //Validamos errores o desconexion
        if( bytes_read == -1)
            break;
        else if( bytes_read ==0){
            printf("Un cliente se ha desconectado\n\n");
            break;
        }
        buf[bytes_read] = '\0';

        //Cremaos valores necesarios para incializar el hiloDeEscritura
        pthread_t thread_id;
        struct HiloEscrituraArgs *hiloArgs = malloc(sizeof(struct HiloEscrituraArgs));
        hiloArgs->clientSocket = clientSocket;
        strncpy(hiloArgs->message, buf, sizeof(hiloArgs->message) - 1);
        hiloArgs->message[sizeof(hiloArgs->message) - 1] = '\0';

        //Creamos el hilo para notificar a los usuarios el mensaje
        pthread_create(&thread_id, NULL, hiloEscrituraUsuarios, (void *)hiloArgs);
        pthread_detach(thread_id);
    }
    //Si hay deconexion se elimina el usuario y se libera memoria
    eliminarCliente(clientSocket);
    close(clientSocket);
}

void *hiloEscrituraUsuarios(void *args){
    //extraemos la informacion de los argumentos
    struct HiloEscrituraArgs *hiloArgs = (struct HiloEscrituraArgs *)args;
    int remitent = hiloArgs->clientSocket;
    char *message = hiloArgs->message;

    char buf[BUFFER_SIZE];
    //Notificamos a los usuarios de manera segura
    pthread_mutex_lock(&mutex);
    for(size_t i = 0; i < myServer->numeroHilos; i++){
        int clientSocket = myServer->hilosUsuarios[i];
        //omitimos la notificacion al usuario que envio el mensaje
        if(clientSocket == remitent)
            continue; 
        //Modificamos la nofiticacion para saber quien envio el mensaje
        char messageToSend[BUFFER_SIZE];
        snprintf(messageToSend, BUFFER_SIZE, "User(%d) -> %s", remitent, message);
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
    printf("El servidor se esta cerrando... \n");
    //Cerramos las conexiones con todos los usuarios
    for(size_t i = 0; i < myServer->numeroHilos; i++)
        close(myServer->hilosUsuarios[i]);

    free(myServer->hilosUsuarios);
    free(myServer);    
    pthread_mutex_destroy(&mutex);
    close(s);
    exit(EXIT_SUCCESS);
}

