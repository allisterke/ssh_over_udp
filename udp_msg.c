#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include<arpa/inet.h>
#include<sys/socket.h>

#include <unistd.h>

#include <errno.h>

#define PORT 11444
#define BUFLEN 1024
#define true 1
#define false 0
#define bool int

struct sockaddr_in si_me, si_other;
char buf[BUFLEN];
int sock;
socklen_t addr_len = sizeof(struct sockaddr);

int connected = false;
int first = true;

char remote_ip[16] = {};
int remote_port = 0;

int tcp_sock = 0;
int client_sock = 0;
char crbuf[BUFLEN];

int client_mode = true;

int server_sock = -1;
char srbuf[BUFLEN];

void *server_read(void *arg) {
	printf("starting reading from server side\n");

	while(true) {
		int len = read(server_sock, srbuf, BUFLEN);
		if(len > 0) {
			printf("read %d bytes\n", len);
			if(sendto(sock, srbuf, len, 0, (struct sockaddr*)&si_other, sizeof(struct sockaddr)) < 0) {
				printf("send to client error\n");
			}
		}
		else {
			printf("server sock read error\n");
			break;
		}
	}
	return NULL;
}

void *wait_for_reply(void *arg) {
	pthread_t tmp;
	int len = 0;
	while(true) {
		len = recvfrom(sock, buf, BUFLEN, 0, (struct sockaddr*)&si_other, &addr_len);
		connected = true;
		if(len >= 0) {
			if(first) {
				first = false;
				printf("connected from %s:%d\n", inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port));
				// keep connection
				sendto(sock, "", 0, 0, (struct sockaddr*)&si_other, sizeof(struct sockaddr));

				strcpy(remote_ip, inet_ntoa(si_other.sin_addr));
				remote_port = ntohs(si_other.sin_port);
			}
			else if(len > 0) {
			//	fwrite(buf, len, 1, stdout);
			//	printf("\n");
				if(client_mode) {
					if(client_sock > 0) {
						if(write(client_sock, buf, len) < 0) {
							printf("client sock write error\n");
							close(client_sock);
							client_sock = -1;
						}
					}
				}
				else {
					if(server_sock < 0) {
						server_sock = socket(AF_INET, SOCK_STREAM, 0);
						if(server_sock < 0) {
							printf("cannot create server sock\n");
						}
						else {
							struct sockaddr_in ssh_addr;
							memset((char *)&ssh_addr, 0, sizeof(ssh_addr));
							ssh_addr.sin_family = AF_INET;
							ssh_addr.sin_port = htons(22);
							inet_aton("127.0.0.1", &ssh_addr.sin_addr);

							if(connect(server_sock, &ssh_addr, sizeof(ssh_addr)) < 0) {
								printf("cannot connect local ssh server\n");
								close(server_sock);
								server_sock = -1;
							}
							else {
								pthread_create(&tmp, NULL, server_read, NULL);
							}
						}
						
					}
					if(server_sock > 0) {
						if(write(server_sock, buf, len) < 0) {
							close(server_sock);
							server_sock = -1;
						}
					}
				}
			}
		}
		else {
			printf(strerror(errno));
			break;
		}
	}
	
	return NULL;
}

void *heartbeat(void *arg) {
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

bool client_tcp(int port) {
	tcp_sock = socket(AF_INET, SOCK_STREAM, 0);
	if(tcp_sock < 0) {
		printf("cannot create tcp listen\n");
		return false;
	}

	struct sockaddr_in listen_addr, client_addr;
	int alen;

	memset((char*)&listen_addr, 0, sizeof(listen_addr));
	listen_addr.sin_family = AF_INET;
	listen_addr.sin_port = htons(port);
	listen_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if(bind(tcp_sock, (struct sockaddr *)&listen_addr, sizeof(struct sockaddr)) < 0) {
		printf("cannot bind tcp listen\n");
		return false;
	}

	if(listen(tcp_sock, 5) < 0) {
		printf("cannot listen client tcp\n");
		return false;
	}

	printf("listenning on %d\n", port);

	while(true) {
		client_sock = accept(tcp_sock, (struct sockaddr *)&client_addr, &alen);
		if(client_sock < 0) {
			printf("accept client sock failed\n");
			printf(strerror(errno));
			continue;
		}

		while(true) {
			int length = read(client_sock, crbuf, BUFLEN);
			if(length < 0) {
				close(client_sock);
				client_sock = 0;
			}
			else {
				printf("read %d bytes from client\n", length);
				sendto(sock, crbuf, length, 0, (struct sockaddr*)&si_other, sizeof(struct sockaddr));
			}
		}
	}

	return true;
}

int main(int argc, char **argv) {
	pthread_t tid;
	void *status;

	if(!init_internal_socket()) {
		printf("cannot init internal socket\n");
		return -1;
	}

	pthread_create(&tid, NULL, wait_for_reply, NULL);

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
				exit(0);
			}
		}
		sleep(1);
	}
	
	pthread_t sid;
	pthread_create(&sid, NULL, heartbeat, NULL); 

	if(strcmp(argv[1], "-s") == 0) {
		client_mode = false;
	}
	else {
		client_tcp(2222);
	}

	
//	char msg[1024];
//	while(gets(msg)) {
//		sendto(sock, msg, strlen(msg), 0, (struct sockaddr*)&si_other, sizeof(struct sockaddr));
//	}

	pthread_join(tid, &status);

	printf("end\n");

	return 0;
}
