/*
 * Copyright (c) 2020-2021, The Linux Foundation. All rights reserved.
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
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

/**
 * DOC: This file init/deint functions for dcs module.
 */

#include "wlan_dcs_init_deinit_api.h"
#include "../../core/src/wlan_dcs.h"
#include "cfg_dcs.h"
#include "cfg_ucfg_api.h"
#include "qdf_util.h"

/**
 * dcs_deinit_core_priv_obj() - Deinitialize DCS core private object
 * @dcs_core: pointer to DCS core private object
 *
 * This function deinitializes the DCS core private object by freeing
 * the timer and destroying the spinlock.
 *
 * Return: QDF_STATUS_SUCCESS for success or error code
 */
static QDF_STATUS
dcs_deinit_core_priv_obj(struct dcs_core_priv_obj *dcs_core)
{
	if (!dcs_core) {
		dcs_err("NULL dcs core");
		return QDF_STATUS_E_FAILURE;
	}

	qdf_timer_free(&dcs_core->dcs_disable_timer);
	qdf_spinlock_destroy(&dcs_core->lock);

	return QDF_STATUS_SUCCESS;
}

/**
 * dcs_deinit_pdev_priv_obj() - Deinitialize DCS PDEV private objects
 * @psoc: psoc object
 *
 * This function deinitializes the DCS PDEV private objects by deinitializing
 * the DCS core objects and freeing the memory.
 *
 * Return: QDF_STATUS_SUCCESS for success or error code
 */
static QDF_STATUS
dcs_deinit_pdev_priv_obj(struct wlan_objmgr_psoc *psoc)
{
	struct dcs_psoc_priv_obj *dcs_psoc_obj;
	uint8_t loop;

	if (!psoc) {
		dcs_err("psoc is NULL");
		return QDF_STATUS_E_INVAL;
	}

	dcs_psoc_obj = wlan_objmgr_psoc_get_comp_private_obj(
			psoc, WLAN_UMAC_COMP_DCS);
	if (!dcs_psoc_obj) {
		dcs_err("dcs psoc private object is NULL");
		return QDF_STATUS_E_FAILURE;
	}

	for (loop = 0; loop < WLAN_DCS_MAX_PDEVS; loop++) {
		if (dcs_psoc_obj->dcs_pdev_priv[loop].dcs_core) {
			dcs_deinit_core_priv_obj(
				dcs_psoc_obj->dcs_pdev_priv[loop].dcs_core);
			qdf_mem_free(
				dcs_psoc_obj->dcs_pdev_priv[loop].dcs_core);
			dcs_psoc_obj->dcs_pdev_priv[loop].dcs_core = NULL;
		}
	}
	dcs_debug("deinit core obj done");

	return QDF_STATUS_SUCCESS;
}

/**
 * dcs_init_core_priv_obj() - Initialize DCS core private object
 * @psoc: psoc object
 * @dcs_core: pointer to DCS core private object
 * @dcs_mode: DCS mode (SAP, XPAN, XR, GO)
 *
 * This function initializes the DCS core private object with values from
 * the psoc_dcs_params structure based on the specified dcs_mode.
 *
 * Return: QDF_STATUS_SUCCESS for success or error code
 */
static QDF_STATUS
dcs_init_core_priv_obj(struct wlan_objmgr_psoc *psoc,
		       struct dcs_core_priv_obj *dcs_core,
		       enum wlan_dcs_mode dcs_mode)
{
	QDF_STATUS status;

	if (!psoc || !dcs_core) {
		dcs_err("NULL param");
		return QDF_STATUS_E_INVAL;
	}

	dcs_init_params_by_mode(psoc, dcs_core, dcs_mode);

	dcs_core->dcs_host_params.dcs_algorithm_process = false;
	dcs_core->dcs_host_params.dcs_enable_timestamp = 0;
	dcs_core->dcs_freq_ctrl_params.dcs_happened_count = 0;
	dcs_core->dcs_freq_ctrl_params.disable_delay_process = false;

	/* Initialize statistics */
	qdf_mem_zero(&dcs_core->dcs_im_stats, sizeof(struct dcs_im_stats));

	/* Initialize spinlock and timer */
	qdf_spinlock_create(&dcs_core->lock);
	status = qdf_timer_init(NULL, &dcs_core->dcs_disable_timer,
				wlan_dcs_disable_timer_fn,
				&dcs_core->dcs_timer_args,
				QDF_TIMER_TYPE_WAKE_APPS);
	if (QDF_IS_STATUS_ERROR(status)) {
		dcs_err("timer init failed, mode %d", dcs_mode);
		qdf_spinlock_destroy(&dcs_core->lock);
		return status;
	}

	dcs_debug("DCS core obj init done, mode %d", dcs_mode);

	return QDF_STATUS_SUCCESS;
}

/**
 * dcs_init_pdev_priv_obj() - Initialize DCS PDEV private objects
 * @psoc: psoc object
 *
 * This function initializes the DCS PDEV private objects by allocating
 * memory for the DCS core objects and initializing them.
 *
 * Return: QDF_STATUS_SUCCESS for success or error code
 */
static QDF_STATUS
dcs_init_pdev_priv_obj(struct wlan_objmgr_psoc *psoc)
{
	struct dcs_psoc_priv_obj *dcs_psoc_obj;
	uint8_t loop;
	QDF_STATUS status;

	if (!psoc) {
		dcs_err("psoc is NULL");
		return QDF_STATUS_E_INVAL;
	}

	dcs_psoc_obj = wlan_objmgr_psoc_get_comp_private_obj(
			psoc, WLAN_UMAC_COMP_DCS);
	if (!dcs_psoc_obj) {
		dcs_err("dcs psoc private object is NULL");
		return QDF_STATUS_E_FAILURE;
	}

	for (loop = 0; loop < WLAN_DCS_MAX_PDEVS; loop++) {
		dcs_psoc_obj->dcs_pdev_priv[loop].dcs_core =
			qdf_mem_malloc(sizeof(struct dcs_core_priv_obj));
		if (!dcs_psoc_obj->dcs_pdev_priv[loop].dcs_core) {
			dcs_err("alloc fail, %d", loop);
			dcs_deinit_pdev_priv_obj(psoc);
			return QDF_STATUS_E_NOMEM;
		}
		status = dcs_init_core_priv_obj(
				psoc,
				dcs_psoc_obj->dcs_pdev_priv[loop].dcs_core,
				DCS_SAP);
		if (QDF_IS_STATUS_ERROR(status)) {
			dcs_err("core init failed, %d", loop);
			dcs_deinit_pdev_priv_obj(psoc);
			return status;
		}
	}
	dcs_debug("init core obj done");

	return QDF_STATUS_SUCCESS;
}

/**
 * wlan_dcs_psoc_obj_create_notification() - dcs psoc create handler
 * @psoc: psoc object
 * @arg_list: Argument list
 *
 * return: QDF_STATUS_SUCCESS for success or error code
 */
static QDF_STATUS
wlan_dcs_psoc_obj_create_notification(struct wlan_objmgr_psoc *psoc,
				      void *arg_list)
{
	QDF_STATUS status;
	struct dcs_psoc_priv_obj *dcs_psoc_obj;
	uint8_t loop;

	dcs_psoc_obj = qdf_mem_malloc(sizeof(*dcs_psoc_obj));

	if (!dcs_psoc_obj)
		return QDF_STATUS_E_NOMEM;

	/* Initialize VDEV tracking for concurrent DCS limit enforcement */
	dcs_psoc_obj->dcs_enable_count = 0;
	qdf_spinlock_create(&dcs_psoc_obj->dcs_psoc_lock);
	for (loop = 0; loop < WLAN_DCS_MAX_VDEVS; loop++)
		dcs_psoc_obj->enabled_vdev_ids[loop] = DCS_INVALID_VDEV_ID;

	status = wlan_objmgr_psoc_component_obj_attach(psoc,
						       WLAN_UMAC_COMP_DCS,
						       dcs_psoc_obj,
						       QDF_STATUS_SUCCESS);

	if (QDF_IS_STATUS_ERROR(status)) {
		dcs_err("dcs pdev obj attach failed");
		qdf_mem_free(dcs_psoc_obj);
		return status;
	}

	dcs_info("dcs psoc object attached");

	return status;
}

/**
 * wlan_dcs_psoc_obj_destroy_notification() - dcs psoc destroy handler
 * @psoc: psoc object
 * @arg_list: Argument list
 *
 * return: QDF_STATUS_SUCCESS for success or error code
 */
static QDF_STATUS
wlan_dcs_psoc_obj_destroy_notification(struct wlan_objmgr_psoc *psoc,
				       void *arg_list)
{
	QDF_STATUS status;
	struct dcs_psoc_priv_obj *dcs_psoc_obj =
		wlan_objmgr_psoc_get_comp_private_obj(psoc, WLAN_UMAC_COMP_DCS);

	if (!dcs_psoc_obj) {
		dcs_err("invalid wifi dcs obj");
		return QDF_STATUS_E_FAULT;
	}

	/* Cleanup VDEV tracking */
	qdf_spinlock_destroy(&dcs_psoc_obj->dcs_psoc_lock);

	dcs_deinit_pdev_priv_obj(psoc);
	status = wlan_objmgr_psoc_component_obj_detach(psoc,
						       WLAN_UMAC_COMP_DCS,
						       dcs_psoc_obj);
	qdf_mem_free(dcs_psoc_obj);

	return status;
}

/**
 * wlan_dcs_pdev_obj_create_notification() - dcs pdev create handler
 * @pdev: pdev object
 * @arg_list: Argument list
 *
 * return: QDF_STATUS_SUCCESS for success or error code
 */
static QDF_STATUS
wlan_dcs_pdev_obj_create_notification(struct wlan_objmgr_pdev *pdev,
				      void *arg_list)
{
	QDF_STATUS status = QDF_STATUS_SUCCESS;
	struct wlan_objmgr_psoc *psoc = wlan_pdev_get_psoc(pdev);

	if (!psoc) {
		dcs_err("NULL psoc");
		return QDF_STATUS_E_FAULT;
	}

	if (wlan_is_vdev_level_dcs_supported(psoc))
		return status;

	/* DCS pdev object created per mac */
	status = dcs_init_pdev_priv_obj(psoc);
	dcs_debug("init pdev obj done");

	return status;
}

/**
 * wlan_dcs_pdev_obj_destroy_notification() - dcs pdev destroy handler
 * @pdev: pdev object
 * @arg_list: Argument list
 *
 * return: QDF_STATUS_SUCCESS for success or error code
 */
static QDF_STATUS
wlan_dcs_pdev_obj_destroy_notification(struct wlan_objmgr_pdev *pdev,
				       void *arg_list)
{
	QDF_STATUS status = QDF_STATUS_SUCCESS;
	struct wlan_objmgr_psoc *psoc = wlan_pdev_get_psoc(pdev);

	if (!psoc) {
		dcs_err("NULL psoc");
		return QDF_STATUS_E_FAULT;
	}

	if (wlan_is_vdev_level_dcs_supported(psoc))
		return status;

	status = dcs_deinit_pdev_priv_obj(psoc);
	dcs_debug("deinit pdev obj done");

	return status;
}

/**
 * dcs_vdev_obj_create_notification() - dcs vdev create handler
 * @vdev: vdev object
 * @arg_list: Argument list
 *
 * This function gets called from object manager when vdev is being created
 *
 * Return: QDF_STATUS_SUCCESS for success or error code
 */
static QDF_STATUS
dcs_vdev_obj_create_notification(struct wlan_objmgr_vdev *vdev, void *arg_list)
{
	struct dcs_vdev_priv_obj *dcs_vdev_obj;
	QDF_STATUS status;
	enum QDF_OPMODE opmode;
	struct wlan_objmgr_psoc *psoc;
	enum wlan_dcs_mode dcs_mode;

	psoc = wlan_vdev_get_psoc(vdev);
	if (!psoc) {
		dcs_err("psoc is null");
		return QDF_STATUS_E_INVAL;
	}

	if (!wlan_is_vdev_level_dcs_supported(psoc)) {
		dcs_debug("vdev DCS not support");
		return QDF_STATUS_SUCCESS;
	}

	opmode = wlan_vdev_mlme_get_opmode(vdev);
	/* Create DCS vdev object only for SAP or P2P GO modes */
	if (opmode == QDF_P2P_GO_MODE) {
		dcs_mode = DCS_GO;
	} else if (opmode == QDF_SAP_MODE) {
		dcs_mode = DCS_SAP;
	} else {
		dcs_debug("Skip DCS vdev obj for opmode %d", opmode);
		return QDF_STATUS_SUCCESS;
	}

	dcs_vdev_obj = qdf_mem_malloc(sizeof(*dcs_vdev_obj));
	if (!dcs_vdev_obj)
		return QDF_STATUS_E_NOMEM;

	dcs_vdev_obj->dcs_core =
		qdf_mem_malloc(sizeof(*dcs_vdev_obj->dcs_core));
	if (!dcs_vdev_obj->dcs_core) {
		qdf_mem_free(dcs_vdev_obj);
		return QDF_STATUS_E_NOMEM;
	}

	status = dcs_init_core_priv_obj(psoc, dcs_vdev_obj->dcs_core, dcs_mode);
	if (QDF_IS_STATUS_ERROR(status)) {
		dcs_err("dcs core obj init failed");
		qdf_mem_free(dcs_vdev_obj->dcs_core);
		qdf_mem_free(dcs_vdev_obj);
		return status;
	}

	status = wlan_objmgr_vdev_component_obj_attach(vdev,
						       WLAN_UMAC_COMP_DCS,
						       dcs_vdev_obj,
						       QDF_STATUS_SUCCESS);

	if (QDF_IS_STATUS_ERROR(status)) {
		dcs_err("dcs vdev obj attach failed");
		dcs_deinit_core_priv_obj(dcs_vdev_obj->dcs_core);
		qdf_mem_free(dcs_vdev_obj->dcs_core);
		qdf_mem_free(dcs_vdev_obj);
		return status;
	}

	dcs_debug("dcs vdev object attached for opmode %d", opmode);

	return status;
}

/**
 * dcs_vdev_obj_destroy_notification() - dcs vdev destroy handler
 * @vdev: vdev object
 * @arg_list: Argument list
 *
 * This function gets called from object manager when vdev is being destroyed
 *
 * Return: QDF_STATUS_SUCCESS for success or error code
 */
static QDF_STATUS
dcs_vdev_obj_destroy_notification(struct wlan_objmgr_vdev *vdev, void *arg_list)
{
	struct dcs_vdev_priv_obj *dcs_vdev_obj;
	QDF_STATUS status;
	struct wlan_objmgr_psoc *psoc;

	psoc = wlan_vdev_get_psoc(vdev);
	if (!psoc) {
		dcs_err("psoc is null");
		return QDF_STATUS_E_INVAL;
	}

	if (!wlan_is_vdev_level_dcs_supported(psoc))
		return QDF_STATUS_SUCCESS;

	dcs_vdev_obj = wlan_objmgr_vdev_get_comp_private_obj(
						vdev, WLAN_UMAC_COMP_DCS);
	if (!dcs_vdev_obj) {
		dcs_debug("no vdev obj, skip cleanup");
		return QDF_STATUS_SUCCESS;
	}

	status = wlan_objmgr_vdev_component_obj_detach(vdev,
						       WLAN_UMAC_COMP_DCS,
						       dcs_vdev_obj);
	if (QDF_IS_STATUS_ERROR(status)) {
		dcs_err("dcs vdev obj detach failed");
		return status;
	}

	if (dcs_vdev_obj->dcs_core) {
		dcs_deinit_core_priv_obj(dcs_vdev_obj->dcs_core);
		qdf_mem_free(dcs_vdev_obj->dcs_core);
	}

	qdf_mem_free(dcs_vdev_obj);

	return status;
}

QDF_STATUS wlan_dcs_init(void)
{
	QDF_STATUS status = QDF_STATUS_SUCCESS;

	status = wlan_objmgr_register_psoc_create_handler(
			WLAN_UMAC_COMP_DCS,
			wlan_dcs_psoc_obj_create_notification,
			NULL);

	if (QDF_IS_STATUS_ERROR(status))
		goto err_psoc_create;

	status = wlan_objmgr_register_psoc_destroy_handler(
			WLAN_UMAC_COMP_DCS,
			wlan_dcs_psoc_obj_destroy_notification,
			NULL);

	if (QDF_IS_STATUS_ERROR(status))
		goto err_psoc_delete;

	status = wlan_objmgr_register_pdev_create_handler(
			WLAN_UMAC_COMP_DCS,
			wlan_dcs_pdev_obj_create_notification,
			NULL);

	if (QDF_IS_STATUS_ERROR(status)) {
		dcs_err("Failed to register dcs pdev create handler");
		goto err_pdev_create;
	}

	status = wlan_objmgr_register_pdev_destroy_handler(
			WLAN_UMAC_COMP_DCS,
			wlan_dcs_pdev_obj_destroy_notification,
			NULL);

	if (QDF_IS_STATUS_ERROR(status)) {
		dcs_err("Failed to register dcs pdev destroy handler");
		goto err_pdev_delete;
	}

	status = wlan_objmgr_register_vdev_create_handler(
			WLAN_UMAC_COMP_DCS,
			dcs_vdev_obj_create_notification,
			NULL);

	if (QDF_IS_STATUS_ERROR(status)) {
		dcs_err("Failed to register dcs vdev create handler");
		goto err_vdev_create;
	}

	status = wlan_objmgr_register_vdev_destroy_handler(
			WLAN_UMAC_COMP_DCS,
			dcs_vdev_obj_destroy_notification,
			NULL);

	if (QDF_IS_STATUS_ERROR(status)) {
		dcs_err("Failed to register dcs vdev destroy handler");
		goto err_vdev_delete;
	}

	return QDF_STATUS_SUCCESS;

err_vdev_delete:
	wlan_objmgr_unregister_vdev_create_handler(
			WLAN_UMAC_COMP_DCS,
			dcs_vdev_obj_create_notification,
			NULL);
err_vdev_create:
	wlan_objmgr_unregister_pdev_destroy_handler(
			WLAN_UMAC_COMP_DCS,
			wlan_dcs_pdev_obj_destroy_notification,
			NULL);
err_pdev_delete:
	wlan_objmgr_unregister_pdev_create_handler(
			WLAN_UMAC_COMP_DCS,
			wlan_dcs_pdev_obj_create_notification,
			NULL);
err_pdev_create:
	wlan_objmgr_unregister_psoc_destroy_handler(
			WLAN_UMAC_COMP_DCS,
			wlan_dcs_psoc_obj_destroy_notification,
			NULL);
err_psoc_delete:
	wlan_objmgr_unregister_psoc_create_handler(
			WLAN_UMAC_COMP_DCS,
			wlan_dcs_psoc_obj_create_notification,
			NULL);
err_psoc_create:
	return status;
}

QDF_STATUS wlan_dcs_deinit(void)
{
	QDF_STATUS status = QDF_STATUS_SUCCESS;

	status = wlan_objmgr_unregister_vdev_destroy_handler(
			WLAN_UMAC_COMP_DCS,
			dcs_vdev_obj_destroy_notification,
			NULL);

	if (QDF_IS_STATUS_ERROR(status))
		return QDF_STATUS_E_FAILURE;

	status = wlan_objmgr_unregister_vdev_create_handler(
			WLAN_UMAC_COMP_DCS,
			dcs_vdev_obj_create_notification,
			NULL);

	if (QDF_IS_STATUS_ERROR(status))
		return QDF_STATUS_E_FAILURE;

	status = wlan_objmgr_unregister_psoc_create_handler(
			WLAN_UMAC_COMP_DCS,
			wlan_dcs_psoc_obj_create_notification,
			NULL);

	if (QDF_IS_STATUS_ERROR(status))
		return QDF_STATUS_E_FAILURE;

	status = wlan_objmgr_unregister_psoc_destroy_handler(
			WLAN_UMAC_COMP_DCS,
			wlan_dcs_psoc_obj_destroy_notification,
			NULL);

	if (QDF_IS_STATUS_ERROR(status))
		return QDF_STATUS_E_FAILURE;

	return QDF_STATUS_SUCCESS;
}

QDF_STATUS wlan_dcs_enable(struct wlan_objmgr_psoc *psoc)
{
	return wlan_dcs_attach(psoc);
}

QDF_STATUS wlan_dcs_disable(struct wlan_objmgr_psoc *psoc)
{
	return wlan_dcs_detach(psoc);
}

#ifdef WLAN_FEATURE_VDEV_DCS
static void
wlan_dcs_psoc_int_vdev(struct wlan_objmgr_psoc *psoc,
		       struct dcs_psoc_priv_obj *dcs_psoc_priv)
{
	uint8_t mode;
	uint32_t ini_val;
	struct psoc_dcs_params *dcs_per_mode_param;

	if (!psoc || !dcs_psoc_priv) {
		dcs_err("null psoc or priv");
		return;
	}
	dcs_per_mode_param = &dcs_psoc_priv->dcs_per_mode_param;
	for (mode = 0; mode < MAX_DCS_MODE_NUM; mode++) {
		ini_val = cfg_get(psoc, CFG_DCS_ENABLE_PER_MODE);
		dcs_per_mode_param->dcs_enable_cfg[mode].val =
			QDF_GET_BITS(ini_val, mode * 8, 8);
		ini_val = cfg_get(psoc,
				  CFG_DCS_INTFR_DETECTION_THRESHOLD_PER_MODE);
		dcs_per_mode_param->intfr_detection_threshold[mode] =
			QDF_GET_BITS(ini_val, mode * 8, 8);
	}
}
#else
static void
wlan_dcs_psoc_int_vdev(struct wlan_objmgr_psoc *psoc,
		       struct dcs_psoc_priv_obj *dcs_psoc_priv)
{
}
#endif

QDF_STATUS wlan_dcs_psoc_open(struct wlan_objmgr_psoc *psoc)
{
	struct dcs_psoc_priv_obj *dcs_psoc_obj;

	if (!psoc) {
		dcs_err("psoc is NULL");
		return QDF_STATUS_E_INVAL;
	}

	dcs_psoc_obj = wlan_objmgr_psoc_get_comp_private_obj(
			psoc, WLAN_UMAC_COMP_DCS);
	if (!dcs_psoc_obj) {
		dcs_err("dcs psoc private object is NULL");
		return QDF_STATUS_E_FAILURE;
	}

	dcs_psoc_obj->dcs_per_mode_param.dcs_debug =
				cfg_get(psoc, CFG_DCS_DEBUG);

	/*
	 * The following two configurations are legacy settings,
	 * as dedicated per‑mode configs now exist.
	 */
	dcs_psoc_obj->dcs_enable_cfg = cfg_get(psoc, CFG_DCS_ENABLE);
	dcs_psoc_obj->intfr_detection_threshold =
			cfg_get(psoc, CFG_DCS_INTFR_DETECTION_THRESHOLD);

	DCS_CFG_GET(psoc, PHY_ERR_PENALTY, uint32_t,
		    dcs_psoc_obj->dcs_per_mode_param.dcs_phy_err_penalty);
	DCS_CFG_GET(psoc, PHY_ERR_THRESHOLD, uint32_t,
		    dcs_psoc_obj->dcs_per_mode_param.dcs_phy_err_threshold);
	DCS_CFG_GET(psoc, RADAR_ERR_THRESHOLD, uint32_t,
		    dcs_psoc_obj->dcs_per_mode_param.dcs_radar_err_threshold);
	DCS_CFG_GET(psoc, COCH_INTFR_THRESHOLD, uint32_t,
		    dcs_psoc_obj->dcs_per_mode_param.dcs_coch_intfr_threshold);
	DCS_CFG_GET(
		psoc, TRANSPORT_SWITCH_RJT_TH_CU, uint32_t,
		dcs_psoc_obj->dcs_per_mode_param.dcs_trnsprt_rjt_threshold_cu);
	DCS_CFG_GET(psoc, USER_MAX_CU, uint32_t,
		    dcs_psoc_obj->dcs_per_mode_param.user_max_cu);
	DCS_CFG_GET(
		psoc, INTFR_DETECTION_WINDOW, uint32_t,
		dcs_psoc_obj->dcs_per_mode_param.dcs_intfr_detection_window);
	DCS_CFG_GET(psoc, TX_ERR_THRESHOLD, uint32_t,
		    dcs_psoc_obj->dcs_per_mode_param.dcs_tx_err_threshold);
	DCS_CFG_GET(psoc, DISABLE_ALGORITHM, bool,
		    dcs_psoc_obj->dcs_per_mode_param.dcs_disable_algorithm);
	DCS_CFG_GET(
		psoc, DISABLE_THRESHOLD_PER_5MINS, uint8_t,
		dcs_psoc_obj->dcs_per_mode_param.dcs_disable_thresh_per_5mins);
	DCS_CFG_GET(psoc, RESTART_DELAY, uint32_t,
		    dcs_psoc_obj->dcs_per_mode_param.dcs_restart_delay);

	wlan_dcs_psoc_int_vdev(psoc, dcs_psoc_obj);

	return QDF_STATUS_SUCCESS;
}
