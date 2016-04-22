/*
 * Copyright (c) 2014-2016 Renesas Electronics Corporation
 * Released under the MIT license
 * http://opensource.org/licenses/mit-license.php
 */

#ifndef __EAVB_H__
#define __EAVB_H__

#include <stdint.h>
#include "ravb_eavb.h"

enum eavb_notify {
	EAVB_NOTIFY_READ  = 0x00000001,
	EAVB_NOTIFY_WRITE = 0x00000002,
};

extern int eavb_open(char *devname, mode_t mode);
extern void eavb_close(int fd);
extern int eavb_set_txparam(int fd, struct eavb_txparam *txparam);
extern int eavb_get_txparam(int fd, struct eavb_txparam *txparam);
extern int eavb_set_rxparam(int fd, struct eavb_rxparam *rxparam);
extern int eavb_get_rxparam(int fd, struct eavb_rxparam *rxparam);
extern int eavb_set_optblockmode(int fd, enum eavb_block blockmode);
extern int eavb_get_optblockmode(int fd, enum eavb_block *blockmode);
extern int eavb_push(int fd, struct eavb_entry *entrybuf, int entrynum);
extern int eavb_take(int fd, struct eavb_entry *entrybuf, int entrynum);
extern int eavb_wait(int fd, int flags, int timeout);
extern int eavb_dma_malloc_page(int fd, struct eavb_dma_alloc *page);
extern void eavb_dma_free_page(int fd, struct eavb_dma_alloc *page);

#endif /* __EAVB_H__ */
