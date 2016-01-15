#include "tap.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include <net/if.h>
#include <linux/if_tun.h>

#include <string>
#include <cstring>
#include <cstdio>

using namespace std;

int tap_alloc(std::string &dev_name) {
	struct ifreq ifr;
	int fd, err;

	fd = open("/dev/net/tun", O_RDWR);

	if(fd >= 0) {
		memset(&ifr, 0, sizeof(ifr));

		ifr.ifr_flags = IFF_TAP;
		if(!dev_name.empty()) {
			strncpy(ifr.ifr_name, dev_name.c_str(), IFNAMSIZ);
		}

		err = ioctl(fd, TUNSETIFF, (void *)&ifr);
		if(err < 0) {
			printf("ioctl TUNSETIFF error\n");
			close(fd);
			return err;
		}

		dev_name = ifr.ifr_name;
	}

	return fd;
}
