/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: ISC
 */

#ifndef _DAL_VNDR_HAL_API_H_
#define _DAL_VNDR_HAL_API_H_

#include "dal_vndr_hal_be.h"
#include "dal_vndr_hal_internal.h"

/**
 * dal_vndr_hal_srng_access_start() - Start (locked) ring access
 *
 * @hal_soc_hdl: Opaque HAL SOC handle
 * @hal_ring_hdl: Ring pointer (Source or Destination ring)
 *
 * This API doesn't implement any byte-order conversion on reading hp/tp.
 * So, Use API only for those srngs for which the target writes hp/tp values to
 * the DDR in the Host order.
 *
 * Return: 0 on success; error on failire
 */
int dal_vndr_hal_srng_access_start(void *hal_soc_hdl, void *hal_ring_hdl);

#endif /* _DAL_VNDR_HAL_API_H_ */
