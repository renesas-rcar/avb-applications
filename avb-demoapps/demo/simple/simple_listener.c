/*
 * Copyright (c) 2014-2016 Renesas Electronics Corporation
 * Released under the MIT license
 * http://opensource.org/licenses/mit-license.php
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdbool.h>

#include "config.h"
#include "eavb_device.h"
#include "simple_listener.h"
#include "stats.h"
#include "common.h"

#include "msrp.h"
#include "eavb.h"

#define PROGNAME "simple_listener"
#define PROGVERSION "0.11"

#define ARRAY_SIZE(a)		(sizeof(a) / sizeof(a[0]))

static int show_version(struct app_config *cfg)
{
	fprintf(stderr, PROGNAME " version " PROGVERSION "\n");
	return 0;
}

static const char *optstring = "d:f:n:m:w:h";
static const struct option long_options[] = {
	{"device",            required_argument, NULL, 'd'},
	{"file",              required_argument, NULL, 'f'},
	{"frame-num",         required_argument, NULL, 'n'},
	{"msrp",              required_argument, NULL, 'm'},
	{"waitmode",          required_argument, NULL, 'w'},
	{"version",           no_argument,       NULL,  1 },
	{"help",              no_argument,       NULL, 'h'},
	{NULL,                0,                 NULL,  0 },
};

static int show_usage(struct app_config *cfg)
{
	fprintf(stderr,
			"usage: " PROGNAME " [options]\n"
			"\n"
			"options:\n"
			"    -d, --device=DEVNAME        specify Ethernet AVB device name (default:/dev/avb_rx0)\n"
			"    -f, --file=NAME             specify file name (default:none)\n"
			"    -n, --frame-num=NUM         specify number of frames (default:0=infinite)\n"
			"    -m, --msrp=MODE             MSRP mode 0:static 1:dynamic (default:1 dynamic)\n"
			"    -w, --waitmode=MODE         specify wait mode (default:0 poll)\n"
			"                                0:poll, 1:blocking(NOWAIT) 2:blocking(WAITALL)\n"
			"    -h, --help                  display this help\n"
			"        --version               print version information\n"
			"\n"
			"examples:\n"
			" " PROGNAME
			" -d /dev/avb_rx0 -f /tmp/dump.bin -n 0 -m 1\n"
			" " PROGNAME " -d /dev/avb_rx1 -n 80000 -m 1\n"
			" " PROGNAME " -m 0\n"
			"\n"
			PROGNAME " version " PROGVERSION "\n");
	return 0;
}

/*
 * config
 */
static int config_init(struct app_config *cfg)
{
	memset(cfg, 0, sizeof(*cfg));

	cfg->entrynum = CONFIG_INIT_ENTRYNUM;
	cfg->SRclassID = MSRP_SR_CLASS_A;
	cfg->SRpriority = MSRP_SR_CLASS_A_PRIO;
	cfg->SRrank = MSRP_RANK;
	cfg->SRvid = MSRP_SR_CLASS_VID;
	cfg->framenums = 0;
	cfg->msrp = MSRP_ON;
	cfg->waitmode = WAIT_MODE_POLL;

	return 0;
}

static int config_parse_fname(char *name)
{
	struct {
		char *name;
		int fd;
	} fd_table[] = {
		{ "stderr", STDERR_FILENO },
		{ "stdout", STDOUT_FILENO },
		{ "-", STDOUT_FILENO },
	};

	int i, len;
	int fd = -1;

	if (!name)
		return -1;

	/* try stdout, stderr */
	for (i = 0; i < ARRAY_SIZE(fd_table); i++) {
		len = strlen(fd_table[i].name);
		if (!strncmp(name, fd_table[i].name, len) && !name[len]) {
			fd = fd_table[i].fd;
			break;
		}
	}

	if (fd < 0)
		fd = open(name, O_WRONLY | O_CREAT | O_TRUNC, 0644);

	return fd;
}

static int config_parse(struct app_config *cfg, int argc, char **argv)
{
	int c;
	int option_index = 0;
	char *dname = NULL;
	char *fname = NULL;

	config_init(cfg);

	/* Process the command line arguments. */
	while (EOF != (c = getopt_long(argc, argv, optstring,
					long_options, &option_index))) {
		switch (c) {
		case 'd':
			dname = strdup(optarg);
			break;
		case 'f':
			fname = strdup(optarg);
			break;
		case 'n':
			cfg->framenums = atol(optarg);
			break;
		case 'm':
			cfg->msrp = atoi(optarg);
			break;
		case 'w':
			cfg->waitmode = atoi(optarg);
			break;
		case 1:
			show_version(cfg);
			exit(EXIT_SUCCESS);
		case 'h':
		default:
			show_usage(cfg);
			exit(EXIT_SUCCESS);
		}
	}

	if ((cfg->msrp < MSRP_OFF) || (cfg->msrp > MSRP_ON)) {
		PRINTF1("[AVB] out of range msrp=%d, specify %d or %d\n",
				cfg->msrp, MSRP_OFF, MSRP_ON);
		return -1;
	}

	if ((cfg->waitmode < WAIT_MODE_POLL) ||
				(cfg->waitmode > WAIT_MODE_BLOCK_WAITALL)) {
		PRINTF1("[AVB] out of range waitmode=%d, specify between %d and %d\n",
				cfg->waitmode, WAIT_MODE_POLL,
				WAIT_MODE_BLOCK_WAITALL);
		return -1;
	}

	if (fname) {
		cfg->fd = config_parse_fname(fname);
		if (cfg->fd < 0) {
			PRINTF("[AVB] cannot open file. %s\n", fname);
			return -1;
		}
		free(fname);
	}

	if (!dname)
		dname = strdup("/dev/avb_rx0");

	cfg->devname = dname;

	return 0;
}

/* signal handler */
static bool sigint;
static void sigint_handler(int s)
{
	sigint = true;
}

static int install_sighandler(int s, void (*handler)(int))
{
	struct sigaction sa;

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = handler;
	sigemptyset(&sa.sa_mask);
	sigaddset(&sa.sa_mask, SIGQUIT);

	if (sigaction(s, &sa, NULL) == -1) {
		perror("sigaction");
		return -1;
	}

	return 0;
}

static int verify_1722packet(void *data)
{
	static int seqno = -1;
	static int error = -1;
	int tmp;
	int ret = 0;

	tmp = get_avtp_sequence_num(data);

	if (seqno != tmp && seqno != -1) {
		if (error == -1) {
			PRINTF("avtp sequence number discontinuity,%d->%d=%d\n",
				seqno, tmp,
				(tmp + (AVTP_SEQUENCE_NUM_MAX + 1) - seqno)
						% (AVTP_SEQUENCE_NUM_MAX + 1));
			error = 1;
		} else {
			error++;
		}
		ret = -1;
	} else {
		if (error != -1) {
			PRINTF("avtp sequence number recovery %d, error count=%d\n",
					tmp, error);
			error = -1;
		}
	}

	seqno = (tmp + 1 + (AVTP_SEQUENCE_NUM_MAX + 1))
				% (AVTP_SEQUENCE_NUM_MAX + 1);

	return ret;
}

static struct eavb_device *eavb_device_new_for_listener
					(char *name, int entrynum)
{
	struct eavb_device *dev;
	int ret;
	struct eavb_rxparam rxparam;

	dev = eavb_device_new(name, entrynum, O_RDWR);
	if (!dev)
		return NULL;

	/* verify that the specified device is avb_rx device */
	ret = eavb_get_rxparam(dev->fd, &rxparam);
	if (ret < 0) {
		PRINTF("[AVB] cannot get rxparam from %s, should be specified avb_rx device file", name);
		goto error;
	}

	/* allocate ether frame buffer and prepare hader */
	{
		int i;
		struct eavb_dma_alloc *p;
		struct eavb_entry *e;
		struct eavb_entryvec *evec = NULL;

		for (i = 0, e = dev->entrybuf, p = dev->framebuf;
				i < dev->entrynum;
				i++, e++, p++) {
			ret = eavb_dma_malloc_page(dev->fd, p);
			if (ret < 0)
				goto error;
			evec = &e->vec[0];
			evec->base = p->dma_paddr;
			evec->len = ETHFRAMELEN_MAX;
		}
	}

	return dev; /* Success */

error:
	eavb_device_free(dev);

	return NULL;
}

static void filedump_process(struct app_config *cfg, int count)
{
	static int total_count;
	struct eavb_device *dev;
	struct eavb_dma_alloc *dma;
	struct eavb_entry *e;
	struct eavb_entryvec *evec;
	struct iovec *iov;
	int ret;
	int i;
	void *packet;
	void *payload;
	int payload_size;

	dev = cfg->device;

	iov = calloc(count, sizeof(*iov));
	if (!iov) {
		PRINTF("[AVB] cannot allocate iovec\n");
		return;
	}

	for (i = 0; i < count; i++) {
		dma = dev->framebuf + (dev->p * sizeof(*dma));
		e = dev->entrybuf + (dev->p * sizeof(*e));
		evec = &e->vec[0];
		packet = dma->dma_vaddr;

		verify_1722packet(packet);
		stats_process(&cfg->stats, evec->len);

		payload_size = get_avtp_stream_data_length(packet);
		payload = packet + AVTP_PAYLOAD_OFFSET;

		PRINTF3("count:%d subtype:%d sequence_num:%d timestamp:%d stream_data_length:%d\n",
				total_count++,
				get_avtp_subtype(packet),
				get_avtp_sequence_num(packet),
				get_avtp_timestamp(packet),
				get_avtp_stream_data_length(packet));

		iov[i].iov_base = payload;
		iov[i].iov_len = payload_size;

		evec->len = ETHFRAMELEN_MAX;
		dev->p = (dev->p + 1) % cfg->entrynum;
	}

	if (cfg->fd) {
		ret = writev(cfg->fd, iov, count);
		if (ret < -1)
			PRINTF1("[AVB] File output error\n");
	}

	free(iov);
}

static int process_wait(struct app_config *cfg, int waitflush)
{
	int events, revents;

	if (!waitflush)
		events = EAVB_NOTIFY_READ | EAVB_NOTIFY_WRITE;
	else
		events = EAVB_NOTIFY_READ;

	if (cfg->waitmode) {
		revents = events;
	} else {
		revents = eavb_wait(cfg->device->fd, events, WAIT_TIME_PROCESS);
		if (revents < 0)
			revents = 0;
	}

	return revents;
}

static int filedump_loop(struct app_config *cfg)
{
	struct eavb_device *dev;
	int tmp, thresh;
	int process_size;

	int inf, repeat;
	bool waitflush;
	int revents;

	dev = cfg->device;

	PRINTF1("[AVB] start file save process loop.\n");

	/* repeat control info */
	repeat = cfg->framenums;
	inf = !repeat;
	waitflush = false;
	thresh = cfg->entrynum / 8;

	if (inf)
		repeat = 1;

	while (inf || !(waitflush && !dev->filled)) {
		revents = process_wait(cfg, waitflush);

		if (revents & EAVB_NOTIFY_WRITE) {
			process_size = dev->remain;

			if (!inf)
				if (process_size > repeat)
					process_size = repeat;

			tmp = dev->push_entry(dev, process_size);
			PRINTF3("-> push entry num of %d from %d\n",
							tmp, dev->wp);
			if (tmp < 0)
				break;

			if (!inf) {
				repeat -= tmp;
				if (repeat <= 0)
					waitflush = true;
			}
		}

		if (revents & EAVB_NOTIFY_READ) {
			tmp = dev->take_entry(dev,
				(dev->filled > thresh) ? thresh : dev->filled);
			if (cfg->waitmode == WAIT_MODE_BLOCK_WAITALL &&
								tmp < 0) {
				/* pull out fractional packets */
				eavb_set_optblockmode(cfg->device->fd,
							EAVB_BLOCK_NOWAIT);
				revents = eavb_wait(cfg->device->fd,
							EAVB_NOTIFY_READ, 1);
				if (revents & EAVB_NOTIFY_READ)
					tmp = dev->take_entry(dev,
						(dev->filled > thresh) ?
						thresh : dev->filled);
			}
			PRINTF3("<- take entry num of %d from %d\n",
						tmp, dev->rp);
			if (tmp < 0)
				break;

			filedump_process(cfg, tmp);
		}

		if (sigint)
			goto finish;
	}

finish:
	PRINTF1("[AVB] finish file save process loop.\n");

	return 0;
}

static struct msrp_ctx *msrp_init(const struct app_config *cfg,
				  uint8_t SRclassID, uint8_t SRpriority)
{
	int ret, i;
	struct mrp_property prop;
	struct msrp_ctx *ctx;

	memset(&prop, 0, sizeof(prop));

	for (i = 0; i < AVTP_STREAMID_SIZE; i++)
		prop.streamid = (prop.streamid << 8) + cfg->StreamID[i];
	prop.vlan     = cfg->SRvid;
	prop.priority = SRpriority;
	prop.rank     = cfg->SRrank;
	prop.class    = SRclassID;
	prop.verbose  = DEBUG_LEVEL;

	ctx = msrp_ctx_init(&prop);
	if (ctx == NULL) {
		PRINTF("[AVB] failed to initialise context.\n");
		return NULL;
	}

	ret = mvrp_join_vlan(ctx);
	if (ret < 0) {
		PRINTF("[AVB] failed to join vlan.\n");
		return NULL;
	}

	ret = msrp_register_domain(ctx);
	if (ret < 0) {
		PRINTF("[AVB] failed to register domain.\n");
		return NULL;
	}

	ret = msrp_query_database(ctx);
	if (ret < 0) {
		PRINTF("[AVB] failed to query MSRP register database.\n");
		return NULL;
	}
	return ctx;
}

static void msrp_exit(struct msrp_ctx *ctx)
{
	int ret;

	if (!ctx) {
		PRINTF("[AVB] msrp_ctx is NULL(%p).\n", ctx);
		return;
	}

	ret = msrp_listener_leave(ctx);
	if (ret < 0)
		PRINTF("[AVB] could send listener ready message.\n");

	ret = msrp_unregister_domain(ctx);
	if (ret < 0)
		PRINTF("[AVB] failed to unregister domain.\n");

	ret = mvrp_leave_vlan(ctx);
	if (ret < 0)
		PRINTF("[AVB] failed to leave vlan.\n");

	ret = msrp_ctx_destroy(ctx);
	if (ret < 0)
		PRINTF("[AVB] failed to destroy context.\n");
}

int main(int argc, char **argv)
{
	int ret = -1;
	char stats_buf[2048];
	struct msrp_ctx *ctx[] = {NULL, NULL};
	struct app_config *cfg = calloc(1, sizeof(*cfg));

	if (!cfg) {
		PRINTF("[AVB] cannot allocate cfg\n");
		return -1;
	}

	if (config_parse(cfg, argc, argv) < 0)
		return -1;

	/* install signal handler */
	install_sighandler(SIGINT, sigint_handler);
	install_sighandler(SIGTERM, sigint_handler);

	cfg->device = eavb_device_new_for_listener(cfg->devname,
						cfg->entrynum);
	if (!cfg->device) {
		PRINTF("[AVB] can't open eavb device %s\n", cfg->devname);
		goto bad_usage;
	}

	if (cfg->waitmode == WAIT_MODE_BLOCK_WAITALL) {
		ret = eavb_set_optblockmode(cfg->device->fd,
						EAVB_BLOCK_WAITALL);
		if (ret != 0) {
			PRINTF("[AVB] cannot set blocking mode\n");
			goto bad_usage;
		}
	}

	/* MSRP */
	if (cfg->msrp) {
		struct {
			uint8_t class;
			uint8_t priority;
		} sr[] = {
			{
				.class = MSRP_SR_CLASS_A,
				.priority = MSRP_SR_CLASS_A_PRIO,
			},
			{
				.class = MSRP_SR_CLASS_B,
				.priority = MSRP_SR_CLASS_B_PRIO,
			}
		};
		int i;

		ret = cfg->device->get_separation_filter(cfg->device,
						(char *)cfg->StreamID);
		if (ret != 0) {
			PRINTF("[AVB] cannot get rxparam\n");
			goto bad_usage;
		}

		PRINTF1("[AVB] %s:  %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\n",
				cfg->devname,
				cfg->StreamID[0], cfg->StreamID[1],
				cfg->StreamID[2], cfg->StreamID[3],
				cfg->StreamID[4], cfg->StreamID[5],
				cfg->StreamID[6], cfg->StreamID[7]);

		for (i = 0; i < ARRAY_SIZE(ctx); i++) {
			ctx[i] = msrp_init(cfg, sr[i].class, sr[i].priority);
			if (ctx[i] == NULL)
				goto bad_usage;
		}

		/* Wait to find the talker(new StreamID from mrpd) */
		while (!sigint) {
			for (i = 0; i < ARRAY_SIZE(ctx); i++) {
				if (msrp_exist_talker(ctx[i])) {
					PRINTF("[AVB] await talker. %d\n", i);
					goto TALKER_READY;
				}
				usleep(20000);
			}
		}
TALKER_READY:
		if (sigint)
			goto bad_usage;

		ret = msrp_listener_ready(ctx[i]);
		if (ret < 0) {
			PRINTF("[AVB] could send listener ready message.\n");
			goto bad_usage;
		}
	}

	ret = filedump_loop(cfg);

	/* report stats */
	stats_report(&cfg->stats, stats_buf, sizeof(stats_buf));
	PRINTF("%s: %s\n", cfg->devname, stats_buf);

bad_usage:
	if (cfg->fd  > 2) {
		close(cfg->fd);
		PRINTF1("[AVB] closed the save file.\n");
	}

	if (cfg->device) {
		if (cfg->device->fd) {
			eavb_close(cfg->device->fd);
			PRINTF1("[AVB] closed the device file.\n");
		}

		/* termination */
		if (cfg->msrp) {
			for (int i = 0; i < ARRAY_SIZE(ctx); i++)
				msrp_exit(ctx[i]);
		}

		eavb_device_free(cfg->device);
	}

	free(cfg);

	if (!ret)
		return 0;

	return -1;
}
