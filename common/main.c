/*
 * (C) Copyright 2000
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

/* #define	DEBUG	*/

#include <common.h>
#include <autoboot.h>
#include <cli.h>
#include <console.h>
#include <version.h>

/* We come here after U-Boot is initialised and ready to process commands */
void main_loop(void)
{
    printf ("Dinesh: We will wait here\n");
    for (;;);
}
