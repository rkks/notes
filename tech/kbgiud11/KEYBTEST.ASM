;-----------------------------------------------------------------------------
;			    Keyboard test program
;-----------------------------------------------------------------------------
comment ^

I wrote this to check out some facts in the keyboard guide. I think it's
pretty self-explanatory. It works only with Tasm, I suppose, but converting
shouldn't be that hard.

Copyright (c) Wout Mertens 1994. Read the legal information in the accompany-
ing text. (kbguide.txt)

^
;-----------------------------------------------------------------------------
;				    Coding
;-----------------------------------------------------------------------------

.286
jumps
locals
dosseg

.model small

;-----------------------------------------------------------------------------
;				Macros/equs
;-----------------------------------------------------------------------------

WaitKBFlag	macro Mask
		local Wait
Wait:
	in	al, 64h
	and	al, Mask
	jnz	Wait
endm

GotKey	macro
	in	al, 61h
	or	al, 80h
	out	61h, al
	and	al, 7fh
	out	61h, al
endm

WaitForKey macro Key
	local Here

Here:
ifnb <Key>
	cmp	Yo, Key
	jne	Here
else
	cmp	Yo, 0
	je	Here
	cmp	Yo, ACK
	je	Here
endif
	mov	Yo, 0
endm

Send	macro b

	mov	al, b
	out	60h, al
	WaitKBFlag 2

endm

ACK		equ 0fah

;-----------------------------------------------------------------------------
;				Stack segment
;-----------------------------------------------------------------------------

.stack

;-----------------------------------------------------------------------------
;				 Data segment
;-----------------------------------------------------------------------------

.data

Yo	db	0			;Current key
Place	dw	0			;See int09

;-----------------------------------------------------------------------------
;				 Code segment
;-----------------------------------------------------------------------------

.code

Program proc

;* Begin
Start:					;Major dirty code ahead!
; * Init regs
	startupcode			;Set ds, ss etc.

	mov	ax, 3509h		;Save old int 9 vector
	int	21h
	push	es
	push	bx

	push	ds			;Set the new one
	mov	ax, seg int09
	mov	ds, ax
	mov	dx, offset int09
	mov	ax, 2509h
	int	21h
	pop	ds

	Send 0f0h			;Set scanset 1
	Send 1
	Send 0f0h			;Read out scanset no.
	Send 0
	WaitForKey			;Wait for non-ACK results

	Send 0f0h			;Ditto, for 2
	Send 2
	Send 0f0h
	Send 0
	WaitForKey

	Send 0f0h			;Ditto, for 3
	Send 3
	Send 0f0h
	Send 0
	WaitForKey

	Send 0f0h			;Set old no 2 back
	Send 2
	WaitForKey 1			;Wait for Escape

	Send 0edh			;Set all LEDs
	Send 7
	WaitForKey 1			;Wait for Escape

	pop	dx			;Get int 9 back
	pop	ds
	mov	ax, 2509h
	int	21h

;* Einde
Einde:
	exitcode

Program endp

int09	proc	far
	pusha
	push	ds
	push	es

	mov	ax, 0b800h		;es->videoseg
	mov	es, ax
	mov	ax, @Data		;ds->data
	mov	ds, ax
	mov	bx, Place		;Where to put the next key
	mov	ah, 70h
	in	al, 60h
	mov	[es:bx], ax

	mov	Yo, al

Go:	GotKey				;Thank keyboard

	add	bx, 2			;Set next place
	mov	Place, bx

	mov	al, 20h 		;End-Of-Interrupt
	out	20h, al

	pop	es
	pop	ds
	popa
	iret

endp

end     Program