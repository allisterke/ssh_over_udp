all: tunnel

tunnel: 
	g++ tunnel.cpp tap.cpp tap.h -o tunnel -pthread -std=c++11

clean:
	rm -f tunnel
	rm -f *.o
