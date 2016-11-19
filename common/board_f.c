/*
 * Copyright (c) 2011 The Chromium OS Authors.
 * (C) Copyright 2002-2006
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * (C) Copyright 2002
 * Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Marius Groeger <mgroeger@sysgo.de>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#define DEBUG 


#include <common.h>
#include <linux/compiler.h>
#include <version.h>
#include <console.h>
#include <environment.h>
#include <dm.h>
#include <initcall.h>
#include <logbuff.h>
#include <malloc.h>
#include <mapmem.h>

#include <os.h>
#include <post.h>
#include <spi.h>
#include <status_led.h>
#include <timer.h>
#include <trace.h>
#include <video.h>
#include <watchdog.h>
#include <linux/errno.h>
#include <asm/io.h>
#include <asm/sections.h>
#include <dm/root.h>
#include <linux/compiler.h>

/*
 * Pointer to initial global data area
 *
 * Here we initialize it if needed.
 */
DECLARE_GLOBAL_DATA_PTR;

__weak void dram_init_banksize(void)
{
	gd->bd->bi_dram[0].start = CONFIG_SYS_SDRAM_BASE;
	gd->bd->bi_dram[0].size = get_effective_memsize();
}

__maybe_unused
static int zero_global_data(void)
{
	memset((void *)gd, '\0', sizeof(gd_t));

	return 0;
}

static int setup_mon_len(void)
{
	gd->mon_len = (ulong)&__bss_end - (ulong)_start;
	return 0;
}

/* Get the top of usable RAM */
__weak ulong board_get_usable_ram_top(ulong total_size)
{
	/*
	 * Detect whether we have so much RAM that it goes past the end of our
	 * 32-bit address space. If so, clip the usable RAM so it doesn't.
	 */
	if (gd->ram_top < CONFIG_SYS_SDRAM_BASE)
		/*
		 * Will wrap back to top of 32-bit space when reservations
		 * are made.
		 */
		return 0;

	return gd->ram_top;
}

__weak phys_size_t board_reserve_ram_top(phys_size_t ram_size)
{
	return ram_size;
}

static int setup_dest_addr(void)
{
	debug("Monitor len: %08lX\n", gd->mon_len);
	/*
	 * Ram is setup, size stored in gd !!
	 */
	debug("Ram size: %08lX\n", (ulong)gd->ram_size);

	/*
	 * Subtract specified amount of memory to hide so that it won't
	 * get "touched" at all by U-Boot. By fixing up gd->ram_size
	 * the Linux kernel should now get passed the now "corrected"
	 * memory size and won't touch it either. This has been used
	 * by arch/powerpc exclusively. Now ARMv8 takes advantage of
	 * thie mechanism. If memory is split into banks, addresses
	 * need to be calculated.
	 */
	gd->ram_size = board_reserve_ram_top(gd->ram_size);

	gd->ram_top = CONFIG_SYS_SDRAM_BASE;
	gd->ram_top += get_effective_memsize();
	gd->ram_top = board_get_usable_ram_top(gd->mon_len);
	gd->relocaddr = gd->ram_top;
	debug("Ram top: %08lX\n", (ulong)gd->ram_top);
	return 0;
}

/* Round memory pointer down to next 4 kB limit */
static int reserve_round_4k(void)
{
	gd->relocaddr &= ~(4096 - 1);
	return 0;
}

static int reserve_lcd(void)
{
	gd->fb_base = CONFIG_FB_ADDR;

	return 0;
}

static int reserve_uboot(void)
{
	/*
	 * reserve memory for U-Boot code, data & bss
	 * round down to next 4 kB limit
	 */
	gd->relocaddr -= gd->mon_len;
	gd->relocaddr &= ~(4096 - 1);

	debug("Reserving %ldk for U-Boot at: %08lx\n", gd->mon_len >> 10,
	      gd->relocaddr);

	gd->start_addr_sp = gd->relocaddr;

	return 0;
}

/* reserve memory for malloc() area */
static int reserve_malloc(void)
{
	gd->start_addr_sp = gd->start_addr_sp - TOTAL_MALLOC_LEN;
	debug("Reserving %dk for malloc() at: %08lx\n",
			TOTAL_MALLOC_LEN >> 10, gd->start_addr_sp);
	return 0;
}

/* (permanently) allocate a Board Info struct */
static int reserve_board(void)
{
	if (!gd->bd) {
		gd->start_addr_sp -= sizeof(bd_t);
		gd->bd = (bd_t *)map_sysmem(gd->start_addr_sp, sizeof(bd_t));
		memset(gd->bd, '\0', sizeof(bd_t));
		debug("Reserving %zu Bytes for Board Info at: %08lx\n",
		      sizeof(bd_t), gd->start_addr_sp);
	}
	return 0;
}

static int reserve_global_data(void)
{
	gd->start_addr_sp -= sizeof(gd_t);
	gd->new_gd = (gd_t *)map_sysmem(gd->start_addr_sp, sizeof(gd_t));
	debug("Reserving %zu Bytes for Global Data at: %08lx\n",
			sizeof(gd_t), gd->start_addr_sp);
	return 0;
}

static int reserve_stacks(void)
{
	/* make stack pointer 16-byte aligned */
	gd->start_addr_sp -= 16;
	gd->start_addr_sp &= ~0xf;

	/*
	 * let the architecture-specific code tailor gd->start_addr_sp and
	 * gd->irq_sp
	 */
	return 0;
}

static int setup_dram_config(void)
{
	/* Ram is board specific, so move it to board code ... */
	dram_init_banksize();

	return 0;
}

static int setup_reloc(void)
{
	if (gd->flags & GD_FLG_SKIP_RELOC) {
		debug("Skipping relocation due to flag\n");
		return 0;
	}

	gd->reloc_off = gd->relocaddr - CONFIG_SYS_TEXT_BASE;
	memcpy(gd->new_gd, (char *)gd, sizeof(gd_t));

	debug("Relocation Offset is: %08lx\n", gd->reloc_off);
	debug("Relocating to %08lx, new gd at %08lx, sp at %08lx\n",
	      gd->relocaddr, (ulong)map_to_sysmem(gd->new_gd),
	      gd->start_addr_sp);

	return 0;
}

static int initf_dm(void)
{
	int ret;

	ret = dm_init_and_scan(true);
	if (ret)
		return ret;

	return 0;
}

static init_fnc_t init_sequence_f[] = {
	setup_mon_len,
	initf_malloc,
	initf_dm,
	/* TODO: can any of this go into arch_cpu_init()? */
	console_init_f,		/* stage 1 init of console */
	/* TODO: unify all these dram functions? */
	dram_init,		/* configure available RAM banks */
	setup_dest_addr,
	reserve_round_4k,
	reserve_lcd,
	reserve_uboot,
	reserve_malloc,
	reserve_board,
	reserve_global_data,
	reserve_stacks,
	setup_dram_config,
	INIT_FUNC_WATCHDOG_RESET
	setup_reloc,
	NULL,
};

void board_init_f(ulong boot_flags)
{
	gd->flags = boot_flags;
	gd->have_console = 0;

	if (initcall_run_list(init_sequence_f))
		hang();

}

