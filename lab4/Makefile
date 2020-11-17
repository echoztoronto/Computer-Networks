TARGET = client
DEPS = packet.h

all: $(TARGET)


client: client.c ${DEPS}
	gcc -o client client.c

clean:
	-rm -f $(TARGET)