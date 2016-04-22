/*
 * Copyright (c) 2014-2016 Renesas Electronics Corporation
 * Released under the MIT license
 * http://opensource.org/licenses/mit-license.php
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "ravb_eavb.h"

#define PROGNAME "avbtool"
#define PROGVERSION "0.2.2"

struct cmd_context {
	const char *devname;
	int fd;
	int argc;
	char **argv;
};

struct option {
	const char *opts;
	int want_device;
	int (*func)(struct cmd_context *);
	char *help;
	char *opthelp;
};

static int do_version(struct cmd_context *ctx)
{
	fprintf(stderr, PROGNAME " version " PROGVERSION "\n");
	return 0;
}

static int show_rxparams(struct cmd_context *ctx)
{
	struct eavb_rxparam rxparam;

	if (ioctl(ctx->fd, EAVB_GETRXPARAM, &rxparam) < 0) {
		perror("EAVB_GETRXPARAM");
		return -1;
	}

	fprintf(stdout, "show_rxparams\n");
	fprintf(stdout, "  streamid %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\n",
			rxparam.streamid[0], rxparam.streamid[1],
			rxparam.streamid[2], rxparam.streamid[3],
			rxparam.streamid[4], rxparam.streamid[5],
			rxparam.streamid[6], rxparam.streamid[7]);

	return 0;
}

static int do_rxparams(struct cmd_context *ctx)
{
	struct eavb_rxparam rxparam;
	const char *param;
	int i, len, ret;

	if (!ctx->argc)
		return show_rxparams(ctx);

	if (ctx->argc != 2) {
		fprintf(stderr, "invalid argument\n");
		return -1;
	}

	if (ioctl(ctx->fd, EAVB_GETRXPARAM, &rxparam) < 0) {
		perror("EAVB_GETRXPARAM");
		return -1;
	}

	param = "streamid";
	len = strlen(param);
	if (!strncmp(*(ctx->argv), param, len) && !((*(ctx->argv))[len])) {
		int streamid[8] = { '\0' };

		ret = sscanf(ctx->argv[1], "%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x",
				&streamid[0], &streamid[1], &streamid[2], &streamid[3],
				&streamid[4], &streamid[5], &streamid[6], &streamid[7]);
		if (ret < 0) {
			perror("sscanf");
			return -1;
		} else if (ret != 8) {
			fprintf(stderr, "invalid argument\n");
			return -1;
		}

		for (i = 0; i < 8; i++)
			rxparam.streamid[i] = streamid[i];

		if (ioctl(ctx->fd, EAVB_SETRXPARAM, &rxparam) < 0) {
			perror("EAVB_SETRXPARAM");
			return -1;
		}
	}

	return show_rxparams(ctx);
}

static int do_ringparams(struct cmd_context *ctx)
{
	struct eavb_avbtool_ringparam ringparam;

	if (ioctl(ctx->fd, EAVB_GRINGPARAM, &ringparam) < 0) {
		perror("EAVB_GRINGPARAM");
		return -1;
	}

	fprintf(stdout, "Ring parameters for %s:\n", ctx->devname);
	fprintf(stdout,
			"Pre-set maximums:\n"
			"RX:             %u\n"
			"RX Mini:        %u\n"
			"RX Jumbo:       %u\n"
			"TX:             %u\n",
			ringparam.rx_max_pending,
			ringparam.rx_mini_max_pending,
			ringparam.rx_jumbo_max_pending,
			ringparam.tx_max_pending);

	fprintf(stdout,
			"Current hardware settings:\n"
			"RX:             %u\n"
			"RX Mini:        %u\n"
			"RX Jumbo:       %u\n"
			"TX:             %u\n",
			ringparam.rx_pending,
			ringparam.rx_mini_pending,
			ringparam.rx_jumbo_pending,
			ringparam.tx_pending);

	fprintf(stdout, "\n");

	return 0;
}

static int do_getdrv(struct cmd_context *ctx)
{
	struct eavb_avbtool_drvinfo drvinfo;

	if (ioctl(ctx->fd, EAVB_GDRVINFO, &drvinfo) < 0) {
		perror("EAVB_GDRVINFO");
		return -1;
	}

	fprintf(stdout,
			"driver: %s\n"
			"version: %s\n"
			"firmware-version: %s\n"
			"bus-info: %s\n"
			"supports-statistics: %s\n"
			"supports-test: %s\n"
			"supports-eeprom-access: %s\n"
			"supports-register-dump: %s\n"
			"supports-priv-flags: %s\n",
			drvinfo.driver,
			drvinfo.version,
			drvinfo.fw_version,
			drvinfo.bus_info,
			(drvinfo.n_stats) ? "yes" : "no",
			(drvinfo.testinfo_len) ? "yes" : "no",
			(drvinfo.eedump_len) ? "yes" : "no",
			(drvinfo.regdump_len) ? "yes" : "no",
			(drvinfo.n_priv_flags) ? "yes" : "no");

	return 0;
}

static struct eavb_avbtool_gstrings *
get_stringset(struct cmd_context *ctx, enum eavb_avbtool_stringset set_id)
{
	struct eavb_avbtool_sset_info *sset_info;
	struct eavb_avbtool_gstrings *strings;
	uint32_t len;

	sset_info = calloc(1, sizeof(*sset_info) + sizeof(uint32_t));
	if (!sset_info) {
		perror("calloc");
		return NULL;
	}

	sset_info->sset_mask = 1 << set_id;

	if (ioctl(ctx->fd, EAVB_GSSET_INFO, sset_info) < 0) {
		perror("EAVB_GSSET_INFO");
		return NULL;
	}

	len = (sset_info->sset_mask) ? sset_info->data[0] : 0;

	strings = calloc(1, sizeof(*strings) + len * EAVB_GSTRING_LEN);
	if (!strings) {
		perror("calloc");
		return NULL;
	}

	strings->string_set = set_id;
	strings->len = len;
	if (len != 0 && ioctl(ctx->fd, EAVB_GSTRINGS, strings) < 0) {
		perror("EAVB_GSTRINGS");
		return NULL;
	}

	return strings;
}

static int do_getstats(struct cmd_context *ctx)
{
	struct eavb_avbtool_gstrings *strings;
	struct eavb_avbtool_stats *stats;
	int i, n_stats, sz_stats;

	strings = get_stringset(ctx, EAVB_SS_STATS);
	if (!strings)
		return -1;

	n_stats = strings->len;
	sz_stats = n_stats * sizeof(uint64_t);

	stats = calloc(1, sz_stats + sizeof(*stats));
	if (!stats) {
		perror("calloc");
		free(strings);
		return -1;
	}

	stats->n_stats = n_stats;
	if (ioctl(ctx->fd, EAVB_GSTATS, stats) < 0) {
		perror("EAVB_GSTATS");
		free(strings);
		free(stats);
		return -1;
	}

	fprintf(stdout, "NIC statistics:\n");
	for (i = 0; i < n_stats; i++) {
		fprintf(stdout, "     %.*s: %llu\n",
				EAVB_GSTRING_LEN,
				&strings->data[i * EAVB_GSTRING_LEN],
				stats->data[i]);
	}

	free(strings);
	free(stats);

	return 0;
}

static int do_channels(struct cmd_context *ctx)
{
	struct eavb_avbtool_channels channels;

	if (ioctl(ctx->fd, EAVB_GCHANNELS, &channels) < 0) {
		perror("EAVB_GCHANNELS");
		return -1;
	}

	fprintf(stdout, "Channel parameters for %s:\n", ctx->devname);
	fprintf(stdout,
			"Pre-set maximums:\n"
			"RX:             %u\n"
			"TX:             %u\n"
			"Other:          %u\n"
			"Combined:       %u\n",
			channels.max_rx,
			channels.max_tx,
			channels.max_other,
			channels.max_combined);

	fprintf(stdout,
			"Current hardware settings:\n"
			"RX:             %u\n"
			"TX:             %u\n"
			"Other:          %u\n"
			"Combined:       %u\n",
			channels.rx_count,
			channels.tx_count,
			channels.other_count,
			channels.combined_count);

	fprintf(stdout, "\n");

	return 0;
}

static int show_usage(struct cmd_context *ctx);

static struct option option_table[] = {
	{ "-r|--rxparams", 1, do_rxparams, "Set/Show Rx parameters",
		"            [ streamid %x:%x:%x:%x:%x:%x:%x:%x ]\n" },
	{ "-g|--show-ring", 1, do_ringparams, "Query RX/TX ring parameters", },
	{ "-i|--driver", 1, do_getdrv, "Show driver information", },
	{ "-S|--statistics", 1, do_getstats, "Show statistics", },
	{ "-l|--show-channels", 1, do_channels, "Query channels", },
	{ "--version", 0, do_version, "Show version number" },
	{ "-h|--help", 0, show_usage, "Show this help" }, /* don't move */
	{}
};

static int show_usage(struct cmd_context *ctx)
{
	const char *progname = PROGNAME;
	const char *progversion = PROGVERSION;
	struct option *opts;

	fprintf(stderr, "%s version %s\nUsage:\n", progname, progversion);
	for (opts = option_table; opts->opts; opts++) {
		fprintf(stderr, "    %s %s %s %s\n",
				progname,
				opts->opts,
				(opts->want_device) ? "DEVNAME" : "",
				opts->help);
		if (opts->opthelp)
			fprintf(stderr, "%s", opts->opthelp);
	}

	return 0;
}

int main(int argc, char **argv)
{
	struct cmd_context ctx;
	struct option *opts;
	const char *opt;
	int len;

	memset(&ctx, 0, sizeof(ctx));

	/* skip prog name */
	argc--;
	argv++;

	if (!argc)
		return show_usage(&ctx);

	for (opts = option_table; opts->opts; opts++) {
		for (opt = opts->opts; ; opt += len + 1) {
			len = strcspn(opt, "|");
			if (!strncmp(*argv, opt, len) && !((*argv)[len])) {
				argc--;
				argv++;
				goto opt_found;
			}
			if (!opt[len])
				break;
		}
	}
	opts--; /* show_usage */

opt_found:
	if (opts->want_device) {
		char *devname = *argv;
		ctx.fd = open(devname, O_RDWR | O_SYNC);
		if (ctx.fd < 0) {
			perror(devname);
			return -1;
		}
		ctx.devname = devname;
		argc--;
		argv++;
	} else
		ctx.fd = -1;

	ctx.argc = argc;
	ctx.argv = argv;

	return opts->func(&ctx);
}
