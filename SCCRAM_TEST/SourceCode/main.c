#include <stdio.h>
#include <msx.h>
#include <sys/ioctl.h>

#define VERSION "V1.02"
#define DATE "2023/06"

#define BUF_SIZE 128

//#include <msx/gfx.h>

unsigned char SelectSlot;		//Select Slot number
unsigned char eseBank;			//eseSCC 6000-7fffh Bank number
unsigned char *addressWrite;	//Write pointer
unsigned char eseSlot;			//eseSCC Slot number
unsigned char maxBank;			//ROM Size
unsigned char faildata;	

int findId;						//Flash ID


//Assembler SubRoutine
void asmCalls(){
#asm
chengePageSlot:
	di
    ld a,(_SelectSlot)	;Slot Number
    ld hl,04000h		;Select page 1
    call 0024h			;call ENASLT

    ld a,(_SelectSlot)	;Slot Number
    ld hl,08000h		;Select page 2
    call 0024h			;call ENASLT
	ret

restoreRAMPage:
RAMPass:
	ld a,000h				;
    ld (07ffeh),a			;Write enable

	ld a,(0f342h)		;Restore RAM Page (RAMAD1)
    ld hl,04000h		;Select page 1
    call 0024h			;call ENASLT

	ld a,(0f343h)		;Restore RAM Page (RAMAD2)
    ld hl,08000h		;Select page 2
    call 0024h			;call ENASLT
	ei
	ld hl,00000h		;find!
	ret
	
RAMErr:
	ld a,(0f342h)		;Restore RAM Page (RAMAD1)
    ld hl,04000h		;Select page 1
    call 0024h			;call ENASLT

	ld a,(0f343h)		;Restore RAM Page (RAMAD2)
    ld hl,08000h		;Select page 2
    call 0024h			;call ENASLT
	ei
	ld hl,0ffffh		;Not find!
	ret
	
checkSramData:
	push hl					;Backup Buffer address
	call chengePageSlot		;Page1 Select PAC Slot
	ld de,04000h			;PAC SRAM address
	ld bc,02000h			;Check Size
	pop hl					;Buffer address load
cmpLoop:
	ld a,(de)				;SRAM Data load
	cpi						;check data
	jr nz,RAMErr			;Compare Error->finsh
	jp po,RAMPass			;Compare Pass->finsh
	inc de					;SRAM Address++
	jr cmpLoop				;Continue Compare

#endasm
}

int writeEseSCC(unsigned char banknum)
{
#asm
	push hl					;Backup Buffer address
	call chengePageSlot		;4000-bfffh Change eseSCC Slot 
	pop hl
	ld a,l
	and 040h
	ld h,a
    ld (07ffeh),a			;upperbank sel

	ld a,l
	and 03fh
    ld (05000h),a			;Bank 4000-5fff

	ld a,l
	or a
	jr z,skipChkData
	ld a,(04000h)
	cp 05ah
	jp z,RAMErr
	
skipChkData:
	ld a,010h				;
	or h
    ld (07ffeh),a			;Write enable

	ld a,l
	ld d,a
	
	rrc a
	rrc a
	and 0c0h
	or l
	xor 05ah
	ld d,a
	ld hl,04000h			;RAM address
	ld bc,02000h			;Transfer Size
loopWr:
    ld (hl),d			;Write enable
	inc d
	inc hl
	dec bc
	ld a,b
	or c
	jr nz,loopWr
	
	jp restoreRAMPage
#endasm
}

void resetEseSCC(unsigned char banknum)
{
#asm
	push hl					;Backup Buffer address
	call chengePageSlot		;4000-bfffh Change eseSCC Slot 
	pop hl

	ld a,l
	and 040h
	ld h,a

	ld a,001h
	ld (07000h),a
	
	ld a,l
	and 03fh
    ld (05000h),a			;Bank 4000-5fff

	ld a,010h				;
	or h
    ld (07ffeh),a			;Write enable

	xor a
	ld (04000h),a
	jp restoreRAMPage
#endasm
}

int readEseSCC_B4000(unsigned char banknum)
{
#asm
	push hl					;Backup Buffer address
	call chengePageSlot		;4000-bfffh Change eseSCC Slot 
	pop hl
	ld a,l
	and 03fh
	ld (05000h),a			;Bank 4000-5fff
	ld de,04000h			;RAM address

readchk:
	ld a,l
	and 040h
    ld (07ffeh),a			;Write enable

	ld a,l
	rrc a
	rrc a
	and 0c0h
	or l
	xor 05ah
	ex de,hl
	ld d,a
	ld bc,02000h			;Transfer Size
loopB4:
	ld a,d
	cp (hl)
	jp nz,CompareErr
chkBack:
	inc d
	inc hl
	dec bc
	ld a,b
	or c
	jr nz,loopB4

	jp restoreRAMPage
CompareErr:
	ld a,b
	or b
	jr nz,CompErrTrue
	ld a,c
	cp 002h
	jr nz,CompErrTrue
	ld a,(HL)
	or a
	jr z,chkBack
CompErrTrue:
	push hl
	ld a,(0f342h)		;Restore RAM Page (RAMAD1)
    ld hl,04000h		;Select page 1
    call 0024h			;call ENASLT

	ld a,(0f343h)		;Restore RAM Page (RAMAD2)
    ld hl,08000h		;Select page 2
    call 0024h			;call ENASLT
	ei
	pop hl
	ld a,(hl)
	ld (_faildata),a
	ret
#endasm
}

int readEseSCC_B6000(unsigned char banknum)
{
#asm
	push hl					;Backup Buffer address
	call chengePageSlot		;4000-bfffh Change eseSCC Slot 
	pop hl
	ld a,l
	and 03fh
    ld (07000h),a			;Bank 4000-5fff
	ld de,06000h			;RAM address
	jr readchk
#endasm
}

int readEseSCC_B8000(unsigned char banknum)
{
#asm
	push hl					;Backup Buffer address
	call chengePageSlot		;4000-bfffh Change eseSCC Slot 
	pop hl
	ld a,l
	and 03fh
    ld (09000h),a			;Bank 4000-5fff
	ld de,08000h			;RAM address
	jr readchk
#endasm
}

int readEseSCC_BA000(unsigned char banknum)
{
#asm
	push hl					;Backup Buffer address
	call chengePageSlot		;4000-bfffh Change eseSCC Slot 
	pop hl
	ld a,l
	and 03fh
    ld (0B000h),a			;Bank 4000-5fff
	ld de,0A000h			;RAM address
	jp readchk
#endasm
}

int readEseSCC_B9800(unsigned char banknum)
{
#asm
	push hl					;Backup Buffer address
	call chengePageSlot		;4000-bfffh Change eseSCC Slot 
	pop hl
	ld a,l
	and 03fh
	ld (09000h),a			;Bank 4000-5fff
	ld de,04000h			;RAM address

	ld a,l
	and 040h
    ld (07ffeh),a			;Write enable

	ld a,l
	rrc a
	rrc a
	and 0c0h
	or l
	xor 05ah
	ex de,hl
	push af
	ld bc,02000h			;Transfer Size
	ld de,09800h 
loopB98:
	or a
	sbc hl,de
	ex af,af'
	add hl,de
	ex af,af'
	jr z,SkipSCCWork
SkipEnd:
	pop af
	cp (hl)
	jp nz,CompareErr
	inc a
	push af
	inc hl
	dec bc
	ld a,b
	or c
	jr nz,loopB98

	pop af
	jp restoreRAMPage

SkipSCCWork:
	ld de,09900h 
dummyLoop:
	inc a
	inc hl
	dec bc
	or a
	sbc hl,de
	ex af,af'
	add hl,de
	ex af,af'
	jr nz,dummyLoop
	push af
	jr SkipEnd

#endasm
}

int chkEseSCC() __z88dk_fastcall __naked {
#asm
	di
	di
    ld a,(_SelectSlot)	;Slot Number
    ld hl,04000h		;Select page 1
    call 0024h			;call ENASLT

    ld a,(_SelectSlot)	;Slot Number
    ld hl,08000h		;Select page 2
    call 0024h			;call ENASLT

	ld hl,07ffeh
	ld a,(hl)			; backup data
	ld b,a			
	or a
	jr z,data_zero
	ld c,000h			; SCC RAM Write disable
	ld (hl),c			;
    ld a,(hl)			; load data
	or a
	jr nz,CheckData

	ld c,001h			; SCC RAM Write disable
	ld (hl),c			;
    ld a,(hl)			; load data
	xor 001h
	jr nz,CheckData

    ld (hl),b			; Restore Data
	jp RAMErr

data_zero:
	ld c,000h			; SCC RAM Write disable
    ld (hl),c			;

CheckData:
	ld hl,04000h		;Check Address 
	call chkData		;Check 4000h
	jp z,RAMErr			;Fail (RAM Slot?)

	ld hl,06000h		;Check Address 
	call chkData		;Check 6000h
	jp z,RAMErr			;Fail (RAM Slot?)
	
	ld a,010h				;
    ld (07ffeh),a			;Write enable

	ld hl,04000h		;Check Address 
	call chkData		;Check 4000h
	jp nz,RAMErr			;Fail (RAM Slot?)
	inc hl				;One more check...
	call chkData		;Check 4001h
	jp nz,RAMErr		;Fail (Not Use/ROM Slot?)

	ld hl,06000h		;Check Address 
	call chkData		;Check 6000h
	jp nz,RAMErr			;Fail (RAM Slot?)
	inc hl				;One more check...
	call chkData		;Check 4001h
	jp nz,RAMErr		;Fail (Not Use/ROM Slot?)
	jp RAMPass

chkData:
	ld a,(hl)			;Read SRAM Data
	ld b,a				;Backup org data
	xor 0ffh			;Data bitflip
	ld (hl),a			;Write SRAM Data
	ld c,(hl)			;Read SRAM Data (Again)
	ld (hl),b			;pop ori Data
	xor c				;2nd Data(XOR)?
	ret					;z = Read/Write PASS 
#endasm
}

void findEseSCC(void){
	unsigned char i;
	unsigned int chipId;
	
	if ((SelectSlot & 0xf0) == 0x80){
		//Expantion Slot Check
		for (i=0;i<4;i++){
			if (chkEseSCC() == 0x0000) {
				eseSlot = SelectSlot;
			}
			SelectSlot = SelectSlot + 4;
		}
	}else{
		//Master Slot Check
		chipId = chkEseSCC();
		if (chkEseSCC() == 0x0000) {
			eseSlot = SelectSlot;
		}
	}
}
	
int main(int argc,char *argv[])
{

	int i;
	unsigned char dat;
	int add;
	
	
	printf("MEGA SCC RAM Test %s\n",VERSION);
	printf("Copyrigth %s @v9938\n\n",DATE);

	if (argc<1){
		printf( "sccram\n");
		printf( "This program will write the selected file to the flash ROM.\n");
		return 0;
	}

	printf("Search Mega-SCC RAM ... ");
	eseSlot = 0;
	//Slot 1 Search
	SelectSlot = *((unsigned char *)0xfcc2) | 0x01;		//EXPTBL (SLOT1)
	findEseSCC();
	//Slot 2 Search
	SelectSlot = *((unsigned char *)0xfcc3) | 0x02;		//EXPTBL (SLOT2)
	findEseSCC();
	//Slot 3 Search
	SelectSlot = *((unsigned char *)0xfcc4) | 0x03;		//EXPTBL (SLOT3)
	findEseSCC();

	if (eseSlot == 0) {								// Not find eseSCC
		printf("NOT find\n");
		printf("Bye...\n");
		return -1;
	}else{
		printf("Find!\n");
		printf("Slot: %02x",eseSlot);
	}

	//Bank set
	SelectSlot = eseSlot;
	eseBank = 0;
	printf("\n\nErase Data ...");
	for (dat=0;dat<0x80;dat++){
		resetEseSCC(dat);
	}
	printf("\nCheck Data Writing...");
	for (dat=0;dat<0x80;dat++){
		if (writeEseSCC(dat)!=0) break;
		if (dat%16 == 0) printf("\nBank%X",(dat>>4));
		printf("%X",0x0f & dat);
	}
	maxBank = dat;

	printf("\n\nVerify(0x4000-5FFF)...");
	for (dat=0;dat<0x80;dat++){
		add = readEseSCC_B4000(dat);
		if (add!=0) {
			printf("\nFail!(BANK%02x:0x%04x %02x)\n",dat,add,faildata);
			return -1;
		}
		if (dat%16 == 0) printf("\nBank%X",(dat>>4));
		printf("%X",0x0f & dat);
	}

	printf("\n\nVerify(0x6000-7FFF)...");
	for (dat=0;dat<0x80;dat++){
		add = readEseSCC_B6000(dat);

		if (add!=0) {
			printf("\nFail!(BANK%02x:0x%04x %02x)\n",dat,add,faildata);
			return -1;
		}
		if (dat%16 == 0) printf("\nBank%X",(dat>>4));
		printf("%X",0x0f & dat);
	}

	printf("\n\nVerify(0x8000-9FFF)...");
	for (dat=0;dat<0x80;dat++){
		if ((dat == 0x3f)||(dat == 0x7f)) {
			add = readEseSCC_B9800(dat);
		}else{
			add = readEseSCC_B8000(dat);
		}
		if (add!=0) {
			printf("\nFail!(BANK%02x:0x%04x %02x)\n",dat,add,faildata);
			return -1;
		}
		if (dat%16 == 0) printf("\nBank%X",(dat>>4));
		printf("%X",0x0f & dat);
	}

	printf("\n\nVerify(0xA000-BFFF)...");
	for (dat=0;dat<0x80;dat++){
		add = readEseSCC_BA000(dat);
		if (add!=0) {
			printf("\nFail!(BANK%02x:0x%04x %02x)\n",dat,add,faildata);
			return -1;
		}
		if (dat%16 == 0) printf("\nBank%X",(dat>>4));
		printf("%X",0x0f & dat);
	}

	printf("\n\nDone. Thank you using!\n");
	return 0;

}
