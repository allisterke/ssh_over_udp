#include <pthread.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <thread>
#include <list>
#include <vector>
#include <iostream>

#include "tap.h"

#define PORT 11555
#define BUFLEN 1300
#define MTU 1200

using namespace std;

struct sockaddr_in si_me, si_other, si_tmp;
int sock;
socklen_t addr_len = sizeof(struct sockaddr);

int connected = false;
int first = true;

void transfer_from_connection_to_tunnel(int fd) {
	vector<char> buf(BUFLEN);
	while(true) {
		int len = recvfrom(sock, buf.data(), buf.size(), 0, (struct sockaddr*)&si_tmp, &addr_len);
		if(len >= 0) {
			if(first) {
				connected = true;
				first = false;
				si_other = si_tmp;
				printf("connected from %s:%d\n", 
					inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port));
				// keep connection
				sendto(sock, "", 0, 0, (struct sockaddr*)&si_other, sizeof(struct sockaddr));
			}
			else if(len > 0) {
				if(memcmp(&si_tmp, &si_other, sizeof(si_other))) {
					printf("receive data from other peer: %s:%d, original peer: %s:%d\n",
						inet_ntoa(si_tmp.sin_addr), ntohs(si_tmp.sin_port),
						inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port));
				} else {
					write(fd, buf.data(), len);
				}
			}
		}
		else {
			printf("read from udp socket error\n");
			break;
		}
	}
}

void transfer_from_tunnel_to_connection(int fd) {
	vector<char> buf(BUFLEN);
	while(true) {
		int len = read(fd, buf.data(), buf.size());
		if(len > 0) {
			if(sendto(sock, buf.data(), len, 0, 
				(struct sockaddr *)&si_other, sizeof(struct sockaddr)) < 0) {
				printf("send to peer error\n");
				break;
			}
		}
		else if(len < 0) {
			printf("read from tunnel error\n");
			break;
		}
	}
}

void heartbeat() {
	while(true) {
		sendto(sock, "", 0, 0, (struct sockaddr*)&si_other, sizeof(struct sockaddr));
		sleep(1);
	}
}

bool init_internal_socket() {
	if((sock=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
		return false;

	memset((char *)&si_me, 0, sizeof(si_me));
	si_me.sin_family = AF_INET;
	si_me.sin_port = htons(PORT);
	si_me.sin_addr.s_addr = htonl(INADDR_ANY);

	if(bind(sock, (struct sockaddr*)&si_me, sizeof(si_me)) == -1)
		return false;

	return true;
}

void config_tap(const char *local_ip, string &dev_name) {
	static char cmd[1024] = {};

	sprintf(cmd, "ip addr add %s/24 dev %s", local_ip, dev_name.c_str());
	system(cmd);

	sprintf(cmd, "ip link set up dev %s", dev_name.c_str());
	system(cmd);

	sprintf(cmd, "ip link set mtu %d dev %s", MTU, dev_name.c_str());
	system(cmd);
}

int main(int argc, char **argv) {
	if(!init_internal_socket()) {
		printf("cannot init internal udp socket\n");
		return -1;
	}

        std::string dev_name;
        int fd = tap_alloc(dev_name);
        if(fd < 0) {
		printf("cannot create tap device\n");
		return -1;
	}

        std::cout << "dev name: " << dev_name << std::endl;
	config_tap(argc > 1 ? argv[1] : "192.168.1.100", dev_name);

	thread in(transfer_from_connection_to_tunnel, fd);

	struct sockaddr_in test_si;
	memset((char *)&test_si, 0, sizeof(test_si));
	test_si.sin_family = AF_INET;
	if(inet_aton(argc > 2 ? argv[2] : "10.214.224.79", &test_si.sin_addr) == 0) {
		printf("cannot create test_si\n");
		return -1;
	}

	while(!connected) {
		for(int i = 1; i < 65535 && !connected; i ++) {
			test_si.sin_port = htons(argc > 3 ? atoi(argv[3]) : i);
			int slen = sendto(sock, "", 0, 0, (struct sockaddr*)&test_si, sizeof(struct sockaddr));
			if(slen < 0) {
				printf("send error in connecting\n");
				return -1;
			}
		}
		sleep(1);
	}

	thread hb(heartbeat);
	thread out(transfer_from_tunnel_to_connection, fd);
	
	in.join();
	out.join();

	printf("end\n");

	return 0;
}
