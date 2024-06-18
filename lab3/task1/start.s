section .data
    flag_input db "-i", 0
    flag_output db "-o", 0
    newline db 0xA, 0
    file_open_error db "Error opening file", 0
    length_file_open_error equ $-file_open_error
    argc_error db "Not enough arguments", 0
    length_argc_error equ $-argc_error

section .bss
    infile resd 1
    outfile resd 1

section .text

extern strncmp
extern strlen

global _start
global system_call

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
    mov     eax,1
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


main:
    push ebp
    mov ebp, esp

    mov dword [infile], 0   ; stdin
    mov dword [outfile], 1  ; stdout

    ; check if there are enough arguments
    cmp dword [ebp+8], 2
    jl not_enough_args

    ; loop over arguments and check for flags
    mov esi, [ebp+12]       ; argv
    add esi, 4              ; skip first argument
    mov ecx, 1              ; counter

check_flags:
    cmp ecx, [ebp+8]    ; check if we reached the end of arguments
    jge end_check_flags

    ; print current arg to perror
    push dword [esi]
    call strlen
    add esp, 4
    push eax
    push dword [esi]
    call perror
    add esp, 8

    ; check if the argument is a flag
    pushad
    push 2 ; number of bytes to check
    push dword [esi]
    push flag_input
    call strncmp
    add esp, 12
    cmp eax, 0
    popad
    je input_flag

    pushad
    push 2 ; number of bytes to check
    push dword [esi]
    push flag_output
    call strncmp
    add esp, 12
    cmp eax, 0
    popad
    je output_flag

    ; if the argument is not a flag, move to the next one
    add esi, 4
    inc ecx
    jmp check_flags

input_flag:
    ; check if there is a file name after the flag
    pushad
    push dword [esi]
    call strlen
    add esp, 4
    cmp eax, 2
    popad
    jle bad_file

    ; open the input file
    mov edi, [esi]
    lea edi, [edi+2]
    pushad
    push edi
    call open_in
    add esp, 4
    popad

    ; move to the next argument
    add esi, 4
    inc ecx
    jmp check_flags

output_flag:
    ; check if there is a file name after the flag
    pushad
    push dword [esi]
    call strlen
    add esp, 4
    cmp eax, 2
    popad
    jle bad_file

    ; open the output file
    mov edi, [esi]
    lea edi, [edi+2]
    pushad
    push edi
    call open_out
    add esp, 4
    popad

    ; move to the next argument
    add esi, 4
    inc ecx
    jmp check_flags

end_check_flags:
    call encode

    jmp main_end

bad_file:
    push length_file_open_error
    push file_open_error
    call perror
    jmp main_end

not_enough_args:
    push length_argc_error
    push argc_error
    call perror
    jmp main_end

main_end:
    push dword [infile]
    call close
    push dword [outfile]
    call close

    mov eax, 0

    mov esp, ebp
    pop ebp
    ret


encode:
    ; TODO - everything written here is just a draft
    push ebp
    mov ebp, esp

    ; how do i save the encoding key?

    ; read from input file
    sub esp, 1024
    push 1024
    push esp
    call read_next
    cmp eax, 0
    je encode_end

    ; write to output file
    push eax
    push esp
    call write

    jmp encode

encode_end:
    mov esp, ebp
    pop ebp
    ret


read_next:
    push ebp
    mov ebp, esp

    push dword [ebp+12]         ; buffer size
    push dword [ebp+8]          ; buffer
    push dword [infile]         ; file descriptor
    push 3                      ; read syscall
    call system_call

    mov esp, ebp
    pop ebp
    ret


write:
    push ebp
    mov ebp, esp

    push dword [ebp+12]         ; message length
    push dword [ebp+8]          ; message
    push dword [outfile]        ; file descriptor
    push 4                      ; write syscall
    call system_call
    
    mov esp, ebp
    pop ebp
    ret


perror:
    push ebp
    mov ebp, esp

    push dword [ebp+12]         ; message length
    push dword [ebp+8]          ; message
    push 2                      ; stderr
    push 4                      ; write syscall
    call system_call

    push 1
    push newline
    push 2
    push 4
    call system_call

    mov esp, ebp
    pop ebp
    ret


open_in:
    push ebp
    mov ebp, esp

    pushad
    push 0                       ; O_RDONLY
    push dword [ebp+8]           ; filename
    push 5                       ; open syscall
    call system_call
    add esp, 12
    popad

    cmp eax, 0                   ; check if file was opened
    jl open_in_fail

    mov dword [infile], eax      ; save file descriptor
    jmp open_in_end

open_in_fail:
    pushad
    push length_file_open_error
    push file_open_error
    call perror
    add esp, 8
    popad

open_in_end:
    mov esp, ebp
    pop ebp
    ret


open_out:
    push ebp
    mov ebp, esp

    pushad
    push 0777
    push 577                     ; O_WRONLY | O_CREAT | O_TRUNC
    push dword [ebp+8]           ; filename
    push 5                       ; open syscall
    call system_call
    add esp, 16
    popad

    cmp eax, 0                   ; check if file was opened
    jl open_out_fail

    mov dword [outfile], eax     ; save file descriptor
    jmp open_out_end

open_out_fail:
    pushad
    push length_file_open_error
    push file_open_error
    call perror
    add esp, 8
    popad

open_out_end:
    mov esp, ebp
    pop ebp
    ret


close:
    push ebp
    mov ebp, esp

    pushad
    push 0
    push 0
    push dword [ebp+8]      ; file pointer
    push 6                  ; close syscall
    call system_call
    add esp, 16
    popad

    mov esp, ebp
    pop ebp
    ret
