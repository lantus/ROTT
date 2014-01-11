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
//	Fixed point arithemtics, implementation.
//
//-----------------------------------------------------------------------------


#ifndef __M_FIXED__
#define __M_FIXED__
 
//
// Fixed point, 32bit as 16.16.
//
#define FRACBITS		16
#define FRACUNIT		(1<<FRACBITS)

typedef int fixed_t;

#ifdef AMIGA
 

#ifdef C_FIXED_MATH
__inline__ fixed FixedMul(fixed a, fixed b)
{
	__int64 scratch1 = (__int64) a * (__int64) b + (__int64) 0x8000;
	return (scratch1 >> 16) & 0xffffffff;
}

__inline__ fixed FixedDiv2(fixed a, fixed b)
{
	__int64 x = (signed int)a;
	__int64 y = (signed int)b;
	__int64 z = x * 65536 / y;
	return (z) & 0xffffffff;
}
#endif


__inline__ fixed FixedMulShift(fixed a, fixed b, fixed shift)
{
	__int64 x = a;
	__int64 y = b;
	__int64 z = x * y;

	return (((unsigned __int64)z) >> shift) & 0xffffffff;
}

 

__inline__ fixed FixedScale(fixed orig, fixed factor, fixed divisor)
{
	__int64 x = orig;
	__int64 y = factor;
	__int64 z = divisor;

	__int64 w = (x * y) / z;

	return (w) & 0xffffffff;
}

 

#else
fixed_t FixedMul	(fixed_t a, fixed_t b);
fixed_t FixedDiv	(fixed_t a, fixed_t b);
#endif


#endif

