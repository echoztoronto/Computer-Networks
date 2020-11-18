TARGET = server client
DEPS = packet.h

all: $(TARGET)

server: server.c ${DEPS}
	gcc -w -o server server.c

client: client.c ${DEPS}
	gcc -w -o client client.c

clean:
	-rm -f $(TARGET)
