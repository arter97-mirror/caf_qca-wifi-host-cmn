/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: ISC
 */

/**
 * DOC: This file contains chipset stats implementstion
 */

#ifndef __WLAN_CP_STATS_CHIPSET_STATS__
#define __WLAN_CP_STATS_CHIPSET_STATS__

#include <wlan_cmn.h>
#include <qdf_status.h>
#include <qdf_trace.h>
#include "wlan_cp_stats_chipset_stats_events.h"

#define MAX_CSTATS_NODE_LENGTH 2048
#define HOST_TYPE_NODE_COUNT 50
#define FW_TYPE_NODE_COUNT 460

#define MAX_CSTATS_VERSION_BUFF_LENGTH 100

#define CSTATS_QMI_EVENT_TYPE 1

#define ANI_NL_MSG_CSTATS_HOST_LOG_TYPE 110
#define ANI_NL_MSG_CSTATS_FW_LOG_TYPE 111

#define CSTATS_MARKER_SZ 6
#define CSTATS_HOST_START_MARKER "CS_HSM"
#define CSTATS_HOST_END_MARKER "CS_HEM"
#define CSTATS_FW_START_MARKER "CS_FSM"
#define CSTATS_FW_END_MARKER  "CS_FEM"

#ifdef QDF_LITTLE_ENDIAN_MACHINE
#define CHIPSET_STATS_MACHINE_ENDIANNESS (0)
#else
#define CHIPSET_STATS_MACHINE_ENDIANNESS (1)
#endif

#define CSTATS_SET_BIT(value, mask) ((value) |= (1 << (mask)))

#define CSTATS_MAC_COPY(to, from) \
	do {\
		to[0] = from[0]; \
		to[1] = from[1]; \
		to[2] = from[2]; \
		to[3] = from[5]; \
	} while (0)

/**
 * enum cstats_types - Types of chipset stats
 * @CSTATS_HOST_TYPE : Host related chipset stats
 * @CSTATS_FW_TYPE : Firmware related chipset stats
 * @CSTATS_MAX_TYPE : Invalid
 */
enum cstats_types {
	CSTATS_HOST_TYPE,
	CSTATS_FW_TYPE,
	CSTATS_MAX_TYPE,
};

struct cstats_tx_rx_ops {
	int (*cstats_send_data_to_usr)(char *buff, unsigned int len,
				       enum cstats_types type,
				       bool is_logging_enable);
};

struct cstats_node {
	qdf_list_node_t node;
	unsigned int radio;
	unsigned int index;
	unsigned int filled_length;
	char logbuf[MAX_CSTATS_NODE_LENGTH];
};

/**
 * struct chipset_stats - Structure to manage chipset statistics logging
 * @cstat_free_list: Array of free list queues for each chipset stats type
 * @cstat_filled_list: Array of filled list queues for each chipset stats type
 * @cstats_lock: Spinlocks to synchronize access to each stats type's resources
 * @ccur_node: Current active node for each chipset stats type
 * @cstat_drop_cnt: Counter for dropped stats entries per type
 * @chipset_stats_push_rbs_delay_val_ms: Delay value (in ms) for pushing stats
 * to RBS
 * @chipset_stats_push_rbs_delay_interval: Interval for pushing stats to RBS
 * @cstats_no_flush: Flag to prevent moving filled nodes to free list after
 * flush
 * @ops: Operations structure for TX/RX callbacks related to chipset stats
 * @is_cstats_ini_enabled: Flag indicating if chipset stats logging is enabled
 * via INI
 * @is_cp_stats_debug_logging_enable: Flag to enable debug logging for CP stats
 * @is_direct_log_dispatch_enabled: Flag to enable direct dispatch of logs to
 * user space
 */
struct chipset_stats {
	qdf_list_t cstat_free_list[CSTATS_MAX_TYPE];
	qdf_list_t cstat_filled_list[CSTATS_MAX_TYPE];
	qdf_spinlock_t cstats_lock[CSTATS_MAX_TYPE];
	struct cstats_node *ccur_node[CSTATS_MAX_TYPE];
	unsigned int cstat_drop_cnt[CSTATS_MAX_TYPE];
	size_t chipset_stats_push_rbs_delay_val_ms;
	size_t chipset_stats_push_rbs_delay_interval;
	bool cstats_no_flush[CSTATS_MAX_TYPE];
	struct cstats_tx_rx_ops ops;
	bool is_cstats_ini_enabled;
	bool is_cp_stats_debug_logging_enable;
	bool is_direct_log_dispatch_enabled;
};

#define wlan_cstats_fw_stats(len, buf) \
	wlan_cp_stats_fw_log_event_dispatcher(CSTATS_FW_TYPE, buf, len);

#define wlan_cstats_host_stats(len, buf) \
	wlan_cp_stats_cstats_write_to_buff(CSTATS_HOST_TYPE, buf, len)

#ifdef WLAN_CHIPSET_STATS
/**
 * wlan_cp_stats_cstats_init() - Initialize chipset stats infra
 *
 * @psoc: pointer to psoc object
 *
 * Return: QDF_STATUS
 */
QDF_STATUS wlan_cp_stats_cstats_init(struct wlan_objmgr_psoc *psoc);

/**
 * wlan_cp_stats_enable_direct_log_dispatch - API to set chipset logging
 * dispatch flag
 * @psoc: Pointer to the psoc object
 * @direct_log_dispatch: Boolean flag to enable or disable direct log dispatch
 *
 * This function sets the internal flag that controls whether enhanced
 * chipset logging is enabled. It is the final handler that updates the
 * CP stats configuration.
 *
 * Return: None
 */
void wlan_cp_stats_enable_direct_log_dispatch(struct wlan_objmgr_psoc *psoc,
					      bool direct_log_dispatch);

/**
 * wlan_cp_stats_cstats_deinit() - Deinitialize chipset stats infra
 *
 * Return: void
 */
void wlan_cp_stats_cstats_deinit(void);

/**
 * wlan_cp_stats_cstats_register_tx_rx_ops() - Register chipset stats ops
 *
 * @ops : tx rx ops
 *
 * Return: void
 */
void wlan_cp_stats_cstats_register_tx_rx_ops(struct cstats_tx_rx_ops *ops);

/**
 * wlan_cp_stats_cstats_write_to_buff() - Write cstats event to buffer
 * @type: CSTATS type identifier (e.g., HOST or FW)
 * @to_be_sent: Pointer to the event payload
 * @length: Length of the event payload
 *
 * Writes chipset statistics (cstats) event data to the appropriate buffer
 * based on the event type. For firmware events, start and end markers are
 * added around the payload. For host events, only the payload is copied,
 * and markers are added during flush to userspace.
 *
 * If the current buffer node lacks sufficient space, a new buffer node is
 * allocated. Optionally logs the event timestamp if debug logging is enabled.
 *
 * Return: void
 */
void wlan_cp_stats_cstats_write_to_buff(enum cstats_types type,
					void *to_be_sent, uint32_t length);

/**
 * wlan_cp_stats_fw_log_event_dispatcher() - Dispatch FW log event based on flag
 * @type:       CSTATS type identifier
 * @event:      Pointer to FW log event data
 * @event_len:  Length of the FW log event data
 *
 * Dispatches firmware log events either directly to userspace or via
 * buffered write, depending on the is_direct_log_dispatch_enabled flag.
 *
 * Return: void
 */
void wlan_cp_stats_fw_log_event_dispatcher(enum cstats_types type, void *event,
					   int event_len);

/**
 * wlan_cp_stats_fw_log_event_direct_flush() - Send FW log directly to userspace
 * @type: CSTATS type identifier
 * @to_be_sent: Pointer to FW log event data
 * @plen: Length of the FW log event data
 *
 * Logs CSTATS_FW_TYPE entries by packaging them with markers
 * and sending to userspace via callback without buffering logic.
 *
 * Return: void
 */
void
wlan_cp_stats_fw_log_event_direct_flush(enum cstats_types type,
					void *to_be_sent, uint32_t plen);

/**
 * wlan_cp_stats_cstats_send_buffer_to_user() - Flush chipset stats to the
 * middleware
 * @type: Type of chipset stats to be sent
 *
 * Return : 0 on success and errno on failure
 */
int wlan_cp_stats_cstats_send_buffer_to_user(enum cstats_types type);

/*
 * wlan_cp_stats_cstats_pkt_log() - Data packets stats
 * @sa - Source addr
 * @da - Destination addr
 * @pkt_type - Packet type
 * @subtype - Subtype
 * @dir - Direction
 * @status - Status
 * @vdev_id - Vdev iD
 * @op_mode - opmode
 *
 * Return : void
 */
void wlan_cp_stats_cstats_pkt_log(uint8_t *sa, uint8_t *da,
				  enum qdf_proto_type pkt_type,
				  enum qdf_proto_subtype subtype,
				  enum qdf_proto_dir dir,
				  enum qdf_dp_tx_rx_status status,
				  uint8_t vdev_id, enum QDF_OPMODE op_mode);
#else
static inline
void wlan_cp_stats_enable_direct_log_dispatch(struct wlan_objmgr_psoc *psoc,
					      bool direct_log_dispatch)
{
}

static inline QDF_STATUS
wlan_cp_stats_cstats_init(struct wlan_objmgr_psoc *psoc)
{
	return 0;
}

static inline void wlan_cp_stats_cstats_deinit(void)
{
}

static inline void
wlan_cp_stats_cstats_register_tx_rx_ops(struct cstats_tx_rx_ops *ops)
{
}

static inline void
wlan_cp_stats_cstats_write_to_buff(enum cstats_types type, void *to_be_sent,
				   uint32_t length)
{
}

static inline void
wlan_cp_stats_fw_log_event_direct_flush(enum cstats_types type,
					void *to_be_sent, uint32_t plen)
{
}

static inline void
wlan_cp_stats_fw_log_event_dispatcher(enum cstats_types type, void *event,
				      int event_len)
{
}

static inline int
wlan_cp_stats_cstats_send_buffer_to_user(enum cstats_types type)
{
	return 0;
}
#endif /* WLAN_CHIPSET_STATS */
#endif /* __WLAN_CP_STATS_CHIPSET_STATS__ */
