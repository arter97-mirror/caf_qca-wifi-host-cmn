/*
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: ISC
 */

 /**
  * DOC: wlan_ipa_logging.h
  *
  *
  */

#include "qdf_threads.h"
#include "qdf_event.h"
#define MAX_LOG_LENGTH 512
#define WLAN_IPA_MAX_LIST_SIZE 64
#define WLAN_IPA_LOGGING(arg, ...) \
	WLAN_IPA_LOGGING_FUNC(__func__, arg, ##__VA_ARGS__)
#define WLAN_IPA_LOGGING_FUNC wlan_ipa_log_message
#define WLAN_IPA_HOST_MSG_MARKER "OPT_DP_HOST"

#ifdef IPA_OPT_WIFI_DP_LOGGING
/**
 * struct wlan_ipa_log_context - structure holding resources for ipa logging
 * @free_list: free node list which can be used for filling logs
 * @filled_list: filled nodes list having logs to send to upper layer
 * @start_event: handle of Event for ipa log thread to signal startup
 * @shutdown_event: event to signal that the wait queue has been stopped
 * @lock: Lock to synchronize access to shared logging resource
 * @wait_q: Wait queue for Logger thread
 * @thread: logger thread
 * @drop_count: log dropped
 * @event_flag: event flag to post events to logger thread
 * @log_truncation: log truncation indication
 */
struct wlan_ipa_log_context {
	qdf_list_t free_list;
	qdf_list_t filled_list;
	qdf_event_t start_event;
	qdf_event_t shutdown_event;
	qdf_spinlock_t lock;
	qdf_wait_queue_head_t wait_q;
	qdf_thread_t *thread;
	uint16_t drop_count;
	unsigned long event_flag;
	bool log_truncation;
};

/**
 * struct wlan_ipa_log_msg - structure holding log msg
 * @node: filled list node
 * @logbuf: buffer to hold log msg
 */
struct wlan_ipa_log_msg {
	qdf_list_node_t node;
	char logbuf[MAX_LOG_LENGTH];
};

/**
 * wlan_ipa_log_message() - get the logs from all the context
 * and post to logger thread
 * @func: logging function
 * @msg: actual log to send
 */
void wlan_ipa_log_message(const char *func, const char *msg, ...);

/**
 * wlan_ipa_logging_sock_init() - init ipa logging resources
 */
QDF_STATUS wlan_ipa_logging_sock_init(void);

/**
 * wlan_ipa_logging_sock_deinit() - deinit ipa logging resources
 */
void wlan_ipa_logging_sock_deinit(void);

#else
static inline
void wlan_ipa_log_message(const char *func, const char *msg, ...)
{
}

static inline
QDF_STATUS wlan_ipa_logging_sock_init(void)
{
	return QDF_STATUS_SUCCESS;
}

static inline
void wlan_ipa_logging_sock_deinit(void)
{
}
#endif
