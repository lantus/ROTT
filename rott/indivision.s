		mc68020

; **** Indivision graphics core driver by Oliver Achten ****

; The Indivision graphics core is a sub-function of the Indivision ECS
; flickerfixer to provide a direct CPU interface to the SDRAM framebuffer.
; Although the access to the framebuffer is realised using a simple register
; driven interface, it means that a chunky buffer in fast-ram can be trans-
; ferred with up to 3.5 MB/s, depending on the bus design of the Amiga or
; accelerator. This figure should easily outperform the C2P6 routines on ECS
; Amigas. There is no DMA penalty involved from setting up a 6 bitplane 
; EHB screen, which cuts CPU access to chip ram by 50%, and no additional CPU 
; time required to perform chunky/planar conversion. Not to mention that it 
; looks much better in 256 colours.

		xdef	_indivision_core
		xdef	_indivision_waitconfigdone
		xdef	_indivision_checkcore
		xdef	_indivision_initscreen
		xdef	_indivision_gfxcopy

		section text,code
;******************************************************************************

;void __asm indivision_core(register __a0 ULONG indivisioncpldbase
;                           register __d2 UWORD coreadd)

_indivision_core:
	movem.l d0-d1/a1,-(sp)

	move.l a0,a1		; set up CPLD register bases
	addq.l #4,a1

	move.w #$5000,(a0)	; unlock CPLD
	move.w #$b000,(a0)

	move.w d2,$dff180	; delay 1 CCK

	move.w #$0,(a0)		; reset flash
	move.w #$2000,(a0)

stop_fpga:
	move.w #$0100,(a1)	; reset fpga
	move.w #$0fff,d0	; delay minimum: 4096 CCK cycles
nconfig_loop:
	move.w d0,$dff180	; reading from chip register defines timing
	dbra d0,nconfig_loop

	move.w d2,(a1)		; load fpga core

	movem.l (sp)+,d0-d1/a1
	rts

;******************************************************************************


;void __asm indivision_waitconfigdone(register __a0 ULONG indivisioncpldbase)

_indivision_waitconfigdone:
	movem.l d0-d2/a1,-(sp)

	move.l a0,a1
	addq.l #8,a1		; base for CPLD register

	move.w #$ffff,d0
	move.w #$f,d1
nconfig_wait:
	move.w d0,$dff180	; make flashy colours and define minimum timing
	move.w (a1),d2		; read FPGA "config done" line
	and.w #$4000,d2
	bne fpga_configured
	dbra d0,nconfig_wait
	dbra d1,nconfig_wait

fpga_configured:
	move.w #$0,(a0)		; deselect flash
	move.w #$f000,(a0)	; lock CPLD

	movem.l (sp)+,d0-d2/a1
	rts

;******************************************************************************

;int __asm indivision_checkcore(register __a0 ULONG indivisionfpgabase
;			        register __d1 UWORD coreid)

_indivision_checkcore:
	movem.l d2-d4/a1,-(sp)

	move.l a0,a1		; set up FPGA register bases
	add.l #$10,a1
	
	moveq #0,d0
	move.w #$ffff,d3	; set wait loop
wait_core_loop1:
	move.w #$0,(a0)		; unlock fpga registers
	move.w #$fa50,(a1)

	move.w #$7,(a0)		; check fpga core id 
	move.w (a1),d4
	and.w #$f000,d4
	cmp.w d1,d4
	bne no_core
	
	move.w #$fff,d2		; now check if fpga id is stable
wait_core_loop2:
	move.w (a1),d4
	and.w #$f000,d4
	cmp.w d1,d4
	bne no_core
	dbra d2,wait_core_loop2

	moveq #-1,d0		; found it!
	bra wait_core_end
no_core:
	dbra d3,wait_core_loop1
wait_core_end:
	move.w #$3,(a0)
	move.w #$0,(a1)
	move.w #$0,(a0)
	move.w #$0,(a1)

	movem.l (sp)+,d2-d4/a1
	rts

;******************************************************************************

;void __asm indivision_initscreen(register __a0 ULONG indivisionfpgabase,
;				  register __d0 UBYTE screenmode)

_indivision_initscreen:
	movem.l a1-a2,-(sp)

	lea.l parameter_512x384(PC),a2
	cmp.b #$06,d0
	beq end_mode_select
	lea.l parameter_640x200(PC),a2
	cmp.b #$05,d0
	beq end_mode_select
	lea.l parameter_320x400(PC),a2
	cmp.b #$04,d0
	beq end_mode_select
	lea.l parameter_1024x768(PC),a2
	cmp.b #$03,d0
	beq end_mode_select
	lea.l parameter_800x600(PC),a2
	cmp.b #$02,d0
	beq end_mode_select
	lea.l parameter_640x400(PC),a2
	cmp.b #$01,d0
	beq end_mode_select
	lea.l parameter_320x200(PC),a2

end_mode_select:
	move.l a0,a1		; set up FPGA register bases
	add.l #$10,a1

	move.w #$0,(a0)		; unlock FPGA
	move.w #$fa50,(a1)

	move.w #$c0,d0		; clear video registers
clear_registers:
	move.w d0,(a0)
	move.l #$0,(a1)
	add.w #$1,d0
	cmp.w #$100,d0
	bne clear_registers

copy_display_params:
	cmp.w #$ffff,(a2)
	beq display_param_end
	move.w (a2)+,(a0)
	move.w (a2)+,(a1)
	bra copy_display_params	

display_param_end:
	move.w #$0,(a0)		; lock FPGA
	move.w #$0,(a1)

	movem.l (sp)+,a1-a2
	rts

parameter_320x200:
	dc.w $00c0,$0005,$00c1,$0000,$00c2,$0001,$00c5,$0000
	dc.w $00d1,$0190,$00d2,$01c1,$00d3,$0000,$00d4,$0030
	dc.w $00d5,$000c,$00d6,$000e,$00d7,$0048,$00d8,$013f
	dc.w $00d9,$0031,$00da,$018f,$00d0,$0005,$ffff,$ffff

parameter_640x400:
	dc.w $00c0,$0002,$00c1,$0000,$00c2,$0001,$00c5,$0000
	dc.w $00d1,$0320,$00d2,$01c1,$00d3,$0000,$00d4,$0060
	dc.w $00d5,$000c,$00d6,$000e,$00d7,$0090,$00d8,$027f
	dc.w $00d9,$0031,$00da,$018f,$00d0,$0001,$ffff,$ffff

parameter_800x600:
	dc.w $00c0,$0001,$00c1,$0000,$00c2,$0001,$00c5,$0000
	dc.w $00d1,$03ff,$00d2,$0270,$00d3,$0000,$00d4,$0048
	dc.w $00d5,$0001,$00d6,$0003,$00d7,$00c7,$00d8,$031f
	dc.w $00d9,$0017,$00da,$0257,$00d0,$0001,$ffff,$ffff

parameter_1024x768:
	dc.w $00c0,$0000,$00c1,$0000,$00c2,$0001,$00c5,$0000
	dc.w $00d1,$0530,$00d2,$0326,$00d3,$0018,$00d4,$00a0
	dc.w $00d5,$0003,$00d6,$0009,$00d7,$012e,$00d8,$03ff
	dc.w $00d9,$0025,$00da,$02ff,$00d0,$0001,$ffff,$ffff

parameter_320x400:
	dc.w $00c0,$0005,$00c1,$0000,$00c2,$0001,$00c5,$0000
	dc.w $00d1,$0190,$00d2,$01c1,$00d3,$0000,$00d4,$0030
	dc.w $00d5,$000c,$00d6,$000e,$00d7,$0048,$00d8,$013f
	dc.w $00d9,$0030,$00da,$018f,$00d0,$0001,$ffff,$ffff

parameter_640x200:
	dc.w $00c0,$0002,$00c1,$0000,$00c2,$0001,$00c5,$0000
	dc.w $00d1,$0320,$00d2,$01c1,$00d3,$0000,$00d4,$0060
	dc.w $00d5,$000c,$00d6,$000e,$00d7,$0090,$00d8,$027f
	dc.w $00d9,$0031,$00da,$018f,$00d0,$0005,$ffff,$ffff

parameter_512x384:
	dc.w $00c0,$0001,$00c1,$0000,$00c2,$0001,$00c5,$0000
	dc.w $00d1,$0298,$00d2,$0326,$00d3,$000c,$00d4,$0050
	dc.w $00d5,$0003,$00d6,$0009,$00d7,$0096,$00d8,$01ff
	dc.w $00d9,$0025,$00da,$02ff,$00d0,$0005,$ffff,$ffff

;******************************************************************************

;void __asm indivision_gfxcopy (register __a0 ULONG indivisionfpgabase,
;                               register __a2 UBYTE *chunky_data,
;                               register __a3 long *palette_data,
;                               register __d0 ULONG gfxaddr,
;                               register __d1 ULONG size);

_indivision_gfxcopy:
	movem.l d2/a1,-(sp)
	
	move.l a0,a1		; set up FPGA register bases
	add.l #$10,a1

	move.w #$0,(a0)		; unlock FPGA
	move.w #$fa50,(a1)

	move.w #$c3,(a0)	; reset palette index
	move.w #0,(a1)
	
	move.w #$c4,(a0)	; set palette data register

	move.w #$ff,d2		; copy 256 entries
copypalette_loop:	
	move.w (a3)+,(a1)
	dbra d2,copypalette_loop 

	move.w #$c6,(a0)	; set SDRAM write address
	move.l d0,(a1)

	lsr.l #5,d1		; divide size by 32
	move.w #$c7,(a0)	; SDRAM write port
gfxcopy_loop:
	move.l (a2)+,(a1)	; Copy 32 bytes
	move.l (a2)+,(a1)	; Loop unrolled to optimize bus speed
	move.l (a2)+,(a1)
	move.l (a2)+,(a1)
	move.l (a2)+,(a1)
	move.l (a2)+,(a1)
	move.l (a2)+,(a1)
	move.l (a2)+,(a1)
	dbra d1,gfxcopy_loop

	move.w #$0,(a0)
	move.w #$0,(a1)	
	
	movem.l (sp)+,d2/a1
	rts
