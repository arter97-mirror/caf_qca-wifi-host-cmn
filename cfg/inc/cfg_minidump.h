/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: ISC
 */

#ifndef __CFG_MINIDUMP_H
#define __CFG_MINIDUMP_H

#include "cfg_define.h"

#ifdef CONFIG_QCA_MINIDUMP

#define CFG_OL_MD_CP_EXT_PDEV \
	CFG_INI_BOOL("ol_md_cp_ext_pdev", true, \
		     "Enable minidump for CP EXT PDEV")

#define CFG_OL_MD_CP_EXT_PSOC \
	CFG_INI_BOOL("ol_md_cp_ext_psoc", true, \
		     "Enable minidump for CP EXT PSOC")

#define CFG_OL_MD_CP_EXT_VDEV \
	CFG_INI_BOOL("ol_md_cp_ext_vdev", true, \
		     "Enable minidump for CP EXT VDEV")

#define CFG_OL_MD_CP_EXT_PEER \
	CFG_INI_BOOL("ol_md_cp_ext_peer", true, \
		     "Enable minidump for CP EXT PEER")

#define CFG_OL_MD_CP_MLO \
	CFG_INI_BOOL("ol_md_cp_mlo", true, \
		     "Enable minidump for CP MLO")

#define CFG_OL_MD_CP_MLO_PEER \
	CFG_INI_BOOL("ol_md_cp_mlo_peer", true, \
		     "Enable minidump for CP MLO PEER")

#define CFG_OL_MD_CP_MGMT_TXRX \
	CFG_INI_BOOL("ol_md_cp_mgmt_txrx", true, \
		     "Enable minidump for CP MGMT TXRX")

#define CFG_OL_MD_CP_MGMT_RX \
	CFG_INI_BOOL("ol_md_cp_mgmt_rx", true, \
		     "Enable minidump for CP MGMT RX")

#define CFG_OL_MD_DP_SOC \
	CFG_INI_BOOL("ol_md_dp_soc", true, \
		     "Enable minidump for DP SOC")

#define CFG_OL_MD_DP_PDEV \
	CFG_INI_BOOL("ol_md_dp_pdev", true, \
		     "Enable minidump for DP PDEV")

#define CFG_OL_MD_DP_VDEV \
	CFG_INI_BOOL("ol_md_dp_vdev", true, \
		     "Enable minidump for DP VDEV")

#define CFG_OL_MD_DP_CFG \
	CFG_INI_BOOL("ol_md_dp_cfg", true, \
		     "Enable minidump for DP CFG")

#define CFG_OL_MD_DP_PEER \
	CFG_INI_BOOL("ol_md_dp_peer", true, \
		     "Enable minidump for DP PEER")

#define CFG_OL_MD_DP_MLO \
	CFG_INI_BOOL("ol_md_dp_mlo", true, \
		     "Enable minidump for DP MLO")

#define CFG_OL_MD_DP_SRNG_REO \
	CFG_INI_BOOL("ol_md_dp_srng_reo", true, \
		     "Enable minidump for DP SRNG REO")

#define CFG_OL_MD_DP_SRNG_TCL \
	CFG_INI_BOOL("ol_md_dp_srng_tcl", true, \
		     "Enable minidump for DP SRNG TCL")

#define CFG_OL_MD_DP_SRNG_WBM \
	CFG_INI_BOOL("ol_md_dp_srng_wbm", true, \
		     "Enable minidump for DP SRNG WBM")

#define CFG_OL_MD_DP_LINK_DESC_BANK \
	CFG_INI_BOOL("ol_md_dp_link_desc_bank", true, \
		     "Enable minidump for DP LINK DESC BANK")

#define CFG_OL_MD_DP_SRNG_RXDMA \
	CFG_INI_BOOL("ol_md_dp_srng_rxdma", true, \
		     "Enable minidump for DP SRNG RXDMA")

#define CFG_OL_MD_DP_HAL_SOC \
	CFG_INI_BOOL("ol_md_dp_hal_soc", true, \
		     "Enable minidump for DP HAL SOC")

#define CFG_OL_MD_DP_MON \
	CFG_INI_BOOL("ol_md_dp_mon", true, \
		     "Enable minidump for DP MON")

#define CFG_OL_MD_DP_MON_PEER \
	CFG_INI_BOOL("ol_md_dp_mon_peer", true, \
		     "Enable minidump for DP MON PEER")

#define CFG_OL_MD_DP_TXRX_PEER \
	CFG_INI_BOOL("ol_md_dp_txrx_peer", true, \
		     "Enable minidump for DP TXRX PEER")

#define CFG_OL_MD_OBJMGR_GLOBAL \
	CFG_INI_BOOL("ol_md_objmgr_global", true, \
		     "Enable minidump for OBJMGR GLOBAL")

#define CFG_OL_MD_DP_GLOBAL_CTX \
	CFG_INI_BOOL("ol_md_dp_global_ctx", true, \
		     "Enable minidump for DP GLOBAL CTX")

#define CFG_OL_MD_OBJMGR_PSOC \
	CFG_INI_BOOL("ol_md_objmgr_psoc", true, \
		     "Enable minidump for OBJMGR PSOC")

#define CFG_OL_MD_OBJMGR_PDEV \
	CFG_INI_BOOL("ol_md_objmgr_pdev", true, \
		     "Enable minidump for OBJMGR PDEV")

#define CFG_OL_MD_OBJMGR_VDEV \
	CFG_INI_BOOL("ol_md_objmgr_vdev", true, \
		     "Enable minidump for OBJMGR VDEV")

#define CFG_OL_MD_OBJMGR_PEER \
	CFG_INI_BOOL("ol_md_objmgr_peer", true, \
		     "Enable minidump for OBJMGR PEER")

#define CFG_MINIDUMP_ALL \
	CFG(CFG_OL_MD_CP_EXT_PDEV) \
	CFG(CFG_OL_MD_CP_EXT_PSOC) \
	CFG(CFG_OL_MD_CP_EXT_VDEV) \
	CFG(CFG_OL_MD_CP_EXT_PEER) \
	CFG(CFG_OL_MD_CP_MLO) \
	CFG(CFG_OL_MD_CP_MLO_PEER) \
	CFG(CFG_OL_MD_CP_MGMT_TXRX) \
	CFG(CFG_OL_MD_CP_MGMT_RX) \
	CFG(CFG_OL_MD_DP_SOC) \
	CFG(CFG_OL_MD_DP_PDEV) \
	CFG(CFG_OL_MD_DP_VDEV) \
	CFG(CFG_OL_MD_DP_CFG) \
	CFG(CFG_OL_MD_DP_PEER) \
	CFG(CFG_OL_MD_DP_MLO) \
	CFG(CFG_OL_MD_DP_SRNG_REO) \
	CFG(CFG_OL_MD_DP_SRNG_TCL) \
	CFG(CFG_OL_MD_DP_SRNG_WBM) \
	CFG(CFG_OL_MD_DP_LINK_DESC_BANK) \
	CFG(CFG_OL_MD_DP_SRNG_RXDMA) \
	CFG(CFG_OL_MD_DP_HAL_SOC) \
	CFG(CFG_OL_MD_DP_MON) \
	CFG(CFG_OL_MD_DP_MON_PEER) \
	CFG(CFG_OL_MD_DP_TXRX_PEER) \
	CFG(CFG_OL_MD_OBJMGR_GLOBAL) \
	CFG(CFG_OL_MD_DP_GLOBAL_CTX) \
	CFG(CFG_OL_MD_OBJMGR_PSOC) \
	CFG(CFG_OL_MD_OBJMGR_PDEV) \
	CFG(CFG_OL_MD_OBJMGR_VDEV) \
	CFG(CFG_OL_MD_OBJMGR_PEER)

#else /* CONFIG_QCA_MINIDUMP */
#define CFG_MINIDUMP_ALL
#endif /* CONFIG_QCA_MINIDUMP */

#endif /* __CFG_MINIDUMP_H */
