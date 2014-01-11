// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// Copyright(C) 1993-1996 Id Software, Inc.
// Copyright(C) 2005 Simon Howard
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
// 02111-1307, USA.
//
// DESCRIPTION:
//      Timer functions.
//
//-----------------------------------------------------------------------------

// Lantus 3/1/2013 - AmigaOS native

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include <devices/timer.h>
#include <proto/exec.h>
 
#include "i_timer.h"
 
static ULONG basetime = 0;
struct MsgPort *timer_msgport;
struct timerequest *timer_ioreq;
struct Library *TimerBase;

static int opentimer(ULONG unit){
	timer_msgport = CreateMsgPort();
	timer_ioreq = CreateIORequest(timer_msgport, sizeof(*timer_ioreq));
	if (timer_ioreq){
		if (OpenDevice(TIMERNAME, unit, (APTR) timer_ioreq, 0) == 0){
			TimerBase = (APTR) timer_ioreq->tr_node.io_Device;
			return 1;
		}
	}
	return 0;
}
static void closetimer(void){
	if (TimerBase){
		CloseDevice((APTR) timer_ioreq);
	}
	DeleteIORequest(timer_ioreq);
	DeleteMsgPort(timer_msgport);
	TimerBase = 0;
	timer_ioreq = 0;
	timer_msgport = 0;
}

static struct timeval startTime;

void startup(){
	GetSysTime(&startTime);
}

ULONG getMilliseconds(){
	struct timeval endTime;

	GetSysTime(&endTime);
	SubTime(&endTime,&startTime);

	return (endTime.tv_secs * 1000 + endTime.tv_micro / 1000);
}

int  I_GetTime (void)
{
    ULONG ticks;

    ticks = getMilliseconds();

    if (basetime == 0)
        basetime = ticks;

    ticks -= basetime;

    return (ticks * TICRATE) / 1000;
}

//
// Same as I_GetTime, but returns time in milliseconds
//

int I_GetTimeMS(void)
{
    ULONG ticks;

    ticks = getMilliseconds();

    if (basetime == 0)
        basetime = ticks;

    return ticks - basetime;
}

// Sleep for a specified number of ms


void I_ExitTimer()
{
    closetimer();
}

void I_Sleep(int ms)
{
    usleep(ms);
}

void I_WaitVBL(int count)
{
    I_Sleep((count * 1000) / 70);
}


void I_InitTimer(void)
{
    // initialize timer

   opentimer(UNIT_VBLANK);
   startup();
}

