/* Copyright (c) 2012-2018, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#ifndef __KGSL_IOMMU_H
#define __KGSL_IOMMU_H

#ifdef CONFIG_QCOM_IOMMU
#include <linux/qcom_iommu.h>
#endif
#include <linux/of.h>
#include "kgsl.h"

/*
 * These defines control the address range for allocations that
 * are mapped into all pagetables.
 */
#define KGSL_IOMMU_GLOBAL_MEM_SIZE	(20 * SZ_1M)
#define KGSL_IOMMU_GLOBAL_MEM_BASE32	0xf8000000
#define KGSL_IOMMU_GLOBAL_MEM_BASE64	0xfc000000

#define KGSL_IOMMU_GLOBAL_MEM_BASE(__mmu)	\
	(MMU_FEATURE(__mmu, KGSL_MMU_64BIT) ? \
		KGSL_IOMMU_GLOBAL_MEM_BASE64 : KGSL_IOMMU_GLOBAL_MEM_BASE32)

#define KGSL_IOMMU_SECURE_SIZE SZ_256M
#define KGSL_IOMMU_SECURE_END(_mmu) KGSL_IOMMU_GLOBAL_MEM_BASE(_mmu)
#define KGSL_IOMMU_SECURE_BASE(_mmu)	\
	(KGSL_IOMMU_GLOBAL_MEM_BASE(_mmu) - KGSL_IOMMU_SECURE_SIZE)

#define KGSL_IOMMU_SVM_BASE32		0x300000
#define KGSL_IOMMU_SVM_END32		(0xC0000000 - SZ_16M)

#define KGSL_IOMMU_VA_BASE64		0x500000000ULL
#define KGSL_IOMMU_VA_END64		0x600000000ULL
/*
 * Note: currently we only support 36 bit addresses,
 * but the CPU supports 39. Eventually this range
 * should change to high part of the 39 bit address
 * space just like the CPU.
 */
#define KGSL_IOMMU_SVM_BASE64		0x700000000ULL
#define KGSL_IOMMU_SVM_END64		0x800000000ULL
#define CP_APERTURE_REG			0
#define CP_SMMU_APERTURE_ID		0x1B
/* TLBSTATUS register fields */
#define KGSL_IOMMU_CTX_TLBSTATUS_SACTIVE BIT(0)

/* IMPLDEF_MICRO_MMU_CTRL register fields */
#define KGSL_IOMMU_IMPLDEF_MICRO_MMU_CTRL_HALT  0x00000004
#define KGSL_IOMMU_IMPLDEF_MICRO_MMU_CTRL_IDLE  0x00000008

/* SCTLR fields */
#define KGSL_IOMMU_SCTLR_HUPCF_SHIFT		8
#define KGSL_IOMMU_SCTLR_CFCFG_SHIFT		7
#define KGSL_IOMMU_SCTLR_CFIE_SHIFT		6

enum kgsl_iommu_reg_map {
	KGSL_IOMMU_CTX_SCTLR = 0,
	KGSL_IOMMU_CTX_TTBR0,
	KGSL_IOMMU_CTX_CONTEXTIDR,
	KGSL_IOMMU_CTX_FSR,
	KGSL_IOMMU_CTX_FAR,
	KGSL_IOMMU_CTX_TLBIALL,
	KGSL_IOMMU_CTX_RESUME,
	KGSL_IOMMU_CTX_FSYNR0,
	KGSL_IOMMU_CTX_FSYNR1,
	KGSL_IOMMU_CTX_TLBSYNC,
	KGSL_IOMMU_CTX_TLBSTATUS,
	KGSL_IOMMU_REG_MAX
};

/* Max number of iommu clks per IOMMU unit */
#define KGSL_IOMMU_MAX_CLKS 5

enum kgsl_iommu_context_id {
	KGSL_IOMMU_CONTEXT_USER = 0,
	KGSL_IOMMU_CONTEXT_SECURE = 1,
	KGSL_IOMMU_CONTEXT_MAX,
};

/* offset at which a nop command is placed in setstate */
#define KGSL_IOMMU_SETSTATE_NOP_OFFSET	1024

/*
 * struct kgsl_iommu_context - Structure holding data about an iommu context
 * bank
 * @dev: pointer to the iommu context's device
 * @name: context name
 * @id: The id of the context, used for deciding how it is used.
 * @cb_num: The hardware context bank number, used for calculating register
 *		offsets.
 * @kgsldev: The kgsl device that uses this context.
 * @fault: Flag when set indicates that this iommu device has caused a page
 * fault
 * @gpu_offset: Offset of this context bank in the GPU register space
 * @default_pt: The default pagetable for this context,
 *		it may be changed by self programming.
 */
struct kgsl_iommu_context {
	struct device *dev;
	const char *name;
	enum kgsl_iommu_context_id id;
	unsigned int cb_num;
	struct kgsl_device *kgsldev;
	int fault;
	void __iomem *regbase;
	unsigned int gpu_offset;
	struct kgsl_pagetable *default_pt;
};

/*
 * struct kgsl_iommu - Structure holding iommu data for kgsl driver
 * @ctx: Array of kgsl_iommu_context structs
 * @regbase: Virtual address of the IOMMU register base
 * @regstart: Physical address of the iommu registers
 * @regsize: Length of the iommu register region.
 * @setstate: Scratch GPU memory for IOMMU operations
 * @clk_enable_count: The ref count of clock enable calls
 * @clks: Array of pointers to IOMMU clocks
 * @vddcx_regulator: Handle to IOMMU regulator
 * @micro_mmu_ctrl: GPU register offset of this glob al register
 * @smmu_info: smmu info used in a5xx preemption
 * @protect: register protection settings for the iommu.
 * @pagefault_suppression_count: Total number of pagefaults
 *				 suppressed since boot.
 */
struct kgsl_iommu {
	struct kgsl_iommu_context ctx[KGSL_IOMMU_CONTEXT_MAX];
	void __iomem *regbase;
	unsigned long regstart;
	unsigned int regsize;
	struct kgsl_memdesc setstate;
	atomic_t clk_enable_count;
	struct clk *clks[KGSL_IOMMU_MAX_CLKS];
	struct regulator *vddcx_regulator;
	unsigned int micro_mmu_ctrl;
	struct kgsl_memdesc smmu_info;
	unsigned int version;
	struct kgsl_protected_registers protect;
	u32 pagefault_suppression_count;
};

/*
 * struct kgsl_iommu_pt - Iommu pagetable structure private to kgsl driver
 * @domain: Pointer to the iommu domain that contains the iommu pagetable
 * @ttbr0: register value to set when using this pagetable
 * @contextidr: register value to set when using this pagetable
 * @attached: is the pagetable attached?
 * @rbtree: all buffers mapped into the pagetable, indexed by gpuaddr
 * @va_start: Start of virtual range used in this pagetable.
 * @va_end: End of virtual range.
 * @svm_start: Start of shared virtual memory range. Addresses in this
 *		range are also valid in the process's CPU address space.
 * @svm_end: End of the shared virtual memory range.
 * @svm_start: 32 bit compatible range, for old clients who lack bits
 * @svm_end: end of 32 bit compatible range
 */
struct kgsl_iommu_pt {
	struct iommu_domain *domain;
	u64 ttbr0;
	u32 contextidr;
	bool attached;

	struct rb_root rbtree;

	uint64_t va_start;
	uint64_t va_end;
	uint64_t svm_start;
	uint64_t svm_end;
	uint64_t compat_va_start;
	uint64_t compat_va_end;
};

/*
 * offset of context bank 0 from the start of the SMMU register space.
 */
#define KGSL_IOMMU_CB0_OFFSET		0x8000
/* size of each context bank's register space */
#define KGSL_IOMMU_CB_SHIFT		12

/* Macros to read/write IOMMU registers */
extern const unsigned int kgsl_iommu_reg_list[KGSL_IOMMU_REG_MAX];

/*
 * Don't use this function directly. Use the macros below to read/write
 * IOMMU registers.
 */
static inline void __iomem *
kgsl_iommu_reg(struct kgsl_iommu_context *ctx, enum kgsl_iommu_reg_map reg)
{
	return ctx->regbase + kgsl_iommu_reg_list[reg];
}

/* Program aperture registers using SCM call */
int kgsl_program_smmu_aperture(void);

#define KGSL_IOMMU_SET_CTX_REG_Q(_ctx, REG, val) \
		writeq_relaxed((val), \
			kgsl_iommu_reg((_ctx), KGSL_IOMMU_CTX_##REG))

#define KGSL_IOMMU_GET_CTX_REG_Q(_ctx, REG) \
		readq_relaxed(kgsl_iommu_reg((_ctx), KGSL_IOMMU_CTX_##REG))

#define KGSL_IOMMU_SET_CTX_REG(_ctx, REG, val) \
		writel_relaxed((val), \
			kgsl_iommu_reg((_ctx), KGSL_IOMMU_CTX_##REG))

#define KGSL_IOMMU_GET_CTX_REG(_ctx, REG) \
		readl_relaxed(kgsl_iommu_reg((_ctx), KGSL_IOMMU_CTX_##REG))


#endif
