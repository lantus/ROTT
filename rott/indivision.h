void __regargs __asm indivision_core(register __a0 ULONG indivisioncpldbase,
                           register __d2 UWORD coreadd);

void __regargs __asm indivision_waitconfigdone(register __a0 ULONG indivisioncpldbase);

ULONG __regargs __asm indivision_checkcore(register __a0 ULONG indivisionfpgabase,
				register __d1 UWORD coreid);

void __regargs __asm indivision_initscreen(register __a0 ULONG indivisionfpgabase,
				 register __d0 UBYTE screenmode);

void __regargs __asm indivision_gfxcopy (register __a0 ULONG indivisionfpgabase,
                               register __a2 UBYTE *chunky_data,
                               register __a3 long *palette_data,
                               register __d0 ULONG gfxaddr,
                               register __d1 ULONG size);
