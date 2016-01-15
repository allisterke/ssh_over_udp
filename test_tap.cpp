#include "tap.h"

#include <unistd.h>

#include <string>
#include <cstdio>
#include <vector>
#include <iostream>

using namespace std;

int main(int argc, char **argv) {

	std::string dev_name;

	int fd = tap_alloc(dev_name);

	if(fd >= 0) {
		std::cout << "dev name: " << dev_name << std::endl;

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
	}
	else {
		printf("tap allocation error\n");
	}

	return 0;
}
			

