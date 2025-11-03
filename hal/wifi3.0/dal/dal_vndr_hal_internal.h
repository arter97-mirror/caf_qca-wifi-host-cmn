/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: ISC
 */

#ifndef _DAL_VNDR_HAL_INTERNAL_H_
#define _DAL_VNDR_HAL_INTERNAL_H_

#include "dal_vndr_hal_be.h"

/**
 * struct dal_vndr_hal_soc - HAL context to be used to access SRNG APIs
 * @dev_base_addr: Device base address
 * @ops: TXRX operations
 */
struct dal_vndr_hal_soc {
	void *dev_base_addr;
	/* TODO: Add ops */
};

#endif /* _DAL_VNDR_HAL_INTERNAL_H_ */
