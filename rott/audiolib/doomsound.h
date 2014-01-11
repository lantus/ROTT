#ifndef _INCLUDE_PRAGMA_DOOMSND_LIB_H
#define _INCLUDE_PRAGMA_DOOMSND_LIB_H

#ifndef CLIB_DOOMSND_PROTOS_H
#include <clib/doomsound_protos.h>
#endif

#if defined(AZTEC_C) || defined(__MAXON__) || defined(__STORM__)
#pragma amicall(DoomSndBase,0x01e,Sfx_SetVol(d0))
#pragma amicall(DoomSndBase,0x024,Sfx_Start(a0,d0,d1,d2,d3,d4))
#pragma amicall(DoomSndBase,0x02a,Sfx_Update(d0,d1,d2,d3))
#pragma amicall(DoomSndBase,0x030,Sfx_Stop(d0))
#pragma amicall(DoomSndBase,0x036,Sfx_Done(d0))
#pragma amicall(DoomSndBase,0x03c,Mus_SetVol(d0))
#pragma amicall(DoomSndBase,0x042,Mus_Register(a0))
#pragma amicall(DoomSndBase,0x048,Mus_Unregister(d0))
#pragma amicall(DoomSndBase,0x04e,Mus_Play(d0,d1))
#pragma amicall(DoomSndBase,0x054,Mus_Stop(d0))
#pragma amicall(DoomSndBase,0x05a,Mus_Pause(d0))
#pragma amicall(DoomSndBase,0x060,Mus_Resume(d0))
#pragma amicall(DoomSndBase,0x066,Mus_Done(d0))
#endif
#if defined(_DCC) || defined(__SASC)
#pragma  libcall DoomSndBase Sfx_SetVol             01e 001
#pragma  libcall DoomSndBase Sfx_Start              024 43210806
#pragma  libcall DoomSndBase Sfx_Update             02a 321004
#pragma  libcall DoomSndBase Sfx_Stop               030 001
#pragma  libcall DoomSndBase Sfx_Done               036 001
#pragma  libcall DoomSndBase Mus_SetVol             03c 001
#pragma  libcall DoomSndBase Mus_Register           042 801
#pragma  libcall DoomSndBase Mus_Unregister         048 001
#pragma  libcall DoomSndBase Mus_Play               04e 1002
#pragma  libcall DoomSndBase Mus_Stop               054 001
#pragma  libcall DoomSndBase Mus_Pause              05a 001
#pragma  libcall DoomSndBase Mus_Resume             060 001
#pragma  libcall DoomSndBase Mus_Done               066 001
#endif

#endif	/*  _INCLUDE_PRAGMA_DOOMSND_LIB_H  */
