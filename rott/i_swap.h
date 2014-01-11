// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// Copyright(C) 1993-1996 Id Software, Inc.
// Copyright(C) 2005,2006,2007 Simon Howard
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
//	Endianess handling, swapping 16bit and 32bit.
//
//-----------------------------------------------------------------------------


#ifndef __I_SWAP__
#define __I_SWAP__
 

// Endianess handling.
// WAD files are stored little endian.

// Just use SDL's endianness swapping functions.

// These are deliberately cast to signed values; this is the behaviour
// of the macros in the original source and some code relies on it.

extern inline short SwapSHORT(short val)
{
	__asm __volatile
	(
		"ror.w	#8,%0"

		: "=d" (val)
		: "0" (val)
	);
	
	return val;
}

extern inline long SwapLONG(long val)
{
	__asm __volatile
	(
		"ror.w	#8,%0 \n\t"
		"swap	%0 \n\t"
		"ror.w	#8,%0"
		
		: "=d" (val)
		: "0" (val)
	);
	
	return val;
}

#define SHORT(x) SwapSHORT(x)
#define LONG(x) SwapLONG(x)

// Defines for checking the endianness of the system.
 
#define SYS_BIG_ENDIAN
 

#endif

