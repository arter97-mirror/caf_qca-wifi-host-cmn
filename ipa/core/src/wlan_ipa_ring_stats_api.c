/*
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
 * DOC: wlan_ipa_ring_stats_api.c
 *
 * IPA ring stats component init/deinit implementation.
 */

#include <wlan_ipa_ring_stats_api.h>
#include <wlan_ipa_ring_stats_priv_api.h>
#include <wlan_objmgr_global_obj.h>
#include <target_if_ipa_ring_stats.h>
#include <wlan_hdd_main.h>

#ifdef WLAN_FEATURE_IPA_RING_STATS

/**
 * ipa_ring_stats_psoc_obj_created_notification() - PSOC create callback
 * @psoc: PSOC object
 * @arg_list: variable argument list (unused)
 *
 * This callback is registered with the object manager during initialization
 * to get notified when a PSOC object is created. It allocates and attaches
 * the IPA ring stats psoc private object.
 *
 * Return: QDF_STATUS_SUCCESS on success, QDF_STATUS_E_** on error
 */
static QDF_STATUS
ipa_ring_stats_psoc_obj_created_notification(struct wlan_objmgr_psoc *psoc,
					     void *arg_list)
{
	QDF_STATUS status = QDF_STATUS_SUCCESS;
	struct ipa_ring_stats_psoc_priv_obj *ipa_obj;

	ipa_obj = qdf_mem_malloc(sizeof(*ipa_obj));
	if (!ipa_obj)
		return QDF_STATUS_E_NOMEM;

	qdf_mem_zero(ipa_obj, sizeof(*ipa_obj));
	qdf_spinlock_create(&ipa_obj->lock);
	ipa_obj->psoc = psoc;

	status = wlan_objmgr_psoc_component_obj_attach(psoc,
						       WLAN_UMAC_COMP_IPA_RING_STATS,
						       ipa_obj,
						       QDF_STATUS_SUCCESS);
	if (QDF_IS_STATUS_ERROR(status)) {
		ipa_stats_err("ipa_ring_stats obj attach with psoc failed");
		goto psoc_attach_failed;
	}

	return QDF_STATUS_SUCCESS;

psoc_attach_failed:
	qdf_spinlock_destroy(&ipa_obj->lock);
	qdf_mem_free(ipa_obj);

	return status;
}

/**
 * ipa_ring_stats_psoc_obj_destroyed_notification() - PSOC delete callback
 * @psoc: PSOC object
 * @arg_list: variable argument list (unused)
 *
 * This callback is registered with the object manager during initialization
 * to get notified when a PSOC object is deleted. It detaches and frees the
 * IPA ring stats psoc private object.
 *
 * Return: QDF_STATUS_SUCCESS on success, QDF_STATUS_E_** on error
 */
static QDF_STATUS
ipa_ring_stats_psoc_obj_destroyed_notification(struct wlan_objmgr_psoc *psoc,
					       void *arg_list)
{
	QDF_STATUS status = QDF_STATUS_SUCCESS;
	struct ipa_ring_stats_psoc_priv_obj *ipa_obj;

	ipa_obj = ipa_ring_stats_get_psoc_priv_obj(psoc);
	if (!ipa_obj) {
		ipa_stats_err("ipa_ring_stats psoc private obj is NULL");
		return QDF_STATUS_E_FAULT;
	}

	status = wlan_objmgr_psoc_component_obj_detach(psoc,
						       WLAN_UMAC_COMP_IPA_RING_STATS,
						       ipa_obj);
	if (QDF_IS_STATUS_ERROR(status))
		ipa_stats_err("ipa_ring_stats psoc obj detach failed");

	qdf_spinlock_destroy(&ipa_obj->lock);
	qdf_mem_free(ipa_obj);

	return status;
}

/**
 * wlan_ipa_ring_stats_init() - init IPA ring stats component
 *
 * Register PSOC create/destroy callbacks and initialize globals used
 * for synchronous IPA ring stats requests.
 *
 * Return: QDF_STATUS_SUCCESS on success, QDF_STATUS_E_** on error
 */
QDF_STATUS wlan_ipa_ring_stats_init(void)
{
	QDF_STATUS status;

	status = wlan_objmgr_register_psoc_create_handler(
				WLAN_UMAC_COMP_IPA_RING_STATS,
				ipa_ring_stats_psoc_obj_created_notification,
				NULL);
	if (QDF_IS_STATUS_ERROR(status)) {
		ipa_stats_err("register psoc create handler failed");
		return status;
	}

	status = wlan_objmgr_register_psoc_destroy_handler(
				WLAN_UMAC_COMP_IPA_RING_STATS,
				ipa_ring_stats_psoc_obj_destroyed_notification,
				NULL);
	if (QDF_IS_STATUS_ERROR(status)) {
		ipa_stats_err("register psoc destroy handler failed");
		goto unregister_psoc_create;
	}

	return QDF_STATUS_SUCCESS;

unregister_psoc_create:
	wlan_objmgr_unregister_psoc_create_handler(
				WLAN_UMAC_COMP_IPA_RING_STATS,
				ipa_ring_stats_psoc_obj_created_notification,
				NULL);

	return status;
}

/**
 * wlan_ipa_ring_stats_deinit() - deinit IPA ring stats component
 *
 * Unregister PSOC create/destroy callbacks and clear globals used
 * for synchronous IPA ring stats requests.
 *
 * Return: QDF_STATUS_SUCCESS on success, QDF_STATUS_E_** on error
 */
QDF_STATUS wlan_ipa_ring_stats_deinit(void)
{
	QDF_STATUS ret = QDF_STATUS_SUCCESS;
	QDF_STATUS status;

	status = wlan_objmgr_unregister_psoc_destroy_handler(
				WLAN_UMAC_COMP_IPA_RING_STATS,
				ipa_ring_stats_psoc_obj_destroyed_notification,
				NULL);
	if (QDF_IS_STATUS_ERROR(status)) {
		ipa_stats_err("unregister psoc destroy handler failed");
		ret = status;
	}

	status = wlan_objmgr_unregister_psoc_create_handler(
				WLAN_UMAC_COMP_IPA_RING_STATS,
				ipa_ring_stats_psoc_obj_created_notification,
				NULL);
	if (QDF_IS_STATUS_ERROR(status)) {
		ipa_stats_err("unregister psoc create handler failed");
		ret = status;
	}

	return ret;
}

#endif /* WLAN_FEATURE_IPA_RING_STATS */
