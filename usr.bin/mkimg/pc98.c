/*-
 * Copyright (c) 2014 Juniper Networks, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__FBSDID("$FreeBSD$");

#include <sys/types.h>
#include <sys/diskpc98.h>
#include <sys/endian.h>
#include <sys/errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "mkimg.h"
#include "scheme.h"

#define	PC98_BOOTCODESZ		8192

static struct mkimg_alias pc98_aliases[] = {
    {	ALIAS_FREEBSD, ALIAS_INT2TYPE(PC98_PTYP_386BSD) },
    {	ALIAS_NONE, 0 }
};

static u_int
pc98_metadata(u_int where)
{
	u_int secs;

	secs = PC98_BOOTCODESZ / secsz;
	return ((where == SCHEME_META_IMG_START) ? secs : 0);
}

static void
pc98_chs(u_short *cyl, u_char *hd, u_char *sec, uint32_t lba __unused)
{

	*cyl = 0xffff;		/* XXX */
	*hd = 0xff;		/* XXX */
	*sec = 0xff;		/* XXX */
}

static int
pc98_write(int fd, lba_t imgsz __unused, void *bootcode)
{
	struct part *part;
	struct pc98_partition *dpbase, *dp;
	u_char *buf;
	int error, ptyp;

	buf = malloc(PC98_BOOTCODESZ);
	if (buf == NULL)
		return (ENOMEM);
	if (bootcode != NULL) {
		memcpy(buf, bootcode, PC98_BOOTCODESZ);
		memset(buf + secsz, 0, secsz);
	} else
		memset(buf, 0, PC98_BOOTCODESZ);
	le16enc(buf + PC98_MAGICOFS, PC98_MAGIC);
	dpbase = (void *)(buf + secsz);
	STAILQ_FOREACH(part, &partlist, link) {
		dp = dpbase + part->index;
		ptyp = ALIAS_TYPE2INT(part->type);
		dp->dp_mid = ptyp;
		dp->dp_sid = ptyp >> 8;
		pc98_chs(&dp->dp_scyl, &dp->dp_shd, &dp->dp_ssect,
		    part->block);
		pc98_chs(&dp->dp_scyl, &dp->dp_shd, &dp->dp_ssect,
		    part->block + part->size - 1);
		if (part->label != NULL)
			memcpy(dp->dp_name, part->label, strlen(part->label));
	}
	error = mkimg_seek(fd, 0);
	if (error == 0) {
		if (write(fd, buf, PC98_BOOTCODESZ) != PC98_BOOTCODESZ)
			error = errno;
	}
	free(buf);
	return (error);
}

static struct mkimg_scheme pc98_scheme = {
	.name = "pc98",
	.description = "PC-9800 disk partitions",
	.aliases = pc98_aliases,
	.metadata = pc98_metadata,
	.write = pc98_write,
	.bootcode = PC98_BOOTCODESZ,
	.labellen = 16,
	.nparts = PC98_NPARTS,
	.maxsecsz = 512
};

SCHEME_DEFINE(pc98_scheme);