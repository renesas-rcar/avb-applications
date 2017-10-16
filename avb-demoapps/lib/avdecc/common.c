/*
 * Copyright (c) 2016 Renesas Electronics Corporation
 * Released under the MIT license
 * http://opensource.org/licenses/mit-license.php
 */

#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>

#include "avdecc_internal.h"

int is_interface_linkup(int sock, const char *interface_name)
{
	struct ifreq ifr;

	if (!interface_name) {
		AVDECC_PRINTF1("[AVDECC] interface_name is NULL.\n");
		return -1;
	}

	memset(&ifr, 0, sizeof(ifr));
	strncpy((char *)ifr.ifr_name, interface_name, IFNAMSIZ);
	if (ioctl(sock, SIOCGIFFLAGS, &ifr) < 0) {
		AVDECC_PRINTF1("[AVDECC] Could not get intreface flags: %s\n",
			strerror(errno));
			return -1;
	}

	if (!(ifr.ifr_flags & IFF_UP))
		return 0;

	return 1;
}

void set_socket_nonblocking(int fd)
{
	int val;
	int flags;

	val = fcntl(fd, F_GETFL, 0);
	flags = O_NONBLOCK;
	val |= flags;
	fcntl(fd, F_SETFL, val);
}

int send_data(
	struct network_info *net_info,
	const uint8_t dest_mac[6],
	const void *payload,
	ssize_t payload_len)
{
	int rc = -1;
	int sent_len;
	struct sockaddr_ll saddr;
	unsigned char buffer[ETH_FRAME_LEN];
	unsigned char *etherhead = buffer;
	unsigned char *data = buffer + ETHERNET_OVERHEAD;
	struct ethhdr *eh = (struct ethhdr *)etherhead;

	if (!net_info) {
		AVDECC_PRINTF1("[AVDECC] net_info is NULL.\n");
		return -1;
	}

	if (!payload) {
		AVDECC_PRINTF1("[AVDECC] payload is NULL.\n");
		return -1;
	}

	saddr.sll_family = AF_PACKET;
	saddr.sll_protocol = htons(net_info->ethertype);
	saddr.sll_ifindex = net_info->interface_id;
	saddr.sll_hatype = 1;
	saddr.sll_pkttype = PACKET_OTHERHOST;
	saddr.sll_halen = ETH_ALEN;
	memcpy(saddr.sll_addr, net_info->mac_addr, ETH_ALEN);
	saddr.sll_addr[6] = 0x00;
	saddr.sll_addr[7] = 0x00;

	if (dest_mac)
		memcpy((void *)buffer, (void *)dest_mac, ETH_ALEN);
	else
		memcpy((void *)buffer,
		       (void *)jdksavdecc_multicast_adp_acmp.value,
		       ETH_ALEN);

	memcpy((void *)(buffer + ETH_ALEN), (void *)net_info->mac_addr, ETH_ALEN);
	eh->h_proto = htons(net_info->ethertype);
	memcpy(data, payload, payload_len);
	do {
		sent_len = sendto(net_info->raw_sock,
				  buffer,
				  payload_len + ETHERNET_OVERHEAD,
				  0,
				  (struct sockaddr *)&saddr,
				  sizeof(saddr));

	} while (sent_len < 0 && (errno == EINTR));

	if (sent_len >= 0)
		rc = sent_len - ETHERNET_OVERHEAD;

	return rc;
}

void close_socket(int fd)
{
	close(fd);
}

int create_socket(struct network_info *net_info, const char *interface_name)
{
	int i;
	int sock;
	struct sockaddr_ll saddr;
	struct packet_mreq mreq;
	struct ifreq ifr;

	if (!net_info) {
		AVDECC_PRINTF1("[AVDECC] net_info is NULL.\n");
		return -1;
	}

	if (!interface_name) {
		AVDECC_PRINTF1("[AVDECC] interface_name is NULL\n");
		return -1;
	}

	sock = socket(AF_PACKET, SOCK_RAW, htons(net_info->ethertype));
	if (sock < 0) {
		AVDECC_PRINTF1("[AVDECC] Could not create raw socket\n");
		return -1;
	}

	strncpy((char *)ifr.ifr_name, interface_name, IFNAMSIZ);
	if (ioctl(sock, SIOCGIFINDEX, &ifr) < 0) {
		AVDECC_PRINTF1("[AVDECC] Could not get interface index: %s\n",
			strerror(errno));
		close(sock);
		return -1;
	}
	net_info->interface_id = ifr.ifr_ifindex;

	if (ioctl(sock, SIOCGIFHWADDR, &ifr) < 0) {
		AVDECC_PRINTF1("[AVDECC] Could not get hw addr: %s\n",
			strerror(errno));
		close(sock);
		return -1;
	}

	strncpy((char *)net_info->ifname, interface_name, IFNAMSIZ);

	for (i = 0; i < ETH_ALEN; ++i)
		net_info->mac_addr[i] = (uint8_t)ifr.ifr_hwaddr.sa_data[i];

	if (ioctl(sock, SIOCGIFFLAGS, &ifr) < 0) {
		AVDECC_PRINTF1("[AVDECC] Could not get flags: %s\n",
			strerror(errno));
		close(sock);
		return -1;
	}

	memset(&saddr, 0, sizeof(saddr));
	saddr.sll_family = AF_PACKET;
	saddr.sll_ifindex = net_info->interface_id;
	saddr.sll_pkttype = PACKET_MULTICAST;
	saddr.sll_protocol = htons(net_info->ethertype);

	net_info->raw_sock = sock;
	if (bind(net_info->raw_sock,
		 (const struct sockaddr *)&saddr,
		 sizeof(saddr)) < 0) {
		AVDECC_PRINTF1("[AVDECC] bind error: %s\n", strerror(errno));
		close(sock);
		return -1;
	}

	memset(&mreq, 0, sizeof(mreq));
	mreq.mr_ifindex = net_info->interface_id;
	mreq.mr_type = PACKET_MR_MULTICAST;
	mreq.mr_alen = ETH_ALEN;
	mreq.mr_address[0] = jdksavdecc_multicast_adp_acmp.value[0];
	mreq.mr_address[1] = jdksavdecc_multicast_adp_acmp.value[1];
	mreq.mr_address[2] = jdksavdecc_multicast_adp_acmp.value[2];
	mreq.mr_address[3] = jdksavdecc_multicast_adp_acmp.value[3];
	mreq.mr_address[4] = jdksavdecc_multicast_adp_acmp.value[4];
	mreq.mr_address[5] = jdksavdecc_multicast_adp_acmp.value[5];

	if (setsockopt(net_info->raw_sock,
		       SOL_PACKET,
		       PACKET_ADD_MEMBERSHIP,
		       &mreq, sizeof(mreq)) < 0) {
		AVDECC_PRINTF1("[AVDECC] setsockopt error: %s\n",
			strerror(errno));
		close(sock);
		return -1;
	}

	set_socket_nonblocking(sock);

	return 0;
}
