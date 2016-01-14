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

void *wait_for_reply(void *arg) {
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
			}
			else if(len > 0) {
				fwrite(buf, len, 1, stdout);
				printf("\n");
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
	if(inet_aton(argc > 1? argv[1] : "10.214.224.79", &test_si.sin_addr) == 0) {
		printf("cannot create test_si\n");
		return -1;
	}

	while(!connected) {
		for(int i = 1; i < 65535 && !connected; i ++) {
			test_si.sin_port = htons(argc > 2? atoi(argv[2]) : i);
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

	printf("connected\n");

	char msg[1024];
	while(gets(msg)) {
		sendto(sock, msg, strlen(msg), 0, (struct sockaddr*)&si_other, sizeof(struct sockaddr));
	}

	pthread_join(tid, &status);

	printf("end\n");

	return 0;
}
