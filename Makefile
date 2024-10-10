all: client.o server.o protocol.o
	gcc -o server server.o protocol.o
	gcc -o client client.o protocol.o

# Regla genérica para compilar .c a .o
%.o:%.c
	gcc -c $< -o $@

clean:
	rm -rf *.o client server docs
doc:
	doxygen