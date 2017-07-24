all:
	g++ src/server/server.c -I inc -o server
	g++ src/client/*.cpp -I inc -o client

server:
	g++ src/server/server.cpp -I inc -o server

client:
	g++ src/client/*.cpp -I inc -o client

pngtest:
	g++ src/pngwriter_tst.cpp -o pngtest `freetype-config --cflags` -Iinc -Llib -lpng -lpngwriter -lz -lfreetype

clean:
	rm -v server
	rm -v client
