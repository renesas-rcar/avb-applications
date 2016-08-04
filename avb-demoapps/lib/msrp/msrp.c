/******************************************************************************

  Copyright (c) 2012, Intel Corporation
  Copyright (C) 2014-2016, Renesas Electronics Corporation
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:

   1. Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.

   3. Neither the name of the Intel Corporation nor the names of its
      contributors may be used to endorse or promote products derived from
      this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE.

******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>
#include <inttypes.h>

#include "msrp.h"

#define LIBVERSION "0.4"

/*
 * internal functions
 */
static int monitor_listener_add(
	struct msrp_ctx *ctx, struct mrpdhelper_notify *n)
{
	int i;

	if (ctx->prop->streamid != n->u.sl.id)
		return -1;

	for (i = 0; i < MSRP_MAX_STREAMS; i++) {
		if (ctx->devices[i].attached &&
		    (ctx->devices[i].address == n->registrar)) {
			return -1;
		}
	}
	for (i = 0; i < MSRP_MAX_STREAMS; i++) {
		if (!ctx->devices[i].attached) {
			ctx->devices[i].attached = true;
			ctx->devices[i].address = n->registrar;
			return 0;
		}
	}

	DEBUG_PRINTF(
		"[MRP] The number of listener has exceeded limit(MSRP_MAX_STREAMS:%d).\n",
		MSRP_MAX_STREAMS);
	return -1;
}

static int monitor_listener_remove(
	struct msrp_ctx *ctx, struct mrpdhelper_notify *n)
{
	int i;

	if (ctx->prop->streamid == n->u.sl.id) {
		for (i = 0; i < MSRP_MAX_STREAMS; i++) {
			if (ctx->devices[i].attached &&
				(ctx->devices[i].address == n->registrar)) {
				ctx->devices[i].attached = false;
				ctx->devices[i].address = 0;
				return 0;
			}
		}
	}

	return -1;
}

static int listener_attribute_process
			(struct msrp_ctx *ctx, struct mrpdhelper_notify *n)
{
	if (ctx == NULL) {
		fprintf(stderr, "[MRP] ctx is NULL. recv listener msg\n");
		return -1;
	}

	if (ctx->prop == NULL) {
		fprintf(stderr, "[MRP] property is NULL. recv listener msg\n");
		return -1;
	}

	if (n == NULL) {
		fprintf(stderr, "[MRP] notify is NULL. recv listener msg\n");
		return -1;
	}

	DEBUG_PRINTF("[MRP] StreamID=%016" SCNx64, n->u.sl.id);

	switch (n->u.sl.substate) {
	case mrpdhelper_listener_declaration_type_ignore:
		DEBUG_PRINTF(" with state ignore\n");
		break;
	case mrpdhelper_listener_declaration_type_asking_failed:
		DEBUG_PRINTF(" with state askfailed\n");
		break;
	case mrpdhelper_listener_declaration_type_ready:
		DEBUG_PRINTF(" with state ready\n");
		break;
	case mrpdhelper_listener_declaration_type_ready_failed:
		DEBUG_PRINTF(" with state readyfail\n");
		break;
	default:
		DEBUG_PRINTF(" with state UNKNOWN (%d\n", n->u.sl.substate);
		break;
	}

	switch (n->notify) {
	case mrpdhelper_notification_leave:
		DEBUG_PRINTF("[MRP] listener leave StreamID=%016"
				SCNx64 "ctx StreamID=%016" SCNx64 "\n",
				n->u.sl.id, ctx->prop->streamid);
		if (monitor_listener_remove(ctx, n) == 0)
			ctx->listeners--;
		break;
	case mrpdhelper_notification_new:
	case mrpdhelper_notification_join:
	default:     /* case mrpdhelper_notification_null */
		DEBUG_PRINTF("[MRP] listener new/join/query StreamID=%016"
				SCNx64 " ctx StreamID=%016" SCNx64 "\n",
				n->u.sl.id, ctx->prop->streamid);
		if (n->u.sl.substate >
			mrpdhelper_listener_declaration_type_asking_failed) {
			if (monitor_listener_add(ctx, n) == 0)
				ctx->listeners++;
		}
		break;
	}
	fflush(stdout);
	return 0;
}

static int talker_attribute_process
			(struct msrp_ctx *ctx, struct mrpdhelper_notify *n)
{
	if (ctx == NULL) {
		fprintf(stderr, "[MRP] ctx is NULL. recv talker msg\n");
		return -1;
	}

	if (ctx->prop == NULL) {
		fprintf(stderr, "[MRP] property is NULL. recv talker msg\n");
		return -1;
	}

	if (n == NULL) {
		fprintf(stderr, "[MRP] notify is NULL. recv talker msg\n");
		return -1;
	}

	DEBUG_PRINTF("[MRP] StreamID=%016" SCNx64 "\n", n->u.st.id);

	switch (n->notify) {
	case mrpdhelper_notification_leave:
		DEBUG_PRINTF("[MRP] talker leave StreamID=%016"
				SCNx64 " ctx StreamID=%016" SCNx64 "\n",
				n->u.st.id, ctx->prop->streamid);
		break;
	case mrpdhelper_notification_new:
	case mrpdhelper_notification_join:
		DEBUG_PRINTF("[MRP] talker new/join/query StreamID=%016"
				SCNx64 " ctx StreamID=%016" SCNx64 "\n",
				n->u.st.id, ctx->prop->streamid);
		if (ctx->prop->streamid == n->u.st.id)
			ctx->talker_found = true;
		break;
	default:
		DEBUG_PRINTF("[MRP] unknown notify is %d\n", n->notify);
		break;
	}
	fflush(stdout);
	return 0;
}

static int msg_process(struct msrp_ctx *ctx, char *buf, int buflen)
{
	int rc = 0;
	struct mrpdhelper_notify n;

	if (ctx == NULL) {
		fprintf(stderr, "[MRP] ctx is NULL. recv msg parse\n");
		return -1;
	}

	if (buf == NULL) {
		fprintf(stderr, "[MRP] massage buf is NULL. recv msg parse\n");
		return -1;
	}

	rc = mrpdhelper_parse_notification(buf, buflen, &n);
	if (rc == 0) {
		switch (n.attrib) {
		case mrpdhelper_attribtype_msrp_listener:
			rc = listener_attribute_process(ctx, &n);
			break;
		case mrpdhelper_attribtype_msrp_talker:
		case mrpdhelper_attribtype_msrp_talker_fail:
			rc = talker_attribute_process(ctx, &n);
			break;
		case mrpdhelper_attribtype_mmrp:
		case mrpdhelper_attribtype_mvrp:
		case mrpdhelper_attribtype_msrp_domain:
		case mrpdhelper_attribtype_msrp_listener_fail:
			DEBUG_PRINTF("[MRP] message from mrpd: %s", buf);
			fflush(stdout);
			break;
		default:
			DEBUG_PRINTF("[MRP] unknown attribute %d\n", n.attrib);
			break;
		}
	}
	free(buf);
	return rc;
}

static void *msrp_monitor_thread(void *arg)
{
	struct msrp_ctx *ctx = NULL;
	char *msgbuf;
	int rc;

	if (arg == NULL) {
		fprintf(stderr, "[MRP] ctx is NULL. thread start\n");
		pthread_exit(NULL);
	}

	ctx = (struct msrp_ctx *)arg;

	if (ctx->mrpd_sock == SOCKET_ERROR) {
		fprintf(stderr, "[MRP] socket is NULL. thread start\n");
		pthread_exit(NULL);
	}

	DEBUG_PRINTF("[MRP] monitor thread start\n");

	while (!ctx->halt_flag) {
		msgbuf = (char *)calloc(1, MRPDCLIENT_MAX_MSG_SIZE);
		if (msgbuf == NULL) {
			fprintf(stderr, "[MRP] massage buf is NULL. recv\n");
			pthread_exit(NULL);
		}
		rc = recv(ctx->mrpd_sock, msgbuf, MRPDCLIENT_MAX_MSG_SIZE, 0);

		if (rc < 0) {
			if (errno == EAGAIN) {
				free(msgbuf);
				continue;
				/* the timeout expired before data was received. */
			} else {
				perror("[MRP] recv");
				free(msgbuf);
				break;
			}
		}
		msg_process(ctx, msgbuf, rc);
	}

	DEBUG_PRINTF("[MRP] monitor thread halted\n");
	pthread_exit(NULL);
}

static int msrp_monitor(struct msrp_ctx *ctx)
{
	if (ctx == NULL) {
		fprintf(stderr, "[MRP] ctx is NULL. thread create\n");
		return -1;
	}

	return pthread_create(&ctx->monitor_thread, NULL,
			msrp_monitor_thread, (void *)ctx);
}

static int mrp_send(struct msrp_ctx *ctx, const char *format, ...)
{
	va_list arg;
	int rc;
	int len;

	if (ctx == NULL) {
		fprintf(stderr, "[MRP] ctx is NULL. msg send\n");
		return -1;
	}

	if (ctx->msgbuf == NULL) {
		fprintf(stderr, "[MRP] ctx msgbuf is NULL. msg send\n");
		return -1;
	}

	if (ctx->mrpd_sock == SOCKET_ERROR) {
		fprintf(stderr, "[MRP] socket is NULL. msg send\n");
		return -1;
	}

	va_start(arg, format);
	len = vsprintf(ctx->msgbuf, format, arg);
	va_end(arg);

	if (ctx->prop->verbose)
		fprintf(stdout, "[MRP] send message -> %s\n", ctx->msgbuf);
	rc = mrpdclient_sendto(ctx->mrpd_sock, ctx->msgbuf, len + 1);

	if (rc == -1)
		perror("sendto");

	return rc;
}

/*
 * public functions
 */
struct msrp_ctx *msrp_ctx_init(struct mrp_property *prop)
{
	struct msrp_ctx *ctx;

	if (prop == NULL) {
		fprintf(stderr, "[MRP] prop is NULL. ctx init\n");
		return NULL;
	}

	ctx = calloc(1, sizeof(struct msrp_ctx));
	if (ctx == NULL) {
		fprintf(stderr, "[MRP] failed to allocate context.\n");
		return NULL;
	}

	ctx->prop = calloc(1, sizeof(struct mrp_property));
	if (ctx->prop == NULL) {
		free(ctx);
		fprintf(stderr, "[MRP] could not allocate prop in context.\n");
		return NULL;
	}

	ctx->msgbuf = calloc(1, MRPDCLIENT_MAX_MSG_SIZE);
	if (ctx->msgbuf == NULL) {
		free(ctx->prop);
		free(ctx);
		fprintf(stderr, "[MRP] could not allocate msgbuf in context.\n");
		return NULL;
	}

	ctx->mrpd_sock = mrpdclient_init();
	if (ctx->mrpd_sock == SOCKET_ERROR) {
		free(ctx->msgbuf);
		free(ctx->prop);
		free(ctx);
		fprintf(stderr, "[MRP] could not create socket in context.\n");
		return NULL;
	}

	ctx->prop->verbose  = prop->verbose;
	ctx->prop->streamid = prop->streamid;
	ctx->prop->destaddr = prop->destaddr;
	ctx->prop->vlan     = prop->vlan;
	ctx->prop->MaxFrameSize      = prop->MaxFrameSize;
	ctx->prop->MaxIntervalFrames = prop->MaxIntervalFrames;
	ctx->prop->priority = prop->priority;
	ctx->prop->rank     = prop->rank;
	ctx->prop->latency  = prop->latency;
	ctx->prop->class    = prop->class;

	ctx->talker_found = false;
	ctx->listeners  = 0;

	ctx->halt_flag = false;

	if (msrp_monitor(ctx)) {
		mrpdclient_close(&ctx->mrpd_sock);
		free(ctx->msgbuf);
		free(ctx->prop);
		free(ctx);
		fprintf(stderr, "[MRP] could not create thread.\n");
		return NULL;
	}

	return ctx;
}

int msrp_ctx_destroy(struct msrp_ctx *ctx)
{
	int rc = 0;

	if (ctx == NULL) {
		fprintf(stderr, "[MRP] ctx is NULL. ctx destroy\n");
		return -1;
	}

	if (ctx->monitor_thread != 0) {
		ctx->halt_flag = true;
		pthread_join(ctx->monitor_thread, NULL);
	}
	if (ctx->mrpd_sock != SOCKET_ERROR) {
		rc = mrpdclient_close(&ctx->mrpd_sock);
		if (rc < 0)
			fprintf(stderr, "[MRP] could not close socket.\n");
	}
	if (ctx->prop != NULL)
		free(ctx->prop);
	if (ctx->msgbuf != NULL)
		free(ctx->msgbuf);
	free(ctx);

	return rc;
}

bool msrp_exist_talker(struct msrp_ctx *ctx)
{
	if (ctx == NULL) {
		fprintf(stderr, "[MRP] ctx is NULL. exist talker\n");
		return 0;
	}

	return ctx->talker_found;
}

int msrp_exist_listener(struct msrp_ctx *ctx)
{
	if (ctx == NULL) {
		fprintf(stderr, "[MRP] ctx is NULL. exist listener\n");
		return 0;
	}

	return ctx->listeners;
}

int msrp_talker_advertise(struct msrp_ctx *ctx)
{
	if (ctx == NULL) {
		fprintf(stderr, "[MRP] ctx is NULL. talker advertise\n");
		return -1;
	}

	if (ctx->prop == NULL) {
		fprintf(stderr, "[MRP] property is NULL. talker advertise\n");
		return -1;
	}

	return mrp_send(ctx,
			"S++:S=%016" SCNx64
			",A=%012" SCNx64
			",V=%04X,Z=%d,I=%d,P=%d,L=%u",
			ctx->prop->streamid,
			ctx->prop->destaddr,
			ctx->prop->vlan,
			ctx->prop->MaxFrameSize,
			ctx->prop->MaxIntervalFrames,
			(ctx->prop->priority << 5) | (ctx->prop->rank << 4),
			ctx->prop->latency);
}

int msrp_talker_unadvertise(struct msrp_ctx *ctx)
{
	if (ctx == NULL) {
		fprintf(stderr, "[MRP] ctx is NULL. talker unadvertise\n");
		return -1;
	}

	if (ctx->prop == NULL) {
		fprintf(stderr, "[MRP] property is NULL. talker unadvertise\n");
		return -1;
	}

	return mrp_send(ctx,
			"S--:S=%016" SCNx64,
			ctx->prop->streamid);
}

int mvrp_join_vlan(struct msrp_ctx *ctx)
{
	if (ctx == NULL) {
		fprintf(stderr, "[MRP] ctx is NULL. join vlan\n");
		return -1;
	}

	if (ctx->prop == NULL) {
		fprintf(stderr, "[MRP] property is NULL. join vlan\n");
		return -1;
	}

	return mrp_send(ctx,
			"V+?:I=%04x",
			ctx->prop->vlan);
}

int mvrp_leave_vlan(struct msrp_ctx *ctx)
{
	if (ctx == NULL) {
		fprintf(stderr, "[MRP] ctx is NULL. leave vlan\n");
		return -1;
	}

	if (ctx->prop == NULL) {
		fprintf(stderr, "[MRP] property is NULL. leave vlan\n");
		return -1;
	}

	return mrp_send(ctx,
			"V--:I=%04x",
			ctx->prop->vlan);
}

int msrp_register_domain(struct msrp_ctx *ctx)
{
	if (ctx == NULL) {
		fprintf(stderr, "[MRP] ctx is NULL. register domain\n");
		return -1;
	}

	if (ctx->prop == NULL) {
		fprintf(stderr, "[MRP] property is NULL. register domain\n");
		return -1;
	}

	return mrp_send(ctx,
			"S+D:C=%d,P=%d,V=%04x",
			ctx->prop->class,
			ctx->prop->priority,
			ctx->prop->vlan);
}

int msrp_unregister_domain(struct msrp_ctx *ctx)
{
	if (ctx == NULL) {
		fprintf(stderr, "[MRP] ctx is NULL. unregister domain\n");
		return -1;
	}

	if (ctx->prop == NULL) {
		fprintf(stderr, "[MRP] property is NULL. unregister domain\n");
		return -1;
	}

	return mrp_send(ctx,
			"S-D:C=%d,P=%d,V=%04x",
			ctx->prop->class,
			ctx->prop->priority,
			ctx->prop->vlan);
}

int msrp_listener_ready(struct msrp_ctx *ctx)
{
	if (ctx == NULL) {
		fprintf(stderr, "[MRP] ctx is NULL. listener ready\n");
		return -1;
	}

	if (ctx->prop == NULL) {
		fprintf(stderr, "[MRP] property is NULL. listener ready\n");
		return -1;
	}

	return mrp_send(ctx,
			"S+L:L=%016" SCNx64
			",D=%d",
			ctx->prop->streamid,
			MSRP_LISTENER_READY);
}

int msrp_listener_leave(struct msrp_ctx *ctx)
{
	if (ctx == NULL) {
		fprintf(stderr, "[MRP] ctx is NULL. listener leave\n");
		return -1;
	}

	if (ctx->prop == NULL) {
		fprintf(stderr, "[MRP] property is NULL. listener leave\n");
		return -1;
	}

	return mrp_send(ctx,
			"S-L:L=%016" SCNx64,
			ctx->prop->streamid);
}

int msrp_query_database(struct msrp_ctx *ctx)
{
	if (ctx == NULL) {
		fprintf(stderr, "[MRP] ctx is NULL. query database\n");
		return -1;
	}

	return mrp_send(ctx, "S??");
}
