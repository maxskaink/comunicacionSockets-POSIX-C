/**
 * @file
 * @brief Protocolo de comunicacion
 * @author Miguel Angel Calambas Vivas <mangelcvivas@unicauca.edu.co>
 * @copyright MIT License
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "protocol.h"

int send_greeting(int s){
    char* message = "Hola!!";
    size_t len = strlen(message);

    if(write(s, message, len)==-1)
        return -1;

    char buf[len+1];

    if( read(s, buf, len) == -1)
        return -1;

    return 1;

}

int receive_greeting(int s){
    size_t bufferSize = 256;
    char buf[256];

    size_t bytes_read = read(s, buf, bufferSize-1);
    if( bytes_read == -1)
        return -1;

    buf[bytes_read] = '\0';

    if(write(s, buf, bytes_read) == -1)
        return -1;

    return 1;

    
}
