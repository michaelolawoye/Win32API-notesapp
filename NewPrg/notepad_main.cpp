#pragma warning(disable:4996)
#include <windows.h>
#include <stdio.h>
#include <math.h>
#include <limits.h>

#include "resource.h"
#include "DEVCAPS.h"


#define BUF_SIZE 5
#define DELETE_CHAR INT_MIN


// Info on caret position
typedef struct Caret {
    int row;
    int col;
    int prev_col;
} Caret;

// info on text metrics
typedef struct textInfo {
    int maxLineWidth;
    TEXTMETRIC tm;
} TextInfo;

typedef struct gapBuffer {
    // pointer to start of entire gapbuffer
    TCHAR* txt_start;
    // pointer to first character of empty buffer
    TCHAR* buffer;
    // pointer to character right after end of empty buffer
    TCHAR* after_buffer;
    // size of entire struct
    int size;
    // index of empty buffer in struct
    int buffer_index;
} GapBuffer, *GapBufferPtr;


LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);
void addBufferChar(TCHAR, GapBuffer*, Caret*, TextInfo);
void deleteBufferChar(GapBuffer*, Caret*, TextInfo);
void adjustCaretPos(int change, Caret*, GapBuffer, TextInfo);
void printBuffer(HWND, HDC, GapBuffer, TextInfo);
int resizeBuffer(GapBuffer*);
void shiftBufferLeft(GapBuffer*, Caret*, TextInfo);
void shiftBufferRight(GapBuffer*, Caret*, TextInfo);
int savetoFile(GapBuffer, char* filename);
int readfromFile(GapBuffer*, char* filename);
void clearBuffer(GapBuffer*);

HWND hDlgModeless;

const TCHAR szClassName[] = TEXT("NotePadApp");


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow) {

    WNDCLASS wclass;
    HWND hwnd;
    MSG msg;

    //Step 1: Registering the Window Class
    wclass.style            = CS_HREDRAW | CS_VREDRAW;
    wclass.lpfnWndProc      = WndProc;
    wclass.cbClsExtra       = 0;
    wclass.cbWndExtra       = 0;
    wclass.hInstance        = hInstance;
    wclass.hIcon            = LoadIcon(NULL, IDI_APPLICATION);
    wclass.hCursor          = LoadCursor(NULL, IDC_ARROW);
    wclass.hbrBackground    = CreateSolidBrush(RGB(200, 200, 200));
    wclass.lpszMenuName     = szClassName;
    wclass.lpszClassName    = szClassName;

    if (!RegisterClass(&wclass)) {
        MessageBox(NULL, TEXT("Window Registration Failed!"), TEXT("Error!"),
            MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    // Step 2: Creating the Window
    hwnd = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        szClassName,
        TEXT("Text editor"),
        WS_OVERLAPPEDWINDOW | WS_VSCROLL | WS_HSCROLL,
        CW_USEDEFAULT, CW_USEDEFAULT, 500, 360,
        NULL, NULL, hInstance, NULL);

    hDlgModeless = CreateDialog(hInstance, TEXT("DlgProc"), hwnd, DlgProc);

    if (hwnd == NULL) {
        MessageBox(NULL, TEXT("Window Creation Failed!"), TEXT("Error!"),
            MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    ShowWindow(hwnd, iCmdShow);
    UpdateWindow(hwnd);

    // Step 3: The Message Loop
    while (GetMessage(&msg, NULL, 0, 0) > 0) {

        if (hDlgModeless == 0 || !IsDialogMessage(hDlgModeless, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int)msg.wParam;
}

// Step 4: the Window Procedure
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    HDC hdc;
    PAINTSTRUCT ps;
    TEXTMETRIC tm;
    SCROLLINFO si;
    static int cxChar, cyChar, cxCaps;
    static int cxClient, cyClient, maxWidth;
    static int vScrollPos, hScrollPos;;
    static Caret caret;
    static TextInfo ti;
    static GapBuffer gapbuffer;

    HBRUSH greyHBrush = CreateSolidBrush(RGB(169, 169, 169));

    si.cbSize = sizeof(SCROLLINFO);

    switch (msg) {
    case WM_CREATE:
    {
        hdc = GetDC(hwnd);

        GetTextMetrics(hdc, &tm);

        cxChar = tm.tmAveCharWidth;
        cxCaps = (tm.tmPitchAndFamily & 1 ? 3 : 2) * cxChar / 2;
        cyChar = tm.tmHeight + tm.tmExternalLeading;

        ReleaseDC(hwnd, hdc);

        maxWidth = 40 * cxChar + 22 * cxCaps;

        caret.row = 0;
        caret.col = 0;

        // initializes empty gapbuffer
        gapbuffer.txt_start = (TCHAR*)malloc(BUF_SIZE*sizeof(TCHAR));
        if (!gapbuffer.txt_start) {
            free(gapbuffer.txt_start);
            perror("Gap buffer initial allocation failed");
            return -1;
        }
        gapbuffer.size = BUF_SIZE;
        gapbuffer.buffer = gapbuffer.txt_start;
        gapbuffer.buffer_index = 0;
        gapbuffer.after_buffer = gapbuffer.buffer + BUF_SIZE;
        

        ti.tm = tm;
        ti.tm.tmAveCharWidth += 1;

        return 0;
    }

    case WM_PAINT:
    {
        hdc = BeginPaint(hwnd, &ps);



        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_SIZE:
    {
        cxClient = LOWORD(lParam);
        cyClient = HIWORD(lParam);

        si.fMask = SIF_RANGE | SIF_PAGE;
        si.nMin = 0;
        si.nMax = NUMLINES2 - 1;
        si.nPage = cyClient / cyChar;
        SetScrollInfo(hwnd, SB_VERT, &si, TRUE);

        si.fMask = SIF_RANGE | SIF_PAGE;
        si.nMin = 0;
        si.nMax = 2 + maxWidth / cxChar;
        si.nPage = cxClient / cxChar;
        SetScrollInfo(hwnd, SB_HORZ, &si, TRUE);

        ti.maxLineWidth = cxClient;

        return 0;
    }

    case WM_COMMAND:
    {
        // Menus
        switch (LOWORD(wParam)) {
        case ID_FILE_NEW:
            clearBuffer(&gapbuffer);
            MessageBeep(0);
            break;

        case ID_FILE_OPEN:
        case ID_FILE_SAVE:
        case ID_FILE_SAVEAS:
            savetoFile(gapbuffer, (char*)"untitled.txt");
            break;
        }
    }

    case WM_VSCROLL:
    {
        si.fMask = SIF_ALL;
        GetScrollInfo(hwnd, SB_VERT, &si);

        vScrollPos = si.nPos;

        switch (LOWORD(wParam))
        {
        case SB_LINEUP:
            si.nPos--;
            break;
        case SB_LINEDOWN:
            si.nPos++;
            break;
        case SB_PAGEUP:
            si.nPos -= 5;
            break;
        case SB_PAGEDOWN:
            si.nPos += 5;
            break;
        case SB_THUMBPOSITION:
            break;
        case SB_THUMBTRACK:
            si.nPos = si.nTrackPos;
            break;
        case SB_TOP:
            si.nPos = si.nMin;
            break;
        case SB_BOTTOM:
            si.nPos = si.nMax;
            break;
        case SB_ENDSCROLL:
            break;
        default:
            break;
        }

        si.fMask = SIF_POS;

        SetScrollInfo(hwnd, SB_VERT, &si, TRUE);
        GetScrollInfo(hwnd, SB_VERT, &si);

        if (si.nPos != vScrollPos) {
            ScrollWindow(hwnd, 0, cyChar * (vScrollPos - si.nPos), NULL, NULL);
            UpdateWindow(hwnd);
        }

        return 0;
    }

    case WM_HSCROLL:
    {
        si.fMask = SIF_ALL;
        GetScrollInfo(hwnd, SB_HORZ, &si);

        hScrollPos = si.nPos;

        switch (LOWORD(wParam))
        {
        case SB_LINEUP:
            si.nPos--;
            break;
        case SB_LINEDOWN:
            si.nPos++;
            break;
        case SB_PAGEUP:
            si.nPos -= 5;
            break;
        case SB_PAGEDOWN:
            si.nPos += 5;
            break;
        case SB_THUMBPOSITION:
            si.nPos = HIWORD(wParam);
            break;
        case SB_THUMBTRACK:
            break;
        case SB_TOP:
            si.nPos = si.nMin;
            break;
        case SB_BOTTOM:
            si.nPos = si.nMax;
            break;
        case SB_ENDSCROLL:
            break;
        default:
            break;
        }

        si.fMask = SIF_POS;

        SetScrollInfo(hwnd, SB_HORZ, &si, TRUE);
        GetScrollInfo(hwnd, SB_HORZ, &si);

        if (si.nPos != hScrollPos) {
            ScrollWindow(hwnd, cxChar * (hScrollPos - si.nPos), 0, NULL, NULL);
            UpdateWindow(hwnd);
        }

        return 0;
    }

    case WM_KEYDOWN:
    {
        hdc = GetDC(hwnd);
        GetTextMetrics(hdc, &tm);

        long char_width, char_height;
        char_height = tm.tmHeight;
        char_width = tm.tmAveCharWidth;

        switch (wParam) {
        case VK_HOME:
            adjustCaretPos(-1, &caret, gapbuffer, ti);
            break;
        case VK_END:
            adjustCaretPos(1, &caret, gapbuffer, ti);
            break;
        case VK_PRIOR:
            SendMessage(hwnd, WM_VSCROLL, SB_PAGEUP, 0);
            break;
        case VK_NEXT:
            SendMessage(hwnd, WM_VSCROLL, SB_PAGEDOWN, 0);
            break;
        case VK_UP:
            SendMessage(hwnd, WM_VSCROLL, SB_LINEUP, 0);
            break;
        case VK_DOWN:
            SendMessage(hwnd, WM_VSCROLL, SB_LINEDOWN, 0);
            break;
        case VK_LEFT:
            shiftBufferLeft(&gapbuffer, &caret, ti);
            break;
        case VK_RIGHT:
            shiftBufferRight(&gapbuffer, &caret, ti);
            break;
        }
        break;
    }

    case WM_CHAR:
    {
        hdc = GetDC(hwnd);

        SelectObject(hdc, GetStockObject(SYSTEM_FIXED_FONT));
        SetBkMode(hdc, TRANSPARENT);
        HideCaret(hwnd);
        for (int i = 0; i < (int)LOWORD(lParam); i++) {

            switch (wParam) {
            case '\b':
                deleteBufferChar(&gapbuffer, &caret, ti);
                break;
                // TEMPORARY BLOCK
            case '0':
                savetoFile(gapbuffer, (char*)"notes.txt");
                break;
            case '#':
                readfromFile(&gapbuffer, (char*)"notes.txt");
                break;
                // END OF TEMPORARY BLOCK
            default:
                addBufferChar((TCHAR)wParam, &gapbuffer, &caret, ti);
                break;
            }
        }
        ShowCaret(hwnd);

        printBuffer(hwnd, hdc, gapbuffer, ti);

        ReleaseDC(hwnd, hdc);

        break;
    }

    case WM_MOUSEMOVE:
    {
        if (wParam & MK_SHIFT) {
            hdc = GetDC(hwnd);
            SetPixel(hdc, LOWORD(lParam), HIWORD(lParam), 0);

            ReleaseDC(hwnd, hdc);
        }
        break;
    }

    case WM_RBUTTONDOWN:
    {
        int x = LOWORD(lParam);
        int y = HIWORD(lParam);

        TCHAR buff[10];

        wsprintf(buff, TEXT("%d"), x);

        hdc = GetDC(hwnd);

        TextOut(hdc, x-20, y-20, buff, 3);

        ReleaseDC(hwnd, hdc);
        
        break;
    }

    case WM_SETFOCUS:
    {
        CreateCaret(hwnd, NULL, cxChar, cyChar);
        ShowCaret(hwnd);

        SetCaretPos(caret.col, caret.row);
        break;
    }

    case WM_KILLFOCUS:
    {
        HideCaret(hwnd);
        DestroyCaret();
        break;
    }

    case WM_CLOSE: {
        DestroyWindow(hwnd);
        break;
    }

    case WM_DESTROY: {
        PostQuitMessage(0);
        break;
    }
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

LRESULT CALLBACK DlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam) {

    switch (msg) {

    case WM_DESTROY:
    {
        DestroyWindow(hdlg);
        hDlgModeless = NULL;
        break;
    }
    }
    return 0;
}

void addBufferChar(TCHAR ch, GapBuffer* gapbuffer, Caret* caret, TextInfo ti) {

    if (gapbuffer->buffer >= gapbuffer->after_buffer) {

        resizeBuffer(gapbuffer);
    }

    gapbuffer->buffer[0] = ch;
    gapbuffer->buffer++;
    gapbuffer->buffer_index++;

    adjustCaretPos(1, caret, *gapbuffer, ti);
    
}

void deleteBufferChar(GapBuffer* gapbuffer, Caret* caret, TextInfo ti) {

    if (gapbuffer->buffer == gapbuffer->txt_start)
        return;
    gapbuffer->buffer = gapbuffer->txt_start + gapbuffer->buffer_index-1;
    gapbuffer->buffer_index--;
    
    // remove character from screen
    gapbuffer->buffer[0] = DELETE_CHAR;

    
    adjustCaretPos(-1, caret, *gapbuffer, ti);

}

// TODO
void adjustCaretPos(int change, Caret* caret, GapBuffer gb, TextInfo ti) {

    caret->col += change * ti.tm.tmAveCharWidth;

    if (caret->col > ti.maxLineWidth) {
        caret->col = 0;
        caret->row += ti.tm.tmHeight;
    }
    else if (caret->col < 0) {
        caret->col = ti.maxLineWidth - ti.tm.tmAveCharWidth;
        caret->row -= ti.tm.tmHeight;

        if (caret->row < 0) {
            caret->col = 0;
            caret->row = 0;
        }
    }
    
    SetCaretPos(caret->col, caret->row);
}

// prints contents of gapbuffer, skipping empty buffer
void printBuffer(HWND hwnd, HDC hdc, GapBuffer gapbuffer, TextInfo ti) {

    TCHAR* szBuffer;
    RECT rect;
    int x = 0, y = 0;

    // clears screen immediately, before printing characters
    InvalidateRect(hwnd, NULL, TRUE);
    UpdateWindow(hwnd);

    int max_char = ti.maxLineWidth / ti.tm.tmAveCharWidth;
    // prints each character in buffer
    for (int i = 0; i < gapbuffer.size; i++) {
        szBuffer = (TCHAR*)calloc(2, sizeof(TCHAR));

        if (!szBuffer) {
            free(szBuffer);
            perror("Character buffer memory allocation failed");
            continue;
        }
        // skip empty buffer after checking first buffer character
        if (i == gapbuffer.buffer_index && i != gapbuffer.after_buffer - gapbuffer.txt_start) { // checks if empty buffer length is zero

            if (gapbuffer.txt_start[i] == DELETE_CHAR) {
                SetRect(&rect, x, y, x + ti.tm.tmAveCharWidth, y + ti.tm.tmHeight);
                InvalidateRect(hwnd, &rect, TRUE);
            }

            // set i to after buffer position (minus one so next loop increments it to correct position)
            i = (int)(gapbuffer.after_buffer - gapbuffer.txt_start) - 1;
            continue;
            
        }

        TextOut(hdc, x, y, szBuffer, wsprintf(szBuffer, TEXT("%c"), gapbuffer.txt_start[i]));

        x += ti.tm.tmAveCharWidth;

        if (szBuffer[0] == '\r' || x >= ti.maxLineWidth) {
            y += ti.tm.tmHeight;
            x = 0;
        }
        
        free(szBuffer);
    }

}

// adds BUF_SIZE to buffer size and readjusts buffer pointers
int resizeBuffer(GapBuffer* gapbuffer) {

    TCHAR* temp = (TCHAR*)realloc(gapbuffer->txt_start, sizeof(TCHAR) * (gapbuffer->size + BUF_SIZE));
    if (!temp) {
        perror("Resizing reallocation failed");
        free(temp);
        return 0;
    }

    int prev_index = gapbuffer->buffer_index;
    int prev_size = gapbuffer->size;
    int buffer_end_index = (int)(gapbuffer->after_buffer - gapbuffer->txt_start);

    gapbuffer->txt_start = temp;

    gapbuffer->buffer = gapbuffer->txt_start + prev_index;
    gapbuffer->after_buffer = gapbuffer->buffer + BUF_SIZE;
    gapbuffer->size += BUF_SIZE;

    // if empty buffer was not at the end of the gapbuffer
    if (prev_index < prev_size) {

        //// shift newly made buffer up to previous buffer index
        for (int i = gapbuffer->size - 1; i >= prev_index + BUF_SIZE; i--) {
            gapbuffer->txt_start[i] = gapbuffer->txt_start[i - BUF_SIZE];
            gapbuffer->txt_start[i - BUF_SIZE] = 0xFD;
        }
    }

    return 1;
}

// shifts empty buffer to the left one character
void shiftBufferLeft(GapBuffer *gapbuffer, Caret* caret, TextInfo ti) {

    // if buffer is at the start, can't shift left
    if (gapbuffer->buffer_index == 0) {
        return;
    }

    // if buffer is empty, expand first
    if (gapbuffer->after_buffer-gapbuffer->buffer < 1) {
        resizeBuffer(gapbuffer);
    }

    // save position of first and last position of empty buffer
    int index = gapbuffer->buffer_index;
    int last_buffer = (int)(gapbuffer->after_buffer - gapbuffer->txt_start)-1;

    // copies character before buffer to last buffer position and updates gapbuffer pointers
    gapbuffer->txt_start[last_buffer] = gapbuffer->txt_start[index - 1];
    gapbuffer->txt_start[index - 1] = 0xFD;
    gapbuffer->buffer--;
    gapbuffer->buffer_index--;
    gapbuffer->after_buffer--;

    adjustCaretPos(-1, caret, *gapbuffer, ti);
}

// shifts empty buffer to the right one character
void shiftBufferRight(GapBuffer *gapbuffer, Caret* caret, TextInfo ti) {

    // if buffer is at the end, can't shift right
    if ((gapbuffer->after_buffer-gapbuffer->txt_start) >= gapbuffer->size) {
        return;
    }

    // if buffer is empty, expand first
    if (gapbuffer->after_buffer - gapbuffer->buffer < 1) {
        resizeBuffer(gapbuffer);
    }

    // save position of start of buffer and end of buffer
    int index = gapbuffer->buffer_index;
    int last_buffer = (int)(gapbuffer->after_buffer - gapbuffer->txt_start) - 1;

    // copies characters after buffer to first buffer position and updates gapuffer pointers
    gapbuffer->txt_start[index] = gapbuffer->txt_start[last_buffer + 1];
    gapbuffer->txt_start[last_buffer + 1] = 0xFD;
    gapbuffer->buffer++;
    gapbuffer->buffer_index++;
    gapbuffer->after_buffer++;

    adjustCaretPos(1, caret, *gapbuffer, ti);
}

// saves current gapbuffer contents to file (creating one if filename doesn't exist)
int savetoFile(GapBuffer gapbuffer, char* filename) {

    FILE *fp = NULL;

    if ((fp = fopen(filename, "w")) == NULL) {
        return 1;
    }

    for (int i = 0; i < gapbuffer.size; i++) {
        if (i == gapbuffer.buffer_index) {
            i += BUF_SIZE-1;
            continue;
        }
        fputc(gapbuffer.txt_start[i], fp);
    }

    fclose(fp);
    return 0;
}

// Clears gapbuffer and loads contents from file into it
int readfromFile(GapBuffer* gapbuffer, char* filename) {

    FILE* fp = NULL;

    if ((fp = fopen(filename, "r")) == NULL) {
        return 1;
    }

    clearBuffer(gapbuffer);

    char curr_char = fgetc(fp);
    int index = 0;

    while (curr_char != EOF) {

        // if index is larger than the size of the current gapbuffer, expand the buffer
        if (index >= gapbuffer->size-1) {
            resizeBuffer(gapbuffer);
            continue;
        }

        gapbuffer->txt_start[index++] = curr_char;
        curr_char = fgetc(fp);
    }

    gapbuffer->buffer = gapbuffer->txt_start + index;
    gapbuffer->after_buffer = gapbuffer->txt_start + gapbuffer->size;
    gapbuffer->buffer_index = index;

    fclose(fp);
    return 0;
}

// clears buffer, resets size and pointers
void clearBuffer(GapBuffer* gapbuffer) {

    for (int i = 0; i < gapbuffer->size; i++) {
        gapbuffer->txt_start[i] = 0xFD;
    }
    TCHAR* temp = (TCHAR*)realloc(gapbuffer->txt_start, sizeof(TCHAR) * (BUF_SIZE + 1));

    if (!temp) {
        return;
    }

    gapbuffer->txt_start = temp;
    gapbuffer->buffer = gapbuffer->txt_start;
    gapbuffer->after_buffer = gapbuffer->buffer + BUF_SIZE;
    gapbuffer->buffer_index = 0;
    gapbuffer->size = BUF_SIZE+1;

    

}
