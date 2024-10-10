// /**
//  * @file
//  * @brief Programa cliente
//  * @author Miguel Angel Calambas Vivas <mangelcvivas@unicauca.edu.co>
//  * @copyright MIT License
//  */
// #include <stdio.h>
// #include <stdlib.h>
// #include <unistd.h>
// // Instalar los manejadores  sigint, sigterm - HARLO
// //Obtener un conector s=socket(...)
// //Conectarse a una direccion(IP, puerto) connect 
// //Envia un send_greeting(s,)
// //receive_greeting(s, )
// //Comunicacion entre el servidor (bucle)
// //cerrar el socket cuando el usuario lo desee

// int main(int argc, char *argv[]){
//     exit(EXIT_SUCCESS);
// }


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 256

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Uso: %s <IP del servidor> <Puerto>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *server_ip = argv[1];
    int server_port = atoi(argv[2]);

    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);

    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        perror("inet_pton");
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("connect");
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    printf("Conectado al servidor %s en el puerto %d\n", server_ip, server_port);

    char buffer[BUFFER_SIZE];
    snprintf(buffer, BUFFER_SIZE, "Hola, servidor!");

    if (write(client_socket, buffer, strlen(buffer)) == -1) {
        perror("write");
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    ssize_t bytes_read = read(client_socket, buffer, BUFFER_SIZE - 1);
    if (bytes_read == -1) {
        perror("read");
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    char message[BUFFER_SIZE];
    int cont = 0;
    while(1){
        snprintf(message, BUFFER_SIZE, "mensaje %d", cont);
        if(write(client_socket, message, strlen(message)) == -1)
            break;
        sleep(1);
        cont++;
    }

    buffer[bytes_read] = '\0';
    printf("Respuesta del servidor: %s\n", buffer);

    close(client_socket);
    return 0;
}