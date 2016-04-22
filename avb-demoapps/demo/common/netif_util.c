/*
 * Copyright (c) 2014-2016 Renesas Electronics Corporation
 * Released under the MIT license
 * http://opensource.org/licenses/mit-license.php
 */

#include "netif_util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/ethtool.h>
#include <linux/sockios.h>
#include <linux/if_ether.h>

#include "avtp.h"

int netif_detect(char *ifname)
{
	struct if_nameindex *if_ni, *i;

	if (if_nametoindex(ifname))
		return 0;

	if_ni = if_nameindex();
	if (if_ni == NULL) {
		perror("if_nameindex");
		return -1;
	}

	/* search valid interface */
	for (i = if_ni; !(i->if_index == 0 && i->if_name == NULL); i++) {
		if (strncmp(i->if_name, "lo", IFNAMSIZ-1)) { /* if not "lo" */
			fprintf(stderr, "netif: not found '%s', fallback using '%s'\n", ifname, i->if_name);
			strcpy(ifname, i->if_name);
			if_freenameindex(if_ni);
			return 0;
		}
	}

	/* not found */
	if_freenameindex(if_ni);

	return -1;
}

int netif_gethwaddr(const char *ifname, unsigned char *hwaddr)
{
	struct ifreq ifr;
	int fd;
	int ret = 0;

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0) {
		perror("socket");
		return -1;
	}

	memset(&ifr, 0, sizeof(ifr));
	strcpy(ifr.ifr_name, ifname);

	if (ioctl(fd, SIOCGIFHWADDR, &ifr)) {
		perror("ioctl");
		ret = -1;
	} else {
		memcpy(hwaddr, ifr.ifr_hwaddr.sa_data, ETH_ALEN);
	}

	close(fd);

	return ret;
}

int netif_getlinkspeed(const char *ifname, int *speed)
{
	int fd;
	int ret = 0;
	struct ifreq ifr;
	struct ethtool_cmd ecmd;

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0) {
		perror("socket");
		return -1;
	}

	memset(&ifr, 0, sizeof(ifr));
	strcpy(ifr.ifr_name, ifname);

	ecmd.cmd = ETHTOOL_GSET;
	ifr.ifr_data = (void *)&ecmd;

	if (ioctl(fd, SIOCETHTOOL, &ifr)) {
		perror("ioctl");
		ret = -1;
	} else {
		*speed = ecmd.speed;
	}

	close(fd);

	return ret;
}


