TARGET = server deliver

all: $(TARGET)

server: server.c
	gcc -o server server.c

deliver: deliver.c
	gcc -lm -o deliver deliver.c

clean:
	-rm -f $(TARGET)