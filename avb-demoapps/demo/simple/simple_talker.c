/*
 * Copyright (c) 2014-2016 Renesas Electronics Corporation
 * Released under the MIT license
 * http://opensource.org/licenses/mit-license.php
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <time.h>
#include <signal.h>
#include <sys/types.h>
#include <fcntl.h>
#include <math.h>
#include <pthread.h>
#include <poll.h>
#include <getopt.h>
#include <limits.h>
#include <stdbool.h>
#include <linux/if_ether.h>
#include <inttypes.h>

#include "eavb.h"
#include "msrp.h"
#include "config.h"
#include "eavb_device.h"
#include "simple_talker.h"
#include "avtp.h"
#include "packet.h"
#include "clock.h"
#include "common.h"

#define PROGNAME "simple_talker"
#define PROGVERSION "0.11"

#define ARRAY_SIZE(a)		(sizeof(a) / sizeof(a[0]))

/* global variables */
static bool read_end;
static unsigned char dest_addr[] = DEST_ADDR;

static int show_version(struct app_config *cfg)
{
	fprintf(stderr, PROGNAME " version " PROGVERSION "\n");
	return 0;
}

static const char *optstring = "c:i:p:u:s:f:F:n:m:w:a:h";
static const struct option long_options[] = {
	{"class",             required_argument, NULL, 'c'},
	{"interface",         required_argument, NULL, 'i'},
	{"ptp",               required_argument, NULL, 'p'},
	{"uid",               required_argument, NULL, 'u'},
	{"payload-size",      required_argument, NULL, 's'},
	{"file",              required_argument, NULL, 'f'},
	{"frame-intervals",   required_argument, NULL, 'F'},
	{"frame-num",         required_argument, NULL, 'n'},
	{"msrp",              required_argument, NULL, 'm'},
	{"waitmode",          required_argument, NULL, 'w'},
	{"dest-addr",         required_argument, NULL, 'a'},
	{"version",           no_argument,       NULL,  1 },
	{"help",              no_argument,       NULL, 'h'},
	{NULL,                0,                 NULL,  0 },
};

static int show_usage(struct app_config *cfg)
{
	fprintf(stderr,
		"usage: " PROGNAME " [options] -f <filename>\n"
		"\n"
		"options:\n"
		"    -c, --class=SRCLASS         specify SRClassID A/B/C (default:'A')\n"
		"    -i, --interface=IFNAME      specify network interface name (default:eth0)\n"
		"    -p, --ptp=CLOCK             specify PTP clock name (default:/dev/ptp0)\n"
		"    -u, --uid=UNIQUEID          specify UniqueID in StreamID (default:1)\n"
		"    -s, --payload-size=SIZE     specify data payload size (default:100)\n"
		"    -f, --file=NAME             specify file name\n"
		"    -F, --frame-intervals=NUM   specify MaxIntervalFrames (default:1)\n"
		"    -n, --frame-num=NUM         specify number of frames (default:0=infinite)\n"
		"    -m, --msrp=MODE             MSRP mode 0:static 1:dynamic (default:1 dynamic)\n"
		"    -w, --waitmode=MODE         specify wait mode (default:0 poll)\n"
		"                                0:poll, 1:blocking(NOWAIT) 2:blocking(WAITALL)\n"
		"    -a, --dest-addr=DEST_ADDR   specify destination MAC address\n"
		"                                (default:%02x:%02x:%02x:%02x:%02x:XX, XX=UniqueID(lower 8 bits))\n"
		"    -h, --help                  display this help\n"
		"        --version               print version information\n"
		"\n"
		"examples:\n"
		" " PROGNAME
		" -i eth1 -c A -u 1 -s 500 -n 0 -m 1 -f /tmp/test.bin\n"
		" " PROGNAME
		" -i eth1 -u 2 -n 80000 -m 1 -f /tmp/test.bin\n"
		" " PROGNAME " -i eth1 -m 0 -f /tmp/test.bin\n"
		"\n"
		PROGNAME " version " PROGVERSION "\n",
		dest_addr[0], dest_addr[1], dest_addr[2],
		dest_addr[3], dest_addr[4]);
	return 0;
}

/*
 * config
 */
static int config_init(struct app_config *cfg)
{
	memset(cfg, 0, sizeof(*cfg));

	cfg->uid = 1;
	cfg->entrynum = CONFIG_INIT_ENTRYNUM;
	cfg->SRclassID = MSRP_SR_CLASS_A;
	cfg->SRpriority = MSRP_SR_CLASS_A_PRIO;
	cfg->SRrank = MSRP_RANK;
	cfg->SRclassIntervalFrames = MSRP_SR_CLASS_A_INTERVAL_FRAMES;
	cfg->SRvid = MSRP_SR_CLASS_VID;
	cfg->payload_size = CONFIG_INIT_PAYLOAD_SIZE;
	cfg->MaxIntervalFrames = 1;
	cfg->framenums = 0;
	cfg->msrp = MSRP_ON;
	cfg->waitmode = WAIT_MODE_POLL;
	memcpy(cfg->dest_addr, dest_addr, ETH_ALEN);

	return 0;
}

static int config_parse_fname(char *name)
{
	struct {
		char *name;
		int fd;
	} fd_table[] = {
		{ "stdin", STDIN_FILENO },
		{ "-", STDIN_FILENO },
	};

	int i, len;
	int fd = -1;

	if (!name)
		return -1;

	/* try stdin */
	for (i = 0; i < ARRAY_SIZE(fd_table); i++) {
		len = strlen(fd_table[i].name);
		if (!strncmp(name, fd_table[i].name, len) && !name[len]) {
			fd = fd_table[i].fd;
			break;
		}
	}

	if (fd < 0)
		fd = open(name, O_RDONLY);

	return fd;
}

static int config_parse(struct app_config *cfg, int argc, char **argv)
{
	int c, i, ret;
	int option_index = 0;
	char *iname = NULL;
	char *fname = NULL;
	char *cname = NULL;
	int header_size = AVTP_CVF_PAYLOAD_OFFSET - ETHOVERHEAD;
	clockid_t clkid;

	config_init(cfg);

	/* Process the command line arguments. */
	while (EOF != (c = getopt_long(argc, argv, optstring,
					long_options, &option_index))) {
		switch (c) {
		case 'c':
			i = ((char *)optarg)[0];
			if (i == 'B' || i == 'b') {
				cfg->SRclassID = MSRP_SR_CLASS_B;
				cfg->SRpriority = MSRP_SR_CLASS_B_PRIO;
				cfg->SRclassIntervalFrames =
						MSRP_SR_CLASS_B_INTERVAL_FRAMES;
			} else if (i == 'C' || i == 'c') {
				cfg->SRclassID = MSRP_SR_CLASS_C;
				cfg->SRpriority = MSRP_SR_CLASS_C_PRIO;
				cfg->SRclassIntervalFrames =
						MSRP_SR_CLASS_C_INTERVAL_FRAMES;
			} else {
				cfg->SRclassID = MSRP_SR_CLASS_A;
				cfg->SRpriority = MSRP_SR_CLASS_A_PRIO;
				cfg->SRclassIntervalFrames =
						MSRP_SR_CLASS_A_INTERVAL_FRAMES;
			}
			break;
		case 'i':
			iname = strdup(optarg);
			break;
		case 'p':
			cname = strdup(optarg);
			break;
		case 'u':
			cfg->uid = atoi(optarg);
			break;
		case 's':
			cfg->payload_size = atoi(optarg);
			break;
		case 'f':
			fname = strdup(optarg);
			break;
		case 'F':
			cfg->MaxIntervalFrames = atoi(optarg);
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
		case 'a':
			ret = sscanf(optarg, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
				     &cfg->dest_addr[0], &cfg->dest_addr[1],
				     &cfg->dest_addr[2], &cfg->dest_addr[3],
				     &cfg->dest_addr[4], &cfg->dest_addr[5]);
			if (ret != ETH_ALEN) {
				PRINTF1("[AVB] Conversion failed mac addr.\n");
				return -1;
			}
			cfg->use_dest_addr = true;
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

	if (!fname) {
		PRINTF1("[AVB] Please specify the file name (-f option).\n");
		return -1;
	}

	if (cfg->MaxIntervalFrames < 1) {
		PRINTF1("[AVB] out of range MaxIntervalFrames=%d, specify greater than 0\n",
				cfg->MaxIntervalFrames);
		return -1;
	}

	if ((cfg->msrp < MSRP_OFF) || (cfg->msrp > MSRP_ON)) {
		PRINTF1("[AVB] out of range msrp=%d, specify %d or %d\n",
				cfg->msrp, MSRP_OFF, MSRP_ON);
		return -1;
	}

	if ((cfg->uid < 0) || (cfg->uid > AVTP_UNIQUE_ID_MAX)) {
		PRINTF1("[AVB] out of range uid=%d, specify between 0 and %d\n",
				cfg->uid, AVTP_UNIQUE_ID_MAX);
		return -1;
	}

	if ((cfg->waitmode < WAIT_MODE_POLL) ||
				(cfg->waitmode > WAIT_MODE_BLOCK_WAITALL)) {
		PRINTF1("[AVB] out of range waitmode=%d, specify between %d and %d\n",
				cfg->waitmode, WAIT_MODE_POLL,
				WAIT_MODE_BLOCK_WAITALL);
		return -1;
	}

	cfg->MaxFrameSize = header_size + cfg->payload_size;
	if ((cfg->MaxFrameSize < ETHFRAMEMTU_MIN) ||
				(cfg->MaxFrameSize > ETHFRAMEMTU_MAX)) {
		PRINTF1("[AVB] out of range the maximum size of a ethernet frame\n"
				"      specify payload size in the range %d-%d\n",
				ETHFRAMEMTU_MIN - header_size,
				ETHFRAMEMTU_MAX - header_size);
		return -1;
	}

	cfg->fd = config_parse_fname(fname);
	if (cfg->fd < 0) {
		PRINTF1("[AVB] cannot open file %s.\n", fname);
		return -1;
	}
	free(fname);

	/* The MAC Address of ethernet is got and it uses for StreamID. */
	{
		if (!iname)
			iname = strdup("eth0");

		if (netif_detect(iname) < 0) {
			PRINTF1("[AVB] not found network interface\n");
			return -1;
		}
		if (netif_gethwaddr(iname, cfg->source_addr) < 0) {
			PRINTF1("[AVB] can't get hw address\n");
			return -1;
		}
		if (netif_getlinkspeed(iname, &cfg->speed) < 0) {
			PRINTF1("[AVB] can't get link speed\n");
			return -1;
		}

		strcpy(cfg->ifname, iname);
		free(iname);
	}

	{
		if (!cname)
			cname = strdup("/dev/ptp0");

		clkid = clock_parse(cname);
		if (clkid == CLOCK_INVALID) {
			PRINTF("[AVB] can't parse clock name %s\n", cname);
			return -1;
		}
		PRINTF("[AVB] clock: select %s (%d)\n", cname, clkid);
		cfg->clkid = clkid;

		free(cname);
	}

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

static int talker_calccbsinfo(struct app_config *cfg, struct eavb_cbsparam *cbs)
{
	uint64_t value;
	int portTransmitRate; /* [bit/sec] */
	double bandwidthFraction;
	int classIntervalFrames;
	uint32_t idleSlope, sendSlope;

	portTransmitRate = cfg->speed * 1000000;
	classIntervalFrames = cfg->SRclassIntervalFrames;
	bandwidthFraction = ((ETHOVERHEAD_REAL + cfg->MaxFrameSize) * 8 *
			classIntervalFrames * cfg->MaxIntervalFrames) /
			(double)portTransmitRate; /* bit */

	PRINTF1("[AVB] SRclass%s MaxFrameSize=%d MaxIntervalFrames=%d BandwidthFraction=%.8f\n",
			(cfg->SRclassID == MSRP_SR_CLASS_A) ? "A" :
			(cfg->SRclassID == MSRP_SR_CLASS_B) ? "B" : "C",
			cfg->MaxFrameSize, cfg->MaxIntervalFrames,
			bandwidthFraction);

	value = (uint64_t)(UINT32_MAX * bandwidthFraction);
	if (value > UINT32_MAX) {
		memset(cbs, 0, sizeof(*cbs));
		PRINTF1("[AVB] out of range the bandwidth fraction, it should be less than 1.0.\n");
		return -1;
	} else {
#if 1
		/* Linear : low accuracy. However, it is compoundable by addition. */
		idleSlope = floor(UINT16_MAX * bandwidthFraction);
		sendSlope = ceil(UINT16_MAX * (1 - bandwidthFraction));
#else
		/* Nonlinear : high accuracy is maximized. */
		if (bandwidthFraction < 0.5) {
			sendSlope = -UINT16_MAX;
			idleSlope = ceil((sendSlope * bandwidthFraction) /
						(bandwidthFraction - 1));
		} else {
			idleSlope = UINT16_MAX;
			sendSlope = ceil((idleSlope * (bandwidthFraction - 1)) /
						bandwidthFraction);
		}
#endif

		cbs->bandwidthFraction = value;
		cbs->sendSlope = sendSlope;
		cbs->idleSlope = idleSlope;
	}

	return 0;
}

/*
 * eavb device
 */
static struct eavb_device *eavb_device_new_for_talker
					(struct app_config *cfg, int id)
{
	struct eavb_device *dev;
	int ret;
	char template[2048];
	int len;
	char *name;

	if (cfg->SRclassID == MSRP_SR_CLASS_A)
		name = "/dev/avb_tx1";
	else
		name = "/dev/avb_tx0";

	dev = eavb_device_new(name, cfg->entrynum, O_RDWR);
	if (!dev)
		return NULL;

	if (cfg->waitmode == WAIT_MODE_BLOCK_WAITALL) {
		ret = eavb_set_optblockmode(dev->fd, EAVB_BLOCK_WAITALL);
		if (ret < 0)
			goto error;
	}

	/* set StreamID and destination address  */
	memcpy(dev->StreamID, cfg->source_addr, ETH_ALEN);
	dev->StreamID[6] = (id & 0xff00) >> 8;
	dev->StreamID[7] = (id & 0x00ff);
	memcpy(dev->dest_addr, cfg->dest_addr, ETH_ALEN);
	if (!cfg->use_dest_addr)
		dev->dest_addr[5] = dev->StreamID[7];

	{
		struct avtp_simple_param param;

		memcpy(param.dest_addr, dev->dest_addr, ETH_ALEN);
		memcpy(param.source_addr, cfg->source_addr, ETH_ALEN);
		param.uniqueid = id;
		param.SRpriority = cfg->SRpriority;
		param.SRvid = cfg->SRvid;
		param.payload_size = cfg->payload_size;

		len = avtp_simple_header_build(template, &param);

		if (len < ETHFRAMELEN_MIN)
			cfg->MaxFrameSize = ETHFRAMEMTU_MIN;
		else
			cfg->MaxFrameSize = len - ETHOVERHEAD;
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
			evec->len = len;
			memcpy(p->dma_vaddr, template, len);
		}
	}

	/* Calculate CBS parameter and set Tx param */
	{
		struct eavb_txparam txparam;

		memset(&txparam, 0, sizeof(txparam));

		ret = talker_calccbsinfo(cfg, &txparam.cbs);
		if (ret < 0)
			goto error;

		ret = eavb_set_txparam(dev->fd, &txparam);
		if (ret < 0)
			goto error;
	}

	return dev; /* Success */

error:
	eavb_device_free(dev);

	return NULL;
}

static int talker_process(struct app_config *cfg, int p, int count)
{
	struct eavb_device *dev;
	static int seqnum;
	int read_size, hlen, payload_size;
	int i;
	uint32_t time_stamp, delta_ts, classIntervalFrames;
	uint64_t t;

	struct eavb_dma_alloc *dma;
	struct eavb_entry *e;
	struct eavb_entryvec *evec;
	struct iovec *iov;
	void *packet = NULL;
	void *payload;

	/* get current timestamp */
	t = clock_getcount(cfg->clkid);
	time_stamp = (uint32_t)t + TSOFFSET * 1000;

	PRINTF3("[AVB] talker proc entry num of %d (timestamp:%u)\n",
							count, time_stamp);

	classIntervalFrames = cfg->SRclassIntervalFrames;
	delta_ts = NSEC_SCALE / (classIntervalFrames * cfg->MaxIntervalFrames);

	hlen = AVTP_CVF_PAYLOAD_OFFSET;
	payload_size = cfg->payload_size;

	dev = cfg->device;

	iov = calloc(count, sizeof(*iov));
	if (!iov) {
		PRINTF("[AVB] cannot allocate iovec\n");
		read_end = true;
		return count;
	}

	for (i = 0; i < count; i++) {
		dma = (dev->framebuf + (dev->p * sizeof(*dma)));
		e = dev->entrybuf + (dev->p * sizeof(*e));
		evec = &e->vec[0];
		packet = dma->dma_vaddr;
		payload = packet + AVTP_CVF_PAYLOAD_OFFSET;

		iov[i].iov_base = payload;
		iov[i].iov_len = payload_size;

		set_avtp_sequence_num(packet, seqnum++);
		set_avtp_timestamp(packet, time_stamp);
		set_avtp_stream_data_length(packet, payload_size);

		time_stamp += delta_ts;

		evec->len = hlen + payload_size;
		dev->p = (dev->p + 1) % cfg->entrynum;
	}

	read_size = readv(cfg->fd, iov, count);
	if (read_size < 0) {
		PRINTF1("[AVB] error : File read\n");
		read_end = true;
	} else if (read_size == 0) {
		PRINTF2("[AVB] File read end.\n");
		count = 0;
		read_end = true;
	} else if (read_size < payload_size * count) {
		i = read_size / payload_size;
		payload_size = read_size % payload_size;
		dev->p = (dev->p + i + cfg->entrynum - count) % cfg->entrynum;
		if (payload_size != 0) {
			dma = (dev->framebuf + (dev->p * sizeof(*dma)));
			e = dev->entrybuf + (dev->p * sizeof(*e));
			evec = &e->vec[0];
			packet = dma->dma_vaddr;
			set_avtp_stream_data_length(packet, payload_size);
			evec->len = hlen + payload_size;
			dev->p = (dev->p + 1) % cfg->entrynum;
			count = i + 1;
		} else {
			count = i;
		}
	}

	free(iov);

	return count;
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

static int process_loop(struct app_config *cfg, struct msrp_ctx *ctx)
{
	struct eavb_device *dev;
	int tmp, thresh;
	int process_size;

	bool inf, waitflush;
	int repeat;
	int revents;

	/* entry control info */
	dev = cfg->device;

	/* repeat control info */
	repeat = cfg->framenums;
	if (repeat)
		inf = false;
	else
		inf = true;

	waitflush = false;
	thresh = cfg->entrynum / 8;

	if (inf)
		repeat = 1;

	while (inf || !waitflush) {
		revents = process_wait(cfg, waitflush);

		if (revents & EAVB_NOTIFY_WRITE) {
			process_size = talker_process
					(cfg, dev->wp, dev->remain);

			if (!inf) {
				if (process_size > repeat)
					process_size = repeat;
			}

			tmp = dev->push_entry(dev, process_size);
			PRINTF3("-> push entry num of %d from %d\n",
							tmp, dev->wp);
			if (tmp < 0)
				break;

			if (!inf) {
				repeat -= tmp;
				if (repeat <= 0) {
					read_end = true;
					inf = true;
				}
			}
		}

		if (revents & EAVB_NOTIFY_READ) {
			tmp = dev->take_entry(dev,
				(dev->filled > thresh) ? thresh : dev->filled);
			PRINTF3("<- take entry num of %d from %d\n",
								tmp, dev->rp);
			if (tmp < 0)
				break;
		}

		if (sigint || (cfg->msrp && !msrp_exist_listener(ctx))) {
			inf = false;
			waitflush = true;
		}

		if (read_end) {
			waitflush = true;
			if (dev->filled == 0)
				inf = false;
		}
	}

	return 0;
}

int main(int argc, char **argv)
{
	struct app_config cfg;
	struct eavb_device *dev;
	struct msrp_ctx *ctx = NULL;
	int ret = -1;

	if (config_parse(&cfg, argc, argv) < 0)
		return -1;

	/* install signal handler */
	install_sighandler(SIGINT, sigint_handler);
	install_sighandler(SIGTERM, sigint_handler);
	signal(SIGUSR1, SIG_IGN);

	dev = eavb_device_new_for_talker(&cfg, cfg.uid);
	if (!dev) {
		PRINTF("[AVB] cannot setup eavb device\n");
		goto bad_usage;
	}
	cfg.device = dev;

	PRINTF1("[AVB] %s: %dMbps / %02x:%02x:%02x:%02x:%02x:%02x+%02x:%02x\n",
			cfg.ifname, cfg.speed,
			dev->StreamID[0], dev->StreamID[1], dev->StreamID[2],
			dev->StreamID[3], dev->StreamID[4], dev->StreamID[5],
			dev->StreamID[6], dev->StreamID[7]);

	if (cfg.msrp) {
		int i;
		struct mrp_property prop;

		memset(&prop, 0, sizeof(prop));

		for (i = 0; i < 8; i++) {
			prop.streamid = (prop.streamid << 8) +
				dev->StreamID[i];
		}
		for (i = 0; i < 6; i++) {
			prop.destaddr = (prop.destaddr << 8) +
				dev->dest_addr[i];
		}
		prop.verbose  = DEBUG_LEVEL;
		prop.vlan     = cfg.SRvid;
		prop.MaxFrameSize = cfg.MaxFrameSize;
		prop.MaxIntervalFrames = cfg.MaxIntervalFrames;
		prop.priority = cfg.SRpriority;
		prop.rank     = cfg.SRrank;
		prop.latency  = LATENCY_TIME_MSRP;
		prop.class    = cfg.SRclassID;

		ctx = msrp_ctx_init(&prop);
		if (ctx == NULL) {
			PRINTF("[AVB] failed to initialise context.\n");
			goto bad_usage;
		}

		ret = mvrp_join_vlan(ctx);
		if (ret < 0) {
			PRINTF("[AVB] failed to join vlan.\n");
			goto bad_usage;
		}

		ret = msrp_register_domain(ctx);
		if (ret < 0) {
			PRINTF("[AVB] failed to register domain.\n");
			goto bad_usage;
		}

		ret = msrp_query_database(ctx);
		if (ret < 0) {
			PRINTF("[AVB] failed to query MSRP register database.\n");
			goto bad_usage;
		}

		PRINTF1("[AVB] advertising stream.\n");

		ret = msrp_talker_advertise(ctx);
		if (ret < 0) {
			PRINTF("[AVB] failed to send talker advertise message.\n");
			goto bad_usage;
		}

		while (!sigint) {
			if (msrp_exist_listener(ctx) > 0) {
				PRINTF1("[AVB] got listener.\n");
				break;
			}
			usleep(20000);
		}
		if (sigint)
			goto bad_usage;
	}

	PRINTF1("[AVB] start process loop.\n");
	process_loop(&cfg, ctx);
	PRINTF1("[AVB] finish process loop.\n");

	ret = 0;

bad_usage:
	if (cfg.fd > 2)
		close(cfg.fd);

	if (cfg.device) {
		if (cfg.device->fd) {
			eavb_close(cfg.device->fd);
			PRINTF1("[AVB] closed the device file.\n");
		}

		if (cfg.msrp && ctx) {
			usleep(TSOFFSET);
			PRINTF1("[AVB] unadvertising stream.\n");
			msrp_talker_unadvertise(ctx);

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

		eavb_device_free(cfg.device);
	}

	if (!ret)
		return 0;

	return -1;
}

