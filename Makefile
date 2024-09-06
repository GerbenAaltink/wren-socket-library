CC = gcc 
CCFLAGS = -lm -Wall -Wextra

server:
	$(CC) rwren.c -o ./build/rwren $(CCFLAGS) 
	./build/rwren server1.wren

client_debug:
# using original wren version to debug since it debugs better than my version
	wren wren client1.wren

client:
	$(CC) rwren.c -o ./build/rwren $(CCFLAGS)
	./build/rwren client1.wren
