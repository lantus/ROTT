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


/* Private data for VOC file */
typedef struct vocstuff {
	unsigned long	rest;			    /* bytes remaining in current block */
	unsigned long	rate;			    /* rate code (byte) of this chunk */
	int 	        silent;		        /* sound or silence? */
	unsigned long	srate;			    /* rate code (byte) of silence */
	unsigned long	blockseek;		    /* start of current output block */
	unsigned long	samples;		    /* number of samples output */
	unsigned long	size;		        /* word length of data */
	unsigned char 	channels;	        /* number of sound channels */
	int             has_extended;       /* Has an extended block been read? */
} vs_t;

/* Size field */
/* SJB: note that the 1st 3 are sometimes used as sizeof(type) */
#define	ST_SIZE_BYTE	1
#define ST_SIZE_8BIT	1
#define	ST_SIZE_WORD	2
#define ST_SIZE_16BIT	2
#define	ST_SIZE_DWORD	4
#define ST_SIZE_32BIT	4
#define	ST_SIZE_FLOAT	5
#define ST_SIZE_DOUBLE	6
#define ST_SIZE_IEEE	7	/* IEEE 80-bit floats. */

/* Style field */
#define ST_ENCODING_UNSIGNED	1 /* unsigned linear: Sound Blaster */
#define ST_ENCODING_SIGN2	2 /* signed linear 2's comp: Mac */
#define	ST_ENCODING_ULAW	3 /* U-law signed logs: US telephony, SPARC */
#define ST_ENCODING_ALAW	4 /* A-law signed logs: non-US telephony */
#define ST_ENCODING_ADPCM	5 /* Compressed PCM */
#define ST_ENCODING_IMA_ADPCM	6 /* Compressed PCM */
#define ST_ENCODING_GSM		7 /* GSM 6.10 33-byte frame lossy compression */

#define	VOC_TERM	0
#define	VOC_DATA	1
#define	VOC_CONT	2
#define	VOC_SILENCE	3
#define	VOC_MARKER	4
#define	VOC_TEXT	5
#define	VOC_LOOP	6
#define	VOC_LOOPEND	7
#define VOC_EXTENDED    8
#define VOC_DATA_16	9

#define RW_SEEK_SET	0	/**< Seek from the beginning of data */
#define RW_SEEK_CUR	1	/**< Seek relative to current read point */
#define RW_SEEK_END	2	/**< Seek relative to the end of data */

/* Macros to easily read and write from an SDL_RWops structure */
#define SDL_RWseek(ctx, offset, whence) (ctx)->seek(ctx, offset, whence)
#define SDL_RWtell(ctx)                 (ctx)->seek(ctx, 0, SEEK_CUR)
#define SDL_RWread(ctx, ptr, size, n)   (ctx)->read(ctx, ptr, size, n)
#define SDL_RWwrite(ctx, ptr, size, n)  (ctx)->write(ctx, ptr, size, n)
#define SDL_RWclose(ctx)                (ctx)->close(ctx)



/** This is the read/write operation structure -- very basic */

typedef struct SDL_RWops {
	/** Seek to 'offset' relative to whence, one of stdio's whence values:
	 *	SEEK_SET, SEEK_CUR, SEEK_END
	 *  Returns the final offset in the data source.
	 */
	int (  *seek)(struct SDL_RWops *context, int offset, int whence);

	/** Read up to 'maxnum' objects each of size 'size' from the data
	 *  source to the area pointed at by 'ptr'.
	 *  Returns the number of objects read, or -1 if the read failed.
	 */
	int (  *read)(struct SDL_RWops *context, void *ptr, int size, int maxnum);

	/** Write exactly 'num' objects each of size 'objsize' from the area
	 *  pointed at by 'ptr' to data source.
	 *  Returns 'num', or -1 if the write failed.
	 */
	int (  *write)(struct SDL_RWops *context, const void *ptr, int size, int num);

	/** Close and free an allocated SDL_FSops structure */
	int (  *close)(struct SDL_RWops *context);

	unsigned long type;
	union {

#ifdef HAVE_STDIO_H
	    struct {
		int autoclose;
	 	FILE *fp;
	    } stdio;
#endif
	    struct {
		unsigned char *base;
	 	unsigned char *here;
		unsigned char *stop;
	    } mem;
	    struct {
		void *data1;
	    } unknown;
	} hidden;

} SDL_RWops;

typedef struct {
        int freq;               /* DSP frequency -- samples per second */
        unsigned short format;          /* Audio data format */
        unsigned char  channels;        /* Number of channels: 1 mono, 2 stereo */
        unsigned char  silence;         /* Audio buffer silence value (calculated) */
        unsigned short samples;         /* Audio buffer size in samples (power of 2) */
        unsigned short padding;         /* Necessary for some compile environments */
        unsigned long size;            /* Audio buffer size in bytes (calculated) */
        /* This function is called when the audio device needs more data.
           'stream' is a pointer to the audio data buffer
           'len' is the length of that buffer in bytes.
           Once the callback returns, the buffer will no longer be valid.
           Stereo samples are stored in a LRLRLR ordering.
        */
        void (*callback)(void *userdata, unsigned char *stream, int len);
        void  *userdata;
} AudioSpec;

typedef struct {
        int allocated;
        unsigned char *abuf;
        unsigned long alen;
        unsigned char volume;
} Mix_Chunk;

extern SDL_RWops *SDL_RWFromMem(void *mem, int size);
extern AudioSpec *Mix_LoadVOC_RW (SDL_RWops *src, int freesrc,
        AudioSpec *spec, unsigned char **audio_buf, unsigned long *audio_len);
        
#endif	/*  _INCLUDE_PRAGMA_DOOMSND_LIB_H  */
