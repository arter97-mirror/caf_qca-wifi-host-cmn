/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: ISC
 */

#ifndef _DAL_VNDR_HAL_BE_H_
#define _DAL_VNDR_HAL_BE_H_
#include <linux/types.h>
#include "dal_vndr_hal_internal.h"

/* See  dal_vndr_hal_defines_be.h fo reference
 * DAL_VNDR_HAL_TX_DESC_LEN_DWORDS is NUM_OF_DWORDS_TCL_DATA_CMD
 * DAL_VNDR_HAL_TX_DESC_LEN_BYTES is 4*NUM_OF_DWORDS_TCL_DATA_CMD
 */
#define DAL_VNDR_HAL_TX_DESC_LEN_DWORDS (8)
#define DAL_VNDR_HAL_TX_DESC_LEN_BYTES  (DAL_VNDR_HAL_TX_DESC_LEN_DWORDS * 4)

/**
 * dal_vndr_hal_default_ops_attach_be() - Add default ops for BE chips
 * @soc: hal_soc handle
 *
 * Return: None
 */
void dal_vndr_hal_default_ops_attach_be(struct dal_vndr_hal_soc *soc);

#endif /* _DAL_VNDR_HAL_BE_H_ */
