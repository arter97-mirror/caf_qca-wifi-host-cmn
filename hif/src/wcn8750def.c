/* Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: ISC
 */

#if defined(WCN8750_HEADERS_DEF)

#undef UMAC
#define WLAN_HEADERS 1

#ifdef CONFIG_BORON
#include "boron_top_reg.h"
#else
#include "beryllium_top_reg.h"
#endif
#include "wcss_version.h"

#define MISSING 0

#define SOC_RESET_CONTROL_OFFSET MISSING
#define GPIO_PIN0_OFFSET                        MISSING
#define GPIO_PIN1_OFFSET                        MISSING
#define GPIO_PIN0_CONFIG_MASK                   MISSING
#define GPIO_PIN1_CONFIG_MASK                   MISSING
#define LOCAL_SCRATCH_OFFSET 0x18
#define GPIO_PIN10_OFFSET MISSING
#define GPIO_PIN11_OFFSET MISSING
#define GPIO_PIN12_OFFSET MISSING
#define GPIO_PIN13_OFFSET MISSING
#define MBOX_BASE_ADDRESS MISSING
#endif /*WCN8750_HEADERS_DEF */
