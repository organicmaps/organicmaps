;
; This file is a part of TTMath Bignum Library
; and is distributed under the (new) BSD licence.
; Author: Christian Kaiser <chk@online.de>
;

; 
; Copyright (c) 2009, Christian Kaiser
; All rights reserved.
; 
; Redistribution and use in source and binary forms, with or without
; modification, are permitted provided that the following conditions are met:
; 
;  * Redistributions of source code must retain the above copyright notice,
;    this list of conditions and the following disclaimer.
;    
;  * Redistributions in binary form must reproduce the above copyright
;    notice, this list of conditions and the following disclaimer in the
;    documentation and/or other materials provided with the distribution.
;    
;  * Neither the name Christian Kaiser nor the names of contributors to this
;    project may be used to endorse or promote products derived
;    from this software without specific prior written permission.
;
; THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
; AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
; IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
; ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
; LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
; CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
; SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
; INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
; CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
; ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
; THE POSSIBILITY OF SUCH DAMAGE.
;

;
; compile with debug info:    ml64.exe /c /Zd /Zi ttmathuint_x86_64_msvc.asm
; compile without debug info: ml64.exe /c ttmathuint_x86_64_msvc.asm
; this creates ttmathuint_x86_64_msvc.obj file which can be linked with your program
;

PUBLIC	ttmath_adc_x64
PUBLIC	ttmath_addindexed_x64
PUBLIC	ttmath_addindexed2_x64
PUBLIC	ttmath_addvector_x64

PUBLIC	ttmath_sbb_x64
PUBLIC	ttmath_subindexed_x64
PUBLIC	ttmath_subvector_x64

PUBLIC	ttmath_rcl_x64
PUBLIC	ttmath_rcr_x64

PUBLIC	ttmath_rcl2_x64
PUBLIC	ttmath_rcr2_x64

PUBLIC	ttmath_div_x64

;
; Microsoft x86_64 convention: http://msdn.microsoft.com/en-us/library/9b372w95.aspx
;
;	"rax, rcx, rdx, r8-r11 are volatile."
;	"rbx, rbp, rdi, rsi, r12-r15 are nonvolatile."
;


.CODE


        ALIGN       8

;----------------------------------------

ttmath_adc_x64				PROC
        ; rcx = p1
        ; rdx = p2
        ; r8 = nSize
        ; r9 = nCarry

        xor		rax, rax
        xor		r11, r11
        sub		rax, r9		; sets CARRY if r9 != 0

		ALIGN 16
 loop1:
		mov		rax,qword ptr [rdx + r11 * 8]
		adc		qword ptr [rcx + r11 * 8], rax
		lea		r11, [r11+1]
		dec		r8
		jnz		loop1

		setc	al
		movzx	rax, al

		ret

ttmath_adc_x64				ENDP

;----------------------------------------

        ALIGN       8

;----------------------------------------

ttmath_addindexed_x64	PROC

        ; rcx = p1
        ; rdx = nSize
        ; r8 = nPos
        ; r9 = nValue

		xor		rax, rax			; rax = result
		sub		rdx, r8				; rdx = remaining count of uints

		add		qword ptr [rcx + r8 * 8], r9
		jc		next1

		ret

next1:
		mov		r9, 1

		ALIGN 16
loop1:
		dec		rdx
		jz		done_with_cy
		lea		r8, [r8+1]
		add		qword ptr [rcx + r8 * 8], r9
		jc		loop1

		ret

done_with_cy:
		lea		rax, [rax+1]		; rax = 1

		ret

ttmath_addindexed_x64	ENDP

;----------------------------------------

        ALIGN       8

;----------------------------------------

ttmath_addindexed2_x64	PROC

        ; rcx = p1 (pointer)
        ; rdx = b  (value size)
        ; r8 = nPos
        ; r9 = nValue1
        ; [esp+0x28] = nValue2

		xor		rax, rax			; return value
		mov		r11, rcx			; table
		sub		rdx, r8				; rdx = remaining count of uints
		mov		r10, [esp+028h]		; r10 = nValue2

		add		qword ptr [r11 + r8 * 8], r9
		lea		r8, [r8+1]
		lea		rdx, [rdx-1]
		adc		qword ptr [r11 + r8 * 8], r10
		jc		next
		ret

		ALIGN 16
loop1:
		lea		r8, [r8+1]
		add		qword ptr [r11 + r8 * 8], 1
		jc		next
		ret

next:
		dec		rdx					; does not modify CY too...
		jnz		loop1
		lea		rax, [rax+1]
		ret

ttmath_addindexed2_x64	ENDP



;----------------------------------------

        ALIGN       8

;----------------------------------------


ttmath_addvector_x64				PROC
        ; rcx = ss1
        ; rdx = ss2
        ; r8 = ss1_size
        ; r9 = ss2_size
        ; [esp+0x28] = result

		mov		r10, [esp+028h]
		sub		r8, r9
        xor		r11, r11				; r11=0, cf=0

		ALIGN 16
 loop1:
		mov		rax, qword ptr [rcx + r11 * 8]
		adc		rax, qword ptr [rdx + r11 * 8]
		mov		qword ptr [r10 + r11 * 8], rax
		inc		r11
		dec		r9
		jnz		loop1

		adc		r9, r9					; r9 has the cf state

		or		r8, r8
		jz		done

		neg		r9						; setting cf from r9
		mov		r9, 0					; don't use xor here (cf is used)
 loop2:
		mov		rax, qword ptr [rcx + r11 * 8]
		adc		rax, r9
		mov		qword ptr [r10 + r11 * 8], rax
		inc		r11
		dec		r8
		jnz		loop2

		adc		r8, r8
		mov		rax, r8
		
		ret

done:
		mov		rax, r9
		ret

ttmath_addvector_x64				ENDP


;----------------------------------------

        ALIGN       8

;----------------------------------------

ttmath_sbb_x64				PROC

        ; rcx = p1
        ; rdx = p2
        ; r8 = nCount
        ; r9 = nCarry

        xor		rax, rax
        xor		r11, r11
        sub		rax, r9				; sets CARRY if r9 != 0

		ALIGN 16
 loop1:
		mov		rax,qword ptr [rdx + r11 * 8]
		sbb		qword ptr [rcx + r11 * 8], rax
		lea		r11, [r11+1]
		dec		r8
		jnz		loop1

		setc	al
		movzx	rax, al

		ret

ttmath_sbb_x64				ENDP

;----------------------------------------

        ALIGN       8

;----------------------------------------

ttmath_subindexed_x64	PROC
        ; rcx = p1
        ; rdx = nSize
        ; r8 = nPos
        ; r9 = nValue

		sub		rdx, r8				; rdx = remaining count of uints

		ALIGN 16
loop1:
		sub		qword ptr [rcx + r8 * 8], r9
		jnc		done

		lea		r8, [r8+1]
		mov		r9, 1
		dec		rdx
		jnz		loop1

		mov		rax, 1
		ret

done:
		xor		rax, rax
		ret

ttmath_subindexed_x64	ENDP



;----------------------------------------

        ALIGN       8

;----------------------------------------

;	the same asm code as in addvector_x64 only two instructions 'adc' changed to 'sbb'

ttmath_subvector_x64				PROC
        ; rcx = ss1
        ; rdx = ss2
        ; r8 = ss1_size
        ; r9 = ss2_size
        ; [esp+0x28] = result

		mov		r10, [esp+028h]
		sub		r8, r9
        xor		r11, r11				; r11=0, cf=0

		ALIGN 16
 loop1:
		mov		rax, qword ptr [rcx + r11 * 8]
		sbb		rax, qword ptr [rdx + r11 * 8]
		mov		qword ptr [r10 + r11 * 8], rax
		inc		r11
		dec		r9
		jnz		loop1

		adc		r9, r9					; r9 has the cf state

		or		r8, r8
		jz		done

		neg		r9						; setting cf from r9
		mov		r9, 0					; don't use xor here (cf is used)
 loop2:
		mov		rax, qword ptr [rcx + r11 * 8]
		sbb		rax, r9
		mov		qword ptr [r10 + r11 * 8], rax
		inc		r11
		dec		r8
		jnz		loop2

		adc		r8, r8
		mov		rax, r8
		
		ret

done:
		mov		rax, r9
		ret

ttmath_subvector_x64				ENDP




;----------------------------------------

        ALIGN       8

;----------------------------------------

ttmath_rcl_x64	PROC
        ; rcx = p1
        ; rdx = b
        ; r8 = nLowestBit

		mov		r11, rcx			; table
		xor		r10, r10
		neg		r8					; CY set if r8 <> 0

		ALIGN 16
loop1:
		rcl		qword ptr [r11 + r10 * 8], 1
		lea		r10, [r10+1]
		dec		rdx
		jnz		loop1

		setc	al
		movzx	rax, al

        ret

ttmath_rcl_x64	ENDP

;----------------------------------------

        ALIGN       8

;----------------------------------------

ttmath_rcr_x64	PROC
        ; rcx = p1
        ; rdx = nSize
        ; r8 = nLowestBit

		xor		r10, r10
		neg		r8					; CY set if r8 <> 0

		ALIGN 16
loop1:
		rcr		qword ptr -8[rcx + rdx * 8], 1
		dec		rdx
		jnz		loop1

		setc	al
		movzx	rax, al

        ret

ttmath_rcr_x64	ENDP

;----------------------------------------

        ALIGN       8

;----------------------------------------

ttmath_div_x64	PROC

        ; rcx = &Hi
        ; rdx = &Lo
        ; r8 = nDiv

        mov		r11, rcx
        mov		r10, rdx

        mov		rdx, qword ptr [r11]
        mov		rax, qword ptr [r10]
        div		r8
        mov		qword ptr [r10], rdx ; remainder
        mov		qword ptr [r11], rax ; value

        ret

ttmath_div_x64	ENDP

;----------------------------------------

        ALIGN       8

;----------------------------------------

ttmath_rcl2_x64	PROC
        ; rcx = p1
        ; rdx = nSize
        ; r8 = bits
        ; r9 = c

        push	rbx

        mov		r10, rcx	; r10 = p1
        xor		rax, rax

        mov		rcx, 64
        sub		rcx, r8

        mov		r11, -1
        shr		r11, cl		; r11 = mask

		mov		rcx, r8		; rcx = count of bits

		mov		rbx, rax	; rbx = old value = 0
		or		r9, r9
		cmovnz	rbx, r11	; if (c) then old value = mask

        mov		r9, rax		; r9 = index (0..nSize-1)

		ALIGN 16
loop1:
		rol		qword ptr [r10+r9*8], cl
		mov		rax, qword ptr [r10+r9*8]
		and		rax, r11
		xor		qword ptr [r10+r9*8], rax
		or		qword ptr [r10+r9*8], rbx
		mov		rbx, rax

		lea		r9, [r9+1]
		dec		rdx

		jnz		loop1

		and		rax, 1
		pop		rbx
        ret

ttmath_rcl2_x64	ENDP

;----------------------------------------

        ALIGN       8

;----------------------------------------

ttmath_rcr2_x64	PROC
        ; rcx = p1
        ; rdx = nSize
        ; r8 = bits
        ; r9 = c

        push	rbx
        mov		r10, rcx	; r10 = p1
        xor		rax, rax

        mov		rcx, 64
        sub		rcx, r8

        mov		r11, -1
        shl		r11, cl		; r11 = mask

		mov		rcx, r8		; rcx = count of bits

		mov		rbx, rax	; rbx = old value = 0
		or		r9, r9
		cmovnz	rbx, r11	; if (c) then old value = mask

        mov		r9, rdx		; r9 = index (0..nSize-1)
		lea		r9, [r9-1]

		ALIGN 16
loop1:
		ror		qword ptr [r10+r9*8], cl
		mov		rax, qword ptr [r10+r9*8]
		and		rax, r11
		xor		qword ptr [r10+r9*8], rax
		or		qword ptr [r10+r9*8], rbx
		mov		rbx, rax

		lea		r9, [r9-1]
		dec		rdx

		jnz		loop1

		rol		rax, 1
		and		rax, 1
		pop		rbx

        ret

ttmath_rcr2_x64	ENDP

END
