/*
 * mathbox.h: math box simulation (Battlezone/Red Baron/Tempest)
 *
 * Copyright 1991, 1992, 1993, 1996 Eric Smith
 *
 * $Header: /home/cvs/mess/src/machine/mathbox.h,v 1.1.1.1 2000/07/16 22:42:21 hjb Exp $
 */

typedef short s16;
typedef int s32;

WRITE_HANDLER( mb_go_w );
READ_HANDLER( mb_status_r );
READ_HANDLER( mb_lo_r );
READ_HANDLER( mb_hi_r );

extern s16 mb_result;
