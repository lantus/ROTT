#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>

#if (defined __WATCOMC__)
#include "dukesnd_watcom.h"
#endif

#if (!defined __WATCOMC__)
#define cdecl
#endif

#include <exec/exec.h>
#include <dos/dos.h>
#include <graphics/gfxbase.h>
#include <devices/audio.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/graphics.h>
#include "rt_def.h"      // ROTT music hack
#include "rt_cfg.h"      // ROTT music hack
#include "rt_util.h"     // ROTT music hack
#include "fx_man.h"
#include "music.h"

#include "doomsound.h"

static __inline__ unsigned long SDL_Swap32(unsigned long x)
{
	__asm__("rorw #8,%0\n\tswap %0\n\trorw #8,%0" : "=d" (x) :  "0" (x) : "cc");
	return x;
}

static __inline__ unsigned short SDL_Swap16(unsigned short x)
{
	__asm__("rorw #8,%0" : "=d" (x) :  "0" (x) : "cc");
	return x;
}
 

/* Functions to read/write memory pointers */

SDL_RWops *SDL_AllocRW(void);

static int mem_seek(SDL_RWops *context, int offset, int whence)
{
        unsigned char *newpos;

        switch (whence) {
                case SEEK_SET:
                        newpos = context->hidden.mem.base+offset;
                        break;
                case SEEK_CUR:
                        newpos = context->hidden.mem.here+offset;
                        break;
                case SEEK_END:
                        newpos = context->hidden.mem.stop+offset;
                        break;
                default:
                        Error("Unknown value for 'whence'");
                        return(-1);
        }
        if ( newpos < context->hidden.mem.base ) {
                newpos = context->hidden.mem.base;
        }
        if ( newpos > context->hidden.mem.stop ) {
                newpos = context->hidden.mem.stop;
        }
        context->hidden.mem.here = newpos;
        return(context->hidden.mem.here-context->hidden.mem.base);
}
static int mem_read(SDL_RWops *context, void *ptr, int size, int maxnum)
{
        int num;

        num = maxnum;
        if ( (context->hidden.mem.here + (num*size)) > context->hidden.mem.stop ) {
                num = (context->hidden.mem.stop-context->hidden.mem.here)/size;
        }
        memcpy(ptr, context->hidden.mem.here, num*size);
        context->hidden.mem.here += num*size;
        return(num);
}
static int mem_write(SDL_RWops *context, const void *ptr, int size, int num)
{
        if ( (context->hidden.mem.here + (num*size)) > context->hidden.mem.stop ) {
                num = (context->hidden.mem.stop-context->hidden.mem.here)/size;
        }
        memcpy(context->hidden.mem.here, ptr, num*size);
        context->hidden.mem.here += num*size;
        return(num);
}
static int mem_close(SDL_RWops *context)
{
        if ( context ) {
                free(context);
                context = NULL;
        }
        return(0);
}

SDL_RWops *SDL_RWFromMem(void *mem, int size)
{
        SDL_RWops *rwops;

        rwops = SDL_AllocRW();
        if ( rwops != NULL ) {
                rwops->seek = mem_seek;
                rwops->read = mem_read;
                rwops->write = mem_write;
                rwops->close = mem_close;
                rwops->hidden.mem.base = (unsigned char *)mem;
                rwops->hidden.mem.here = rwops->hidden.mem.base;
                rwops->hidden.mem.stop = rwops->hidden.mem.base+size;
        }
        return(rwops);
}

SDL_RWops *SDL_AllocRW(void)
{
        SDL_RWops *area;

        area = (SDL_RWops *)malloc(sizeof *area);
      
        return(area);
}

void SDL_FreeRW(SDL_RWops *area)
{
        free(area);
}



static int voc_check_header(SDL_RWops *src)
{
    /* VOC magic header */
    unsigned char  signature[20];  /* "Creative Voice File\032" */
    unsigned short datablockofs;
 
    SDL_RWseek(src, 0, RW_SEEK_SET);

    if (SDL_RWread(src, signature, sizeof (signature), 1) != 1)
        return(0);

    if (memcmp(signature, "Creative Voice File\032", sizeof (signature)) != 0) {
        Error("Unrecognized file type (not VOC)");
        return(0);
    }

        /* get the offset where the first datablock is located */
    if (SDL_RWread(src, &datablockofs, sizeof (unsigned short), 1) != 1)
        return(0);

    datablockofs = SDL_Swap16(datablockofs);

    if (SDL_RWseek(src, datablockofs, RW_SEEK_SET) != datablockofs)
        return(0);
     
    return(1);  /* success! */
} /* voc_check_header */


/* Read next block header, save info, leave position at start of data */
static int voc_get_block(SDL_RWops *src, vs_t *v, AudioSpec *spec)
{
    unsigned char bits24[3];
    unsigned char uc, block;
    unsigned long sblen = 0;
    unsigned short new_rate_short;
    unsigned long new_rate_long;
    unsigned char trash[6];
    unsigned short period;
    unsigned int i;

    v->silent = 0;
    while (v->rest == 0)
    {          
        if (SDL_RWread(src, &block, sizeof (block), 1) != 1)
            return 1;  /* assume that's the end of the file. */

        if (block == VOC_TERM)
            return 1;

        if (SDL_RWread(src, bits24, sizeof (bits24), 1) != 1)
            return 1;  /* assume that's the end of the file. */
        
        /* Size is an 24-bit value. Ugh. */
        sblen = ( (bits24[0]) | (bits24[1] << 8) | (bits24[2] << 16) );

        switch(block)
        {
            case VOC_DATA:
                if (SDL_RWread(src, &uc, sizeof (uc), 1) != 1)
                    return 0;
 
                /* When DATA block preceeded by an EXTENDED     */
                /* block, the DATA blocks rate value is invalid */
                if (!v->has_extended)
                {
                    if (uc == 0)
                    {
                        Error("VOC Sample rate is zero?");
                        return 0;
                    }

                    if ((v->rate != -1) && (uc != v->rate))
                    {
                        Error("VOC sample rate codes differ");
                        return 0;
                    }

                    v->rate = uc;
                    spec->freq = (unsigned short)(1000000.0/(256 - v->rate));
                    v->channels = 1;
                }

                if (SDL_RWread(src, &uc, sizeof (uc), 1) != 1)
                    return 0;

                if (uc != 0)
                {
                    Error("VOC decoder only interprets 8-bit data");
                    return 0;
                }

                v->has_extended = 0;
                v->rest = sblen - 2;
                v->size = ST_SIZE_BYTE;
                        
                return 1;

            case VOC_DATA_16:

                if (SDL_RWread(src, &new_rate_long, sizeof (new_rate_long), 1) != 1)
                    return 0;
                new_rate_long = SDL_Swap32(new_rate_long);
                if (new_rate_long == 0)
                {
                    Error("VOC Sample rate is zero?");
                    return 0;
                }
                if ((v->rate != -1) && (new_rate_long != v->rate))
                {
                    Error("VOC sample rate codes differ");
                    return 0;
                }
                v->rate = new_rate_long;
                spec->freq = new_rate_long;

                if (SDL_RWread(src, &uc, sizeof (uc), 1) != 1)
                    return 0;

                switch (uc)
                {
                    case 8:  v->size = ST_SIZE_BYTE; break;
                    case 16: v->size = ST_SIZE_WORD; break;
                    default:
                        Error("VOC with unknown data size");
                        return 0;
                }

                if (SDL_RWread(src, &v->channels, sizeof (unsigned char), 1) != 1)
                    return 0;

                if (SDL_RWread(src, trash, sizeof (unsigned char), 6) != 6)
                    return 0;

                v->rest = sblen - 12;
                return 1;

            case VOC_CONT: 
                v->rest = sblen;
                return 1;

            case VOC_SILENCE:
                 
                if (SDL_RWread(src, &period, sizeof (period), 1) != 1)
                    return 0;
                period = SDL_Swap16(period);

                if (SDL_RWread(src, &uc, sizeof (uc), 1) != 1)
                    return 0;
                if (uc == 0)
                {
                    Error("VOC silence sample rate is zero");
                    return 0;
                }

                /*
                 * Some silence-packed files have gratuitously
                 * different sample rate codes in silence.
                 * Adjust period.
                 */
                if ((v->rate != -1) && (uc != v->rate))
                    period = (unsigned short)((period * (256 - uc))/(256 - v->rate));
                else
                    v->rate = uc;
                v->rest = period;
                v->silent = 1;
                return 1;

            case VOC_LOOP:
            case VOC_LOOPEND:
                                                
                for(i = 0; i < sblen; i++)   /* skip repeat loops. */
                {
                    if (SDL_RWread(src, trash, sizeof (unsigned char), 1) != 1)
                        return 0;
                }
                break;

            case VOC_EXTENDED:
                 
                /* An Extended block is followed by a data block */
                /* Set this byte so we know to use the rate      */
                /* value from the extended block and not the     */
                /* data block.                     */
                v->has_extended = 1;
                if (SDL_RWread(src, &new_rate_short, sizeof (new_rate_short), 1) != 1)
                    return 0;
                     
                new_rate_short = SDL_Swap16(new_rate_short);
                if (new_rate_short == 0)
                {
                   Error("VOC sample rate is zero");
                   return 0;
                }
                if ((v->rate != -1) && (new_rate_short != v->rate))
                {
                   Error("VOC sample rate codes differ");
                   return 0;
                }
                v->rate = new_rate_short;

                if (SDL_RWread(src, &uc, sizeof (uc), 1) != 1)
                    return 0;

                if (uc != 0)
                {
                    Error("VOC decoder only interprets 8-bit data");
                    return 0;
                }

                if (SDL_RWread(src, &uc, sizeof (uc), 1) != 1)
                    return 0;

                if (uc)
                    spec->channels = 2;  /* Stereo */
                
                /* Needed number of channels before finishing
                   compute for rate */
                spec->freq = (256000000L/(65536L - v->rate))/spec->channels;
                /* An extended block must be followed by a data */
                /* block to be valid so loop back to top so it  */
                /* can be grabed.                */
                continue;

            case VOC_MARKER:
                 
                if (SDL_RWread(src, trash, sizeof (unsigned char), 2) != 2)
                    return 0;

                /* Falling! Falling! */

            default:  /* text block or other krapola. */
             
                for(i = 0; i < sblen; i++)
                {
                    if (SDL_RWread(src, &trash, sizeof (unsigned char), 1) != 1)
                        return 0;
                }
                
                
                if (block == VOC_TEXT)
                    continue;    /* get next block */
        }
    }

    return 1;
}


static int voc_read(SDL_RWops *src, vs_t *v, unsigned char *buf, AudioSpec *spec)
{
    int done = 0;
    unsigned char silence = 0x80;

    if (v->rest == 0)
    {
        if (!voc_get_block(src, v , spec ))
            return 0;
    }

    if (v->rest == 0)
        return 0;

    if (v->silent)
    {



        /* Fill in silence */
        memset(buf, silence, v->rest);
        done = v->rest;
        v->rest = 0;
    }

    else
    {
        done = SDL_RWread(src, buf, 1, v->rest);
        v->rest -= done;
        if (v->size == ST_SIZE_WORD)
        {
            unsigned short *samples = (unsigned short *)buf;
            for (; v->rest > 0; v->rest -= 2)
            {
                *samples = SDL_Swap16(*samples);
                samples++;
            }

            done >>= 1;
        }
    }

    return done;
} /* voc_read */



/* don't call this directly; use Mix_LoadWAV_RW() for now. */
AudioSpec *Mix_LoadVOC_RW (SDL_RWops *src, int freesrc,
        AudioSpec *spec, unsigned char **audio_buf, unsigned long *audio_len)
{
    vs_t v;
    int was_error = 1;
    int samplesize;
    unsigned char *fillptr;
    void *ptr;

    if ( (!src) || (!audio_buf) || (!audio_len) )   /* sanity checks. */
        goto done;

    if ( !voc_check_header(src) )
        goto done;

    v.rate = -1;
    v.rest = 0;
    v.has_extended = 0;
    *audio_buf = NULL;
    *audio_len = 0;
    memset(spec, '\0', sizeof (AudioSpec));

    if (!voc_get_block(src, &v, spec))
        goto done;

    if (v.rate == -1)
    {
        Error("VOC data had no sound!");
        goto done;
    }

    spec->format = ((v.size == ST_SIZE_WORD) ? 0x8010 : 0x0008);
    
    
    if (spec->channels == 0)
        spec->channels = v.channels;

    *audio_len = v.rest;
    *audio_buf = malloc(v.rest);
    if (*audio_buf == NULL)
        goto done;

    fillptr = *audio_buf;

    while (voc_read(src, &v, fillptr, spec) > 0)
    {
        if (!voc_get_block(src, &v, spec))
            goto done;

        *audio_len += v.rest;
        ptr = realloc(*audio_buf, *audio_len);
                
        if (ptr == NULL)
        {
            free(*audio_buf);
            *audio_buf = NULL;
            *audio_len = 0;
            goto done;
        }

        *audio_buf = ptr;
        fillptr = ((unsigned char *) ptr) + (*audio_len - v.rest);
    }

    spec->samples = (unsigned short)(*audio_len / v.size);

    was_error = 0;  /* success, baby! */

    /* Don't return a buffer that isn't a multiple of samplesize */
    samplesize = ((spec->format & 0xFF)/8)*spec->channels;
    *audio_len &= ~(samplesize-1);

done:
        
    if (src)
    {
        if (freesrc)
            SDL_RWclose(src);
        else
            SDL_RWseek(src, 0, RW_SEEK_SET);
    }

    if ( was_error )
    { 
        spec = NULL;
    }
    
    return(spec);
} /* Mix_LoadVOC_RW */



#define CREA		0x61657243		/* "Crea" */

unsigned long SDL_ReadLE32 (SDL_RWops *src)
{
	unsigned long value;

	SDL_RWread(src, &value, (sizeof value), 1);
	return(SDL_Swap32(value));
}

/* Load a wave file */
Mix_Chunk *Mix_LoadWAV_RW(SDL_RWops *src, int freesrc)
{
	unsigned long magic;
	Mix_Chunk *chunk;
	AudioSpec *loaded, wavespec;
  
	/* rcg06012001 Make sure src is valid */
	if ( ! src ) {
		Error("Mix_LoadWAV_RW with NULL src");
		return(NULL);
	}

 
	/* Allocate the chunk memory */
	chunk = (Mix_Chunk *)malloc(sizeof(Mix_Chunk));
	if ( chunk == NULL ) {
		Error("Out of memory");
		if ( freesrc ) {
			SDL_RWclose(src);
		}
		return(NULL);
	}
 
    /* Find out what kind of audio file this is */
    magic = SDL_ReadLE32(src);
    
	/* Seek backwards for compatibility with older loaders */
	SDL_RWseek(src, -(int)sizeof(unsigned long), RW_SEEK_CUR);
    
        switch (magic) {
            
                case CREA:
                        loaded = Mix_LoadVOC_RW(src, freesrc, &wavespec,
                                        (unsigned char**)&chunk->abuf, &chunk->alen);
                        break;
                default:
                        Error("ERROR : Unrecognized sound file type : Expected 0x%x , Found 0x%x \n", CREA, magic);
                        return(0);                        
        }
 
	if ( !loaded ) { 
		/* The individual loaders have closed src if needed */
		free(chunk);
		return(NULL);
	}
  
	chunk->allocated = 1;
	chunk->volume = 128;
  
	return(chunk);
}

