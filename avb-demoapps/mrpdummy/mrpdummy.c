/*
 * Copyright (c) 2016 Renesas Electronics Corporation
 * Released under the MIT license
 * http://opensource.org/licenses/mit-license.php
 */

#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <syslog.h>
#include <signal.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/mman.h>
#include <sys/user.h>
#include <sys/socket.h>
#include <linux/if.h>
#include <netpacket/packet.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/ethernet.h>
#include <sys/un.h>
#include <inttypes.h>

#include "msrp.h"
#include "mrpdummy.h"

#define PROG_STR    "mrpdummy"
#define VERSION_STR "0.2"

/* Polling interval time */
#define MRPDUMMY_POLLING_TIME        (20000)

/*  Max Min  */
#define PARAM_DESTADDR_MAX           (0xffffffffffff)
#define PARAM_VLAN_ID_MIN            (1)
#define PARAM_VLAN_ID_MAX            (4094)
#define PARAM_MAXFRAME_SIZE_MIN      (46)
#define PARAM_MAXFRAME_SIZE_MAX      (1500)
#define PARAM_LATENCY_MIN            (0)
#define PARAM_LATENCY_MAX            (0xFFFFFFFF)
#define PARAM_SRCLASS_ID_MIN         (0)
#define PARAM_SRCLASS_ID_MAX         (7)
#define PARAM_SRCLASS_PRIORITY_MIN   (0)
#define PARAM_SRCLASS_PRIORITY_MAX   (7)

static bool sigint;
static void sigint_handler(int s)
{
	sigint = true;
	PRINTF1("catch SIGINT!\n");
}

static int process_listener(struct msrp_ctx *ctx)
{
	int rc = 0;

	if (!ctx)
		return -1;

	fprintf(stdout, "waiting talker.\n");

	/* await talker */
	while (!sigint) {
		msrp_query_database(ctx);
		if (msrp_exist_talker(ctx)) {
			PRINTF1("await talker.\n");
			break;
		}
		usleep(MRPDUMMY_POLLING_TIME);
	}

	if (sigint)
		return -1;

	rc = msrp_listener_ready(ctx);
	if (rc < 0) {
		fprintf(stderr, "could not send listener ready message.\n");
		return -1;
	}

	fprintf(stdout, "reservation completion\n");

	while (!sigint)
		usleep(MRPDUMMY_POLLING_TIME);

	rc = msrp_listener_leave(ctx);
	if (rc < 0) {
		fprintf(stderr, "could not send listener leave message.\n");
		return -1;
	}

	return 0;
}

static int process_talker(struct msrp_ctx *ctx)
{
	int rc = 0;

	if (!ctx)
		return -1;

	PRINTF2("STREAM ID=%016" SCNx64 "\n", ctx->prop->streamid);

	rc = msrp_talker_advertise(ctx);
	if (rc < 0) {
		fprintf(stderr, "failed to send talker advertise message.\n");
		return -1;
	}

	fprintf(stdout, "waiting listener.\n");

	while (!sigint) {
		if (msrp_exist_listener(ctx) > 0) {
			PRINTF1("await listener.\n");
			break;
		}
		usleep(MRPDUMMY_POLLING_TIME);
	}

	if (sigint) {
		rc = msrp_talker_unadvertise(ctx);
		if (rc < 0) {
			fprintf(stderr,
				"failed to send talker unadvertise message.\n");
		}

		return -1;
	}

	fprintf(stdout, "reservation completion\n");

	while (!sigint) {
		if (msrp_exist_listener(ctx) <= 0) {
			PRINTF1("listener leave!!\n");
			break;
		}
		usleep(MRPDUMMY_POLLING_TIME);
	}

	rc = msrp_talker_unadvertise(ctx);
	if (rc < 0) {
		fprintf(stderr, "failed to send talker unadvertise message.\n");
		return -1;
	}

	return 0;
}

static void usage(void)
{
	fprintf(stderr,
		"\n"
		"usage: " PROG_STR " [options] &\n"
		"\n"
		"options:\n"
		"    -m [pattern]           specify operation mode\n"
		"        0: run talker mode (default)\n"
		"        1: run listener mode\n"
		"        2: join vlan\n"
		"        3: leave vlan\n"
		"        4: registrer domain\n"
		"        5: unregistrer domain\n"
		"    -S [StreamID]          (default:0000000000000000)\n"
		"    -A [DestAddr]          (default:91e0f0000e80)\n"
		"    -V [VLAN]              (default:2)\n"
		"    -Z [MaxFrameSize]      (default:84)\n"
		"    -I [MaxIntervalFrames] (default:1)\n"
		"    -c [SRclassID]         (default:6 [5:B,6:A])\n"
		"    -p [SRclassPriority]   (default:3 [2:B,3:A])\n"
		"    -r [rank]              (default:1 [0:emergency,1:non-emergency])\n"
		"    -L [latency]           (default:3900)\n"
		"    -v                     set verbose\n"
		"    -h                     show this message\n"
		"\n" PROG_STR " v" VERSION_STR
		", Copyright (c) 2013-2016, Renesas Electronics Corporation\n"
		"\n");
}

static int command_parse(struct mrp_property *prop, int argc, char **argv)
{
	int c, tmp;
	long tmp_latency = 0;
	char *e;
	int mode = MODE_TALKER;

	/* Process the command line arguments. */
	while (EOF != (c = getopt(argc, argv, "hm:S:A:V:Z:I:p:r:c:L:v"))) {
		switch (c) {
		case 'm':
			mode = atoi(optarg);
			break;
		case 'S':
			prop->streamid = strtoull(optarg, &e, 16);
			if (*e != '\0' || errno != 0 || optarg[0] == '-') {
				fprintf(stderr,
					"Conversion failed stream id\n");
				return -1;
			}
			break;
		case 'A':
			prop->destaddr = strtoull(optarg, &e, 16);
			if (*e != '\0' || errno != 0 || optarg[0] == '-') {
				fprintf(stderr,
					"Conversion failed mac addr\n");
				return -1;
			}
			break;
		case 'V':
			prop->vlan = atoi(optarg);
			break;
		case 'Z':
			prop->MaxFrameSize = atoi(optarg);
			break;
		case 'I':
			prop->MaxIntervalFrames = atoi(optarg);
			break;
		case 'c':
			tmp = atoi(optarg);
			if ((prop->class == MSRP_SR_CLASS_A) &&
			    (prop->priority == MSRP_SR_CLASS_A_PRIO) &&
			    (tmp == MSRP_SR_CLASS_B))
				prop->priority = MSRP_SR_CLASS_B_PRIO;
			else if ((prop->class == MSRP_SR_CLASS_B) &&
				 (prop->priority == MSRP_SR_CLASS_B_PRIO) &&
				 (tmp == MSRP_SR_CLASS_A))
				prop->priority = MSRP_SR_CLASS_A_PRIO;
			prop->class = tmp;
			break;
		case 'p':
			tmp = atoi(optarg);
			if ((prop->class == MSRP_SR_CLASS_A) &&
			    (prop->priority == MSRP_SR_CLASS_A_PRIO) &&
			    (tmp == MSRP_SR_CLASS_B_PRIO))
				prop->class = MSRP_SR_CLASS_B;
			else if ((prop->class == MSRP_SR_CLASS_B) &&
				 (prop->priority == MSRP_SR_CLASS_B_PRIO) &&
				 (tmp == MSRP_SR_CLASS_A_PRIO))
				prop->class = MSRP_SR_CLASS_A;
			prop->priority = tmp;
			break;
		case 'r':
			prop->rank = atoi(optarg);
			break;
		case 'L':
			tmp_latency = strtol(optarg, &e, 10);
			prop->latency = tmp_latency;
			if (*e != '\0' || errno != 0 || optarg[0] == '-') {
				fprintf(stderr,
					"Conversion failed latency\n");
				return -1;
			}
			break;
		case 'v':
			prop->verbose = true;
			break;
		case 'h':
		default:
			usage();
			exit(EXIT_SUCCESS);
		}
	}

	if (mode < MODE_TALKER || (mode > MODE_UNREGISTER_DOMAIN)) {
		fprintf(stderr, "invalid mode=%d\n", mode);
		return -1;
	}

	if (prop->destaddr < 0 || prop->destaddr > PARAM_DESTADDR_MAX) {
		fprintf(stderr,
			"invalid  dest mac addr=%012" SCNx64 "\n",
			prop->destaddr);
		return -1;
	}

	if (prop->vlan < PARAM_VLAN_ID_MIN || prop->vlan > PARAM_VLAN_ID_MAX) {
		fprintf(stderr, "out of range VLAN_ID=%d 1 to 4094\n",
			prop->vlan);
		return -1;
	}

	if (prop->MaxFrameSize < PARAM_MAXFRAME_SIZE_MIN ||
	    prop->MaxFrameSize > PARAM_MAXFRAME_SIZE_MAX) {
		fprintf(stderr,
			"out of range MaxFrameSize=%d 46 to 1500\n",
			prop->MaxFrameSize);
		return -1;
	}

	if (prop->MaxIntervalFrames < 1) {
		fprintf(stderr,
			"out of range MaxIntervalFrames=%d, specify greater than 0\n",
			prop->MaxIntervalFrames);
		return -1;
	}

	if (prop->class < PARAM_SRCLASS_ID_MIN ||
	    prop->class > PARAM_SRCLASS_ID_MAX) {
		fprintf(stderr, "out of range SRclassID=%d\n",
			prop->class);
		return -1;
	}

	if (prop->priority < PARAM_SRCLASS_PRIORITY_MIN ||
	    prop->priority > PARAM_SRCLASS_PRIORITY_MAX) {
		fprintf(stderr,
			"out of range SRclassPriority=%d\n",
			prop->priority);
		return -1;
	}

	if (tmp_latency < PARAM_LATENCY_MIN ||
		tmp_latency > PARAM_LATENCY_MAX) {
		fprintf(stderr,
			"out of range latency=%ld 0 to 4294967295\n",
			tmp_latency);
		return -1;
	}

	if (prop->rank != MSRP_RANK_NON_EMERGENCY &&
	    prop->rank != MSRP_RANK_EMERGENCY) {
		fprintf(stderr, "invalid emergency rank=%d\n",
			prop->rank);
		return -1;
	}

	return mode;
}

int main(int argc, char *argv[])
{
	static struct mrp_property prop = {
		.streamid = STREAM_ID,
		.destaddr = DEST_ADDR,
		.vlan     = MSRP_SR_CLASS_VID,
		.MaxFrameSize = MRPDUMMY_FRAME_SIZE_DEFAULT,
		.MaxIntervalFrames = MRPDUMMY_INTERVAL_FRAME_DEFAULT,
		.priority = MSRP_SR_CLASS_A_PRIO,
		.rank     = MSRP_RANK,
		.latency  = LATENCY_TIME_MSRP,
		.class    = MSRP_SR_CLASS_A,
	};
	struct msrp_ctx *ctx = NULL;
	int rc = 0;
	struct sigaction sa;
	int mode;

	mode = command_parse(&prop, argc, argv);
	if (mode < 0) {
		fprintf(stderr, "failed to parse option.\n");
		return -1;
	}

	/* install signal handler */
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = sigint_handler;
	sa.sa_flags = SA_RESTART;
	sigemptyset(&sa.sa_mask);
	sigaddset(&sa.sa_mask, SIGQUIT);

	if (sigaction(SIGINT, &sa, NULL) == -1) {
		perror("sigaction SIGINT");
		return -1;
	}

	ctx = msrp_ctx_init(&prop);
	if (!ctx) {
		fprintf(stderr, "failed to initialise context.\n");
		return -1;
	}

	switch (mode) {
	case MODE_TALKER:
		rc = process_talker(ctx);
		break;
	case MODE_LISTENER:
		rc = process_listener(ctx);
		break;
	case MODE_JOIN_VLAN:
		rc = mvrp_join_vlan(ctx);
		break;
	case MODE_LEAVE_VLAN:
		rc = mvrp_leave_vlan(ctx);
		break;
	case MODE_REGISTER_DOMAIN:
		rc = msrp_register_domain(ctx);
		break;
	case MODE_UNREGISTER_DOMAIN:
		rc = msrp_unregister_domain(ctx);
		break;
	}
	if (rc < 0) {
		fprintf(stderr,
			"failed to execute the bandwidth reservation.\n");
	}

	rc = msrp_ctx_destroy(ctx);
	if (rc < 0) {
		fprintf(stderr,
			"failed to destroy context.\n");
	}

	return rc;
}
