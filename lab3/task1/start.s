%define stdin           0
%define stdout          1
%define stderr          2

%define syscall_exit    1
%define syscall_read    3
%define syscall_write   4
%define syscall_open    5
%define syscall_close   6

%define O_RDONLY        0x0
%define O_WRONLY        0x1
%define O_CREAT         0x40
%define O_TRUNC         0x200


section .data
    flag_input db "-i", 0
    flag_output db "-o", 0
    newline db 0xA, 0
    file_open_error db "Error opening file", 0
    length_file_open_error equ $-file_open_error
    char_buffer db 0

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


main:
    push ebp
    mov ebp, esp

    mov dword [infile], 0   ; stdin
    mov dword [outfile], 1  ; stdout

    ; loop over arguments and check for flags
    mov esi, [ebp+12]       ; argv
    add esi, 4              ; skip first argument
    mov ecx, 1              ; counter

check_flags:
    cmp ecx, [ebp+8]    ; check if we reached the end of arguments
    jge end_check_flags

    ; print current arg to stderr
    pushad
    push dword [esi]
    call strlen
    add esp, 4
    mov byte [char_buffer], al
    popad
    pushad
    xor eax, eax
    mov al, byte [char_buffer]
    push eax
    push dword [esi]
    call printerr
    add esp, 8
    popad

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

main_end:
    ; close the input and output files
    mov ebx, [infile]
    cmp ebx, 0
    je skip_close_infile
    mov eax, syscall_close
    pushad
    int 0x80
    popad

skip_close_infile:
    mov ebx, [outfile]
    cmp ebx, 1
    je skip_close_outfile
    mov eax, syscall_close
    pushad
    int 0x80
    popad

skip_close_outfile:
    mov eax, 0

    mov esp, ebp
    pop ebp
    ret


encode:
    push ebp
    mov ebp, esp

    ; read a character from input file
    pushad
    mov edx, 1
    mov ecx, char_buffer
    mov ebx, dword [infile]
    mov eax, syscall_read
    int 0x80

    cmp eax, 0
    popad
    jle encode_end

    cmp byte [char_buffer], 0
    je encode_end

    ; check if the character is in the range 'A' to 'z'
    cmp byte [char_buffer], 'A'
    jl encode_write
    cmp byte [char_buffer], 'z'
    jg encode_write
    cmp byte [char_buffer], 'z'
    je encode_z

    ; increment the character by 1
    inc byte [char_buffer]

encode_write:
    ; write the character to the output file
    pushad
    mov edx, 1
    mov ecx, char_buffer
    mov ebx, dword [outfile]
    mov eax, syscall_write
    int 0x80
    popad

    jmp encode

encode_z:
    mov byte [char_buffer], 'A'
    jmp encode_write

encode_end:
    ; mov esp, ebp
    ; pop ebp
    ; ret

    ; exit here because i feel like it B^)
    mov ebx, 0
    mov eax, syscall_exit
    int 0x80


printerr:
    push ebp
    mov ebp, esp

    push dword [ebp+12]         ; message length
    push dword [ebp+8]          ; message
    push stderr
    push syscall_write
    call system_call
    add esp, 16

    push 1
    push newline
    push stderr
    push syscall_write
    call system_call
    add esp, 16

    mov esp, ebp
    pop ebp
    ret


open_in:
    push ebp
    mov ebp, esp

    mov ecx, O_RDONLY
    mov ebx, dword [ebp+8]       ; filename
    mov eax, syscall_open
    pushad
    int 0x80
    cmp eax, 0                   ; check if file was opened
    jl open_in_fail

    mov dword [infile], eax      ; save file descriptor
    popad
    jmp open_in_end

open_in_fail:
    push length_file_open_error
    push file_open_error
    call printerr
    add esp, 8
    popad

open_in_end:
    mov esp, ebp
    pop ebp
    ret


open_out:
    push ebp
    mov ebp, esp

    mov edx, 0777o               ; file permissions
    mov ecx, O_WRONLY | O_CREAT | O_TRUNC
    mov ebx, dword [ebp+8]       ; filename
    mov eax, syscall_open
    pushad
    int 0x80

    cmp eax, 0                   ; check if file was opened
    jl open_out_fail

    mov dword [outfile], eax     ; save file descriptor
    popad
    jmp open_out_end

open_out_fail:
    push length_file_open_error
    push file_open_error
    call printerr
    add esp, 8
    popad

open_out_end:
    mov esp, ebp
    pop ebp
    ret
