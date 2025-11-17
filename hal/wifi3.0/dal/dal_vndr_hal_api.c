/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: ISC
 */

#include "dal_vndr_hal_api.h"
#include "dal_vndr_hal_internal.h"
#include "dal_vndr_hal_be.h"

/**
 * dal_vndr_hal_ops_attach() - Attach HAL operations
 * @hal_soc: HAL SoC handle
 *
 * This function attaches the default HAL operations.
 *
 * Return: void
 */
void dal_vndr_hal_ops_attach(void *hal_soc)
{
	struct dal_vndr_hal_soc *hal_soc_hdl =
		(struct dal_vndr_hal_soc *)hal_soc;

	if (!hal_soc_hdl)
		return;
	/*
	 * Currently directly calling BE ops attach
	 * In future, this will be based on target type
	 * and call appropriate ops attach function
	 * based on target type
	 * For now, only BE is supported, so directly calling
	 * BE ops attach function
	 */
	dal_vndr_hal_default_ops_attach_be(hal_soc_hdl);
}
