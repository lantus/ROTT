#include <stdlib.h>
#include <string.h>

#include "dsl.h"
#include "util.h"

#include <exec/exec.h>
#include <dos/dos.h>
#include <graphics/gfxbase.h>
#include <devices/audio.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/graphics.h>

#include "doomsound.h"

int numChannels;
int use_libsamplerate = 0;
/**********************************************************************/
#define MAXSFXVOICES    16   /* max number of Sound Effects with server */
#define MAXNUMCHANNELS   4   /* max number of Amiga sound channels */

extern volatile int MV_MixPage;

static int DSL_ErrorCode = DSL_Ok;

static int mixer_initialized = 0;

static void ( *_CallBackFunc )( void );
static volatile char *_BufferStart;
static int _BufferSize;
static int _NumDivisions;
static int _SampleRate;
static int _remainder;

struct Library *DoomSndBase = NULL;
static unsigned char *blank_buf;

/*
possible todo ideas: cache sdl/sdl mixer error messages.
*/

char *DSL_ErrorString( int ErrorNumber )
{
	char *ErrorString;
	
	switch (ErrorNumber) {
		case DSL_Warning:
		case DSL_Error:
			ErrorString = DSL_ErrorString(DSL_ErrorCode);
			break;
		
		case DSL_Ok:
			ErrorString = "SDL Driver ok.";
			break;
		
		case DSL_SDLInitFailure:
			ErrorString = "SDL Audio initialization failed.";
			break;
		
		case DSL_MixerActive:
			ErrorString = "SDL Mixer already initialized.";
			break;	
	
		case DSL_MixerInitFailure:
			ErrorString = "SDL Mixer initialization failed.";
			break;
			
		default:
			ErrorString = "Unknown SDL Driver error.";
			break;
	}
	
	return ErrorString;
}

static void DSL_SetErrorCode(int ErrorCode)
{
	DSL_ErrorCode = ErrorCode;
}

int DSL_Init( void )
{
	DSL_SetErrorCode(DSL_Ok);
	
    if ((DoomSndBase = OpenLibrary ("doomsound.library",37)) != NULL) 
    {
        Sfx_SetVol(64);
        Mus_SetVol(64);
        numChannels = 4; 
    }	
	
	return DSL_Ok;
}

void DSL_Shutdown( void )
{
	DSL_StopPlayback();
	
    if (DoomSndBase != NULL) 
    {
        CloseLibrary (DoomSndBase);
        DoomSndBase = NULL;
    }
}

void mixer_callback( void *stream, int len )
{
	unsigned char *stptr;
	unsigned char *fxptr;
	int copysize;
	
	/* len should equal _BufferSize, else this is screwed up */
	
	printf("mixer_callback\n");

	stptr = (unsigned char *)stream;
	
	if (_remainder > 0) {
		copysize = min(len, _remainder);
		
		fxptr = (unsigned char *)(&_BufferStart[MV_MixPage * 
			_BufferSize]);
		
		memcpy(stptr, fxptr+(_BufferSize-_remainder), copysize);
		
		len -= copysize;
		_remainder -= copysize;
		
		stptr += copysize;
	}
	
	while (len > 0) {
		/* new buffer */
		
		_CallBackFunc();
		
		fxptr = (unsigned char *)(&_BufferStart[MV_MixPage * 
			_BufferSize]);
			
	

		copysize = min(len, _BufferSize);
		
		memcpy(stptr, fxptr, copysize);
		
		len -= copysize;
		
		stptr += copysize;
	}
	
	_remainder = len;
			   if (DoomSndBase != NULL) {
   Sfx_Start (fxptr, MV_MixPage , 11025,
           64, 0, _BufferSize );

}	
}


int   DSL_BeginBufferedPlayback( char *BufferStart,
      int BufferSize, int NumDivisions, unsigned SampleRate,
      int MixMode, void ( *CallBackFunc )( void ) )
{
	unsigned short format;
	unsigned char *tmp;
	int channels;
	int chunksize;
		
	if (mixer_initialized) {
		DSL_SetErrorCode(DSL_MixerActive);
		
		return DSL_Error;
	}
	
	_CallBackFunc = CallBackFunc;
	_BufferStart = BufferStart;
	_BufferSize = (BufferSize / NumDivisions);
	_NumDivisions = NumDivisions;
	_SampleRate = SampleRate;

	_remainder = 0;
 
    mixer_callback(_BufferStart, _BufferSize);
	
	mixer_initialized = 1;
	
	return DSL_Ok;
}

void DSL_StopPlayback( void )
{
 
	if (DoomSndBase != NULL) {
    Sfx_Stop(0);
    return;
    }
	mixer_initialized = 0;
}

unsigned DSL_GetPlaybackRate( void )
{
	return _SampleRate;
}

unsigned long DisableInterrupts( void )
{
	return 0;
}

void RestoreInterrupts( unsigned long flags )
{
    

}
