/*
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: ISC
 */

 /**
  * DOC: wlan_ipa_logging.c
  *
  *
  */

/* Include Files */
#include <wlan_ipa_main.h>
#include <wlan_ipa_logging.h>
#define WLAN_IPA_THREAD_NAME_MAX 20

#ifdef IPA_OPT_WIFI_DP_LOGGING
struct wlan_ipa_log_context g_ipa_logging_ctx;
static struct wlan_ipa_log_msg *g_ipa_log_msg;

static inline int wlan_ipa_logging_thread(void *arg)
{
	return 0;
}

static inline QDF_STATUS wlan_ipa_allocate_log_msg(void)
{
	int i;

	g_ipa_log_msg = qdf_mem_malloc(WLAN_IPA_MAX_LIST_SIZE *
				       sizeof(struct wlan_ipa_log_msg));
	if (!g_ipa_log_msg)
		return QDF_STATUS_E_NOMEM;

	qdf_spin_lock_bh(&g_ipa_logging_ctx.lock);
	for (i = 0; i < WLAN_IPA_MAX_LIST_SIZE; i++) {
		qdf_list_insert_back(&g_ipa_logging_ctx.free_list,
				     &g_ipa_log_msg[i].node);
	}

	qdf_spin_unlock_bh(&g_ipa_logging_ctx.lock);
	return QDF_STATUS_SUCCESS;
}

QDF_STATUS wlan_ipa_logging_sock_init(void)
{
	char log_thread_name[WLAN_IPA_THREAD_NAME_MAX] = {0};

	qdf_scnprintf(log_thread_name, sizeof(log_thread_name),
		      "ipa_log_thread");
	qdf_list_create(&g_ipa_logging_ctx.free_list,
			WLAN_IPA_MAX_LIST_SIZE);
	qdf_list_create(&g_ipa_logging_ctx.filled_list,
			WLAN_IPA_MAX_LIST_SIZE);
	qdf_spinlock_create(&g_ipa_logging_ctx.lock);
	if (QDF_IS_STATUS_ERROR(wlan_ipa_allocate_log_msg())) {
		ipa_err("Could not allocate memory for log_msg");
		qdf_spinlock_destroy(&g_ipa_logging_ctx.lock);
		return QDF_STATUS_E_FAILURE;
	}

	qdf_event_create(&g_ipa_logging_ctx.start_event);
	qdf_event_create(&g_ipa_logging_ctx.shutdown_event);
	g_ipa_logging_ctx.event_flag = 0;
	qdf_init_waitqueue_head(&g_ipa_logging_ctx.wait_q);
	g_ipa_logging_ctx.thread = qdf_create_thread(wlan_ipa_logging_thread,
						     NULL,
						     log_thread_name);
	if (!g_ipa_logging_ctx.thread) {
		ipa_err("could not create ipa_log_thread");
		qdf_event_destroy(&g_ipa_logging_ctx.start_event);
		qdf_event_destroy(&g_ipa_logging_ctx.shutdown_event);
		qdf_mem_free(g_ipa_log_msg);
		qdf_spinlock_destroy(&g_ipa_logging_ctx.lock);
		return QDF_STATUS_E_FAILURE;
	}

	qdf_wake_up_process(g_ipa_logging_ctx.thread);
	g_ipa_logging_ctx.drop_count = 0;
	return QDF_STATUS_SUCCESS;
}

void wlan_ipa_logging_sock_deinit(void)
{
	qdf_event_destroy(&g_ipa_logging_ctx.shutdown_event);
	qdf_event_destroy(&g_ipa_logging_ctx.start_event);
	qdf_mem_free(g_ipa_log_msg);
	qdf_spinlock_destroy(&g_ipa_logging_ctx.lock);
}

#endif
