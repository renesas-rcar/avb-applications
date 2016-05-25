/*
 * Copyright (c) 2014-2016 Renesas Electronics Corporation
 * Released under the MIT license
 * http://opensource.org/licenses/mit-license.php
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "eavb_device.h"
#include "eavb.h"

#define EAVBDEVICE_DEBUG (0)

static int eavb_device_get_separation_filter(
		struct eavb_device *dev, char streamid[AVTP_STREAMID_SIZE])
{
	int ret = 0;
	int i;
	struct eavb_rxparam rxparam;

	if (!dev)
		return -1;

	ret = eavb_get_rxparam(dev->fd, &rxparam);
	if (ret < 0)
		return ret;

	for (i = 0; i < AVTP_STREAMID_SIZE; i++)
		streamid[i] = rxparam.streamid[i];

	return ret;
}

static int eavb_device_take_entry(struct eavb_device *dev, int count)
{
	int ret, tmp;
	char *buf;

	if (!dev)
		return -1;

	buf = dev->entryworkbuf;

	ret = eavb_take(dev->fd, (struct eavb_entry *)buf, count);

	if (ret <= 0)
		return ret;

	if (dev->rp + ret > dev->entrynum) {
		tmp = dev->entrynum - dev->rp;
		memcpy(dev->entrybuf+(dev->rp*sizeof(struct eavb_entry)),
				buf, tmp * sizeof(struct eavb_entry));
		memcpy(dev->entrybuf, buf+(tmp*sizeof(struct eavb_entry)),
				(ret - tmp) * sizeof(struct eavb_entry));
	} else {
		memcpy(dev->entrybuf+(dev->rp*sizeof(struct eavb_entry)),
				buf, ret*sizeof(struct eavb_entry));
	}

	dev->remain += ret;
	dev->filled -= ret;

#if EAVBDEVICE_DEBUG
	fprintf(stderr, "eavb_device: take num of %d from %d (remain:%d, filled:%d)\n",
			ret, dev->rp, dev->remain, dev->filled);
#endif

	dev->rp = (dev->rp + ret) % dev->entrynum;

	return ret;
}

static int eavb_device_push_entry(struct eavb_device *dev, int count)
{
	int ret, tmp;
	char *buf;
	void *from, *to;

	if (!dev)
		return -1;

	buf = dev->entryworkbuf;

	if (dev->wp + count > dev->entrynum) {
		tmp = dev->entrynum - dev->wp;

		from = dev->entrybuf+(dev->wp*sizeof(struct eavb_entry));
		to = buf;
		memcpy(to, from, tmp * sizeof(struct eavb_entry));

		from = dev->entrybuf,
		to = buf+(tmp*sizeof(struct eavb_entry));
		memcpy(to, from, (count - tmp) * sizeof(struct eavb_entry));

		ret = eavb_push(dev->fd, (struct eavb_entry *)buf, count);
	} else {
		buf = dev->entrybuf+(dev->wp*sizeof(struct eavb_entry));
		ret = eavb_push(dev->fd, (struct eavb_entry *)buf, count);
	}

	if (ret <= 0)
		return ret;

	dev->remain -= ret;
	dev->filled += ret;

#if EAVBDEVICE_DEBUG
	fprintf(stderr, "eavb_device: push num of %d from %d (remain:%d, filled:%d)\n",
			ret, dev->wp, dev->remain, dev->filled);
#endif

	dev->wp = (dev->wp + ret) % dev->entrynum;

	return ret;
}

/*
 * public functions
 */
/* TODO unified routine with Talker and Listener */
struct eavb_device *eavb_device_new(char *name, int entrynum, mode_t mode)
{
	struct eavb_device *dev;
	int fd = -1;

	if (!name)
		return NULL;

	dev = calloc(1, sizeof(*dev));
	if (!dev)
		return NULL;

	dev->entrynum = entrynum;
	dev->remain = entrynum;
	dev->get_separation_filter = eavb_device_get_separation_filter;
	dev->take_entry = eavb_device_take_entry;
	dev->push_entry = eavb_device_push_entry;

	/* open device */
	fd = eavb_open(name, mode);
	if (fd < 0)
		goto error;
	dev->fd = fd;

	/* allocate entry buffer */
	dev->entrybuf = calloc(dev->entrynum, sizeof(struct eavb_entry));
	if (!dev->entrybuf) {
		fprintf(stderr, "[AVB] cannot allocate entrybuf\n");
		goto error;
	}

	/* allocate entry work buffer */
	dev->entryworkbuf = calloc(dev->entrynum, sizeof(struct eavb_entry));
	if (!dev->entryworkbuf) {
		fprintf(stderr, "[AVB] cannot allocate entryworkbuf\n");
		goto error;
	}

	/* allocate frame info buffer */
	dev->framebuf = calloc(dev->entrynum, sizeof(struct eavb_dma_alloc));
	if (!dev->framebuf) {
		fprintf(stderr, "[AVB] cannot allocate framebuf\n");
		goto error;
	}

	return dev; /* Success */

error:
	if (dev->framebuf)
		free(dev->framebuf);
	if (dev->entryworkbuf)
		free(dev->entryworkbuf);
	if (dev->entrybuf)
		free(dev->entrybuf);
	if (fd > 0)
		eavb_close(fd);
	free(dev);

	return NULL;
}

void eavb_device_free(struct eavb_device *dev)
{
	if (!dev)
		return;

	if (dev->framebuf)
		free(dev->framebuf);
	if (dev->entryworkbuf)
		free(dev->entryworkbuf);
	if (dev->entrybuf)
		free(dev->entrybuf);
	if (dev->fd > 0)
		eavb_close(dev->fd);
	free(dev);
}
