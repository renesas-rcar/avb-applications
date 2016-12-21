/*
 * Copyright (c) 2016 Renesas Electronics Corporation
 * Released under the MIT license
 * http://opensource.org/licenses/mit-license.php
 */

#include <stdio.h>
#include <stdint.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <getopt.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <inttypes.h>
#include <limits.h>
#include <linux/if_ether.h>
#include <errno.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include <inih/ini.h>

#include "avblauncher.h"
#include "avtp.h"
#include "netif_util.h"
#include "avdecc.h"

#define PROGNAME "avblauncher"
#define PROGVERSION "0.2"

/* Max Min */
#define VLAN_ID_MIN          1
#define VLAN_ID_MAX          4094
#define MAXFRAME_SIZE_MIN    46
#define MAXFRAME_SIZE_MAX    1500
#define SRCLASS_ID_MIN       0
#define SRCLASS_ID_MAX       7
#define SRCLASS_PRIORITY_MIN 0
#define SRCLASS_PRIORITY_MAX 7
#define UNIQUE_ID_MIN        1
#define UNIQUE_ID_MAX        USHRT_MAX
#define FRAMEINTERVALS_MIN   1
#define FRAMEINTERVALS_MAX   USHRT_MAX
#define LATENCY_MIN          0
#define LATENCY_MAX          UINT_MAX

enum states {
	INITIAL,
	ESCAPE,
	OTHER,
	END,
	FIRST_DOLLAR,
	DOLLAR_ESCAPE,
	DOLLAR_DOLLAR,
	DOLLAR_OTHER,
	DOLLAR_END,
	STATE_NUM
};

static int ini2ctx_mode(struct app_context *ctx, const char *value);
static int ini2ctx_gptp(struct app_context *ctx, const char *value);
static int ini2ctx_srp(struct app_context *ctx, const char *value);
static int ini2ctx_avdecc(struct app_context *ctx, const char *value);
static int ini2ctx_destaddr(struct app_context *ctx, const char *value);
static int ini2ctx_streamid(struct app_context *ctx, const char *value);
static int ini2ctx_application(struct app_context *ctx, const char *value);
static int ini2ctx_srclass(struct app_context *ctx, const char *value);
static int ini2ctx_srpriority(struct app_context *ctx, const char *value);
static int ini2ctx_vid(struct app_context *ctx, const char *value);
static int ini2ctx_uid(struct app_context *ctx, const char *value);
static int ini2ctx_maxframesize(struct app_context *ctx, const char *value);
static int ini2ctx_frameintervals(struct app_context *ctx, const char *value);
static int ini2ctx_latency(struct app_context *ctx, const char *value);

struct parser_data {
	enum INDEX_CONTEXT_MEMBER index;
	char *name;
	int (*func)(struct app_context *ctx, const char *value);
};

static struct parser_data parser_table[] = {
	{ IDX_OPERATING_MODE, INI_MODE           , ini2ctx_mode          },
	{ IDX_USE_GPTP      , INI_GPTP           , ini2ctx_gptp          },
	{ IDX_USE_SRP       , INI_SRP            , ini2ctx_srp           },
	{ IDX_USE_AVDECC    , INI_AVDECC         , ini2ctx_avdecc        },
	{ IDX_SR_CLASS      , INI_SR_CLASS       , ini2ctx_srclass       },
	{ IDX_SR_PRIORITY   , INI_SR_PRIORITY    , ini2ctx_srpriority    },
	{ IDX_VID           , INI_VLAN_ID        , ini2ctx_vid           },
	{ IDX_UID           , INI_UNIQUE_ID      , ini2ctx_uid           },
	{ IDX_STREAM_ID     , INI_STREAM_ID      , ini2ctx_streamid      },
	{ IDX_DEST_ADDR     , INI_DEST_ADDR      , ini2ctx_destaddr      },
	{ IDX_MAXFRAMESIZE  , INI_MAX_FRAMESIZE  , ini2ctx_maxframesize  },
	{ IDX_FRAMEINTERVALS, INI_FRAME_INTERVALS, ini2ctx_frameintervals},
	{ IDX_LATENCY       , INI_LATENCY        , ini2ctx_latency       },
	{ IDX_APPLICATION   , INI_APPLICATION    , ini2ctx_application   },
	{ IDX_MAX_NUM       , NULL               , NULL                  }
};

static bool sigint;
static void sigint_handler(int s)
{
	sigint = true;
	PRINTF1("catch SIGINT!\n");
}

static int show_usage(void)
{
	fprintf(stderr,
		"\nusage: " PROGNAME "[options] [ini file]\n"
		"\n"
		"options:\n"
		"    -g=CONFIG_FILE execute daemon_cl.\n"
		"                   specify configuration file for daemon_cl.\n"
		"    -m             execute mrpd\n"
		"    -a             execute maap_daemon\n"
		"    -i=IFNAME      specify network interface name (default:eth0)\n"
		"    -h             display this help\n"
		"\n"
		"examples:\n"
		" " PROGNAME " -g gptp_cfg.ini -m -i eth0 sample.ini\n"
		"\n"
		PROGNAME " version " PROGVERSION "\n");

	return 0;
}

static int assigned_uint64_data(
		const char *value, uint64_t *data)
{
	char *e;

	errno = 0;
	*data = strtoll(value, &e, 10);
	if (errno != 0) {
		perror("strtoll");
		return INI_FAILURE;
	}

	if (e == value) {
		fprintf(stderr, "%s: not a decimal number\n", value);
		return INI_FAILURE;
	}

	return INI_SUCCESS;
}

static int assigned_bool_data(
		char *param1, char *param2, const char *value, bool *data)
{
	if (strcasecmp(param1, value) == 0)
		*data = true;
	else if (strcasecmp(param2, value) == 0)
		*data = false;
	else
		return INI_FAILURE;

	return INI_SUCCESS;
}

static int ini2ctx_mode(struct app_context *ctx, const char *value)
{
	if (strcasecmp("talker", value) == 0)
		ctx->operating_mode = TALKER_MODE;
	else if (strcasecmp("listener", value) == 0)
		ctx->operating_mode = LISTENER_MODE;
	else
		return INI_FAILURE;

	return INI_SUCCESS;
}

static int ini2ctx_gptp(struct app_context *ctx, const char *value)
{
	return assigned_bool_data("enable", "disable", value, &ctx->use_gptp);
}

static int ini2ctx_srp(struct app_context *ctx, const char *value)
{
	return assigned_bool_data("enable", "disable", value, &ctx->use_srp);
}

static int ini2ctx_avdecc(struct app_context *ctx, const char *value)
{
	if (strcasecmp("disable", value) == 0) {
		ctx->use_avdecc = false;
		return INI_SUCCESS;
	}

	ctx->use_avdecc = true;
	strcpy(ctx->avdecc_entity, value);
	return INI_SUCCESS;
}

static int ini2ctx_destaddr(struct app_context *ctx, const char *value)
{
	int rc = 0;

	if (strcasecmp("maap", value) == 0) {
		ctx->use_maap = true;
		return INI_SUCCESS;
	}
	ctx->use_maap = false;

	rc = sscanf(value, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
		&ctx->dest_addr[0], &ctx->dest_addr[1],
		&ctx->dest_addr[2], &ctx->dest_addr[3],
		&ctx->dest_addr[4], &ctx->dest_addr[5]);
	if (rc < 0) {
		perror("sscanf(): dest_addr");
		return INI_FAILURE;
	}

	return INI_SUCCESS;
}

static int ini2ctx_streamid(struct app_context *ctx, const char *value)
{
	int rc = 0;

	rc = sscanf(value, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
		&ctx->streamid[0], &ctx->streamid[1],
		&ctx->streamid[2], &ctx->streamid[3],
		&ctx->streamid[4], &ctx->streamid[5],
		&ctx->streamid[6], &ctx->streamid[7]);
	if (rc < 0) {
		perror("sscanf(): streamid");
		return INI_FAILURE;
	}

	return INI_SUCCESS;
}

static int ini2ctx_application(struct app_context *ctx, const char *value)
{
	/* It is guaranteed that the end of "value" is "\0". */
	strcpy(ctx->app_cmd, value);

	return INI_SUCCESS;
}

static int ini2ctx_srclass(struct app_context *ctx, const char *value)
{
	return assigned_uint64_data(value, &ctx->srclass);
}

static int ini2ctx_srpriority(struct app_context *ctx, const char *value)
{
	return assigned_uint64_data(value, &ctx->srpriority);
}

static int ini2ctx_vid(struct app_context *ctx, const char *value)
{
	return assigned_uint64_data(value, &ctx->vid);
}

static int ini2ctx_uid(struct app_context *ctx, const char *value)
{
	return assigned_uint64_data(value, &ctx->uid);
}

static int ini2ctx_maxframesize(struct app_context *ctx, const char *value)
{
	return assigned_uint64_data(value, &ctx->maxframesize);
}

static int ini2ctx_frameintervals(struct app_context *ctx, const char *value)
{
	return assigned_uint64_data(value, &ctx->frameintervals);
}

static int ini2ctx_latency(struct app_context *ctx, const char *value)
{
	return assigned_uint64_data(value, &ctx->latency);
}

static int ini_parse_handler(void *user, const char *section, const char *name,
			const char *value)
{
	struct app_context *ctx = (struct app_context *)user;
	struct parser_data *p = NULL;
	int rc = 0;

	if (strcasecmp(INI_SECTION_NAME, section) != 0) {
		fprintf(stderr, "unknown section name %s\n", section);
		return INI_FAILURE;
	}

	for (p = parser_table; p->name != NULL; p++) {
		if (strcmp(name, p->name) == 0) {
			rc = p->func(ctx, value);
			if (rc == INI_SUCCESS)
				ctx->required[p->index] = true;
			return rc;
		}
	}

	fprintf(stderr, "unknown paramter name %s\n", name);

	return INI_FAILURE;
}

static int convert_word(struct app_context *ctx, char *word, char *conv)
{
	int rc = 0;

	if (strcmp(word, "$MODE$") == 0) {
		if (ctx->operating_mode == TALKER_MODE)
			rc = snprintf(conv, MAXSTR, "talker");
		else
			rc = snprintf(conv, MAXSTR, "listener");
	} else if (strcmp(word, "$GPTP$") == 0) {
		if (ctx->use_gptp == true)
			rc = snprintf(conv, MAXSTR, "enable");
		else
			rc = snprintf(conv, MAXSTR, "disable");
	} else if (strcmp(word, "$SRP$") == 0) {
		if (ctx->use_srp == true)
			rc = snprintf(conv, MAXSTR, "enable");
		else
			rc = snprintf(conv, MAXSTR, "disable");
	} else if (strcmp(word, "$AVDECC$") == 0) {
		if (ctx->use_avdecc == true)
			rc = snprintf(conv, MAXSTR, "%s", ctx->avdecc_entity);
		else
			rc = snprintf(conv, MAXSTR, "disable");
	} else if (strcmp(word, "$DEST_ADDR$") == 0) {
		rc = snprintf(conv, MAXSTR, "%02x:%02x:%02x:%02x:%02x:%02x",
			ctx->dest_addr[0], ctx->dest_addr[1],
			ctx->dest_addr[2], ctx->dest_addr[3],
			ctx->dest_addr[4], ctx->dest_addr[5]);
	} else if (strcmp(word, "$STREAM_ID$") == 0) {
		rc = snprintf(conv, MAXSTR,
			"%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x",
			ctx->streamid[0], ctx->streamid[1],
			ctx->streamid[2], ctx->streamid[3],
			ctx->streamid[4], ctx->streamid[5],
			ctx->streamid[6], ctx->streamid[7]);
	} else if (strcmp(word, "$SR_CLASS$") == 0) {
		rc = snprintf(conv, MAXSTR, "%" PRIu64, ctx->srclass);
	} else if (strcmp(word, "$SR_PRIORITY$") == 0) {
		rc = snprintf(conv, MAXSTR, "%" PRIu64, ctx->srpriority);
	} else if (strcmp(word, "$VLAN_ID$") == 0) {
		rc = snprintf(conv, MAXSTR, "%" PRIu64, ctx->vid);
	} else if (strcmp(word, "$UNIQUE_ID$") == 0) {
		rc = snprintf(conv, MAXSTR, "%" PRIu64, ctx->uid);
	} else if (strcmp(word, "$MAX_FRAMESIZE$") == 0) {
		rc = snprintf(conv, MAXSTR, "%" PRIu64, ctx->maxframesize);
	} else if (strcmp(word, "$FRAME_INTERVALS$") == 0) {
		rc = snprintf(conv, MAXSTR, "%" PRIu64, ctx->frameintervals);
	} else if (strcmp(word, "$LATENCY$") == 0) {
		rc = snprintf(conv, MAXSTR, "%" PRIu64, ctx->latency);
	} else {
		PRINTF1("%s is not a conversion word\n", word);
		rc = snprintf(conv, MAXSTR, "%s", word);
	}

	PRINTF2("convert %s to %s\n", word, conv);

	return rc;
}

static enum states change_next_state(char cmd, int now_state)
{
	enum states rc = INITIAL;

	if (cmd == '\\') {
		if (now_state < FIRST_DOLLAR)
			rc = ESCAPE;
		else
			rc = DOLLAR_ESCAPE;
	} else if (cmd == '$') {
		if (now_state < FIRST_DOLLAR)
			rc = FIRST_DOLLAR;
		else
			rc = DOLLAR_DOLLAR;
	} else if (cmd == '\0') {
		if (now_state < FIRST_DOLLAR)
			rc = END;
		else
			rc = DOLLAR_END;
	} else {
		if (now_state < FIRST_DOLLAR)
			rc = OTHER;
		else
			rc = DOLLAR_OTHER;
	}

	return rc;
}

static int convert_app_cmd(struct app_context *ctx)
{
	char *cmd = ctx->app_cmd;

	int search_p = 0;
	int tmp_p = 0;
	int word_p = 0;
	int len = 0;
	int error = 0;
	int num;
	char conv[MAXSTR] = {'\0'};
	char word[MAXSTR] = {'\0'};
	char tmp[MAXSTR] = {'\0'};

	enum states current_state = INITIAL;

	PRINTF2("before conversion of app_cmd: '%s'\n", cmd);

	len = strlen(cmd);
	while (search_p <= len && !error) {
		switch (current_state) {
		case INITIAL:
			break;
		case ESCAPE:
			if (strlen(tmp) < MAXSTR) {
				tmp[tmp_p] = cmd[search_p+1];
				search_p += 2;
				tmp_p++;
			} else {
				error = 1;
			}
			break;
		case OTHER:
			if (strlen(tmp) < MAXSTR) {
				tmp[tmp_p] = cmd[search_p];
				search_p++;
				tmp_p++;
			} else {
				error = 1;
			}
			break;
		case END:
			if (strlen(tmp) < MAXSTR)
				tmp[tmp_p+1] = '\0';
			else
				error = 1;
			break;
		case FIRST_DOLLAR:
			if (strlen(word) < MAXSTR) {
				word_p = 0;
				word[word_p] = cmd[search_p];
				word_p++;
				search_p++;
			} else {
				error = 1;
			}
			break;
		case DOLLAR_ESCAPE:
			if (strlen(word) < MAXSTR) {
				word[word_p] = cmd[search_p+1];
				word_p++;
				search_p += 2;
			} else {
				error = 1;
			}
			break;
		case DOLLAR_DOLLAR:
			if (strlen(word) < MAXSTR) {
				word[word_p] = cmd[search_p];
				word[word_p+1] = '\0';
				num = convert_word(ctx, word, conv);
				if ((strlen(tmp) + num) < MAXSTR) {
					strncpy(&tmp[tmp_p], conv, num+1);
					tmp_p += num;
					search_p++;
					current_state = INITIAL;
				} else {
					error = 1;
				}
			} else {
				error = 1;
			}
			break;
		case DOLLAR_OTHER:
			if (strlen(word) < MAXSTR) {
				word[word_p] = cmd[search_p];
				search_p++;
				word_p++;
			} else {
				error = 1;
			}
			break;
		case DOLLAR_END:
			num = strlen(word);
			if ((num < MAXSTR) && (strlen(tmp)+num < MAXSTR)) {
				word[word_p] = '\0';
				strncpy(&tmp[tmp_p], word, num+1);
			} else {
				error = 1;
			}
			break;
		default:
			current_state = END;
			break;
		}
		if ((current_state == END) ||
			(current_state == DOLLAR_END) || error)
			break;
		current_state = change_next_state(cmd[search_p], current_state);
	}

	num = strlen(tmp);
	if (!error && (num < MAXSTR)) {
		strncpy(ctx->app_cmd, tmp, num+1);
		PRINTF2("after conversion of app_cmd: '%s'\n", ctx->app_cmd);

		return 0;
	}

	return -1;
}

static void print_context(struct app_context *ctx)
{
	PRINTF2("************ CONTEXT ************\n");
	PRINTF2("operating_mode : %d\n", ctx->operating_mode);
	PRINTF2("use_gptp       : %d\n", ctx->use_gptp);
	PRINTF2("use_srp        : %d\n", ctx->use_srp);
	PRINTF2("use_maap       : %d\n", ctx->use_maap);
	PRINTF2("use_avdecc     : %d\n", ctx->use_avdecc);
	PRINTF2("srclass        : %" PRIu64 "\n", ctx->srclass);
	PRINTF2("srpriority     : %" PRIu64 "\n", ctx->srpriority);
	PRINTF2("vid            : %" PRIu64 "\n", ctx->vid);
	PRINTF2("uid            : %" PRIu64 "\n", ctx->uid);
	PRINTF2("streamid       : %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\n",
		ctx->streamid[0], ctx->streamid[1],
		ctx->streamid[2], ctx->streamid[3],
		ctx->streamid[4], ctx->streamid[5],
		ctx->streamid[6], ctx->streamid[7]);
	PRINTF2("src_addr       : %02x:%02x:%02x:%02x:%02x:%02x\n",
		ctx->src_addr[0], ctx->src_addr[1],
		ctx->src_addr[2], ctx->src_addr[3],
		ctx->src_addr[4], ctx->src_addr[5]);
	PRINTF2("dest_addr      : %02x:%02x:%02x:%02x:%02x:%02x\n",
		ctx->dest_addr[0], ctx->dest_addr[1],
		ctx->dest_addr[2], ctx->dest_addr[3],
		ctx->dest_addr[4], ctx->dest_addr[5]);
	PRINTF2("maxframesize   : %" PRIu64 "\n", ctx->maxframesize);
	PRINTF2("frameintervals : %" PRIu64 "\n", ctx->frameintervals);
	PRINTF2("latency        : %" PRIu64 "\n", ctx->latency);
	PRINTF2("mrpdummy_pid   : %d\n", ctx->mrpdummy_pid);
	PRINTF2("app_pid        : %d\n", ctx->app_pid);
	PRINTF2("app_cmd        : %s\n", ctx->app_cmd);
	PRINTF2("avdecc_entity  : %s\n", ctx->avdecc_entity);
	PRINTF2("err_flag       : %d\n", ctx->err_flag);
	PRINTF2("*********************************\n");
}

static int context_error_check(struct app_context *ctx)
{

	bool err_flag = 0;

	if (ctx->required[IDX_OPERATING_MODE] == false) {
		fprintf(stderr, "MODE is not set\n");
		err_flag = 1;
	}

	if (ctx->required[IDX_USE_GPTP] == false) {
		fprintf(stderr, "GPTP is not set\n");
		err_flag = 1;
	}

	if (ctx->required[IDX_USE_SRP] == false) {
		fprintf(stderr, "SRP is not set\n");
		err_flag = 1;
	}

	if (ctx->required[IDX_USE_AVDECC] == false) {
		fprintf(stderr, "AVDECC is not set\n");
		err_flag = 1;
	}

	if (ctx->required[IDX_SR_CLASS] == false) {
		fprintf(stderr, "SR_CLASS is not set\n");
		err_flag = 1;
	} else {
		if (ctx->srclass < SRCLASS_ID_MIN
			|| SRCLASS_ID_MAX < ctx->srclass) {
			fprintf(stderr,
				"out of range SR_CLASS = %" PRIu64 "\n",
					ctx->srclass);
				err_flag = 1;
		}
	}

	if (ctx->required[IDX_SR_PRIORITY] == false) {
		fprintf(stderr, "SR_PRIORITY is not set\n");
		err_flag = 1;
	} else {
		if (ctx->srpriority < SRCLASS_PRIORITY_MIN
			|| SRCLASS_PRIORITY_MAX < ctx->srpriority) {
			fprintf(stderr,
				"out of range SR_PRIORITY = %" PRIu64 "\n",
				ctx->srpriority);
			err_flag = 1;
		}
	}

	if (ctx->required[IDX_VID] == false) {
		fprintf(stderr, "VLAN_ID is not set\n");
		err_flag = 1;
	} else {
		if (ctx->vid < VLAN_ID_MIN || VLAN_ID_MAX < ctx->vid) {
			fprintf(stderr,
				"out of range VLAN_ID = %" PRIu64 "\n",
				ctx->vid);
			err_flag = 1;
		}
	}

	if (ctx->required[IDX_APPLICATION] == false) {
		fprintf(stderr, "APPLICATION is not set\n");
		err_flag = 1;
	}

	if (ctx->operating_mode == LISTENER_MODE) {
		if (ctx->required[IDX_STREAM_ID] == false) {
			fprintf(stderr, "STREAM_ID is not set\n");
			err_flag = 1;
		}
	} else {
		if (ctx->required[IDX_UID] == false) {
			fprintf(stderr, "UNIQUE_ID is not set\n");
			err_flag = 1;
		} else {
			if (ctx->uid < UNIQUE_ID_MIN || UNIQUE_ID_MAX < ctx->uid) {
				fprintf(stderr,
					"out of range UNIQUE_ID = %" PRIu64 "\n",
					ctx->uid);
				err_flag = 1;
			}
		}

		if (ctx->required[IDX_DEST_ADDR] == false) {
			fprintf(stderr, "DEST_ADDR is not set\n");
			err_flag = 1;
		}
		if (ctx->required[IDX_MAXFRAMESIZE] == false) {
			fprintf(stderr, "MAX_FRAMESIZE is not set\n");
			err_flag = 1;
		} else {
			if (ctx->maxframesize < MAXFRAME_SIZE_MIN ||
				MAXFRAME_SIZE_MAX < ctx->maxframesize) {
				fprintf(stderr,
					"out of range MAX_FRAMESIZE = %" PRIu64 "\n",
					ctx->maxframesize);
				err_flag = 1;
			}
		}

		if (ctx->required[IDX_FRAMEINTERVALS] == false) {
			fprintf(stderr, "FRAME_INTERVALS is not set\n");
			err_flag = 1;
		} else {
			if (ctx->frameintervals < FRAMEINTERVALS_MIN
				|| FRAMEINTERVALS_MAX < ctx->frameintervals) {
				fprintf(stderr,
					"out of range FRAME_INTERVALS = %" PRIu64 "\n",
					ctx->frameintervals);
				err_flag = 1;
			}
		}

		if (ctx->required[IDX_LATENCY] == false) {
			fprintf(stderr, "LATENCY is not set\n");
			err_flag = 1;
		} else {
			if (ctx->latency < LATENCY_MIN || LATENCY_MAX < ctx->latency) {
				fprintf(stderr,
					"out of range LATENCY = %" PRIu64 "\n",
					ctx->latency);
				err_flag = 1;
			}
		}
	}

	if (err_flag)
		return -1;

	return 0;
}

static int generate_ctx_data(char *argv, struct app_context *ctx)
{
	int rc = 0;

	rc = ini_parse(argv, ini_parse_handler, ctx);
	if (rc < 0) {
		fprintf(stderr, "could not load %s. this file is skiped.\n",
			argv);
		return -1;
	}

	return 0;
}

static int create_contexts(int argc, char **argv, struct app_context *ctx)
{
	int i = 0;
	int rc = 0;
	int ctx_num = 0;

	for (i = 0; i < argc; i++) {
		rc = generate_ctx_data(argv[i], &ctx[ctx_num]);
		if (rc < 0)
			continue;

		ctx[ctx_num].ini_name = argv[i];
		ctx_num++;
	}

	return ctx_num;
}

static int wait_child_process(pid_t pid)
{
	int status;

	pid_t r = waitpid(pid, &status, 0);

	if (r < 0) {
		perror("waitpid()");
		return -1;
	}

	if (!WIFEXITED(status)) {
		fprintf(stderr, "pid=%d, status=%04x\n", pid, status);
		return -1;
	}

	return WEXITSTATUS(status);
}

static pid_t exe_command(char *cmdline, int *infp, int *outfp)
{
	int p_stdin[2], p_stdout[2];
	pid_t pid;
	int rc;

	/* Create two of pipes. */
	if (pipe(p_stdin) < 0) {
		perror("command");
		return -1;
	}

	if (pipe(p_stdout) < 0) {
		perror("command");
		close(p_stdin[READ]);
		close(p_stdin[WRITE]);
		return -1;
	}

	pid = fork();

	if (pid < 0) {
		perror("command");
		close(p_stdin[READ]);
		close(p_stdin[WRITE]);
		close(p_stdout[READ]);
		close(p_stdout[WRITE]);
		return -1;
	} else if (pid == 0) {
		close(p_stdout[WRITE]);
		close(p_stdin[READ]);
		dup2(p_stdout[READ], STDIN_FILENO);
		dup2(p_stdin[WRITE], STDOUT_FILENO);
		close(p_stdout[READ]);
		close(p_stdin[WRITE]);

		rc = execlp("/bin/sh", "/bin/sh", "-c", cmdline, NULL);
		if (rc < 0) {
			perror("command");
			close(p_stdout[READ]);
			close(p_stdin[WRITE]);
			exit(1);
		}
	}

	close(p_stdout[READ]);
	close(p_stdin[WRITE]);

	if (outfp)
		*outfp = p_stdout[WRITE];

	if (infp)
		*infp  = p_stdin[READ];

	return pid;
}

static int open_gptp_data(char *shm_name, int *daemon_cl_shm_fd,
	char **shared_mem_adr)
{
	int fd = -1;
	char *ptr = NULL;

	fd = shm_open(shm_name, O_RDWR, 0);
	if (fd == -1) {
		perror("shm_open()");
		return -1;
	}

	ptr = (char *)mmap(NULL, DEAMON_CL_SHM_SIZE,
			PROT_READ | PROT_WRITE,
			MAP_SHARED, fd, 0);
	if (ptr == MAP_FAILED) {
		perror("mmap()");
		*shared_mem_adr = NULL;
		shm_unlink(shm_name);
		return -1;
	}

	*daemon_cl_shm_fd = fd;
	*shared_mem_adr = ptr;

	return 0;
}

static void close_gptp_data(int shm_fd, char *shared_mem_adr)
{
	if (shared_mem_adr != NULL)
		munmap(shared_mem_adr, DEAMON_CL_SHM_SIZE);

	if (shm_fd != -1)
		close(shm_fd);
}

static int get_gptp_data(char *shared_mem_adr, struct gptp_time_data *td)
{
	int rc = 0;

	rc = pthread_mutex_lock((pthread_mutex_t *) shared_mem_adr);
	if (rc != 0) {
		perror("pthread_mutex_lock");
		return -1;
	}

	memcpy(td, shared_mem_adr + sizeof(pthread_mutex_t), sizeof(*td));

	rc = pthread_mutex_unlock((pthread_mutex_t *) shared_mem_adr);
	if (rc != 0) {
		perror("pthread_mutex_unlock");
		return -1;
	}

	return 0;
}

static int wait_ascapable(struct app_context *ctx)
{
	int rc = 0;
	struct gptp_time_data ptpdata;

	while (!sigint) {
		rc = get_gptp_data(ctx->gptp_shm_addr, &ptpdata);
		if (rc < 0) {
			fprintf(stderr, "could not get gptp data.\n");
			return -1;
		}

		if (ptpdata.ascapable) {
			PRINTF1("asCapable enabled.\n");
			return 0;
		}
		usleep(SLEEP_TIME);
	}

	return -1;
}

static int open_maap_data(char **shared_mem_adr)
{
	int shmid;
	char *ptr = NULL;

	shmid = shmget(MAAP_DAEMON_SHM_KEY, MAAP_DAEMON_SHM_SIZE, 0);
	if (shmid < 0) {
		perror("shmget()");
		return -1;
	}

	ptr = (char *)shmat(shmid, (void *)0, 0);
	if (ptr == (char *)-1) {
		perror("shmat()");
		return -1;
	}

	*shared_mem_adr = ptr;

	return 0;
}

static int get_maap_data(char *shared_mem_adr, uint8_t *dest_addr)
{
	if (!shared_mem_adr[0])
		return -1;

	memcpy(dest_addr, shared_mem_adr, MAAP_DAEMON_SHM_SIZE);

	return 0;
}

static void close_maap_data(char *shared_mem_adr)
{
	if (!shared_mem_adr)
		shmdt(shared_mem_adr);
}

static int wait_maap_addr(struct app_context *ctx)
{
	int rc = 0;

	while (!sigint) {
		rc = get_maap_data(ctx->maap_shm_addr, ctx->dest_addr);
		if (rc == 0) {
			PRINTF2("MAAP addr: %02x:%02x:%02x:%02x:%02x:%02x\n",
				ctx->dest_addr[0], ctx->dest_addr[1],
				ctx->dest_addr[2], ctx->dest_addr[3],
				ctx->dest_addr[4], ctx->dest_addr[5]);
			return 0;
		}
		usleep(SLEEP_TIME);
	}

	return -1;
}

static int pidof(char *name)
{
	int rc = 0;
	char cmd[MAXSTR] = {'\0'};

	snprintf(cmd, MAXSTR, "pidof %s", name);
	rc = system(cmd);

	return rc;
}

static int start_daemon(char *cmd_name, char *option, int opt_flag)
{
	int pid;
	char cmd[MAXSTR] = {'\0'};

	snprintf(cmd, MAXSTR, "%s %s", cmd_name, option);

	if (pidof(cmd_name) == 0) {
		PRINTF("%s already exists.\n", cmd_name);
		return 0;
	}

	if (!opt_flag)
		return -1;

	pid = exe_command(cmd, NULL, NULL);
	if (pid < 0) {
		fprintf(stderr, "could not execute %s.\n", cmd_name);
		return -1;
	}
	PRINTF("execute %s pid:[%d]\n", cmd_name, pid);

	return 0;
}

static struct domain_info *add_domain_info(
	int srclass, int srpriority, struct domain_info *list)
{
	struct domain_info *p = list;
	struct domain_info *new_domain;

	new_domain =
		(struct domain_info *)calloc(1, sizeof(struct domain_info));
	if (!new_domain) {
		fprintf(stderr, "could not allocat domain_info\n");
		return NULL;
	}

	new_domain->srclass    = srclass;
	new_domain->srpriority = srpriority;
	new_domain->next = NULL;

	if (list == NULL)
		return new_domain;

	while (p->next != NULL)
		p = p->next;
	p->next = new_domain;

	return list;
}

static struct connect_info *add_connect_list(
	struct app_context *ctx, struct connect_info *c_info)
{
	struct connect_info *p = c_info;
	struct connect_info *new_info;

	new_info =
		(struct connect_info *)calloc(1, sizeof(struct connect_info));
	if (!new_info) {
		fprintf(stderr, "could not allocat c_info\n");
		return NULL;
	}

	new_info->vid = ctx->vid;
	new_info->domain =
		add_domain_info(ctx->srclass, ctx->srpriority, NULL);
	new_info->next = NULL;

	if (c_info == NULL)
		return new_info;

	while (p->next != NULL)
		p = p->next;
	p->next = new_info;

	return c_info;
}

static struct connect_info *connect_avb_network(
	struct app_context *ctx,
	int ctx_num,
	struct connect_info *c_info)
{
	int i = 0;
	pid_t pid;
	char cmd[MAXSTR] = {'\0'};
	struct connect_info *tmp_l  = NULL;
	struct domain_info  *tmp_d  = NULL;

	for (i = 0; i < ctx_num; i++) {
		if (ctx[i].err_flag || ctx[i].use_srp == 0) {
			continue;
		} else {
			if (c_info == NULL) {
				c_info = add_connect_list(&ctx[i], c_info);
			} else {
				tmp_d = c_info->domain;
				if ((tmp_d->srclass == ctx[i].srclass)
					&& (tmp_d->srpriority
						== ctx[i].srpriority)) {
					/* already join vlan/srp domain */
					continue;
				} else if ((tmp_d->srclass == ctx[i].srclass)
					&& (tmp_d->srpriority
						!= ctx[i].srpriority)) {
					/* context error */
					fprintf(stderr,
						"same vlan/class, but priority is different.\n"
						"This context(%s) is ignored.\n"
						"vlan:%" PRIu64 ", class:%" PRIu64 ":, priority:%" PRIu64"\n",
						ctx[i].ini_name,
						ctx[i].vid,
						ctx[i].srclass,
						ctx[i].srpriority);
					ctx[i].err_flag = 1;
					break;
				}
				tmp_d = add_domain_info(ctx[i].srclass,
							ctx[i].srpriority,
							tmp_d);
			}
		}
	}

	for (tmp_l = c_info; tmp_l != NULL; tmp_l = tmp_l->next) {
		/* join vlan */
		snprintf(cmd, MAXSTR, "%s -m 2 -V %d -v",
			MRPDUMMY_CMD, tmp_l->vid);
		pid = exe_command(cmd, NULL, NULL);
		wait_child_process(pid);

		PRINTF2("join vlan: vid:%d\n", c_info->vid);

		/* join srp domain */
		for (tmp_d = c_info->domain; tmp_d != NULL;
			tmp_d = tmp_d->next) {
			snprintf(cmd, MAXSTR,
				"%s -m 4 -V %d -c %d -p %d -v",
				MRPDUMMY_CMD, tmp_l->vid,
				tmp_d->srclass, tmp_d->srpriority);
			pid = exe_command(cmd, NULL, NULL);
			wait_child_process(pid);

			PRINTF2("join domain: vid:%d class:%d priority:%d\n",
				c_info->vid,
				tmp_d->srclass,
				tmp_d->srpriority);
		}
	}

	return c_info;
}

static void disconnect_avb_network(struct connect_info *c_info)
{
	int pid;
	char cmd[MAXSTR] = {'\0'};

	struct connect_info *tmp_l = NULL;
	struct domain_info  *tmp_d = NULL;

	struct connect_info *next_l = NULL;
	struct domain_info  *next_d = NULL;

	for (tmp_l = c_info; tmp_l != NULL; tmp_l = tmp_l->next) {

		for (tmp_d = c_info->domain; tmp_d != NULL; tmp_d = next_d) {
			snprintf(cmd, MAXSTR, "%s -m 5 -V %d -c %d -p %d -v",
				MRPDUMMY_CMD, tmp_l->vid,
				tmp_d->srclass, tmp_d->srpriority);
			pid = exe_command(cmd, NULL, NULL);
			wait_child_process(pid);

			PRINTF2("leave domain: vid:%d class:%d priority:%d\n",
				tmp_l->vid,
				tmp_d->srclass,
				tmp_d->srpriority);

			next_d = tmp_d->next;
			free(tmp_d);
		}
	}

	for (tmp_l = c_info; tmp_l != NULL; tmp_l = next_l) {
		snprintf(cmd, MAXSTR, "%s -m 3 -V %d -v",
			MRPDUMMY_CMD, tmp_l->vid);
		pid = exe_command(cmd, NULL, NULL);
		wait_child_process(pid);

		PRINTF2("leave vlan: %d\n", tmp_l->vid);

		next_l = tmp_l->next;
		free(tmp_l);
	}
}

static int stream_reservation_for_talker(struct app_context *ctx, int *fd_r)
{
	int i = 0;
	uint64_t tmp = 0;
	uint8_t streamid[AVTP_STREAMID_SIZE];
	char streamid_str[MAXSTR] = {'\0'};
	char destaddr_str[MAXSTR] = {'\0'};
	char cmd[MAXSTR] = {'\0'};

	memcpy(streamid, ctx->src_addr, ETH_ALEN);
	streamid[6] = (ctx->uid & 0xff00) >> 8;
	streamid[7] = (ctx->uid & 0x00ff);

	for (i = 0; i < AVTP_STREAMID_SIZE; i++)
		tmp = (tmp << 8) + streamid[i];

	snprintf(streamid_str, MAXSTR, "%016" SCNx64 "", tmp);

	tmp = 0;
	for (i = 0; i < ETH_ALEN; i++)
		tmp = (tmp << 8) + ctx->dest_addr[i];
	snprintf(destaddr_str, MAXSTR, "%012" SCNx64 "", tmp);

	snprintf(cmd, MAXSTR,
		"%s -m 0 -S %s -A %s -Z %" PRIu64 " -I %" PRIu64 " -L %" PRIu64 " -v",
		MRPDUMMY_CMD,
		streamid_str,
		destaddr_str,
		ctx->maxframesize,
		ctx->frameintervals,
		ctx->latency);

	ctx->mrpdummy_pid = exe_command(cmd, fd_r, NULL);
	if (ctx->mrpdummy_pid < 0) {
		fprintf(stderr, "failed to execute mrpdummy command\n");
		return -1;
	}

	PRINTF1("execute mrpdummy pid:[%d]\n", ctx->mrpdummy_pid);

	return 0;
}

static int stream_reservation_for_listener(struct app_context *ctx, int *fd_r)
{
	int i = 0;
	uint64_t tmp = 0;
	char streamid_str[MAXSTR] = {'\0'};
	char cmd[MAXSTR] = {'\0'};

	for (i = 0; i < AVTP_STREAMID_SIZE; i++)
		tmp = (tmp << 8) + ctx->streamid[i];
	snprintf(streamid_str, MAXSTR, "%016" SCNx64 "", tmp);

	snprintf(cmd, MAXSTR, "%s -m 1 -S %s -v",
		MRPDUMMY_CMD,
		streamid_str);

	ctx->mrpdummy_pid = exe_command(cmd, fd_r, NULL);
	if (ctx->mrpdummy_pid < 0) {
		fprintf(stderr, "failed to execute mrpdummy command\n");
		return -1;
	}

	PRINTF1("execute mrpdummy pid:[%d]\n", ctx->mrpdummy_pid);

	return 0;
}


static int start_stream_reservation(struct app_context *ctx)
{

	int rc = 0;
	int fd_r;
	char buffer[1024];

	if (ctx->operating_mode == TALKER_MODE)
		rc = stream_reservation_for_talker(ctx, &fd_r);
	else
		rc = stream_reservation_for_listener(ctx, &fd_r);

	if (rc < 0) {
		fprintf(stderr, "failed to stream reservation.\n");
		return -1;
	}

	PRINTF("waiting for stream reservation\n");

	while (!sigint) {
		rc = read(fd_r, buffer, sizeof(buffer));
		if (rc < 0) {
			perror("read");
			break;
		}

		if (strstr(buffer, "reservation completion") != NULL) {
			PRINTF("completed stream reservation\n");
			return 0;
		}
	}

	kill(ctx->mrpdummy_pid, SIGINT);
	wait_child_process(ctx->mrpdummy_pid);

	fprintf(stderr, "cancel stream reservation\n");

	return -1;
}

static int stop_stream_reservation(pid_t pid)
{
	int rc;
	/* sending SIGINT signal to mrpdummy */
	rc = kill(pid, SIGINT);
	if (rc < 0) {
		perror("kill");
		return -1;
	}

	rc = wait_child_process(pid);

	return 0;
}

static int start_application(struct app_context *ctx)
{
	int rc;

	rc = convert_app_cmd(ctx);
	if (rc < 0) {
		fprintf(stderr, "%s: convert_app_cmd is error\n",
			ctx->ini_name);
		return -1;
	}

	return exe_command(ctx->app_cmd, NULL, NULL);
}

static int start_avdecc_process(struct app_context *ctx)
{
	int role = ROLE_TALKER;

	if (ctx->operating_mode == TALKER_MODE)
		role = ROLE_TALKER;
	else
		role = ROLE_LISTENER;

	ctx->avdecc = avdecc_init(ctx->avdecc_entity, ctx->eth_interface, role);
	if (!ctx->avdecc) {
		fprintf(stderr, "failed to create context data of AVDECC\n");
		return -1;
	}

	return 0;
}

static int wait_avdecc_connection(struct app_context *ctx, uint16_t current_configuration, uint16_t unique_id)
{
	int rc = 0;
	
	while(1) {
		if (ctx->operating_mode == TALKER_MODE) {
			rc = avdecc_get_connection_count(ctx->avdecc, current_configuration, unique_id);
		} else {
			rc = avdecc_get_connected_from_listener_stream_info(ctx->avdecc, current_configuration, unique_id);
		}
		if (rc > 0) {
			return 0;
		} else if (rc < 0) {
			fprintf(stderr, "failed to get connection status\n");
			return -1;
		}

		if (sigint) {
			fprintf(stderr, "failed to connect avdecc stream\n");
			return -1;
		}

		usleep(SLEEP_TIME);
	}

	return -1;
}

static int start_avdecc_connection_management(struct app_context *ctx)
{
	int i = 0;
	int rc = 0;
	uint8_t streamid[AVTP_STREAMID_SIZE];
	uint64_t tmp_stream_id = 0;
	uint64_t tmp_dest_addr = 0;
	uint16_t current_configuration = 0;
	uint16_t unique_id = 0;
	current_configuration = avdecc_get_current_configuration(ctx->avdecc);

	if (ctx->operating_mode == TALKER_MODE) {
		memcpy(streamid, ctx->src_addr, ETH_ALEN);
		streamid[6] = (ctx->uid & 0xff00) >> 8;
		streamid[7] = (ctx->uid & 0x00ff);

		for (i = 0; i < AVTP_STREAMID_SIZE; i++)
			tmp_stream_id = (tmp_stream_id << 8) + streamid[i];

		if (ctx->operating_mode == TALKER_MODE) {
			for (i = 0; i < ETH_ALEN; i++)
				tmp_dest_addr = (tmp_dest_addr << 8) + ctx->dest_addr[i];
		}

		rc = avdecc_set_talker_stream_info(ctx->avdecc, current_configuration, unique_id, tmp_stream_id, tmp_dest_addr, 2);
		if (rc < 0) {
			fprintf(stderr, "failed to set talker stream info\n");
			return -1;
		}
	}

	rc = avdecc_acmp_process_start(ctx->avdecc);

	if (rc < 0) {
		fprintf(stderr, "failed to start acmp process\n");
		return -1;
	}

	fprintf(stderr, "waiting connection of avdecc entity\n");
	fprintf(stderr, "please send acmp command from controller\n");

	rc = wait_avdecc_connection(ctx, current_configuration, unique_id);
	if (rc < 0) {
		fprintf(stderr, "failed to connect other avdecc entity\n");
		return -1;
	}

	if (ctx->operating_mode == LISTENER_MODE) {
		rc = avdecc_get_stream_id_from_listener_stream_info(ctx->avdecc, current_configuration, unique_id, ctx->streamid);
		if (rc < 0) {
			fprintf(stderr, "failed to get stream_id\n");
			return -1;
		}
		PRINTF2("avdecc get talker streamid : %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\n",
			ctx->streamid[0], ctx->streamid[1],
			ctx->streamid[2], ctx->streamid[3],
			ctx->streamid[4], ctx->streamid[5],
			ctx->streamid[6], ctx->streamid[7]);
	}

	return 0;
}

static int stop_avdecc_process(struct app_context *ctx)
{
	int rc = 0;

	rc = avdecc_terminate(ctx->avdecc);
	if (rc < 0) {
		fprintf(stderr, "failed to terminate avdecc process\n");
		return -1;
	}

	return 0;
}

static void streaming_thread(void *arg)
{
	int rc;
	bool reserved_stream = false;
	struct app_context *ctx = (struct app_context *)arg;

	if (ctx->use_avdecc) {
		rc = start_avdecc_process(ctx);
		if (rc < 0)
			goto end;
	}

	if (ctx->use_gptp) {
		rc = wait_ascapable(ctx);
		if (rc < 0)
			goto end;
	}

	if (ctx->operating_mode == TALKER_MODE) {
		if (ctx->use_maap) {
			rc = wait_maap_addr(ctx);
			if (rc < 0)
				goto end;
		}
	}

	if (ctx->use_avdecc) {
		rc = start_avdecc_connection_management(ctx);
		if (rc < 0)
			goto avdecc_end;
		fprintf(stderr, "success to connect avdecc stream\n");
	}

	if (ctx->operating_mode != TALKER_MODE) {
		ctx->app_pid = start_application(ctx);
		if (ctx->app_pid < 0) {
			fprintf(stderr, "failed to execute application\n");
			goto end;
		}
	}

	if (ctx->use_srp) {
		rc = start_stream_reservation(ctx);
		if (rc < 0)
			goto end;
		reserved_stream = true;
	}

	if (ctx->operating_mode == TALKER_MODE) {
		ctx->app_pid = start_application(ctx);
		if (ctx->app_pid < 0)
			fprintf(stderr, "failed to execute application\n");
	}

	if (ctx->app_pid > 0) {
		PRINTF("Waiting for termination of App.(pid:%d)\n",
			ctx->app_pid);
		wait_child_process(ctx->app_pid);
		PRINTF("Application has ended.(pid:%d)\n", ctx->app_pid);
	}

	if (reserved_stream) {
		rc = stop_stream_reservation(ctx->mrpdummy_pid);
		if (rc < 0)
			fprintf(stderr, "failed to stop mrpdummy\n");
	}
avdecc_end:
	if (ctx->use_avdecc) {
		rc = stop_avdecc_process(ctx);
		if (rc < 0)
			goto end;
		fprintf(stderr, "success to terminate avdecc process\n");
	}

end:
	pthread_exit(NULL);
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

static int check_and_update_ctx_data(
	struct app_context *ctx,
	bool srp_daemon_started,
	uint8_t *src_addr,
	char *daemon_cl_shm_addr,
	char *maap_daemon_shm_addr)
{
	int rc = 0;

	if (ctx->use_gptp && !daemon_cl_shm_addr) {
		fprintf(stderr,
			"This context(%s) require gptp daemon but is not running.\n",
			ctx->ini_name);
		ctx->err_flag = 1;
	}

	if (ctx->use_srp == true && !srp_daemon_started) {
		fprintf(stderr,
			"This context(%s) require srp daemon but is not running.\n",
			ctx->ini_name);
		ctx->err_flag = 1;
	}

	if (ctx->use_maap &&
	    ctx->operating_mode == TALKER_MODE &&
	    !maap_daemon_shm_addr) {
		fprintf(stderr,
			"This context(%s) require maap daemon but is not running.\n",
			ctx->ini_name);
		ctx->err_flag = 1;
	}

	rc = context_error_check(ctx);
	if (rc < 0) {
		fprintf(stderr, "%s: context_error_check is error\n",
			ctx->ini_name);
		ctx->err_flag = 1;
	}

	memcpy(ctx->src_addr, src_addr, ETH_ALEN);
	ctx->gptp_shm_addr = daemon_cl_shm_addr;
	ctx->maap_shm_addr = maap_daemon_shm_addr;

	if (ctx->err_flag == 1)
		return -1;

	return 0;
}

int main(int argc, char *argv[])
{
	int i = 0;
	int rc = 0;
	int ctx_num = 0;
	int c;
	int daemon_cl_shm_fd = -1;
	int daemon_cl_retry_num = 0;
	char *daemon_cl_shm_addr = NULL;
	bool srp_daemon_started = false;
	int maap_daemon_retry_num = 0;
	char *maap_daemon_shm_addr = NULL;
	char *iname = NULL;
	struct app_context *ctx = NULL;
	struct connect_info *c_info = NULL;
	char cmd_option[MAXSTR] = {'\0'};

	bool opt_g = false;
	char *gptp_config_file = NULL;
	bool opt_m  = false;
	bool opt_a  = false;
	uint8_t src_addr[ETH_ALEN];

	install_sighandler(SIGINT, sigint_handler);

	/* process of command line option */
	while (EOF != (c = getopt(argc, argv, "g:mhai:"))) {
		switch (c) {
		case 'g':
			opt_g = true;
			gptp_config_file = strdup(optarg);
			break;
		case 'm':
			opt_m = true;
			break;
		case 'a':
			opt_a = true;
			break;
		case 'i':
			iname = strdup(optarg);
			break;
		case 'h':
		default:
			show_usage();
			exit(1);
		}
	}

	if (!iname)
		iname = strdup("eth0");

	if (!gptp_config_file)
		gptp_config_file = strdup("gptp_cfg.ini");

	if (netif_detect(iname) < 0) {
		fprintf(stderr, "could not found network interface\n");
		return -1;
	}

	if (netif_gethwaddr(iname, src_addr) < 0) {
		fprintf(stderr, "could not get hw address.\n");
		return -1;
	}

	/* run daemon_cl & open gptp data */
	snprintf(cmd_option, MAXSTR, "%s -S -F %s", iname, gptp_config_file);
	if (start_daemon(GPTP_DAEMON_CMD, cmd_option, opt_g) == 0) {
		while (open_gptp_data(DAEMON_CL_SHM_NAME, &daemon_cl_shm_fd,
					&daemon_cl_shm_addr) < 0) {
			daemon_cl_retry_num++;
			if (daemon_cl_retry_num == DAEMON_CL_MAX_RETRY_NUM) {
				fprintf(stderr,
					"could not open shared memory of daemon_cl.\n");
				return -1;
			}
			usleep(SLEEP_TIME);
			fprintf(stderr, "retry to open gptp data\n");
		}
		fprintf(stderr, "opened gptp data.\n");
	}

	/* run mrpd */
	snprintf(cmd_option, MAXSTR, "-i %s -mvs", iname);
	if (start_daemon(SRP_DAEMON_CMD, cmd_option, opt_m) == 0)
		srp_daemon_started = true;

	/* run maap_daemon */
	snprintf(cmd_option, MAXSTR, "-i %s", iname);
	if (start_daemon(MAAP_DAEMON_CMD, cmd_option, opt_a) == 0) {
		while (open_maap_data(&maap_daemon_shm_addr)) {
			maap_daemon_retry_num++;
			if (maap_daemon_retry_num ==
			    MAAP_DAEMON_MAX_RETRY_NUM) {
				fprintf(stderr,
					"could not open shared memory of maap_daemon.\n");
				return -1;
			}
			usleep(SLEEP_TIME);
			fprintf(stderr, "retry to open maap data\n");
		}
		fprintf(stderr, "open maap data.\n");
	}

	/* create context */
	argc -= optind;
	argv += optind;

	ctx = (struct app_context *)calloc(argc,
				sizeof(struct app_context));
	if (!ctx) {
		fprintf(stderr, "could not allocate ctx data area\n");
		return -1;
	}

	ctx_num = create_contexts(argc, argv, ctx);
	if (ctx_num < 0) {
		fprintf(stderr, "could not create any context.");
		return -1;
	}

	/* updating and error checking of a context */
	for (i = 0; i < ctx_num; i++) {
		strcpy(ctx[i].eth_interface, iname);
		rc = check_and_update_ctx_data(
				&ctx[i],
				srp_daemon_started,
				src_addr,
				daemon_cl_shm_addr,
				maap_daemon_shm_addr);
		if (rc < 0)
			fprintf(stderr, "%s included the error.\n",
				ctx[i].ini_name);
	}

	for (i = 0; i < ctx_num; i++)
		print_context(&ctx[i]);

	/* join vlan and srp domain */
	c_info = connect_avb_network(ctx, ctx_num, c_info);

	/* streaming */
	for (i = 0; i < ctx_num; i++) {
		if (!ctx[i].err_flag) {
			pthread_create(&ctx[i].thread, NULL,
					(void *)streaming_thread,
					(void *)&ctx[i]);
		} else {
			fprintf(stderr,
				"There is an error in the context(%s). Do not process\n",
				ctx[i].ini_name);
		}
	}

	for (i = 0; i < ctx_num; i++)
		if (!ctx[i].err_flag)
			pthread_join(ctx[i].thread, NULL);

	/* close gptp data */
	if (daemon_cl_shm_fd != -1)
		close_gptp_data(daemon_cl_shm_fd, daemon_cl_shm_addr);

	/* close maap data */
	close_maap_data(maap_daemon_shm_addr);

	/* leave vlan and srp domain */
	disconnect_avb_network(c_info);

	/* release context */
	free(ctx);
	free(iname);
	free(gptp_config_file);

	return 0;
}
