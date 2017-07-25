all:
	g++ src/server/server.cpp -o server `freetype-config --cflags` -Iinc -Llib -lpng -lpngwriter -lz -lfreetype
	g++ src/client/*.cpp -o client -Iinc

server:
	g++ src/server/server.cpp -o server `freetype-config --cflags` -Iinc -Llib -lpng -lpngwriter -lz -lfreetype

client:
	g++ src/client/*.cpp -o client -Iinc

pngtest:
	g++ src/pngwriter_tst.cpp -o pngtest `freetype-config --cflags` -Iinc -Llib -lpng -lpngwriter -lz -lfreetype

clean:
	rm -v server
	rm -v client
