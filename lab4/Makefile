TARGET = server client
DEPS = packet.h

all: $(TARGET)

server: server.c ${DEPS}
	gcc -w -lm -o server server.c

client: client.c ${DEPS}
	gcc -w -pthread -o client client.c

clean:
	-rm -f $(TARGET)
