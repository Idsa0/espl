section .text
global _sum

_sum:
    push ebp 			; Save caller state
    mov ebp, esp
    sub esp, 4 			; Leave space for local var on stack
    pushad 				; Save some more caller state
    mov eax, [ebp+8]	; Copy function args to registers: leftmost...
    mov ebx, [ebp+12] 	; Next argument...
    add eax,ebx			; sum 2 arguments
    mov [ebp-4], eax 	; Save returned value...
    popad 				; Restore caller state (registers)
    mov eax, [ebp-4] 	; place returned value where caller can see it
    add esp, 4 			; Restore caller state
    pop ebp 			; Restore caller state
    ret
