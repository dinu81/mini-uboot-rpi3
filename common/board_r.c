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

#include <common.h>
#include <command.h>
#include <console.h>
#include <dm.h>
#include <environment.h>
#include <fdtdec.h>
#include <initcall.h>
#include <logbuff.h>
#include <malloc.h>
#include <mapmem.h>
#include <stdio_dev.h>
#include <asm/sections.h>
#include <dm/root.h>
#include <linux/compiler.h>
#include <linux/err.h>
#include <efi_loader.h>

DECLARE_GLOBAL_DATA_PTR;

ulong monitor_flash_len;

static int initr_trace(void)
{
	return 0;
}

static int initr_reloc(void)
{
	/* tell others: relocation done */
	gd->flags |= GD_FLG_RELOC | GD_FLG_FULL_MALLOC_INIT;

	return 0;
}

static int initr_caches(void)
{
	/* Enable caches */
	enable_caches();
	return 0;
}

static int initr_reloc_global_data(void)
{
	monitor_flash_len = _end - __image_copy_start;

	return 0;
}

static int initr_barrier(void)
{
	return 0;
}

static int initr_malloc(void)
{
	ulong malloc_start;

	debug("Pre-reloc malloc() used %#lx bytes (%ld KB)\n", gd->malloc_ptr,
	      gd->malloc_ptr / 1024);

	/* The malloc area is immediately below the monitor copy in DRAM */
	malloc_start = gd->relocaddr - TOTAL_MALLOC_LEN;
	mem_malloc_init((ulong)map_sysmem(malloc_start, TOTAL_MALLOC_LEN),
			TOTAL_MALLOC_LEN);
	return 0;
}

static int initr_env(void)
{
	return 0;
}

static int run_main_loop(void)
{
	/* main_loop() can return to retry autoboot, if so just run it again */
	for (;;)
		main_loop();
	return 0;
}

/*
 * Over time we hope to remove these functions with code fragments and
 * stub funtcions, and instead call the relevant function directly.
 *
 * We also hope to remove most of the driver-related init and do it if/when
 * the driver is later used.
 *
 * TODO: perhaps reset the watchdog in the initcall function after each call?
 */
init_fnc_t init_sequence_r[] = {
	initr_trace,
	initr_reloc,
	initr_caches,
	initr_reloc_global_data,
	initr_barrier,
	initr_malloc,
	initr_env,
    /* Dinesh without this rainbow */
	stdio_add_devices,
    /* Dinesh Very Important */
	console_init_r,		/* fully init console as a device */
	run_main_loop,
};

void board_init_r(gd_t *new_gd, ulong dest_addr)
{
	if (initcall_run_list(init_sequence_r)) {
		hang();
    }

	/* NOTREACHED - run_main_loop() does not return */
	hang();
}
