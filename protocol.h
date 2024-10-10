/**
 * @file
 * @brief Interfaz del protocolo de ocmunicacion
 * @author Miguel Angel Calambas Vivas <mangelcvivas@unicauca.edu.co>
 * @copyright MIT License
 */

/**
 * @brief Envia un mensaje de saludo
 * @param s Socket al que se envia el mensaje
 */
int send_greeting(int s);
/**
 * @brief Recibe un mensaje de saludo
 * @param s Socket del que se recibe el mensaje
 */
int receive_greeting(int s);
