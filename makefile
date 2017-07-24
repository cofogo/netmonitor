all:
	g++ src/*.cpp -I inc -o client
	gcc src/server.c -I inc -o server

server:
	gcc src/server.c -I inc -o server

client:
	g++ src/*.cpp -I inc -o client

clean:
	rm -v server
	rm -v client
