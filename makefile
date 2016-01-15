all: tunnel

tunnel: tunnel.cpp tap.cpp tap.h
	echo $<
	g++ $< -o tunnel -lpthread

clean:
	rm -f tunnel
	rm -f *.o
