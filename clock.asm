format PE GUI 4.0
entry start

include 'win32a.inc'

IDT_TIMER = 1

section '.text' code readable executable

start:
    invoke  GetModuleHandle, 0
    mov     [hInstance], eax

    ; Rejestracja klasy okna
    mov     [wc.style], 0
    mov     [wc.lpfnWndProc], wndproc
    mov     eax, [hInstance]
    mov     [wc.hInstance], eax
    mov     [wc.hCursor], 0
    mov     [wc.hbrBackground], 0
    mov     [wc.lpszClassName], class_name
    invoke  RegisterClass, wc

    ; Tworzenie okna
    invoke  CreateWindowEx, WS_EX_LAYERED + WS_EX_TOOLWINDOW, class_name, 0, WS_POPUP, 100,100,200,100,0,0,[hInstance],0
    mov     [hWnd], eax

    ; Przezroczystoœæ i wierzch
    invoke  SetLayeredWindowAttributes, [hWnd], 0FF00FFh, 0, LWA_COLORKEY
    invoke  ShowWindow, [hWnd], SW_SHOW
    invoke  UpdateWindow, [hWnd]
    invoke  SetWindowPos, [hWnd], -1, 0, 0, 0, 0, SWP_NOMOVE + SWP_NOSIZE + SWP_NOACTIVATE

    ; Timer 1000 ms
    invoke  SetTimer, [hWnd], IDT_TIMER, 1000, 0

msg_loop:
    invoke  GetMessage, msg, 0, 0, 0
    or      eax, eax
    jz      end_loop
    invoke  TranslateMessage, msg
    invoke  DispatchMessage, msg
    jmp     msg_loop

end_loop:
    invoke  KillTimer, [hWnd], IDT_TIMER
    invoke  ExitProcess, 0

; ----------------------------------------

proc wndproc hwnd, msg, wparam, lparam
    cmp     [msg], WM_DESTROY
    je      .wm_destroy

    cmp     [msg], WM_PAINT
    je      .wm_paint

    cmp     [msg], WM_TIMER
    je      .wm_timer

    invoke  DefWindowProc, [hwnd], [msg], [wparam], [lparam]
    ret

  .wm_destroy:
    invoke  PostQuitMessage, 0
    ret

  .wm_paint:
    invoke  BeginPaint, [hwnd], ps
    mov     ebx, eax

    invoke  GetClientRect, [hwnd], rect
    invoke  CreateSolidBrush, 0FF00FFh
    mov     esi, eax
    invoke  FillRect, ebx, rect, esi
    invoke  DeleteObject, esi

    invoke  GetLocalTime, systime

    lea edi, [time_buf]
    movzx   eax, word [systime.wHour]
    call write_two_digits

    mov word [time_buf+4], ':'

    lea edi, [time_buf+6]
    movzx   eax, word [systime.wMinute]
    call write_two_digits

    mov word [time_buf+10], ':'

    lea edi, [time_buf+12]
    movzx   eax, word [systime.wSecond]
    call write_two_digits

    mov word [time_buf+16], 0

    invoke  CreateFont, 36,0,0,0,700,0,0,0,0,0,0,5,0, font_name
    invoke  SelectObject, ebx, eax
    invoke  SetBkMode, ebx, TRANSPARENT
    invoke  SetTextColor, ebx, 000000h
    invoke  DrawTextW, ebx, time_buf, -1, rect, DT_CENTER + DT_VCENTER + DT_SINGLELINE
    invoke  EndPaint, [hwnd], ps
    ret

  .wm_timer:
    invoke InvalidateRect, [hwnd], 0, TRUE
    ret
endp

proc write_two_digits
    xor edx, edx
    mov ecx, 10
    div ecx             ; eax / 10 › eax = dziesi¹tki, edx = jednosci
    add al, '0'         ; AL = dziesi¹tki + '0'
    mov [edi], ax       ; pierwszy znak UTF-16
    add dl, '0'         ; DL = jednosci + '0'
    mov [edi+2], dx     ; drugi znak UTF-16
    ret
endp

section '.data' data readable writeable

wc         WNDCLASS 0
msg        MSG
ps         PAINTSTRUCT
rect       RECT
systime    SYSTEMTIME

class_name db 'ClockClass',0
font_name  db 'Segoe UI',0
time_buf   dw 20 dup(0)
hInstance  dd 0
hWnd       dd 0

section '.idata' import data readable writeable

  library kernel32, 'KERNEL32.DLL',\
          user32,   'USER32.DLL',\
          gdi32,    'GDI32.DLL'

  import kernel32,\
         GetModuleHandle, 'GetModuleHandleA',\
         GetLocalTime,    'GetLocalTime',\
         ExitProcess,     'ExitProcess'

  import user32,\
         RegisterClass,   'RegisterClassA',\
         CreateWindowEx,  'CreateWindowExA',\
         DefWindowProc,   'DefWindowProcA',\
         ShowWindow,      'ShowWindow',\
         UpdateWindow,    'UpdateWindow',\
         GetMessage,      'GetMessageA',\
         TranslateMessage,'TranslateMessage',\
         DispatchMessage, 'DispatchMessageA',\
         PostQuitMessage, 'PostQuitMessage',\
         BeginPaint,      'BeginPaint',\
         EndPaint,        'EndPaint',\
         GetClientRect,   'GetClientRect',\
         DrawTextW,       'DrawTextW',\
         SetLayeredWindowAttributes, 'SetLayeredWindowAttributes',\
         SetWindowPos,    'SetWindowPos',\
         FillRect,        'FillRect',\
         InvalidateRect,  'InvalidateRect',\
         SetTimer,        'SetTimer',\
         KillTimer,       'KillTimer'

  import gdi32,\
         CreateSolidBrush,'CreateSolidBrush',\
         CreateFont,      'CreateFontA',\
         SelectObject,    'SelectObject',\
         SetTextColor,    'SetTextColor',\
         DeleteObject,    'DeleteObject',\
         SetBkMode,       'SetBkMode'
