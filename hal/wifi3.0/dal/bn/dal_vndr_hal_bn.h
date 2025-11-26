/* SPDX-License-Identifier: ISC */
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#ifndef _DAL_VNDR_HAL_BN_H_
#define _DAL_VNDR_HAL_BN_H_
#include <linux/types.h>
#include "dal_vndr_hal_defines_bn.h"
#include "dal_vndr_hal_be.h"
#include "dal_vndr_hal_api.h"
#include "dal_vndr_hal_defines_be.h"
#include "dal_vndr_hal_internal.h"

/* See  dal_vndr_hal_defines_bn.h fo reference
 * DAL_VNDR_HAL_TX_DESC_LEN_DWORDS is NUM_OF_DWORDS_TCL_ASSIST_CMD
 * DAL_VNDR_HAL_TX_DESC_LEN_BYTES is 4*NUM_OF_DWORDS_TCL_ASSIST_CMD
 */
#define DAL_VNDR_HAL_TX_DESC_LEN_DWORDS (8)
#define DAL_VNDR_HAL_TX_DESC_LEN_BYTES  (DAL_VNDR_HAL_TX_DESC_LEN_DWORDS * 4)

/**
 * dal_vndr_hal_default_ops_attach_bn() - Add default ops for BN chips
 * @soc: hal_soc handle
 *
 * Return: None
 */
void dal_vndr_hal_default_ops_attach_bn(struct dal_vndr_hal_soc *soc);

#endif /* _DAL_VNDR_HAL_BN_H_ */
