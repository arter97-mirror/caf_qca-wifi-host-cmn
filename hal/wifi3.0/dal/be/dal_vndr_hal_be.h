/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: ISC
 */

#ifndef _DAL_VNDR_HAL_BE_H_
#define _DAL_VNDR_HAL_BE_H_
#include "dal_vndr_hal_defines_be.h"
#include "dal_vndr_hal_internal.h"

/**
 * dal_vndr_hal_default_ops_attach_be() - Add default ops for BE chips
 * @soc: hal_soc handle
 *
 * Return: None
 */
void dal_vndr_hal_default_ops_attach_be(struct dal_vndr_hal_soc *soc);

#endif /* _DAL_VNDR_HAL_BE_H_ */
