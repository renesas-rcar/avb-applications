/*
 * Copyright (c) 2014-2016 Renesas Electronics Corporation
 * Released under the MIT license
 * http://opensource.org/licenses/mit-license.php
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <poll.h>
#include <errno.h>

#include "eavb.h"

#define LIBVERSION "0.3"

static int lastseq_no;

/*
 * open stream queue
 *
 * @devname  specify device name
 * @mode     specify open mode
 */
int eavb_open(char *devname, mode_t mode)
{
	int fd;

	fd = open(devname, mode);
	if (fd < 0) {
		perror(devname);
		return -1;
	}

	return fd;
}

/*
 * close stream queue
 *
 * @fd      specify fd of stream queue
 */
void eavb_close(int fd)
{
	close(fd);
}

/*
 * set Tx parameter of stream queue
 *
 * @fd      specify fd of stream queue
 * @txparam structure of stream tx parameter
 */
int eavb_set_txparam(int fd, struct eavb_txparam *txparam)
{
	int ret;

	ret = ioctl(fd, EAVB_SETTXPARAM, txparam);
	if (ret < 0) {
		perror("EAVB_SETTXPARAM");
		return -1;
	}

	return 0;
}

/*
 * get Tx parameter of stream queue
 *
 * @fd      specify fd of stream queue
 * @txparam structure of stream tx parameter
 */
int eavb_get_txparam(int fd, struct eavb_txparam *txparam)
{
	int ret;

	ret = ioctl(fd, EAVB_GETTXPARAM, txparam);
	if (ret < 0) {
		perror("EAVB_GETTXPARAM");
		return -1;
	}

	return 0;
}

/*
 * set Rx parameter of stream queue
 *
 * @fd      specify fd of stream queue
 * @rxparam structure of stream rx parameter
 */
int eavb_set_rxparam(int fd, struct eavb_rxparam *rxparam)
{
	int ret;

	ret = ioctl(fd, EAVB_SETRXPARAM, rxparam);
	if (ret < 0) {
		perror("EAVB_SETRXPARAM");
		return -1;
	}

	return 0;
}

/*
 * get Rx parameter of stream queue
 *
 * @fd      specify fd of stream queue
 * @rxparam structure of stream rx parameter
 */
int eavb_get_rxparam(int fd, struct eavb_rxparam *rxparam)
{
	int ret;

	ret = ioctl(fd, EAVB_GETRXPARAM, rxparam);
	if (ret < 0) {
		perror("EAVB_GETRXPARAM");
		return -1;
	}

	return 0;
}

/*
 * set Block mode option of stream queue
 *
 * @fd        specify fd of stream queue
 * @blockmode block mode parameter
 */
int eavb_set_optblockmode(int fd, enum eavb_block blockmode)
{
	int ret;
	struct eavb_option opt;

	opt.id = EAVB_OPTIONID_BLOCKMODE;
	opt.param = blockmode;

	ret = ioctl(fd, EAVB_SETOPTION, &opt);
	if (ret < 0) {
		perror("EAVB_SETOPTION");
		return -1;
	}

	return 0;
}

/*
 * get Block mode of stream queue
 *
 * @fd        specify fd of stream queue
 * @blockmode block mode parameter
 */
int eavb_get_optblockmode(int fd, enum eavb_block *blockmode)
{
	int ret;
	struct eavb_option opt;

	opt.id = EAVB_OPTIONID_BLOCKMODE;

	ret = ioctl(fd, EAVB_GETOPTION, &opt);
	if (ret < 0) {
		perror("EAVB_GETOPTION");
		return -1;
	}

	*blockmode = opt.param;

	return 0;
}

/*
 * push stream entry to strema queue
 *
 * @fd       specify fd of stream queue
 * @entrybuf base address of stream entry buffer (user, virtual)
 * @entrynum number of stream entries
 */
int eavb_push(int fd, struct eavb_entry *entrybuf, int entrynum)
{
	struct eavb_entry *e;
	int i, push, ret;

	if (entrynum == 0)
		return 0;

	if (entrynum < 0) {
		perror("invalid arguments");
		return -1;
	}

	for (i = 0, e = entrybuf; i < entrynum; i++, e++)
		e->seq_no = lastseq_no + i;

	ret = write(fd, (void *)entrybuf, entrynum*sizeof(*e));
	if (ret < 0) {
		if (errno == EAGAIN) {
			return 0;
		} else {
			perror("cannot push entry");
			return -1;
		}
	}

	push = ret / sizeof(*e);
	lastseq_no += push;

	return push;
}

/*
 * take stream log entry from strema queue
 *
 * @fd       specify fd of stream queue
 * @entrybuf base address of stream entry buffer (user, virtual)
 * @entrynum max number of stream entries
 */
int eavb_take(int fd, struct eavb_entry *entrybuf, int entrynum)
{
	struct eavb_entry *e;
	int ret;

	if (entrynum == 0)
		return 0;

	if (entrynum < 0) {
		perror("invalid arguments");
		return -1;
	}

	ret = read(fd, (void *)entrybuf, entrynum*sizeof(*e));
	if (ret < 0) {
		if (errno == EAGAIN) {
			return 0;
		} else {
			perror("cannot take entry");
			return -1;
		}
	}

	return ret / sizeof(*e);
}

/*
 * wait ready of push or take entry
 *
 * @fd       specify fd of stream queue
 * @flags    target of wait events (specify bit OR)
 * @timeout  timeout
 */
#define N_FD (1)
int eavb_wait(int fd, int flags, int timeout)
{
	struct pollfd pollfd[N_FD];
	int ret;

	pollfd[0].fd = fd;
	pollfd[0].events = 0;

	if (flags & EAVB_NOTIFY_READ)
		pollfd[0].events |= POLLIN;
	if (flags & EAVB_NOTIFY_WRITE)
		pollfd[0].events |= POLLOUT;

	ret = poll(pollfd, N_FD, timeout);
	if (ret < 0) {
		if (errno != EINTR)
			perror("poll failed");
		return -1;
	}
	if (!ret)
		return 0;

	ret = 0;
	if (pollfd[0].revents & POLLIN)
		ret |= EAVB_NOTIFY_READ;
	if (pollfd[0].revents & POLLOUT)
		ret |= EAVB_NOTIFY_WRITE;

	return ret;
}


/*
 * allocate page from kernel
 *
 * @fd       specify fd of stream queue
 * @page     page information
 */
int eavb_dma_malloc_page(int fd, struct eavb_dma_alloc *page)
{
	int ret;

	if (!page) {
		perror("invalid pointer");
		return -1;
	}

	ret = ioctl(fd, EAVB_MAPPAGE, page);
	if (ret < 0) {
		perror("EAVB_MAPPAGE");
		return -1;
	}

	page->dma_vaddr = (void *)mmap(NULL,
			page->mmap_size,
			PROT_READ | PROT_WRITE,
			MAP_SHARED,
			fd,
			page->dma_paddr);

	if (MAP_FAILED == page->dma_vaddr) {
		perror("mmap");
		return -1;
	}

	return 0;
}

/*
 * free page to kernel
 *
 * @fd       specify fd of stream queue
 * @page     page information
 */
void eavb_dma_free_page(int fd, struct eavb_dma_alloc *page)
{
	if (!page)
		return;

	if (!page->dma_paddr || !page->dma_vaddr)
		return;

	munmap(page->dma_vaddr, page->mmap_size);
	ioctl(fd, EAVB_UNMAPPAGE, page);

	page->dma_paddr = 0;
	page->dma_vaddr = NULL;
	page->mmap_size = 0;

	return;
}
