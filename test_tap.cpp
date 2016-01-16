#include "tap.h"

#include <unistd.h>

#include <string>
#include <cstdio>
#include <vector>
#include <iostream>
#include <thread>

using namespace std;

void config_tap(const char *local_ip, string &dev_name) {
	static char cmd[1024] = {};

	sprintf(cmd, "ip addr add %s/24 dev %s", local_ip, dev_name.c_str());
	system(cmd);

	sprintf(cmd, "ip link set up dev %s", dev_name.c_str());
	system(cmd);

	sprintf(cmd, "ip link set mtu %d dev %s", 1200, dev_name.c_str());
	system(cmd);
}

int main(int argc, char **argv) {

	std::string dev_name;

	int fd = tap_alloc(dev_name);

	if(fd >= 0) {
		std::cout << "dev name: " << dev_name << std::endl;

		config_tap(argc > 1 ? argv[1] : "192.168.1.100", dev_name);

		thread t([=] {
			vector<char> buf;
			buf.resize(1024);
			while(true) {
				int len = read(fd, buf.data(), buf.size());
				if(len < 0) {
					printf("read error\n");
					break;
				}
				else {
					printf("read %d bytes\n", len);
				}
			}
			close(fd);
		});
		t.join();

	}
	else {
		printf("tap allocation error\n");
	}

	return 0;
}
			

