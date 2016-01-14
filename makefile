all:
	gcc udp_msg.c -lpthread -o udp_msg -g -std=c99

clean:
	rm udp_msg
