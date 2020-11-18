TARGET = server client
DEPS = packet.h

all: $(TARGET)

server: server.c ${DEPS}
	gcc -lm -o server server.c

client: client.c ${DEPS}
	gcc -pthread -o client client.c

clean:
	-rm -f $(TARGET)