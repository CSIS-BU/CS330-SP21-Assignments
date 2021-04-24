default: c

all: c

c: server-c client-c router-c

client-c: client.c
	gcc client.c -o client-c

server-c: server.c
	gcc server.c -o server-c

router-c: router.c
	gcc router.c -o router-c

clean:
	rm -f router-c server-c client-c
