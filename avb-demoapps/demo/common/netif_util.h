/*
 * Copyright (c) 2014-2016 Renesas Electronics Corporation
 * Released under the MIT license
 * http://opensource.org/licenses/mit-license.php
 */

#ifndef __NETIF_UTIL_H__
#define __NETIF_UTIL_H__

extern int netif_detect(char *ifname);
extern int netif_gethwaddr(const char *ifname, unsigned char *hwaddr);
extern int netif_getlinkspeed(const char *ifname, int *speed);

#endif /* __NETIF_UTIL_H__ */
