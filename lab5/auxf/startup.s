%macro print 2
        pushad
        mov     eax,4
        mov     ebx,1
        mov     ecx,%1
        mov     edx,%2
        INT     0x80
        popad
%endmacro

section .rodata
str1:   db "(:^..^:) starting up the code",10,0
        
section .text
global startup:function startup_end-startup ;  int startup(int argc,char** argv,int(*func)(int,char**))
startup:
        push	ebp             ; Save caller state
	mov	ebp, esp
        sub	esp, 4          ; Leave space for local var on stack
        pushad                  ; Save some more caller state
        mov     ecx,[ebp+8]     ; Gets argc from C
        print str1,30
	mov	ebx, [ebp+12]   ; Gets argv (char**) from C
        mov     eax,ecx
        dec     eax
        shl     eax,2
        
loop1:                          ; Push evry char* of argv for _start
                mov     edx,ebx
                add     edx,eax                
                push    dword[edx]
                sub     eax,4
                loop    loop1,ecx

        mov     ecx,[ebp+8]     ; Gets argc from C
        push    ecx             ; Push argc for _start
        mov     ebx,[ebp+16]    ; Gets the function addres from C
        jmp     ebx             ; Jumps the begining of this fonction
        mov	[ebp-4], eax    ; Save returned value...
	popad			; Restore caller state (registers)
	mov	eax, [ebp-4]    ; place returned value where caller can see it
	add	esp, 4		; Cleans the stuck
	pop	ebp		; Restore caller state
	ret                     ; Back to caller
startup_end

