/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: ISC
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * DOC: contains MLO manager public file containing link reconfiguration
 * functionality
 */
#ifndef _WLAN_MLO_LINK_RECFG_H_
#define _WLAN_MLO_LINK_RECFG_H_

#include <wlan_mlo_mgr_public_structs.h>
#include <wlan_cm_public_struct.h>

#define WLAN_MAX_ML_RECFG_LINK_COUNT 16
#define WLAN_MIN_DIALOG_TOKEN         1
#define WLAN_MAX_DIALOG_TOKEN         0xFF
#define LINK_RECFG_RSP_TIMEOUT 5000

#ifdef WLAN_FEATURE_11BN_SMD
#define MLO_LINK_RECFG_MAX_SCS_IDS 8
#endif

struct mlo_link_recfg_context;
struct link_recfg_rx_rsp;
struct roam_offload_synch_ind; /* forward declaration for SMD cached copy */

/**
 * enum wlan_link_recfg_sm_state - Link Reconfiguration states
 * @WLAN_LINK_RECFG_S_INIT: Default state, IDLE state
 * @WLAN_LINK_RECFG_S_START: State when Link Reconfig starts
 * @WLAN_LINK_RECFG_S_ADD_LINK: State for Link Add request
 * @WLAN_LINK_RECFG_S_XMIT_REQ: State for Link recfg request frame sending
 * @WLAN_LINK_RECFG_S_DEL_LINK: State for Link Del request
 * @WLAN_LINK_RECFG_S_COMPLETED: State when Link Reconfig is completed
 * @WLAN_LINK_RECFG_S_ABORT: State when Link Reconfig is Aborted
 * @WLAN_LINK_RECFG_S_TTLM: State when Link Reconfig TTLM handling
 * @WLAN_LINK_RECFG_S_WAIT: State when Link Reconfig is waiting
 * @WLAN_LINK_RECFG_S_MAX: Max State
 * @WLAN_LINK_RECFG_SS_IDLE: Link Reconfig substate Idle
 * @WLAN_LINK_RECFG_SS_START_PENDING: Link reconfig start pending for
 * serialization active
 * @WLAN_LINK_RECFG_SS_START_ACTIVE: Link reconfig for serialization active
 * @WLAN_LINK_RECFG_SS_DEL_LINK_WAIT_SET_LINK: Link Reconfig is Del link wait
 * for set link cmd rsp
 * @WLAN_LINK_RECFG_SS_DEL_LINK_WAIT_LINK_SW: Link Reconfig is wait for link
 * switch delete/disconnect
 * @WLAN_LINK_RECFG_SS_DEL_LINK_ABORT_WAIT_SET_LINK: Link Reconfig abort wait
 * for set link cmd rsp
 * @WLAN_LINK_RECFG_SS_DEL_LINK_ABORT_WAIT_LINK_SW: Link Reconfig abort wait
 * for link switch done
 * @WLAN_LINK_RECFG_SS_ADD_LINK_WAIT_ADD_CONN: Link Reconfig is Add link as
 * partner
 * @WLAN_LINK_RECFG_SS_ADD_LINK_WAIT_LINK_SW: Link Reconfig is wait for link
 * switch add/connect
 * @WLAN_LINK_RECFG_SS_ADD_LINK_ABORT_WAIT_ADD_CONN: Link Reconfig is Aborted
 * while add link
 * @WLAN_LINK_RECFG_SS_ADD_LINK_ABORT_WAIT_LINK_SW: Link Reconfig is Aborted
 * while wait for link switch
 * @WLAN_LINK_RECFG_SS_WAIT_SMD_EXEC: Link Reconfig substate waiting for SMD
 * execution
 * @WLAN_LINK_RECFG_SS_MAX: Max SubState
 */
enum wlan_link_recfg_sm_state {
	WLAN_LINK_RECFG_S_INIT,
	WLAN_LINK_RECFG_S_START,
	WLAN_LINK_RECFG_S_DEL_LINK,
	WLAN_LINK_RECFG_S_XMIT_REQ,
	WLAN_LINK_RECFG_S_ADD_LINK,
	WLAN_LINK_RECFG_S_COMPLETED,
	WLAN_LINK_RECFG_S_ABORT,
	WLAN_LINK_RECFG_S_TTLM,
	WLAN_LINK_RECFG_S_WAIT,
	WLAN_LINK_RECFG_S_MAX,
	/* substates */
	WLAN_LINK_RECFG_SS_IDLE,
	WLAN_LINK_RECFG_SS_START_PENDING,
	WLAN_LINK_RECFG_SS_START_ACTIVE,
	WLAN_LINK_RECFG_SS_DEL_LINK_WAIT_SET_LINK,
	WLAN_LINK_RECFG_SS_DEL_LINK_WAIT_LINK_SW,
	WLAN_LINK_RECFG_SS_DEL_LINK_ABORT_WAIT_SET_LINK,
	WLAN_LINK_RECFG_SS_DEL_LINK_ABORT_WAIT_LINK_SW,
	WLAN_LINK_RECFG_SS_ADD_LINK_WAIT_ADD_CONN,
	WLAN_LINK_RECFG_SS_ADD_LINK_WAIT_LINK_SW,
	WLAN_LINK_RECFG_SS_ADD_LINK_ABORT_WAIT_ADD_CONN,
	WLAN_LINK_RECFG_SS_ADD_LINK_ABORT_WAIT_LINK_SW,
	WLAN_LINK_RECFG_SS_WAIT_SMD_EXEC,
	WLAN_LINK_RECFG_SS_MAX,
};

/**
 * enum wlan_link_recfg_sm_evt - Link Reconfig related events
 * Note: make sure to update ttlm_sm_event_names on updating this enum
 * @WLAN_LINK_RECFG_SM_EV_USER_REQ: Link Reconfiguration request from STA
 * @WLAN_LINK_RECFG_SM_EV_FW_IND: Link Reconfiguration AP initiated request
 * @WLAN_LINK_RECFG_SM_EV_START: Link Reconfiguration start event
 * @WLAN_LINK_RECFG_SM_EV_ACTIVE: Link Reconfiguration is active
 * @WLAN_LINK_RECFG_SM_EV_DEL_LINK: Link Reconfiguration for delete link
 * @WLAN_LINK_RECFG_SM_EV_ADD_LINK: Link Reconfiguration for add link
 * @WLAN_LINK_RECFG_SM_EV_XMIT_REQ: Link Reconfiguration event for TX req
 * @WLAN_LINK_RECFG_SM_EV_XMIT_STATUS: Link Reconfiguration event for TX status
 * @WLAN_LINK_RECFG_SM_EV_RX_RSP: Link Reconfiguration event for RX response
 * @WLAN_LINK_RECFG_SM_EV_SET_LINK_RSP: Link Reconfiguration event response for
 * set link
 * @WLAN_LINK_RECFG_SM_EV_LINK_SWITCH_IND: Link switch indication event
 * @WLAN_LINK_RECFG_SM_EV_LINK_SWITCH_RSP: Link reconfiguration event for link
 * switch.
 * @WLAN_LINK_RECFG_SM_EV_ADD_CONN_RSP: Link Reconfiguration event for add
 * connect rsp
 * @WLAN_LINK_RECFG_SM_EV_DISCONNECT_IND: Link Reconfiguration event for
 * disconnect ind
 * @WLAN_LINK_RECFG_SM_EV_ROAM_START_IND: Link Reconfiguration event for roam
 * start ind
 * @WLAN_LINK_RECFG_SM_EV_COMPLETED: Link Reconfiguration completed
 * @WLAN_LINK_RECFG_SM_EV_SER_TIMEOUT: Link Reconfiguration serialization
 * timeout
 * @WLAN_LINK_RECFG_SM_EV_SM_TIMEOUT: generic timeout in substate
 * @WLAN_LINK_RECFG_SM_EV_RX_RSP_TIMEOUT: Link Reconfig response timed out
 * @WLAN_LINK_RECFG_SM_EV_UPDATE_TTLM: Update TTLM due to link reconfig
 * @WLAN_LINK_RECFG_SM_EV_SMD_ROAM_START: Link Reconfiguration event for SMD
 * roam start
 * @WLAN_LINK_RECFG_SM_EV_WAIT_SMD_EXEC: Link Reconfiguration event for wait
 * for SMD execution
 * @WLAN_LINK_RECFG_SM_EV_SMD_ADD_LINK: Link recfg evt for SMD add link.
 * @WLAN_LINK_RECFG_SM_EV_SMD_ROAM_COMPLETED: Link recfg evt for SMD roaming
 * completed
 * @WLAN_LINK_RECFG_SM_EV_MAX: Max event
 */
enum wlan_link_recfg_sm_evt {
	WLAN_LINK_RECFG_SM_EV_USER_REQ,
	WLAN_LINK_RECFG_SM_EV_FW_IND,
	WLAN_LINK_RECFG_SM_EV_START,
	WLAN_LINK_RECFG_SM_EV_ACTIVE,
	WLAN_LINK_RECFG_SM_EV_DEL_LINK,
	WLAN_LINK_RECFG_SM_EV_ADD_LINK,
	WLAN_LINK_RECFG_SM_EV_XMIT_REQ,
	WLAN_LINK_RECFG_SM_EV_XMIT_STATUS,
	WLAN_LINK_RECFG_SM_EV_RX_RSP,
	WLAN_LINK_RECFG_SM_EV_SET_LINK_RSP,
	WLAN_LINK_RECFG_SM_EV_LINK_SWITCH_IND,
	WLAN_LINK_RECFG_SM_EV_LINK_SWITCH_RSP,
	WLAN_LINK_RECFG_SM_EV_ADD_CONN_RSP,
	WLAN_LINK_RECFG_SM_EV_DISCONNECT_IND,
	WLAN_LINK_RECFG_SM_EV_ROAM_START_IND,
	WLAN_LINK_RECFG_SM_EV_COMPLETED,
	WLAN_LINK_RECFG_SM_EV_SER_TIMEOUT,
	WLAN_LINK_RECFG_SM_EV_SM_TIMEOUT,
	WLAN_LINK_RECFG_SM_EV_RX_RSP_TIMEOUT,
	WLAN_LINK_RECFG_SM_EV_UPDATE_TTLM,
	WLAN_LINK_RECFG_SM_EV_SMD_ROAM_START,
	WLAN_LINK_RECFG_SM_EV_WAIT_SMD_EXEC,
	WLAN_LINK_RECFG_SM_EV_SMD_ADD_LINK,
	WLAN_LINK_RECFG_SM_EV_SMD_ROAM_COMPLETED,
	WLAN_LINK_RECFG_SM_EV_MAX,
};

/**
 * enum link_recfg_failure_reason - link recfg failure reason code enum
 * @link_recfg_success: link recfg successfully
 * @link_recfg_create_tran_failed: can't create transition list
 * @link_recfg_set_link_cmd_timeout: set link cmd timeout
 * @link_recfg_set_link_cmd_rejected: set link cmd rejected
 * @link_recfg_del_link_wait_fw_link_switch_timeout: wait for fw link switch
 * timeout
 * @link_recfg_del_link_fw_link_switch_rejected: fw link switch rejected in
 * delete link
 * @link_recfg_del_link_link_switch_comp_with_fail: link switch complete
 * with failure
 * @link_recfg_rsp_timeout: Link Reconfiguration response timeout.
 * @link_recfg_concurrency_failed: Link reconfig failed due to concurrency
 * @link_recfg_aborted_neg_ttlm_ongoing: Link Reconfiguration aborted
 * due to ongoing TTLM.
 * @link_recfg_nb_sb_disconnect: nb/sb disconnect causing abort
 * @link_recfg_tx_failed: tx status is failed
 * @link_recfg_rsp_status_failure: response status is error.
 */
enum link_recfg_failure_reason {
	link_recfg_success = 0,
	link_recfg_create_tran_failed = 1,
	link_recfg_set_link_cmd_timeout = 2,
	link_recfg_set_link_cmd_rejected = 3,
	link_recfg_del_link_wait_fw_link_switch_timeout = 4,
	link_recfg_del_link_fw_link_switch_rejected = 5,
	link_recfg_del_link_link_switch_comp_with_fail = 6,
	link_recfg_rsp_timeout = 7,
	link_recfg_concurrency_failed = 8,
	link_recfg_aborted_neg_ttlm_ongoing = 9,
	link_recfg_nb_sb_disconnect = 10,
	link_recfg_tx_failed = 11,
	link_recfg_rsp_status_failure = 12,
};

/**
 * enum link_recfg_type - link recfg type enum
 * @link_recfg_undefined: link recfg type undefined.
 * @link_recfg_del_only: delete link only
 * @link_recfg_add_only: add link only
 * @link_recfg_del_add_common_link: delete and add link with common link
 * present
 * @link_recfg_del_add_no_common_link: delete and add link with no
 * common link present
 * @link_recfg_two_frm_del_add_common_link: del and add by 2 action
 * frame with common link present
 * @link_recfg_st_prep_add_link: SMD roaming ST preparation add
 * target links
 * @link_recfg_st_exec_add_link: SMD roaming ST execution add
 * target link
 * @link_recfg_st_exec: SMD roaming ST execution for link reconfiguration
 */
enum link_recfg_type {
	link_recfg_undefined,
	link_recfg_del_only,
	link_recfg_add_only,
	link_recfg_del_add_common_link,
	link_recfg_del_add_no_common_link,
	link_recfg_two_frm_del_add_common_link,
	link_recfg_st_prep_add_link,
	link_recfg_st_exec_add_link,
	link_recfg_st_exec,
};

/**
 * struct mlo_link_recfg_status_list - Data Structure for link
 * reconfiguration response status list
 * @link_id: IEEE Link id
 * @status_code: wlan protocol status code
 */
struct mlo_link_recfg_status_list {
	uint8_t link_id;
	enum wlan_status_code status_code;
};

/**
 * struct wlan_mlo_link_recfg_rsp - Data Structure for link
 * reconfiguration response
 * @is_valid: Set to true if link reconfig resp is valid
 * @dialog_token: dialog token received in Link Recfg response frame.
 * @count: number of reconfig status duple in the mlo_link_recfg_status_list
 * @recfg_status_list: Reconfiguration status duple list
 * @grp_key_data: Group key data len and ptr
 * @oci_ie: OCI IE
 * @mlo_ie: Basic Multi link IE
 * @type: Type field (0=ST prep, 1=ST exec) (SMD only)
 * @status_code: Overall wlan protocol status code (SMD only)
 * @smd_bss_trans_params: SMD BSS Transition Parameters element (SMD only)
 * @assigned_aid: AID assigned by target AP MLD (SMD only)
 * @key_delivery: Key Delivery element (SMD per-PTK mode only)
 * @mscs_descriptor: MSCS Descriptor element (SMD per-PTK mode only)
 * @diffie_hellman_param: Diffie-Hellman Parameter element (SMD per-PTK mode only)
 * @nonce: Nonce element (SMD per-PTK mode only)
 * @mic: MIC element (SMD per-PTK mode only)
 */
struct wlan_mlo_link_recfg_rsp {
	bool is_valid;
	uint8_t dialog_token;
	uint8_t count;
	struct mlo_link_recfg_status_list recfg_status_list[WLAN_MAX_ML_RECFG_LINK_COUNT];
	struct element_info grp_key_data;
	struct element_info oci_ie;
	struct element_info mlo_ie;
#ifdef WLAN_FEATURE_11BN_SMD
	uint8_t type;
	enum wlan_status_code status_code;
	struct element_info smd_bss_trans_params;
	uint16_t assigned_aid;
	/* Per-PTK mode optional elements */
	struct element_info key_delivery;
	struct element_info mscs_descriptor;
	struct element_info diffie_hellman_param;
	struct element_info nonce;
	struct element_info mic;
#endif /* WLAN_FEATURE_11BN_SMD */
};

/**
 * struct wlan_mlo_link_recfg_req - Data Structure because of link
 *  reconfiguration request
 * @vdev_id: VDEV ID of the primary link in the MLO connection. This field
 *           holds information regarding all the links of the ML connection
 * @add_link_info: Information about links to be added during reconfiguration.
 *                 Contains link IDs, addresses, and other parameters for
 *                 links being added to the MLO connection
 * @del_link_info: Information about links to be deleted during reconfiguration.
 *                 Contains link IDs and parameters for links being removed
 *                 from the MLO connection
 * @is_user_req: Flag indicating if the request originated from user space or
 *               framework (true) or from firmware indication (false)
 * @is_curr_req: Flag indicating if this is the current active link
 *               reconfiguration request being processed
 * @is_fw_ind_received: Flag indicating whether a firmware link reconfiguration
 *                      event/indication has been received
 * @recfg_type: Type of link reconfiguration operation being performed.
 *              See enum link_recfg_type for possible values (delete only,
 *              add only, delete and add with/without common link, etc.)
 * @join_pending_vdev_id: VDEV ID used for no-common link reconfiguration
 *                        scenarios. This vdev is used to trigger peer
 *                        association after receiving the reconfiguration
 *                        response from the AP
 * @fw_ind_param: Parameters received from firmware link reconfiguration
 *                event/indication. Contains firmware-provided information
 *                about the requested link changes
 * @mld_addr: Multi-Link Device (MLD) MAC address. This is the MLD-level
 *            address for the MLO connection
 * @send_two_link_recfg_frms: Flag to split a single Link Reconfiguration
 *                            request into 2 separate action frames. Used when
 *                            the reconfiguration cannot be completed in a
 *                            single frame
 * @st_prep_link_recfg: Flag indicating SMD (Seamless Multi-link Device) roaming
 *                      state preparation for link reconfiguration. Set to true
 *                      when preparing target links during SMD roaming ST
 *                      (State Transition) preparation phase
 * @st_exec_link_recfg: Flag indicating SMD roaming state execution for link
 *                      reconfiguration. Set to true when executing link
 *                      reconfiguration during SMD roaming ST execution phase
 */
struct wlan_mlo_link_recfg_req {
	uint8_t vdev_id;
	struct wlan_mlo_link_recfg_info add_link_info;
	struct wlan_mlo_link_recfg_info del_link_info;
	bool is_user_req;
	bool is_curr_req;
	bool is_fw_ind_received;
	enum link_recfg_type recfg_type;
	uint8_t join_pending_vdev_id;
	struct wlan_mlo_link_recfg_ind_param fw_ind_param;
	uint8_t mld_addr[QDF_MAC_ADDR_SIZE];
	bool send_two_link_recfg_frms;
	bool st_prep_link_recfg;
	bool st_exec_link_recfg;
};

/**
 * struct mlo_link_recfg_state_req - Link Reconfig add/del/xmit state
 * request param
 * @add_link_info: add link info
 * @del_link_info: del link info
 * @dialog_token: Dialog token
 * @peer_mac: peer mac address to indicate on which link to send recfg frame
 * @recfg_type: Type of link reconfiguration operation being performed.
 *              See enum link_recfg_type for possible values (delete only,
 *              add only, delete and add with/without common link, etc.)
 * @ctx_notransfer_flags: SMD BSS Transition context no-transfer flags
 * @scs_list_present: Whether SCS list is present in SMD BSS Transition
 * @num_scs_ids: Number of valid SCS IDs in scs_ids array
 * @scs_ids: Array of SCS IDs for SMD BSS Transition
 */
struct mlo_link_recfg_state_req {
	struct wlan_mlo_link_recfg_info add_link_info;
	struct wlan_mlo_link_recfg_info del_link_info;
	uint8_t dialog_token;
	struct qdf_mac_addr peer_mac;
	enum link_recfg_type recfg_type;
#ifdef WLAN_FEATURE_11BN_SMD
	uint8_t ctx_notransfer_flags;
	bool scs_list_present;
	uint8_t num_scs_ids;
	uint8_t scs_ids[MLO_LINK_RECFG_MAX_SCS_IDS];
#endif
};

/**
 * typedef state_abort_handler - link recfg abort callback
 * @psoc: psoc object
 *
 * Return: QDF_STATUS
 */
typedef QDF_STATUS (*state_abort_handler)(struct wlan_objmgr_psoc *psoc);

/**
 * typedef state_pre_link_add_handler - pre link add callback
 * @recfg_ctx: recfg context
 * @req: link recfg request
 *
 * Used in non-common link case, to be invoked when trigger connect to new
 * Added link.
 *
 * Return: QDF_STATUS
 */
typedef QDF_STATUS (*state_pre_link_add_handler)(
			struct mlo_link_recfg_context *recfg_ctx,
			struct mlo_link_recfg_state_req *req);

/**
 * typedef state_defer_rsp_handler - defer the action frame response handler
 * @recfg_ctx: recfg context
 * @recfg_resp_data: recfg response data
 * @event_data_len: recfg response data len
 *
 * Return: QDF_STATUS
 */
typedef QDF_STATUS (*state_defer_rsp_handler)(
			struct mlo_link_recfg_context *recfg_ctx,
			struct link_recfg_rx_rsp *recfg_resp_data,
			uint16_t event_data_len);

/**
 * typedef state_proc_defer_rsp_handler - process deferred the action
 * frame response handler
 * @recfg_ctx: recfg context
 *
 * Return: QDF_STATUS
 */
typedef QDF_STATUS (*state_proc_defer_rsp_handler)(
			struct mlo_link_recfg_context *recfg_ctx);
typedef QDF_STATUS (*two_frm_handler)(struct mlo_link_recfg_context *recfg_ctx);

/**
 * struct mlo_link_recfg_state_tran - Link Reconfig state transition
 * info
 * @state: target transition state
 * @event: first event id for the state
 * @req: state request param, also the event data
 * @abort_handler: error handler if error happens in the state,
 * it will be invoked after link config completed
 * @two_frame_xmit_handler: two frame xmit handler callback
 * @pre_link_add_handler: pre link add callback
 * @defer_rsp_handler: defer response processing
 * @proc_defer_rsp_handler: process deferred response
 */
struct mlo_link_recfg_state_tran {
	enum wlan_link_recfg_sm_state state;
	enum wlan_link_recfg_sm_evt event;
	struct mlo_link_recfg_state_req req;
	state_abort_handler abort_handler;
	two_frm_handler two_frame_xmit_handler;
	state_pre_link_add_handler pre_link_add_handler;
	state_defer_rsp_handler defer_rsp_handler;
	state_proc_defer_rsp_handler proc_defer_rsp_handler;
};

/* WLAN_LINK_RECFG_SM_EV_XMIT_TX_DONE */
struct link_recfg_tx_result {
	QDF_STATUS status;
};

/* WLAN_LINK_RECFG_SM_EV_RX_RSP */
struct link_recfg_rx_rsp {
	QDF_STATUS status;
};

/**
 * struct set_link_resp - event data of
 * WLAN_LINK_RECFG_SM_EV_SET_LINK_RSP
 * @status: 0 for set link success, otherwise set link failed
 */
struct set_link_resp {
	uint32_t status;
};

/**
 * struct link_switch_ind - event data of
 * WLAN_LINK_RECFG_SM_EV_LINK_SWITCH_IND
 * @status: link witch start status code
 */
struct link_switch_ind {
	QDF_STATUS status;
};

/**
 * struct link_switch_rsp - event data of
 * WLAN_LINK_RECFG_SM_EV_LINK_SWITCH_RSP
 * @status: link witch rsp status code
 */
struct link_switch_rsp {
	QDF_STATUS status;
};

/* struct add_link_conn_resp - event data of
 * WLAN_LINK_RECFG_SM_EV_ADD_CONN_RSP
 * @status: connect resp status code
 */
struct add_link_conn_rsp {
	QDF_STATUS status;
};

/* WLAN_LINK_RECFG_SM_EV_DISCONNECT_IND */
struct disconnect_ind {
};

/* WLAN_LINK_RECFG_SM_EV_ROAM_START_IND */
struct roam_ind {
};

/* WLAN_LINK_RECFG_SM_EV_COMPLETED */
struct recfg_completed {
};

#define MAX_RECFG_TRANSITION 7

/**
 * struct mlo_link_recfg_state_sm - Link Reconfig state machine
 * @mlrc_sm_lock: SM lock
 * @sm_hdl: SM handlers
 * @link_recfg_state: Current state
 * @link_recfg_substate: Current substate
 * @state_list: link reconfig state transition list
 * @curr_state_idx: current transition index
 * @sm_timer: state machine timer
 */
struct mlo_link_recfg_state_sm {
	qdf_mutex_t mlrc_sm_lock;
	struct wlan_sm *sm_hdl;
	enum wlan_link_recfg_sm_state link_recfg_state;
	enum wlan_link_recfg_sm_state link_recfg_substate;
	struct mlo_link_recfg_state_tran state_list[MAX_RECFG_TRANSITION];
	int8_t curr_state_idx;
	qdf_mc_timer_t sm_timer;
};


/**
 * struct recfg_done_data_hdr - recfg done ctx header
 * @node: list node
 */
struct recfg_done_data_hdr {
	qdf_list_node_t node;
};

#define MAX_NUM_FRAMES 4
/**
 * struct wlan_mlo_link_recfg_bitmap: User based Link reconfig bitmap
 * @num_frames: Number of frames used to send link reconfig request
 * @add_link_bitmap: Bitmap of link IDs of links to be added
 * @delete_link_bitmap: Bitmap of link IDs of links to be removed
 */
struct wlan_mlo_link_recfg_bitmap {
	uint8_t num_frames;
	uint16_t add_link_bitmap[MAX_NUM_FRAMES];
	uint16_t delete_link_bitmap[MAX_NUM_FRAMES];
};

/**
 * struct cached_link_assoc_rsp - Cached link association response
 * @link_id: Link ID
 * @assoc_rsp: Association response frame
 * @valid: Whether this cache entry is valid
 *
 * Structure to cache generated link-specific association responses
 * for firmware use during ST execution phase.
 */
struct cached_link_assoc_rsp {
	uint8_t link_id;
	struct element_info assoc_rsp;
	bool valid;
};

/**
 * struct mlo_link_recfg_context - Link reconfiguration data structure.
 * @psoc: psoc object
 * @ml_dev: ml dev context
 * @curr_recfg_req: Last link recfg request received from FW or user
 * @curr_recfg_rsp: Last link recfg response received from AP
 * @sm: link reconfig sm context
 * @set_link_req: set link request
 * @macaddr_updating_vdev_id: vdev id for which mac address is being updated
 * @old_macaddr_updating_vdev: old mac address of the vdev
 * @req_frame: link recfg request frame
 * @rsp_frame: link recfg response frame
 * @link_recfg_bm: link recfg bitmap
 * @rsp_rx_frame: link recfg response rx frame
 * @link_recfg_rsp_timer: link recfg response timer
 * @link_recfg_status: link recfg status
 * @last_dialog_token: last dialog token
 * @internal_reason_code: Internal failure reason code
 * @copied_recfg_req: Copied recfg req
 * @recfg_indication_work: recfg done work queue
 * @recfg_done_list: recfg done data struct list
 * @num_vdev_repurpose_req: Number of vdev repurpose TLVs (priority ordered)
 * @vdev_repurpose_req: Array to handle up to MAX BSS links TLVs
 * @smd_transition_ie: SMD Transition IE from FW
 * @tgt_ap_link_bitmap: Target AP requested setup IEEE links bitmap
 *                      (from notif_params1). This bitmap indicates which
 *                      links of the target AP MLD are requested to be set
 *                      up during SMD roaming.
 * @smd_roam_in_progress: bool smd roam in progress
 * @current_link_index: Index of the current link being processed in SMD roaming
 * @st_exec_in_progress: Flag indicating SMD ST execution is in progress
 * @cached_sync_ind: Cached roam sync indication
 * @cached_assoc_rsp: Cached per-link association responses for ST execution
 */
struct mlo_link_recfg_context {
	struct wlan_objmgr_psoc *psoc;
	struct wlan_mlo_dev_context *ml_dev;
	struct wlan_mlo_link_recfg_req curr_recfg_req;
	struct wlan_mlo_link_recfg_rsp curr_recfg_rsp;
	struct mlo_link_recfg_state_sm sm;
	struct mlo_link_set_active_req *set_link_req;
	uint8_t macaddr_updating_vdev_id;
	struct qdf_mac_addr old_macaddr_updating_vdev;
	struct element_info req_frame;
	struct element_info rsp_frame;
	struct wlan_mlo_link_recfg_bitmap link_recfg_bm;
	struct element_info rsp_rx_frame;
	qdf_mc_timer_t link_recfg_rsp_timer;
	QDF_STATUS link_recfg_status;
	uint8_t last_dialog_token;
	enum link_recfg_failure_reason internal_reason_code;
	struct wlan_mlo_link_recfg_req copied_recfg_req;
	qdf_work_t recfg_indication_work;
	qdf_list_t recfg_done_list;
#ifdef WLAN_FEATURE_11BN_SMD
	uint8_t num_vdev_repurpose_req;
	struct smd_vdev_repurpose_req vdev_repurpose_req[WLAN_MAX_ML_BSS_LINKS];
	struct smd_transition_ie_info smd_transition_ie;
	uint16_t tgt_ap_link_bitmap;
	bool smd_roam_in_progress;
	uint8_t current_link_index;
	bool st_exec_in_progress;
	struct roam_offload_synch_ind *cached_sync_ind;
	struct cached_link_assoc_rsp cached_assoc_rsp[WLAN_MAX_ML_BSS_LINKS];
#endif
};

static inline void
ml_link_recfg_sm_lock_create(struct wlan_mlo_dev_context *mldev)
{
	qdf_mutex_create(&mldev->link_recfg_ctx->sm.mlrc_sm_lock);
}

static inline void
ml_link_recfg_sm_lock_destroy(struct wlan_mlo_dev_context *mldev)
{
	qdf_mutex_destroy(&mldev->link_recfg_ctx->sm.mlrc_sm_lock);
}

static inline void
ml_link_recfg_sm_lock_acquire(struct wlan_mlo_dev_context *mldev)
{
	qdf_mutex_acquire(&mldev->link_recfg_ctx->sm.mlrc_sm_lock);
}

static inline void
ml_link_recfg_sm_lock_release(struct wlan_mlo_dev_context *mldev)
{
	qdf_mutex_release(&mldev->link_recfg_ctx->sm.mlrc_sm_lock);
}

#ifdef WLAN_FEATURE_11BE_MLO
/**
 * mlo_link_recfg_validate_roam_invoke() - Validate roam invoke
 * allow if link recfg is in-progress
 * @psoc: psoc object
 * @vdev: vdev object
 *
 * Return: QDF_STATUS
 */
QDF_STATUS mlo_link_recfg_validate_roam_invoke(
		struct wlan_objmgr_psoc *psoc,
		struct wlan_objmgr_vdev *vdev);

/**
 * mlo_link_recfg_set_mac_addr_resp() - handle link recfg set mac
 * addr response
 * @vdev: vdev object
 * @resp_status: set mac resp result
 *
 * Return: QDF_STATUS
 */
QDF_STATUS mlo_link_recfg_set_mac_addr_resp(struct wlan_objmgr_vdev *vdev,
					    uint8_t resp_status);

/**
 * mlo_link_recfg_set_link_resp() - Handle link recfg set link
 * response event
 * @vdev: vdev object
 * @result: set link response result
 *
 * Return: void
 */
void mlo_link_recfg_set_link_resp(struct wlan_objmgr_vdev *vdev,
				  uint32_t result);

/**
 * mlo_link_recfg_get_add_partner_links() - Get current added
 * partner links
 * @vdev: vdev object
 * @ml_partner_info: New added partner links
 *
 * Return: QDF_STATUS
 */
QDF_STATUS
mlo_link_recfg_get_add_partner_links(
		struct wlan_objmgr_vdev *vdev,
		struct mlo_partner_info *ml_partner_info);

/**
 * mlo_mgr_link_recfg_indication_event_handler() - Handle fw link recfg event
 * @psoc: psoc object
 * @evt_params: fw link recfg event data
 *
 * Return: QDF_STATUS
 */
QDF_STATUS
mlo_mgr_link_recfg_indication_event_handler(
			struct wlan_objmgr_psoc *psoc,
			struct wlan_mlo_link_recfg_ind_param *evt_params);

/**
 * mlo_link_recfg_init_state() - Set the current state of link switch
 * to init state.
 * @mlo_dev_ctx: MLO dev context
 *
 * Sets the current state of link switch to MLO_LINK_SWITCH_STATE_IDLE with
 * MLO dev context lock held.
 *
 * Return: void
 */
void mlo_link_recfg_init_state(struct wlan_mlo_dev_context *mlo_dev_ctx);

/**
 * mlo_link_recfg_trans_next_state() - Transition to next state based
 * on current state.
 * @mlo_dev_ctx: MLO dev context
 *
 * Move to next state in link recfg process based on current state with
 * SM link reconfig lock held.
 *
 * Return: void
 */
QDF_STATUS
mlo_link_recfg_trans_next_state(struct wlan_mlo_dev_context *mlo_dev_ctx);

/**
 * mlo_link_recfg_trans_abort_state() - Transition to abort trans state.
 * @mlo_dev_ctx: ML dev context pointer of VDEV
 *
 * Transition the current link recfg state to ABORT
 * state, no further state transitions are allowed in the ongoing link recfg
 * request.
 *
 * Return: void
 */
void
mlo_link_recfg_trans_abort_state(struct wlan_mlo_dev_context *mlo_dev_ctx);

/**
 * mlo_is_link_recfg_in_progress() - Check in MLO dev context
 * if the last received link recfg is in progress.
 * @vdev: VDEV object manager
 *
 * The API is to be called for VDEV which has MLO dev context and link reconfig
 * context initialized. Returns the value of 'is_in_progress' flag in last received
 * link reconfig request.
 *
 * Return: bool
 */
bool mlo_is_link_recfg_in_progress(struct wlan_objmgr_vdev *vdev);

/**
 * mlo_link_recfg_notify() - API to notify registered link recfg notify
 * callbacks.
 * @vdev: VDEV object manager
 * @req: Link recfg request params from FW.
 *
 * The API calls all the registered link recfg notifiers with appropriate
 * reason for notifications. Callback handlers to take necessary action based
 * on the reason.
 * If any callback returns error API will return error or else success.
 *
 * Return: QDF_STATUS.
 */
QDF_STATUS
mlo_link_recfg_notify(struct wlan_objmgr_vdev *vdev,
		      struct wlan_mlo_link_recfg_req *req);

/**
 * mlo_link_recfg_validate_request() - Validate link reconfiguration request
 * received from FW.
 * @vdev: VDEV object manager
 * @req: Request params from FW
 *
 * The API performs initial validation of link recfg params received from FW
 * before serializing the link recfg cmd. If any of the params is invalid or
 * the current status of MLO manager can't allow link recfg, the API returns
 * failure and link recfg has to be terminated.
 *
 * Return: QDF_STATUS
 */
QDF_STATUS
mlo_link_recfg_validate_request(struct wlan_objmgr_vdev *vdev,
				struct wlan_mlo_link_recfg_req *req);

/**
 * mlo_is_link_recfg_supported() - API to check link recfg
 * support with vdev
 * @vdev: vdev object
 *
 * Return: bool
 */
bool
mlo_is_link_recfg_supported(struct wlan_objmgr_vdev *vdev);

/**
 * mlo_link_recfg_request_params() - Link recfg request params from FW.
 * @psoc: PSOC object manager
 * @evt_params: Link recfg params received from FW.
 *
 * The @params contain link recfg request parameters received from FW or user
 * as an indication to host to trigger link recfg sequence.
 * If the @params are not valid link recfg will be terminated.
 *
 * Return: QDF_STATUS
 */
QDF_STATUS mlo_link_recfg_request_params(struct wlan_objmgr_psoc *psoc,
					 void *evt_params);

/**
 * mlo_link_recfg_init() - API to initialize link reconfiguration
 * @psoc: PSOC object manager
 * @ml_dev: MLO dev context
 *
 * Initializes the MLO link recfg context in @ml_dev and allocates various
 * buffers needed.
 *
 * Return: QDF_STATUS
 */
QDF_STATUS mlo_link_recfg_init(struct wlan_objmgr_psoc *psoc,
			       struct wlan_mlo_dev_context *ml_dev);

/**
 * mlo_link_recfg_timer_init() - API to initialize link recfg
 * rsp timer
 *@recfg_ctx: ML Reconfig context
 *
 * Initialize the MLO link reconfiguration rsp timer
 *
 * Return: void
 */
void mlo_link_recfg_timer_init(struct mlo_link_recfg_context *recfg_ctx);

/**
 * mlo_link_recfg_deinit() - API to de-initialize link recfg
 * @ml_dev: MLO dev context
 *
 * De-initialize the MLO link reconfiguration context
 * in @ml_dev on and frees memory
 * allocated as part of initialization.
 *
 * Return: QDF_STATUS
 */
QDF_STATUS mlo_link_recfg_deinit(struct wlan_mlo_dev_context *ml_dev);

/**
 * mlo_link_recfg_timer_deinit() - API to de-initialize link recfg timer
 * @recfg_ctx: ML Reconfig context
 *
 * De-initialize the MLO link reconfiguration timer
 *
 * Return: void
 */
void mlo_link_recfg_timer_deinit(struct mlo_link_recfg_context *recfg_ctx);

/**
 * mlo_link_recfg_rx_rsp_timeout_cb() - API to handle link recfg
 * response timeout callback.
 * @user_data: ML Reconfig context
 *
 * Callback api to handle link recfg rsp timed out
 *
 * Return: void
 */
void mlo_link_recfg_rx_rsp_timeout_cb(void *user_data);

/**
 * mlo_link_recfg_get_state() - API to get SM link recfg state
 * @mlo_dev_ctx: MLO dev context
 *
 * API to get current SM link reconfiguration state
 *
 * Return: QDF_STATUS
 */
enum wlan_link_recfg_sm_state
mlo_link_recfg_get_state(struct wlan_mlo_dev_context *mlo_dev_ctx);

/**
 * mlo_link_recfg_get_substate() - API to get SM link recfg sub state
 * @mlo_dev_ctx: MLO dev context
 *
 * API to get current SM link reconfiguration sub state
 *
 * Return: QDF_STATUS
 */
enum wlan_link_recfg_sm_state
mlo_link_recfg_get_substate(struct wlan_mlo_dev_context *mlo_dev_ctx);

/**
 * mlo_link_recfg_sm_deliver_event() - Delivers event to Link Reconfiguraion
 * manager SM
 * @mlo_dev_ctx: ML dev context
 * @event: Link reconfig SM event
 * @data_len: data size
 * @data: event data
 *
 * API to dispatch event to Link reconfig SM with lock. To be used while posting
 * events from API called from public API. i.e. indication/response/request
 * from any other module or NB/SB req/resp.
 *
 * Context: Can be called from any context, This should be called in case
 * SM lock is not taken, the API will take the lock before posting to SM.
 *
 * Return: SUCCESS: on handling event
 *         FAILURE: If event not handled
 */
QDF_STATUS
mlo_link_recfg_sm_deliver_event(struct wlan_mlo_dev_context *mlo_dev_ctx,
				enum wlan_link_recfg_sm_evt event,
				uint16_t data_len, void *data);

/**
 * mlo_link_recfg_sm_deliver_event_sync() - Delivers event to Link Reconfiguration SM while
 * holding lock
 * @mlo_dev_ctx: mlo dev ctx
 * @event: Link Reconfiguration event
 * @data_len: data size
 * @data: event data
 *
 * API to dispatch event to Link Reconfiguration SM without lock,
 * in case lock is already held.
 *
 * Context: Can be called from any context, This should be called in case
 * SM lock is already taken. If lock is not taken use
 * mlo_mgr_link_recfg_sm_deliver_event API instead.
 *
 * Return: SUCCESS: on handling event
 *         FAILURE: If event not handled
 */
QDF_STATUS
mlo_link_recfg_sm_deliver_event_sync(struct wlan_mlo_dev_context *mlo_dev_ctx,
				     enum wlan_link_recfg_sm_evt event,
				     uint16_t data_len, void *data);

/**
 * mlo_link_recfg_linksw_start_indication() - Delivers link switch start event
 * @vdev: vdev
 * @start_status: link switch start status
 *
 * API to dispatch Link switch start event to reconfig sm.
 *
 * Return: QDF_STATUS
 */
QDF_STATUS
mlo_link_recfg_linksw_start_indication(struct wlan_objmgr_vdev *vdev,
				       QDF_STATUS start_status);

/**
 * mlo_link_recfg_linksw_completion_indication() - Delivers link switch
 * completion event
 * @vdev: vdev
 * @comp_status: link switch completion status
 *
 * API to dispatch Link switch completion event to reconfig sm.
 *
 * Return: QDF_STATUS
 */
QDF_STATUS
mlo_link_recfg_linksw_completion_indication(struct wlan_objmgr_vdev *vdev,
					    QDF_STATUS comp_status);

/**
 * mlo_link_recfg_add_connect_done_indication() - Delivers link add connect
 * completion event
 * @vdev: vdev
 * @comp_status: link add connect completion status
 *
 * API to dispatch Link add connect completion event to reconfig sm.
 *
 * Return: QDF_STATUS
 */
QDF_STATUS
mlo_link_recfg_add_connect_done_indication(
				struct wlan_objmgr_vdev *vdev,
				QDF_STATUS comp_status);

QDF_STATUS
mlo_link_recfg_create_transition_list(
			struct mlo_link_recfg_context *recfg_ctx,
			struct wlan_mlo_link_recfg_req *recfg_req);
/**
 * mlo_link_recfg_send_request_frame() - Send Link Reconfiguration action
 * request frame
 * @recfg_ctx: recfg ctx pointer
 * @req: Link Reconfig event data pointer
 *
 * API to send Link Reconfiguration action request frame
 *
 * Return: qdf status
 */
QDF_STATUS
mlo_link_recfg_send_request_frame(
		struct mlo_link_recfg_context *recfg_ctx,
		struct mlo_link_recfg_state_req *req);

/**
 * mlo_link_recfg_rx_rsp() - Handler for Link Reconfiguration
 * action response frame
 * @vdev: vdev pointer
 * @event: Link reconfig SM event
 * @rx_pkt_info: RX frame
 *
 * API to send Link Reconfiguration action response frame
 *
 * Return: qdf status
 */
QDF_STATUS mlo_link_recfg_rx_rsp(struct wlan_objmgr_vdev *vdev,
				 enum wlan_link_recfg_sm_evt event,
				 uint8_t *rx_pkt_info);

#ifdef WLAN_FEATURE_11BN_SMD
/**
 * mlo_uhr_link_recfg_rx_rsp() - Handle UHR Link Reconfiguration Response
 * @vdev: vdev object
 * @event: Link reconfiguration state machine event
 * @rx_pkt_info: RX packet info
 *
 * This function handles UHR Link Reconfiguration Response frames received
 * during SMD roaming. It parses the response and delivers events to the
 * link reconfiguration state machine.
 *
 * Return: QDF_STATUS
 */
QDF_STATUS
mlo_uhr_link_recfg_rx_rsp(struct wlan_objmgr_vdev *vdev,
			  enum wlan_link_recfg_sm_evt event,
			  uint8_t *rx_pkt_info);
#endif /* WLAN_FEATURE_11BN_SMD */

/**
 * mlo_link_recfg_link_add_join_req() - handle add link join request
 * @vdev: vdev pointer
 *
 * API to handle add link join request for non-common link case
 *
 * Return: qdf status
 */
QDF_STATUS
mlo_link_recfg_link_add_join_req(struct wlan_objmgr_vdev *vdev);

/**
 * mlo_link_recfg_is_start_as_active() - check link start as active state
 * when vdev is started on the link
 * @vdev: vdev pointer
 *
 * For no-common link cases, L1 -> L2, or L1 L2 -> L3, the added link
 * has to be active state. The flag will be sent in vdev start.
 *
 * Return: true if start the link with active state after vdev started
 */
bool mlo_link_recfg_is_start_as_active(struct wlan_objmgr_vdev *vdev);

/**
 * mlo_link_recfg_dialog_token() - Generate dialog token for
 * for Link Reconfiguration action request frame
 * @recfg_ctx: Link reconfig context pointer
 * @req: mlo link reconfig req pointer
 *
 * API to generate dialog token for Link Reconfiguration
 * action request frame
 *
 * Return: uint8_t
 */
uint8_t
mlo_link_recfg_dialog_token(struct mlo_link_recfg_context *recfg_ctx,
			    struct mlo_link_recfg_state_req *req);

/**
 * mlo_link_recfg_ctx_free_ies() -Free link recfg ctx ies
 * @ctx: link reconfig ctx pointer
 *
 * API to free link reconfig context ies
 *
 * Return: none
 */
void
mlo_link_recfg_ctx_free_ies(struct mlo_link_recfg_context *ctx);

/**
 * mlo_link_recfg_store_key() -Store unicast key
 * @ctx: link reconfig ctx pointer
 * @req: Link recfg request pointer
 *
 * API to stote unicast keys during link reconfig addition.
 *
 * Return: qdf_status success/fail
 */
QDF_STATUS
mlo_link_recfg_store_key(struct mlo_link_recfg_context *ctx,
			 struct mlo_link_recfg_state_req *req);

/**
 * mlo_link_recfg_save_unicast_key() -Save unicast keys for added link
 * @ctx: link reconfig ctx pointer
 * @vdev: vdev obj pointer
 * @link_addr: self link address
 * @ap_link_addr: AP link address
 * @link_id: link id
 *
 * API to save unicast keys for added link
 *
 * Return: qdf_status success/fail
 */
QDF_STATUS
mlo_link_recfg_save_unicast_key(struct mlo_link_recfg_context *ctx,
				struct wlan_objmgr_vdev *vdev,
				struct qdf_mac_addr *link_addr,
				struct qdf_mac_addr *ap_link_addr,
				uint8_t link_id);
/**
 * mlo_link_recfg_install_unicast_keys() -Install unicast keys for added link
 * @vdev: vdev obj pointer
 *
 * API to install unicast keys for added link
 *
 * Return: none
 */
void
mlo_link_recfg_install_unicast_keys(struct wlan_objmgr_vdev *vdev);

/**
 * mlo_link_recfg_get_mlo_ctx() - Get ML dev context
 * @recfg_ctx: ptr to recfg ctx
 *
 * Return: poeintr to mlo dev context
 */
struct wlan_mlo_dev_context *
mlo_link_recfg_get_mlo_ctx(struct mlo_link_recfg_context *recfg_ctx);

/**
 * mlo_link_recfg_get_psoc() - Get link recfg ctx psoc
 * @recfg_ctx: ptr to recfg ctx
 *
 * Return: psoc pointer
 */
struct wlan_objmgr_psoc *
mlo_link_recfg_get_psoc(struct mlo_link_recfg_context *recfg_ctx);

/**
 * mlo_link_recfg_get_curr_tran_req() - Get link recfg ctx curr tran req
 * @recfg_ctx: ptr to recfg ctx
 *
 * Return: mlo_link_recfg_state_tran pointer
 */
struct mlo_link_recfg_state_tran *
mlo_link_recfg_get_curr_tran_req(struct mlo_link_recfg_context *recfg_ctx);

#ifdef WLAN_FEATURE_11BE_MLO_ADV_FEATURE
/**
 * mlo_mgr_link_recfg_req_cmd_handler() - Handle link recfg req
 * from user space
 * @psoc: psoc object
 * @req: userspace link recfg command
 *
 * Return: QDF_STATUS
 */
QDF_STATUS
mlo_mgr_link_recfg_req_cmd_handler(
			struct wlan_objmgr_psoc *psoc,
			struct  mlo_link_recfg_user_req_params *req);
#endif
/**
 * mlo_link_recfg_abort_if_in_progress() -Abort link recfg in progress
 * @vdev: Vdev pointer
 * @is_link_switch_discon: is link switch disconnect
 *
 * API to abort link reconfig if in progress.
 * Return: none
 */
void
mlo_link_recfg_abort_if_in_progress(struct wlan_objmgr_vdev *vdev,
				    bool is_link_switch_discon);

/**
 * mlo_link_recfg_get_link_bitmap() - Build bitmaps and counts for link-recfg
 * @recfg_ctx: Link reconfiguration context. Must contain a valid @ml_dev.
 * @recfg_req: Link reconfiguration request containing add/del link info.
 * @add_link_set: (out) Bitmap of IEEE link IDs requested to be added
 *	(from @recfg_req->add_link_info). Bit N set => link_id N is in add list.
 * @add_link_num: (out) Number of links requested to be added
 *	(same as @recfg_req->add_link_info.num_links).
 * @del_link_set: (out) Bitmap of IEEE link IDs requested to be deleted
 *	(from @recfg_req->del_link_info). Bit N set => link_id N is in del list.
 * @del_link_num: (out) Number of links requested to be deleted
 *	(same as @recfg_req->del_link_info.num_links).
 * @curr_link_set: (out) Bitmap of currently tracked (non-deleted) IEEE link IDs
 *	for this MLO connection, derived from @recfg_ctx->ml_dev->link_ctx.
 *	Links are included only if:
 *	 - ap_link_addr is non-zero
 *	 - link_id != WLAN_INVALID_LINK_ID
 *	 - LS_F_AP_REMOVAL_BIT is not set in link_status_flags
 * @curr_link_num: (out) Count of links contributing to @curr_link_set.
 * @curr_standby_set: (out) Bitmap of currently tracked standby links (subset of
 *	@curr_link_set) where vdev_id == WLAN_INVALID_VDEV_ID.
 * @curr_standby_num: (out) Count of links contributing to @curr_standby_set.
 *
 * Note: This API updates output bitmaps via '|=' and increments counts via '++'.
 * Callers must initialize *add_link_set, *del_link_set, *curr_link_set,
 * *curr_standby_set and the corresponding counters to 0 before calling.
 *
 * Return: QDF_STATUS_SUCCESS on success,
 *	   QDF_STATUS_E_INVAL if @recfg_ctx does not contain a valid ml_dev.
 */
QDF_STATUS
mlo_link_recfg_get_link_bitmap(struct mlo_link_recfg_context *recfg_ctx,
			       struct wlan_mlo_link_recfg_req *recfg_req,
			       uint32_t *add_link_set,
			       uint8_t *add_link_num,
			       uint32_t *del_link_set,
			       uint8_t *del_link_num,
			       uint32_t *curr_link_set,
			       uint8_t *curr_link_num,
			       uint32_t *curr_standby_set,
			       uint8_t *curr_standby_num);

/**
 * mlo_link_recfg_set_tx_link_addr() - Select peer (AP link) MAC address for
 * Link Reconfiguration request frame transmission
 * @recfg_ctx: Link reconfiguration context. Must contain a valid MLO dev ctx.
 * @recfg_req: Link reconfiguration request being processed (unused for now but
 *             kept for symmetry with other helpers / future use).
 * @req: State-machine request structure to be used for TX. On success, this
 *       function fills @req->peer_mac with the selected AP link address.
 * @candidate_link_set: Bitmap of candidate IEEE link IDs that are allowed for
 *                      transmitting the Link Reconfiguration request action
 *                      frame. Bit N set => link_id N is eligible.
 *
 * This helper chooses which AP link address (BSSID) should be used as the peer
 * address for sending the Link Reconfiguration request action frame. It prefers
 * a candidate link that has a valid vdev and is currently connected. If no such
 * connected candidate is found, it falls back to a candidate standby link (vdev
 * id is invalid) if available.
 *
 * Return: QDF_STATUS_SUCCESS on success, otherwise QDF_STATUS_E_INVAL if no
 * suitable candidate link is found or required objects are missing.
 */
QDF_STATUS
mlo_link_recfg_set_tx_link_addr(
			struct mlo_link_recfg_context *recfg_ctx,
			struct wlan_mlo_link_recfg_req *recfg_req,
			struct mlo_link_recfg_state_req *req,
			uint32_t candidate_link_set);

/**
 * mlo_link_recfg_tranistion_to_next_state() - API to transition to next state
 * @recfg_ctx: Link reconfiguration context. Must contain a valid MLO dev ctx.
 *
 * Return: QDF_STATUS_SUCCESS on success, otherwise QDF_STATUS_E_FAILURE
 *
 */
QDF_STATUS
mlo_link_recfg_tranistion_to_next_state(
			struct mlo_link_recfg_context *recfg_ctx);

/**
 * mlo_link_recfg_update_state_req_from_rsp() - API to update state req from rsp
 * @recfg_ctx: Link reconfiguration context. Must contain a valid MLO dev ctx.
 * @tran: Link reconfiguration state tran ptr.
 *
 * Return: void
 *
 */
void
mlo_link_recfg_update_state_req_from_rsp(
			struct mlo_link_recfg_context *recfg_ctx,
			struct mlo_link_recfg_state_tran *tran);

/**
 * mlo_link_recfg_update_scan_mlme() - Update scan entry connection state
 * during link reconfiguration
 * @vdev: Pointer to vdev object manager. This vdev is used to identify the
 *        MLO connection and access the scan database for updating the
 *        connection state of the corresponding AP link entry.
 * @ap_link_addr: Pointer to the AP link MAC address (BSSID) for which the
 *                scan entry connection state needs to be updated. This
 *                identifies the specific link in the scan database.
 * @assoc_state: New connection state to be set for the scan entry. This
 *               indicates whether the link is being connected, disconnected,
 *               or in another connection state as defined by enum
 *               scan_entry_connection_state.
 *
 * This API updates the connection state of a scan entry in the scan database
 * during link reconfiguration operations. It is typically called when links
 * are being added or removed from an MLO connection to reflect the current
 * association state of each link. The scan entry is identified by the AP link
 * address (BSSID) and updated with the new connection state.
 *
 * Context: This function can be called during link reconfiguration state
 * transitions, particularly when adding or deleting links in an MLO connection.
 *
 * Return: void
 */
void
mlo_link_recfg_update_scan_mlme(struct wlan_objmgr_vdev *vdev,
				struct qdf_mac_addr *ap_link_addr,
				enum scan_entry_connection_state
				assoc_state);
#else
static inline void
mlo_link_recfg_update_scan_mlme(struct wlan_objmgr_vdev *vdev,
				struct qdf_mac_addr *ap_link_addr,
				enum scan_entry_connection_state
				assoc_state)
{
}

static inline void
mlo_link_recfg_update_state_req_from_rsp(
			struct mlo_link_recfg_context *recfg_ctx,
			struct mlo_link_recfg_state_tran *tran)
{
}

static inline QDF_STATUS
mlo_link_recfg_tranistion_to_next_state(
			struct mlo_link_recfg_context *recfg_ctx)
{
	return QDF_STATUS_E_NOSUPPORT;
}

static inline QDF_STATUS
mlo_link_recfg_set_tx_link_addr(
			struct mlo_link_recfg_context *recfg_ctx,
			struct wlan_mlo_link_recfg_req *recfg_req,
			struct mlo_link_recfg_state_req *req,
			uint32_t candidate_link_set)
{
	return QDF_STATUS_E_NOSUPPORT;
}

static inline QDF_STATUS
mlo_link_recfg_get_link_bitmap(struct mlo_link_recfg_context *recfg_ctx,
			       struct wlan_mlo_link_recfg_req *recfg_req,
			       uint32_t *add_link_set,
			       uint8_t *add_link_num,
			       uint32_t *del_link_set,
			       uint8_t *del_link_num,
			       uint32_t *curr_link_set,
			       uint8_t *curr_link_num,
			       uint32_t *curr_standby_set,
			       uint8_t *curr_standby_num)
{
	return QDF_STATUS_E_NOSUPPORT;
}

static inline QDF_STATUS
mlo_link_recfg_validate_roam_invoke(
		struct wlan_objmgr_psoc *psoc,
		struct wlan_objmgr_vdev *vdev)
{
	return QDF_STATUS_SUCCESS;
}

static inline void
mlo_link_recfg_install_unicast_keys(struct wlan_objmgr_vdev *vdev)
{
}

static inline QDF_STATUS
mlo_link_recfg_save_unicast_key(struct mlo_link_recfg_context *ctx,
				struct wlan_objmgr_vdev *vdev,
				struct qdf_mac_addr *link_addr,
				struct qdf_mac_addr *ap_link_addr,
				uint8_t link_id)
{
	return QDF_STATUS_E_NOSUPPORT;
}

static inline QDF_STATUS
mlo_link_recfg_store_key(struct mlo_link_recfg_context *ctx,
			 struct mlo_link_recfg_state_req *req)
{
	return QDF_STATUS_E_NOSUPPORT;
}

static inline void
mlo_link_recfg_abort_if_in_progress(struct wlan_objmgr_vdev *vdev,
				    bool is_link_switch_discon)
{
}

static inline void
mlo_link_recfg_ctx_free_ies(struct mlo_link_recfg_context *ctx)
{
}

static inline QDF_STATUS
mlo_link_recfg_set_mac_addr_resp(struct wlan_objmgr_vdev *vdev,
				 uint8_t resp_status)
{
	return QDF_STATUS_SUCCESS;
}

static inline void
mlo_link_recfg_set_link_resp(struct wlan_objmgr_vdev *vdev,
			     uint32_t result)
{
}

static inline QDF_STATUS
mlo_link_recfg_get_add_partner_links(
		struct wlan_objmgr_vdev *vdev,
		struct mlo_partner_info *ml_partner_info)
{
	return QDF_STATUS_SUCCESS;
}

static inline QDF_STATUS
mlo_mgr_link_recfg_indication_event_handler(
			struct wlan_objmgr_psoc *psoc,
			struct wlan_mlo_link_recfg_ind_param *evt_params)
{
	return QDF_STATUS_SUCCESS;
}

static inline uint8_t
mlo_link_recfg_dialog_token(struct mlo_link_recfg_context *recfg_ctx,
			    struct mlo_link_recfg_state_req *req)
{
	return 0;
}

static inline QDF_STATUS
mlo_link_recfg_send_request_frame(
		struct mlo_link_recfg_context *recfg_ctx,
		struct mlo_link_recfg_state_req *req)
{
	return QDF_STATUS_E_NOSUPPORT;
}

static inline QDF_STATUS
mlo_link_recfg_rx_rsp(struct wlan_objmgr_vdev *vdev,
		      enum wlan_link_recfg_sm_evt event,
		      uint8_t *rx_pkt_info)
{
	return QDF_STATUS_E_NOSUPPORT;
}

static inline QDF_STATUS
mlo_link_recfg_link_add_join_req(struct wlan_objmgr_vdev *vdev)
{
	return QDF_STATUS_E_NOSUPPORT;
}

static inline bool
mlo_link_recfg_is_start_as_active(struct wlan_objmgr_vdev *vdev)
{
	return false;
}

static inline QDF_STATUS
mlo_link_recfg_sm_deliver_event(struct wlan_mlo_dev_context *mlo_dev_ctx,
				enum wlan_link_recfg_sm_evt event,
				uint16_t data_len, void *data)
{
	return QDF_STATUS_E_NOSUPPORT;
}

static inline QDF_STATUS
mlo_link_recfg_sm_deliver_event_sync(struct wlan_mlo_dev_context *mlo_dev_ctx,
				     enum wlan_link_recfg_sm_evt event,
				     uint16_t data_len, void *data)
{
	return QDF_STATUS_E_NOSUPPORT;
}

static inline QDF_STATUS
mlo_link_recfg_linksw_start_indication(struct wlan_objmgr_vdev *vdev,
				       QDF_STATUS start_status)
{
	return QDF_STATUS_SUCCESS;
}

static inline QDF_STATUS
mlo_link_recfg_linksw_completion_indication(struct wlan_objmgr_vdev *vdev,
					    QDF_STATUS comp_status)
{
	return QDF_STATUS_SUCCESS;
}

static inline enum wlan_link_recfg_sm_state
mlo_link_recfg_get_state(struct wlan_mlo_dev_context *mlo_dev_ctx)
{
	return WLAN_LINK_RECFG_S_MAX;
}

static inline enum wlan_link_recfg_sm_state
mlo_link_recfg_get_substate(struct wlan_mlo_dev_context *mlo_dev_ctx)
{
	return WLAN_LINK_RECFG_SS_MAX;
}

static inline bool
mlo_is_link_recfg_supported(struct wlan_objmgr_vdev *vdev)
{
	return false;
}

static inline QDF_STATUS
mlo_link_recfg_init(struct wlan_objmgr_psoc *psoc,
		    struct wlan_mlo_dev_context *ml_dev)
{
	return QDF_STATUS_SUCCESS;
}

static inline void
mlo_link_recfg_init_state(struct wlan_mlo_dev_context *mlo_dev_ctx)
{
}

static inline QDF_STATUS
mlo_link_recfg_trans_next_state(struct wlan_mlo_dev_context *mlo_dev_ctx)
{
	return QDF_STATUS_E_INVAL;
}

static inline void
mlo_link_recfg_trans_abort_state(struct wlan_mlo_dev_context *mlo_dev_ctx)
{
}

static inline bool
mlo_is_link_recfg_in_progress(struct wlan_objmgr_vdev *vdev)
{
	return false;
}

static inline QDF_STATUS
mlo_link_recfg_notify(struct wlan_objmgr_vdev *vdev,
		      struct wlan_mlo_link_recfg_req *req)
{
	return QDF_STATUS_E_NOSUPPORT;
}

static inline QDF_STATUS
mlo_link_recfg_validate_request(struct wlan_objmgr_vdev *vdev,
				struct wlan_mlo_link_recfg_req *req)
{
	return QDF_STATUS_E_NOSUPPORT;
}

static inline QDF_STATUS
mlo_link_recfg_request_params(struct wlan_objmgr_psoc *psoc,
			      void *evt_params)
{
	return QDF_STATUS_E_NOSUPPORT;
}

static inline QDF_STATUS
mlo_link_recfg_create_transition_list(
			struct mlo_link_recfg_context *recfg_ctx,
			struct wlan_mlo_link_recfg_req *recfg_req)
{
	return QDF_STATUS_E_NOSUPPORT;
}

static inline struct wlan_mlo_dev_context *
mlo_link_recfg_get_mlo_ctx(struct mlo_link_recfg_context *recfg_ctx)
{
	return NULL;
}

static inline struct wlan_objmgr_psoc *
mlo_link_recfg_get_psoc(struct mlo_link_recfg_context *recfg_ctx)
{
	return NULL;
}

static inline struct mlo_link_recfg_state_tran *
mlo_link_recfg_get_curr_tran_req(struct mlo_link_recfg_context *recfg_ctx)
{
	return NULL;
}
#endif

/* =====================================================================
 * Target AP Capability Structures (used with WLAN_FEATURE_11BN_SMD)
 * ===================================================================== */

/**
 * struct smd_target_ap_link_caps - Target AP link capabilities for SMD roaming
 * @link_id: Link ID
 * @capability_info: Capability Information field (2 octets)
 * @capability_info_present: Whether capability info was extracted
 * @supported_rates: Supported Rates element
 * @ext_supported_rates: Extended Supported Rates element
 * @ht_cap: HT Capabilities element
 * @ht_cap_present: Whether HT capabilities were extracted
 * @vht_cap: VHT Capabilities element
 * @vht_cap_present: Whether VHT capabilities were extracted
 * @he_cap: HE Capabilities element
 * @he_cap_present: Whether HE capabilities were extracted
 * @eht_cap: EHT Capabilities element
 * @eht_cap_present: Whether EHT capabilities were extracted
 * @uhr_cap: UHR Capabilities element
 * @uhr_cap_present: Whether UHR capabilities were extracted
 * @ext_cap: Extended Capabilities element
 * @ext_cap_present: Whether extended capabilities were extracted
 *
 * Structure to hold extracted capabilities for a single link from
 * target AP's per-STA profile in Basic ML IE.
 */
struct smd_target_ap_link_caps {
	uint8_t link_id;
	uint16_t capability_info;
	bool capability_info_present;
	struct element_info supported_rates;
	struct element_info ext_supported_rates;
	struct element_info ht_cap;
	bool ht_cap_present;
	struct element_info vht_cap;
	bool vht_cap_present;
	struct element_info he_cap;
	bool he_cap_present;
	struct element_info eht_cap;
	bool eht_cap_present;
	struct element_info uhr_cap;
	bool uhr_cap_present;
	struct element_info ext_cap;
	bool ext_cap_present;
};

/**
 * struct smd_target_ap_caps - Complete target AP capabilities for SMD roaming
 * @num_links: Number of links with extracted capabilities
 * @link_caps: Array of per-link capabilities
 * @mld_capabilities: MLD-level capabilities (EMLSR, max links, etc.)
 * @mld_cap_present: Whether MLD capabilities were extracted
 */
struct smd_target_ap_caps {
	uint8_t num_links;
	struct smd_target_ap_link_caps link_caps[WLAN_MAX_ML_BSS_LINKS];
	/* MLD-level capabilities from Common Info */
	struct {
		bool emlsr_support;
		uint8_t max_simultaneous_links;
	} mld_capabilities;
	bool mld_cap_present;
};

#ifdef WLAN_FEATURE_11BN_SMD

static inline void
wlan_mlo_init_smd_target_ap_caps(struct smd_target_ap_caps *caps)
{
	if (!caps)
		return;
	qdf_mem_zero(caps, sizeof(*caps));
}

static inline void
wlan_mlo_cleanup_smd_target_ap_caps(struct smd_target_ap_caps *caps)
{
	uint8_t i;

	if (!caps)
		return;

	for (i = 0; i < caps->num_links; i++) {
		struct smd_target_ap_link_caps *link_cap = &caps->link_caps[i];

		qdf_mem_free(link_cap->supported_rates.ptr);
		link_cap->supported_rates.ptr = NULL;
		qdf_mem_free(link_cap->ext_supported_rates.ptr);
		link_cap->ext_supported_rates.ptr = NULL;
		qdf_mem_free(link_cap->ht_cap.ptr);
		link_cap->ht_cap.ptr = NULL;
		qdf_mem_free(link_cap->vht_cap.ptr);
		link_cap->vht_cap.ptr = NULL;
		qdf_mem_free(link_cap->he_cap.ptr);
		link_cap->he_cap.ptr = NULL;
		qdf_mem_free(link_cap->eht_cap.ptr);
		link_cap->eht_cap.ptr = NULL;
		qdf_mem_free(link_cap->uhr_cap.ptr);
		link_cap->uhr_cap.ptr = NULL;
		qdf_mem_free(link_cap->ext_cap.ptr);
		link_cap->ext_cap.ptr = NULL;
	}

	qdf_mem_zero(caps, sizeof(*caps));
}

/**
 * extract_target_ap_capabilities() - Extract target AP capabilities from scan cache
 * @mlo_ie: Pointer to ML IE data
 * @mlo_ie_len: Length of ML IE
 * @target_caps: Output structure to store extracted capabilities
 * @ctx: Link reconfiguration context
 *
 * Return: QDF_STATUS_SUCCESS on success, error code otherwise
 */
QDF_STATUS
extract_target_ap_capabilities(uint8_t *mlo_ie,
                               qdf_size_t mlo_ie_len,
                               struct smd_target_ap_caps *target_caps,
                               struct mlo_link_recfg_context *ctx);

QDF_STATUS
mlo_uhr_link_recfg_parse_st_prep_rsp(
	struct mlo_link_recfg_context *ctx,
	struct wlan_mlo_link_recfg_rsp *link_recfg_rsp,
	uint8_t *rx_pkt_info,
	struct wlan_action_frame *action_frm,
	uint16_t *ie_offset);

#else
static inline QDF_STATUS
extract_target_ap_capabilities(uint8_t *mlo_ie,
                               qdf_size_t mlo_ie_len,
                               struct smd_target_ap_caps *target_caps,
                               struct mlo_link_recfg_context *ctx)
{
	return QDF_STATUS_E_NOSUPPORT;
}

static inline QDF_STATUS
mlo_uhr_link_recfg_parse_st_prep_rsp(
	struct mlo_link_recfg_context *ctx,
	struct wlan_mlo_link_recfg_rsp *link_recfg_rsp,
	uint8_t *rx_pkt_info,
	struct wlan_action_frame *action_frm,
	uint16_t *ie_offset)
{
	return QDF_STATUS_E_NOSUPPORT;
}
#endif /* WLAN_FEATURE_11BN_SMD */
#endif /* _WLAN_MLO_LINK_RECFG_H_ */
