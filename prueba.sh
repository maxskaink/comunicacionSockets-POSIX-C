#!/bin/bash


# Ejecutar el cliente 20 veces
for i in {1..20}
do
    echo "Ejecutando cliente $i"
    ./client 127.0.0.1 8080
done

# Esperar a que todos los procesos del cliente terminen
wait
echo "Todos los clientes han terminado"