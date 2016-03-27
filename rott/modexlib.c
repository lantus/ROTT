/*
Copyright (C) 1994-1995 Apogee Software, Ltd.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include <stdarg.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

 
#include <dos/dos.h>
#include <graphics/gfx.h>
#include <graphics/gfxbase.h>
#include <graphics/gfxmacros.h>
#include <intuition/intuition.h>
#include <libraries/asl.h>
#include <libraries/lowlevel.h>
#include <devices/gameport.h>
#include <devices/timer.h>
#include <devices/keymap.h>
#include <devices/input.h>
#include <devices/inputevent.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/graphics.h>
#include <proto/layers.h>
#include <proto/intuition.h>
#include <proto/asl.h>
#include <proto/keymap.h>
#include <proto/lowlevel.h>

#include <clib/commodities_protos.h> 
#include <cybergraphx/cybergraphics.h> 
#include <proto/cybergraphics.h>
 
 
#include <stdlib.h>
#include <sys/stat.h>

#include "modexlib.h"
//MED
#include "memcheck.h"

#include "rt_net.h" // for GamePaused
#include "rt_view.h"
#include "rt_in.h"
#include "isr.h"
#include "amiga_keysym.h"
static SDLKey MISC_keymap[256];
void amiga_InitKeymap(void);
static unsigned int scancodes[SDLK_LAST];
// GLOBAL VARIABLES

// Palette translation for Indivision GFX
static byte ptranslate[128]=
{
	0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,
	0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0x7b,0x7c,0x7d,0x7e,0x7f,
	0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f, 
	0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,
	0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x4b,0x4c,0x4d,0x4e,0x4f,
	0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,
	0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6a,0x6b,0x6c,0x6d,0x6e,0x6f,
	0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a,0x5b,0x5c,0x5d,0x5e,0x5f,
};
int  video_indigfx = 0;            // Indivision GFX mode

static struct ScreenModeRequester *video_smr = NULL;
 
static struct Window *video_window = NULL;
static struct BitMap video_bitmap[3] = {
  {0, 0, 0, 0, 0, {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL}},
  {0, 0, 0, 0, 0, {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL}},
  {0, 0, 0, 0, 0, {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL}}
};
static PLANEPTR video_raster[3] = {NULL, NULL, NULL};    /* contiguous bitplanes */
static UBYTE *video_compare_buffer[3] = {NULL, NULL, NULL};    /* in fastmem */
static struct RastPort video_rastport[3];
static struct ScreenBuffer *video_sb[2] = {NULL, NULL};
static struct DBufInfo *video_db = NULL;
static struct MsgPort *video_mp = NULL;

boolean StretchScreen=0;//bná++
extern boolean iG_aimCross;
extern boolean sdl_fullscreen;
extern int iG_X_center;
extern int iG_Y_center;
char 	   *iG_buf_center;
extern int cpu_type;
  
int    linewidth;
//int    ylookup[MAXSCREENHEIGHT];
int    ylookup[600];//just set to max res
byte  *page1start;
byte  *page2start;
byte  *page3start;
int    screensize;
byte  *bufferofs;
byte  *displayofs;
boolean graphicsmode=false;
char        *bufofsTopLimit;
char        *bufofsBottomLimit;

void VH_UpdateScreen (void);
#define REG(xn, parm) parm __asm(#xn)
#define REGARGS __regargs
#define STDARGS __stdargs
#define SAVEDS __saveds
#define ALIGNED __attribute__ ((aligned(4))
#define FAR
#define CHIP
#define INLINE __inline__
 
void REGARGS indivision_core(
REG( a0, ULONG indivisioncpldbase),
REG( d2, UWORD coreadd)
);

void REGARGS indivision_waitconfigdone(
REG( a0, ULONG indivisioncpldbase)
);

ULONG REGARGS indivision_checkcore(
REG(a0, ULONG indivisionfpgabase),
REG(d1, UWORD coreid)
);

void REGARGS indivision_initscreen(
REG(a0, ULONG indivisionfpgabase),
REG(d0, UBYTE screenmode)
);

void REGARGS  indivision_gfxcopy (
REG(a0, ULONG indivisionfpgabase),
REG(a2, UBYTE *chunky_data),
REG(a3, long *palette_data),
REG(d0, ULONG gfxaddr),
REG(d1, ULONG size)
);
                                
extern void REGARGS c2p1x1_8_c5_bm_040(
REG(d0, UWORD chunky_x),
REG(d1, UWORD chunky_y),
REG(d2, UWORD offset_x),
REG(d3, UWORD offset_y),
REG(a0, UBYTE *chunky_buffer),
REG(a1, struct BitMap *bitmap));

extern void REGARGS c2p1x1_6_c5_bm_040(
REG(d0, UWORD chunky_x),
REG(d1, UWORD chunky_y),
REG(d2, UWORD offset_x),
REG(d3, UWORD offset_y),
REG(a0, UBYTE *chunky_buffer),
REG(a1, struct BitMap *bitmap));

extern void REGARGS c2p1x1_8_c5_bm(
REG(d0, UWORD chunky_x),
REG(d1, UWORD chunky_y),
REG(d2, UWORD offset_x),
REG(d3, UWORD offset_y),
REG(a0, UBYTE *chunky_buffer),
REG(a1, struct BitMap *bitmap));

extern void REGARGS c2p1x1_6_c5_bm(
REG(d0, UWORD chunky_x),
REG(d1, UWORD chunky_y),
REG(d2, UWORD offset_x),
REG(d3, UWORD offset_y),
REG(a0, UBYTE *chunky_buffer),
REG(a1, struct BitMap *bitmap));

#include "amiga_mmu.h"

extern void mmu_stuff2(void);
extern void mmu_stuff2_cleanup(void);

int mmu_chunky = 0;
int mmu_active = 0;

 
void DrawCenterAim ();


static UWORD emptypointer[] = {
  0x0000, 0x0000,    /* reserved, must be NULL */
  0x0000, 0x0000,     /* 1 row of image data */
  0x0000, 0x0000    /* reserved, must be NULL */
};
 
enum videoMode
{
    VideoModeAGA,
    VideoModeEHB,
    VideoModeRTG,
    VideoModeINDIVISION
};
 
enum videoMode vidMode = VideoModeAGA;

// RTG Stuff

struct Library *CyberGfxBase = NULL;
static APTR video_bitmap_handle = NULL;
 

#ifdef DOS
 
#else
 
#ifndef STUB_FUNCTION

/* rt_def.h isn't included, so I just put this here... */
#if !defined(ANSIESC)
#define STUB_FUNCTION fprintf(stderr,"STUB: %s at " __FILE__ ", line %d, thread %d\n",__FUNCTION__,__LINE__,getpid())
#else
#define STUB_FUNCTION
#endif

#endif

/*
====================
=
= GraphicsMode
=
====================
*/

int usegamma = 0;
byte *I_VideoBuffer = NULL;
boolean screensaver_mode = false;
static UBYTE __attribute__ ((aligned (4))) screenpixels[320*200];
static boolean initialized = false;
static byte *disk_image = NULL;
static byte *saved_background;
 
int mouse_threshold = 10;

int usemouse = 1;
static boolean display_fps_dots;
boolean screenvisible = 1;

/** Hardware window */
struct Window *_hardwareWindow;
/** Hardware screen */
struct Screen *_hardwareScreen;
// Hardware double buffering.
struct ScreenBuffer *_hardwareScreenBuffer[2];
byte _currentScreenBuffer; 
signed short mx, my;
word mb = 0;
//
// I_SetPalette
//
 
static ULONG colorsAGA[770];
static ULONG video_colourtable[1 + 3*256 + 1];
static UBYTE video_xlate[256] = {0x00}; /* xlate 8-bit to 6-bit EHB */
 

void I_SetPalette (byte *palette)
{
    int i;
    int j = 1;
    static UBYTE gpalette[3*256];
  
    switch (vidMode)
    {
        case VideoModeAGA:
        case VideoModeRTG:
            for (i=0; i<256; ++i)
            {
                colorsAGA[j]   = gammatable[(gammaindex<<6)+(*palette++)] << 26;
                colorsAGA[j+1] = gammatable[(gammaindex<<6)+(*palette++)] << 26;
                colorsAGA[j+2] = gammatable[(gammaindex<<6)+(*palette++)] << 26;

                j+=3;
            }

            colorsAGA[0]=((256)<<16) ;
            colorsAGA[((256*3)+1)]=0x00000000;

            LoadRGB32(&_hardwareScreen->ViewPort, &colorsAGA);

            break;
        case VideoModeINDIVISION:

        // Indivision GFX uses 15bit palette

            if (playstate != ex_stillplaying)
            {
		      for (i=0; i<128; i++)
			     video_colourtable[ptranslate[i]]=
                    ( (((UBYTE)(gammatable[palette[6*i+3]]) & 0xf8) >> 3)  << 2|
                      (((UBYTE)(gammatable[palette[6*i+4]]) & 0xf8) << 2)  << 2|
           			  (((UBYTE)(gammatable[palette[6*i+5]]) & 0xf8) << 7)  << 2|
    				  (((UBYTE)(gammatable[palette[6*i+0]]) & 0xf8) << 13) << 2|
                      (((UBYTE)(gammatable[palette[6*i+1]]) & 0xf8) << 18) << 2|
      				  (((UBYTE)(gammatable[palette[6*i+2]]) & 0xf8) << 23) << 2);
            }
            break;
        case VideoModeEHB:

            if (playstate != ex_stillplaying)
            {
                i = 3 * 256 - 1;
                do {
                    gpalette[i] = gammatable[palette[i]]<< 2;
                } while (--i >= 0);

                video_colourtable[0] = (32 << 16) + 0;
                median_cut (gpalette, &video_colourtable[1], video_xlate);
                video_colourtable[33] = 0;

                LoadRGB32(&_hardwareScreen->ViewPort, &video_colourtable);

            }

            break;

    }
}
 


void I_ShutdownGraphics(void)
{
    int config_done = 0;
    int i = 0;
    
   if (_hardwareWindow) {
        ClearPointer(_hardwareWindow);
        CloseWindow(_hardwareWindow);
        _hardwareWindow = NULL;
    }
 
    if (_hardwareScreenBuffer[0]) { 
        WaitBlit();
        FreeScreenBuffer(_hardwareScreen, _hardwareScreenBuffer[0]);
    }

    if (_hardwareScreenBuffer[1]) { 
        WaitBlit();
        FreeScreenBuffer(_hardwareScreen, _hardwareScreenBuffer[1]);
    }

    if (_hardwareScreen) { 
        CloseScreen(_hardwareScreen);
        _hardwareScreen = NULL;
    }    
    
    if (mmu_active)
    {
        WaitBlit();
		mmu_mark(screenpixels,(320 * 200 + 4095) & (~0xFFF),mmu_chunky,SysBase);
		mmu_active = 0;
    }    
    
    if (vidMode ==  VideoModeINDIVISION )
    {
		config_done = 0;

		while (config_done==0)
		{
			indivision_core(0xdff1f0,0x0c00);
			i = TimeDelay(UNIT_VBLANK,1,0);
			indivision_waitconfigdone(0xdff1f0);
			if (indivision_checkcore(0xdff0ac,0x1000)!=0)
				config_done = 1;	
		}
		i = TimeDelay(UNIT_VBLANK,1,0);
		
          for (i = 0; i < 3; i++) {
            if (video_raster[i] != NULL) {
              FreeRaster (video_raster[i], 320,  200);
              video_raster[i] = NULL;
          }
        }
  		
    }    
        
 
    if (CyberGfxBase)
    {
        CloseLibrary(CyberGfxBase);
        CyberGfxBase = NULL;
    }
}


void GraphicsMode ( void )
{    
    char titlebuffer[256];
    static int    firsttime=1;
    uint i = 0;
    ULONG modeId = INVALID_ID;
    UBYTE *base_address;
    int video_depth = 8;
    int retries,config_done;
    struct Rectangle rect;
     
    _hardwareWindow = NULL;
    _hardwareScreenBuffer[0] = NULL;
    _hardwareScreenBuffer[1] = NULL;
    _currentScreenBuffer = 0;
    _hardwareScreen = NULL;

    if (firsttime)
    {
      
        firsttime = 0;
       // screenpixels = (unsigned char *)malloc(320*200);
          
        amiga_InitKeymap();
        
        if (CheckParm("EHB"))
        {
            printf("64 Color EHB mode set \n");
            vidMode = VideoModeEHB;
            video_depth = 6;
            modeId = EXTRAHALFBRITE_KEY;
            
            if (CheckParm("NTSC"))
                modeId |= NTSC_MONITOR_ID;
                 
        }
        else if (CheckParm("INDIVISION"))
        {
 
	 
    	printf ("Indivision GFX screen: %d x %d (8bit)\n", 320, 200);

        /* Then, open a dummy screen with one bitplane
           to minimize interference from chipset DMA    */
		
		video_depth = 1;

       	for (i = 0; i < (3); i++) 
		{ 
        		if ((video_raster[i] = (PLANEPTR)AllocRaster (320,
                                             video_depth * 200)) == NULL)
            			Error ("AllocRaster() for Indivision GFX failed");
        	
		    memset (video_raster[i], 0, video_depth * RASSIZE (320, 200));
        	InitBitMap (&video_bitmap[i], video_depth, 320, 200);
          	video_bitmap[i].Planes[0] = video_raster[i];
        	InitRastPort (&video_rastport[i]);
        	video_rastport[i].BitMap = &video_bitmap[i];
        
		    SetAPen (&video_rastport[i], 1);
        	SetBPen (&video_rastport[i], 0);
        	SetDrMd (&video_rastport[i], JAM2);
         
		}
		
		rect.MinX = 0;
        rect.MinY = 0;
        rect.MaxX = 320 - 1;
        rect.MaxY = 200 - 1;
     	
		if ((_hardwareScreen = OpenScreenTags (NULL,
            		SA_Type,        CUSTOMSCREEN | CUSTOMBITMAP,
            		SA_DisplayID,   0x00000000,
            		SA_DClip,       (ULONG)&rect,
            		SA_Width,       320,
            		SA_Height,      200,
            		SA_Depth,       video_depth,             
            		SA_Draggable,	FALSE,
            		SA_AutoScroll,	FALSE,
            		SA_Exclusive,	TRUE,
            		SA_Quiet,	TRUE,
            		SA_BitMap,      &video_bitmap[0], /* custom bitmap, contiguous planes */
            		TAG_DONE,       0)) == NULL) 
		{
        		Error ("OpenScreen() for Indivision GFX failed");
      		}

        /* Indivision GFX initialization */
        /* First, intialize Indivision GFX core */
		
		retries = 16;
		config_done = 0;

        // Apologies for the crude way to kick Indivision ECS safely into GFX mode

		while ((retries > 0) && (config_done==0))
		{
			indivision_core(0xdff1f0,0x3c00); 		// $3c00 = Indivision GFX Core
			
			i = TimeDelay(UNIT_VBLANK,1,0);			// 1 second to configure FPGA	
			
			indivision_waitconfigdone(0xdff1f0);		// Wait for FPGA startup
			if (indivision_checkcore(0xdff0ac,0x3000)!=0)
				config_done = 1;
			else
				retries--;	
		}

		if (retries == 0)
		{
			video_indigfx = 0;
			Error("could not init Indivision GFX hardware\n");
		}

        /* Now initialize Indivision GFX core
           for 8 bit direct chunky graphics  */

		i = TimeDelay(UNIT_VBLANK,2,0);
		indivision_initscreen(0xdff0ac,0);
		
		vidMode = VideoModeINDIVISION;
 
        }
        else if (CheckParm("CGX"))
        {
            printf("RTG mode set \n");
            vidMode = VideoModeRTG;    
            video_depth = 8;
            
            CyberGfxBase = OpenLibrary ("cybergraphics.library", 0);
        	if (CyberGfxBase == NULL) {
		      Error("Cannot open cybergraphics.library");
        	}            
        	
            if (CyberGfxBase != NULL)
            {
                modeId = BestCModeIDTags(CYBRBIDTG_NominalWidth, 320,
                                         CYBRBIDTG_NominalHeight, 200,
                                         CYBRBIDTG_Depth,video_depth,
                                         TAG_DONE);        	
            }
        }
        else
        {
            vidMode = VideoModeAGA;
            printf("256 Color AGA mode set \n");
            video_depth = 8;
            
            modeId = BestModeID(BIDTAG_NominalWidth, 320,
                    BIDTAG_NominalHeight, 200,
        	        BIDTAG_DesiredWidth, 320,
        	        BIDTAG_DesiredHeight, 200,
        	        BIDTAG_Depth, video_depth,
        	        BIDTAG_MonitorID, CheckParm("NTSC") ? NTSC_MONITOR_ID : PAL_MONITOR_ID,
        	        TAG_END);
        }

 
        
        printf("CPU : %d\n", cpu_type); 
            
        if (vidMode != VideoModeINDIVISION)
        { 
            if(modeId == INVALID_ID) {
              Error("ERROR : Could not find a valid screen mode.\n");
            }
        
            _hardwareScreen = OpenScreenTags(NULL,
                         SA_Depth, video_depth,
                         SA_DisplayID, modeId,
                         SA_Width, 320,
                         SA_Height,200,
                         SA_Type, CUSTOMSCREEN,
                         SA_Overscan, OSCAN_TEXT,
                         SA_Quiet,TRUE,
                         SA_ShowTitle, FALSE,
                         SA_Draggable, FALSE,
                         SA_Exclusive, TRUE,
                         SA_AutoScroll, FALSE,
                         TAG_END);                 
        }
    
        if ((CheckParm("MMU")) && cpu_type >= 68040 && vidMode != VideoModeRTG)
    	{
    		mmu_chunky = mmu_mark(screenpixels,(320 * 200 + 4095) & (~0xFFF),CM_WRITETHROUGH,SysBase);
    		mmu_active = 1;

            printf("MMU Active\n", cpu_type);
	    }        
        
        // Create the hardware screen.
        
        
        _hardwareScreenBuffer[0] = AllocScreenBuffer(_hardwareScreen, NULL, SB_SCREEN_BITMAP);
        _hardwareScreenBuffer[1] = AllocScreenBuffer(_hardwareScreen, NULL, 0);
 
        _currentScreenBuffer = 1;
        
        _hardwareWindow = OpenWindowTags(NULL,
                      	    WA_Left, 0,
                			WA_Top, 0,
                			WA_Width, 320,
                			WA_Height, 200,
                			WA_Title, NULL,
        					SA_AutoScroll, FALSE,
                			WA_CustomScreen, (ULONG)_hardwareScreen,
                			WA_Backdrop, TRUE,
                			WA_Borderless, TRUE,
                			WA_DragBar, FALSE,
                			WA_Activate, TRUE,
                			WA_SimpleRefresh, TRUE,
                			WA_NoCareRefresh, TRUE,
                			WA_ReportMouse, TRUE,
                			WA_RMBTrap, TRUE,
                      	    WA_IDCMP,  IDCMP_RAWKEY | IDCMP_MOUSEMOVE | IDCMP_DELTAMOVE | IDCMP_MOUSEBUTTONS,
                      	    TAG_END);
        
        SetPointer (_hardwareWindow, emptypointer, 1, 16, 0, 0);
   
 
        // Clear the screen to black.
        
        I_VideoBuffer = screenpixels;
                
        memset(I_VideoBuffer, 0, 320 * 200);
        
        initialized = true;
 
    } 
}

/*
====================
=
= SetTextMode
=
====================
*/
void SetTextMode ( void )
{
    I_ShutdownGraphics();
}

/*
====================
=
= TurnOffTextCursor
=
====================
*/
void TurnOffTextCursor ( void )
{
}

/*
====================
=
= WaitVBL
=
====================
*/

 
void WaitVBL( void )
{
 
}

/*
=======================
=
= VL_SetVGAPlaneMode
=
=======================
*/

void VL_SetVGAPlaneMode ( void )
{
   int i,offset;

    GraphicsMode();

//
// set up lookup tables
//
//bna--   linewidth = 320;
   linewidth = iGLOBAL_SCREENWIDTH;

   offset = 0;

   for (i=0;i<iGLOBAL_SCREENHEIGHT;i++)
      {
      ylookup[i]=offset;
      offset += linewidth;
      }

//    screensize=MAXSCREENHEIGHT*MAXSCREENWIDTH;
    screensize=iGLOBAL_SCREENHEIGHT*iGLOBAL_SCREENWIDTH;
 
    page1start=screenpixels;
    page2start=screenpixels;
    page3start=screenpixels;
    displayofs = page1start;
    bufferofs = page2start;

	iG_X_center = iGLOBAL_SCREENWIDTH / 2;
	iG_Y_center = (iGLOBAL_SCREENHEIGHT / 2)+10 ;//+10 = move aim down a bit

	iG_buf_center = bufferofs + (screensize/2);//(iG_Y_center*iGLOBAL_SCREENWIDTH);//+iG_X_center;

	bufofsTopLimit =  bufferofs + screensize - iGLOBAL_SCREENWIDTH;
	bufofsBottomLimit = bufferofs + iGLOBAL_SCREENWIDTH;
 
    XFlipPage ();
}

/*
=======================
=
= VL_CopyPlanarPage
=
=======================
*/
void VL_CopyPlanarPage ( byte * src, byte * dest )
{
 
    memcpy(dest,src,screensize);
 
}

/*
=======================
=
= VL_CopyPlanarPageToMemory
=
=======================
*/
void VL_CopyPlanarPageToMemory ( byte * src, byte * dest )
{
 
      memcpy(dest,src,screensize);
 
}

/*
=======================
=
= VL_CopyBufferToAll
=
=======================
*/
void VL_CopyBufferToAll ( byte *buffer )
{
 
}

/*
=======================
=
= VL_CopyDisplayToHidden
=
=======================
*/
void VL_CopyDisplayToHidden ( void )
{
   VL_CopyBufferToAll ( displayofs );
}

/*
=================
=
= VL_ClearBuffer
=
= Fill the entire video buffer with a given color
=
=================
*/

void VL_ClearBuffer (byte *buf, byte color)
{
 
  memset((byte *)buf,color,screensize);
 
}

/*
=================
=
= VL_ClearVideo
=
= Fill the entire video buffer with a given color
=
=================
*/

void VL_ClearVideo (byte color)
{

  memset (screenpixels, color, iGLOBAL_SCREENWIDTH*iGLOBAL_SCREENHEIGHT);
 
}

/*
=================
=
= VL_DePlaneVGA
=
=================
*/

void VL_DePlaneVGA (void)
{
}

void I_FinishUpdate (void)
{
    int i;
    static UBYTE pix[320*200];
    UBYTE *base_address;
    long width = 0;
    long height = 0;
    
    if (vidMode == VideoModeAGA)
    {
        if (cpu_type >= 68040)
            c2p1x1_8_c5_bm_040(320,200,0,0,screenpixels,_hardwareScreenBuffer[_currentScreenBuffer]->sb_BitMap);
        else
            c2p1x1_8_c5_bm(320,200,0,0,screenpixels,_hardwareScreenBuffer[_currentScreenBuffer]->sb_BitMap);

        ChangeScreenBuffer(_hardwareScreen, _hardwareScreenBuffer[_currentScreenBuffer]);
        _currentScreenBuffer = _currentScreenBuffer ^ 1;

    }
    else if (vidMode == VideoModeEHB)
    {
        for (  i = 0; i < (320 * 200); i++) {
            pix[i] = video_xlate[screenpixels[i]];
        }

        if (cpu_type >= 68040)
            c2p1x1_6_c5_bm_040(320,200,0,0,pix,_hardwareScreenBuffer[_currentScreenBuffer]->sb_BitMap);
        else
            c2p1x1_6_c5_bm(320,200,0,0,pix,_hardwareScreenBuffer[_currentScreenBuffer]->sb_BitMap);


        ChangeScreenBuffer(_hardwareScreen, _hardwareScreenBuffer[_currentScreenBuffer]);
        _currentScreenBuffer = _currentScreenBuffer ^ 1;

    }
    else if (vidMode == VideoModeINDIVISION)
    {
        indivision_gfxcopy(0xdff0ac,screenpixels,(long *)&video_colourtable[0],0x0,76800);    
    }
    else
    {
        video_bitmap_handle = LockBitMapTags (_hardwareScreen->ViewPort.RasInfo->BitMap,
                                              LBMI_BASEADDRESS, &base_address,
                                              TAG_DONE);
        if (video_bitmap_handle) {
            CopyMemQuick (screenpixels, base_address, 320 * 200);
            UnLockBitMap (video_bitmap_handle);
            video_bitmap_handle = NULL;
        }
    }
 
}
/* C version of rt_vh_a.asm */

void VH_UpdateScreen (void)
{ 	
 
	I_FinishUpdate();
}


/*
=================
=
= XFlipPage
=
=================
*/

void XFlipPage ( void )
{
 
    I_FinishUpdate();
 
}

#endif


void EnableScreenStretch(void)
{
    StretchScreen = 0;
}

void DisableScreenStretch(void)
{
 
}


// bna section -------------------------------------------
static void StretchMemPicture ()
{
 
}

// bna function added start
extern	boolean ingame;
int		iG_playerTilt;

void DrawCenterAim ()
{
 
}
 
void amiga_InitKeymap(void)
{
	int i;

	/* Map the miscellaneous keys */
	for ( i=0; i<256; ++i )
		MISC_keymap[i] = SDLK_UNKNOWN;
 
	MISC_keymap[65] = SDLK_BACKSPACE;
	MISC_keymap[66] = SDLK_TAB;
	MISC_keymap[70] = SDLK_CLEAR;
	MISC_keymap[70] = SDLK_DELETE;
	MISC_keymap[68] = SDLK_RETURN; 
	MISC_keymap[69] = SDLK_ESCAPE;
	MISC_keymap[70] = SDLK_DELETE;
	
	MISC_keymap[0x01] = SDLK_1;
	MISC_keymap[0x02] = SDLK_2;
	MISC_keymap[0x03] = SDLK_3;
	MISC_keymap[0x04] = SDLK_4;
	MISC_keymap[0x05] = SDLK_5;            	
	MISC_keymap[0x06] = SDLK_6;            		
	MISC_keymap[0x07] = SDLK_7;            	
	MISC_keymap[0x08] = SDLK_8;            	
	MISC_keymap[0x09] = SDLK_9;            	        	
	MISC_keymap[0x0A] = SDLK_0;       
    
    MISC_keymap[0x40] = SDLK_SPACE;       
    
    MISC_keymap[0x20] = SDLK_a;             
    MISC_keymap[0x35] = SDLK_b;      
    MISC_keymap[0x33] = SDLK_c;      
    MISC_keymap[0x22] = SDLK_d;      
    MISC_keymap[0x12] = SDLK_e;      
    MISC_keymap[0x23] = SDLK_f;      
    MISC_keymap[0x24] = SDLK_g;      
    MISC_keymap[0x25] = SDLK_h;      
    MISC_keymap[0x17] = SDLK_i;      
    MISC_keymap[0x26] = SDLK_j;      
    MISC_keymap[0x27] = SDLK_k;      
    MISC_keymap[0x28] = SDLK_l;      
    MISC_keymap[0x37] = SDLK_m;      
    MISC_keymap[0x36] = SDLK_n;      
    MISC_keymap[0x18] = SDLK_o;      
    MISC_keymap[0x19] = SDLK_p;      
    MISC_keymap[0x10] = SDLK_q;      
    MISC_keymap[0x13] = SDLK_r;   
    MISC_keymap[0x21] = SDLK_s;
    MISC_keymap[0x14] = SDLK_t;
    MISC_keymap[0x16] = SDLK_u;
    MISC_keymap[0x34] = SDLK_v;
    MISC_keymap[0x11] = SDLK_w;
    MISC_keymap[0x32] = SDLK_x;           
    MISC_keymap[0x15] = SDLK_y;               
    MISC_keymap[0x31] = SDLK_z;                   
    
    MISC_keymap[0x0B] = SDLK_MINUS;                   
    MISC_keymap[0x0C] = SDLK_EQUALS;                   
    MISC_keymap[0x1A] = SDLK_LEFTBRACKET;                   
    MISC_keymap[0x1B] = SDLK_RIGHTBRACKET;                   
    MISC_keymap[0x0D] = SDLK_BACKSLASH;                   
    MISC_keymap[0x29] = SDLK_SEMICOLON;      
                 
    MISC_keymap[0x2A] = SDLK_BACKQUOTE;                                               
    MISC_keymap[0x38] = SDLK_COMMA;                                               
    MISC_keymap[0x39] = SDLK_PERIOD;                                               
    MISC_keymap[0x3A] = SDLK_SLASH;                                                           
      	        		
 
	MISC_keymap[15] = SDLK_KP0;		/* Keypad 0-9 */
	MISC_keymap[29] = SDLK_KP1;
	MISC_keymap[30] = SDLK_KP2;
	MISC_keymap[31] = SDLK_KP3;
	MISC_keymap[45] = SDLK_KP4;
	MISC_keymap[46] = SDLK_KP5;
	MISC_keymap[47] = SDLK_KP6;
	MISC_keymap[61] = SDLK_KP7;
	MISC_keymap[62] = SDLK_KP8;
	MISC_keymap[63] = SDLK_KP9;
	MISC_keymap[60] = SDLK_KP_PERIOD;
	MISC_keymap[92] = SDLK_KP_DIVIDE;
	MISC_keymap[93] = SDLK_KP_MULTIPLY;
	MISC_keymap[74] = SDLK_KP_MINUS;
	MISC_keymap[94] = SDLK_KP_PLUS;
	MISC_keymap[67] = SDLK_KP_ENTER;
//	MISC_keymap[XK_KP_Equal&0xFF] = SDLK_KP_EQUALS;

	MISC_keymap[76] = SDLK_UP;
	MISC_keymap[77] = SDLK_DOWN;
	MISC_keymap[78] = SDLK_RIGHT;
	MISC_keymap[79] = SDLK_LEFT;
/*
	MISC_keymap[XK_Insert&0xFF] = SDLK_INSERT;
	MISC_keymap[XK_Home&0xFF] = SDLK_HOME;
	MISC_keymap[XK_End&0xFF] = SDLK_END;
*/
// Mappati sulle parentesi del taastierino
	MISC_keymap[90] = SDLK_PAGEUP;
	MISC_keymap[91] = SDLK_PAGEDOWN;

	MISC_keymap[80] = SDLK_F1;
	MISC_keymap[81] = SDLK_F2;
	MISC_keymap[82] = SDLK_F3;
	MISC_keymap[83] = SDLK_F4;
	MISC_keymap[84] = SDLK_F5;
	MISC_keymap[85] = SDLK_F6;
	MISC_keymap[86] = SDLK_F7;
	MISC_keymap[87] = SDLK_F8;
	MISC_keymap[88] = SDLK_F9;
	MISC_keymap[89] = SDLK_F10;
//	MISC_keymap[XK_F11&0xFF] = SDLK_F11;
//	MISC_keymap[XK_F12&0xFF] = SDLK_F12;
//	MISC_keymap[XK_F13&0xFF] = SDLK_F13;
//	MISC_keymap[XK_F14&0xFF] = SDLK_F14;
//	MISC_keymap[XK_F15&0xFF] = SDLK_F15;

//	MISC_keymap[XK_Num_Lock&0xFF] = SDLK_NUMLOCK;
	MISC_keymap[98] = SDLK_CAPSLOCK;
//	MISC_keymap[XK_Scroll_Lock&0xFF] = SDLK_SCROLLOCK;
	MISC_keymap[97] = SDLK_RSHIFT;
	MISC_keymap[96] = SDLK_LSHIFT;
	MISC_keymap[99] = SDLK_LCTRL;
	MISC_keymap[99] = SDLK_LCTRL;
	MISC_keymap[101] = SDLK_RALT;
	MISC_keymap[100] = SDLK_LALT;
//	MISC_keymap[XK_Meta_R&0xFF] = SDLK_RMETA;
//	MISC_keymap[XK_Meta_L&0xFF] = SDLK_LMETA;
	MISC_keymap[103] = SDLK_LSUPER; /* Left "Windows" */
	MISC_keymap[102] = SDLK_RSUPER; /* Right "Windows */

	MISC_keymap[95] = SDLK_HELP;
	
	
    memset(scancodes, '\0', sizeof (scancodes));
    scancodes[SDLK_ESCAPE]          = sc_Escape;
    scancodes[SDLK_1]               = sc_1;
    scancodes[SDLK_2]               = sc_2;
    scancodes[SDLK_3]               = sc_3;
    scancodes[SDLK_4]               = sc_4;
    scancodes[SDLK_5]               = sc_5;
    scancodes[SDLK_6]               = sc_6;
    scancodes[SDLK_7]               = sc_7;
    scancodes[SDLK_8]               = sc_8;
    scancodes[SDLK_9]               = sc_9;
    scancodes[SDLK_0]               = sc_0;
    
    //scancodes[SDLK_EQUALS]          = 0x4E;
    scancodes[SDLK_EQUALS]          = sc_Equals;
    
    scancodes[SDLK_BACKSPACE]       = sc_BackSpace;
    scancodes[SDLK_TAB]             = sc_Tab;
    scancodes[SDLK_q]               = sc_Q;
    scancodes[SDLK_w]               = sc_W;
    scancodes[SDLK_e]               = sc_E;
    scancodes[SDLK_r]               = sc_R;
    scancodes[SDLK_t]               = sc_T;
    scancodes[SDLK_y]               = sc_Y;
    scancodes[SDLK_u]               = sc_U;
    scancodes[SDLK_i]               = sc_I;
    scancodes[SDLK_o]               = sc_O;
    scancodes[SDLK_p]               = sc_P;
    scancodes[SDLK_LEFTBRACKET]     = sc_OpenBracket;
    scancodes[SDLK_RIGHTBRACKET]    = sc_CloseBracket;
    scancodes[SDLK_RETURN]          = sc_Return;
    scancodes[SDLK_LCTRL]           = sc_Control;
    scancodes[SDLK_a]               = sc_A;
    scancodes[SDLK_s]               = sc_S;
    scancodes[SDLK_d]               = sc_D;
    scancodes[SDLK_f]               = sc_F;
    scancodes[SDLK_g]               = sc_G;
    scancodes[SDLK_h]               = sc_H;
    scancodes[SDLK_j]               = sc_J;
    scancodes[SDLK_k]               = sc_K;
    scancodes[SDLK_l]               = sc_L;
    scancodes[SDLK_SEMICOLON]       = 0x27;
    scancodes[SDLK_QUOTE]           = 0x28;
    scancodes[SDLK_BACKQUOTE]       = 0x29;
    
    /* left shift, but ROTT maps it to right shift in isr.c */
    scancodes[SDLK_LSHIFT]          = sc_RShift; /* sc_LShift */
    
    scancodes[SDLK_BACKSLASH]       = 0x2B;
    /* Accept the German eszett as a backslash key */
    scancodes[SDLK_WORLD_63]        = 0x2B;
    scancodes[SDLK_z]               = sc_Z;
    scancodes[SDLK_x]               = sc_X;
    scancodes[SDLK_c]               = sc_C;
    scancodes[SDLK_v]               = sc_V;
    scancodes[SDLK_b]               = sc_B;
    scancodes[SDLK_n]               = sc_N;
    scancodes[SDLK_m]               = sc_M;
    scancodes[SDLK_COMMA]           = sc_Comma;
    scancodes[SDLK_PERIOD]          = sc_Period;
    scancodes[SDLK_SLASH]           = 0x35;
    scancodes[SDLK_RSHIFT]          = sc_RShift;
    scancodes[SDLK_KP_DIVIDE]       = 0x35;
    
    /* 0x37 is printscreen */
    //scancodes[SDLK_KP_MULTIPLY]     = 0x37;
    
    scancodes[SDLK_LALT]            = sc_Alt;
    scancodes[SDLK_RALT]            = sc_Alt;
    scancodes[SDLK_MODE]            = sc_Alt;
    scancodes[SDLK_RCTRL]           = sc_Control;
    scancodes[SDLK_SPACE]           = sc_Space;
    scancodes[SDLK_CAPSLOCK]        = sc_CapsLock;
    scancodes[SDLK_F1]              = sc_F1;
    scancodes[SDLK_F2]              = sc_F2;
    scancodes[SDLK_F3]              = sc_F3;
    scancodes[SDLK_F4]              = sc_F4;
    scancodes[SDLK_F5]              = sc_F5;
    scancodes[SDLK_F6]              = sc_F6;
    scancodes[SDLK_F7]              = sc_F7;
    scancodes[SDLK_F8]              = sc_F8;
    scancodes[SDLK_F9]              = sc_F9;
    scancodes[SDLK_F10]             = sc_F10;
    scancodes[SDLK_F11]             = sc_F11;
    scancodes[SDLK_F12]             = sc_F12;
    scancodes[SDLK_NUMLOCK]         = 0x45;
    scancodes[SDLK_SCROLLOCK]       = 0x46;
    
    //scancodes[SDLK_MINUS]           = 0x4A;
    scancodes[SDLK_MINUS]           = sc_Minus;
    
    scancodes[SDLK_KP7]             = sc_Home;
    scancodes[SDLK_KP8]             = sc_UpArrow;
    scancodes[SDLK_KP9]             = sc_PgUp;
    scancodes[SDLK_HOME]            = sc_Home;
    scancodes[SDLK_UP]              = sc_UpArrow;
    scancodes[SDLK_PAGEUP]          = sc_PgUp;
    // Make this a normal minus, for viewport changing
    //scancodes[SDLK_KP_MINUS]        = 0xE04A;
    scancodes[SDLK_KP_MINUS]        = sc_Minus;
    scancodes[SDLK_KP4]             = sc_LeftArrow;
    scancodes[SDLK_KP5]             = 0x4C;
    scancodes[SDLK_KP6]             = sc_RightArrow;
    scancodes[SDLK_LEFT]            = sc_LeftArrow;
    scancodes[SDLK_RIGHT]           = sc_RightArrow;
    
    //scancodes[SDLK_KP_PLUS]         = 0x4E;
    scancodes[SDLK_KP_PLUS]         = sc_Plus;
    
    scancodes[SDLK_KP1]             = sc_End;
    scancodes[SDLK_KP2]             = sc_DownArrow;
    scancodes[SDLK_KP3]             = sc_PgDn;
    scancodes[SDLK_END]             = sc_End;
    scancodes[SDLK_DOWN]            = sc_DownArrow;
    scancodes[SDLK_PAGEDOWN]        = sc_PgDn;
    scancodes[SDLK_DELETE]          = sc_Delete;
    scancodes[SDLK_KP0]             = sc_Insert;
    scancodes[SDLK_INSERT]          = sc_Insert;
    scancodes[SDLK_KP_ENTER]        = sc_Return;
    	
}

ULONG amiga_TranslateKey(int code)
{
    UBYTE buffer[8] = {0};
    int k = 0; 
	int actual = 0;
 
	k = MISC_keymap[code];
     
	/* Get the translated SDL virtual keysym */
	
	if (k == SDLK_UNKNOWN)
	{
		struct InputEvent	ie;
		                  
		ie.ie_Class				= IECLASS_RAWKEY; 
		ie.ie_Code				= code;  	 
       
		actual = MapRawKey(&ie, &buffer, 8,  NULL);
        
        
		if (actual == 1)
		{
            
            		 
			k = buffer[0];
		 
		}
	}
    
    k = scancodes[k];
    
	return(k);
} 

void doEvents (void)
{    
    
    ULONG class;
    UWORD code;
    WORD mousex, mousey;
    static WORD mousebuttons = 0;
    struct IntuiMessage *msg;
    static ULONG previous = 0;
  
    int rawKey;
	ULONG k;
    int keyon;
    int strippedkey;
    int extended;     
    
    if (_hardwareWindow != NULL && _hardwareWindow->UserPort != NULL) {
        while ((msg = (struct IntuiMessage *)GetMsg (_hardwareWindow->UserPort)) != NULL) {
            
            ReplyMsg ((struct Message *)msg);  /* reply after xlating key */
            
            class = msg->Class;
            code = msg->Code;
            mousex = msg->MouseX;
            mousey = msg->MouseY;
      
      
            if( !(code&IECODE_UP_PREFIX) )
            {
                k = amiga_TranslateKey(code);               
        	    keyon = 0;        	             
            }
            else
            {
                /* Key release? */
 
            	k = amiga_TranslateKey(code&~IECODE_UP_PREFIX);         
                k+=128;
                keyon = 1;                                       
            }
 
      
            strippedkey = k & 0x7f;
     
            if (k != 0)
            {
                if (keyon)           // Up event
                    Keystate[strippedkey]=0;
                else                 // Down event
                {
                    Keystate[strippedkey]=1;
                    LastScan = k;
                }
            
                KeyboardQueue[ Keytail ] = k;
                Keytail = ( Keytail + 1 )&( KEYQMAX - 1 );
            }
            
            if (class == IDCMP_MOUSEMOVE)
            {
                mx = (mousex << 3);
            }
            else if (class == IDCMP_MOUSEBUTTONS)
            {
                mousebuttons = 0;
                                
                switch (msg->Code) {
                    case SELECTDOWN:  
                        mousebuttons |= 1;
                        break;
                      case SELECTUP: 
                        mousebuttons &= ~1;
                        break;
                      case MENUDOWN:
                        mousebuttons |= 4;
                        break;
                      case MENUUP:
                        mousebuttons &= ~4;
                        break;
                      case MIDDLEDOWN:
                        mousebuttons |= 2;
                        break;
                      case MIDDLEUP:
                        mousebuttons &= ~2;
                        break;
                      default:
                        break;
            
                }

                mb = mousebuttons;
            }

    }
  }
      
}

