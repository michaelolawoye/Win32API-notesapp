#include <windows.h>
#include <stdio.h>
#include <math.h>

#include "DEVCAPS.h"


#define BUF_SIZE 10


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
    // start of struct
    TCHAR* txt_start;
    // start of empty buffer
    TCHAR* buffer;
    // end of empty buffer
    TCHAR* buffer_end;
    // size of entire struct
    int size;
    // index of empty buffer in struct
    int buffer_index;
} GapBuffer, *GapBufferPtr;


LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void addBufferChar(TCHAR, GapBuffer*);
void deleteChar(GapBuffer*);
void adjustCaretPos(Caret*, GapBuffer, int x_change, int y_change, TextInfo);
void printBuffer(HWND, HDC, GapBuffer, TextInfo);
int resizeBuffer(GapBuffer*);

const TCHAR g_szClassName[] = TEXT("APITESTS");


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow) {

    WNDCLASS wclass;
    HWND hwnd;
    MSG Msg;

    //Step 1: Registering the Window Class
    wclass.style = CS_HREDRAW | CS_VREDRAW;
    wclass.lpfnWndProc = WndProc;
    wclass.cbClsExtra = 0;
    wclass.cbWndExtra = 0;
    wclass.hInstance = hInstance;
    wclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wclass.hCursor = LoadCursor(NULL, IDC_ARROW);
    wclass.hbrBackground = CreateSolidBrush(RGB(169, 169, 169));
    wclass.lpszMenuName = NULL;
    wclass.lpszClassName = g_szClassName;

    if (!RegisterClass(&wclass)) {
        MessageBox(NULL, TEXT("Window Registration Failed!"), TEXT("Error!"),
            MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    // Step 2: Creating the Window
    hwnd = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        g_szClassName,
        TEXT("The title of my window"),
        WS_OVERLAPPEDWINDOW | WS_VSCROLL | WS_HSCROLL,
        CW_USEDEFAULT, CW_USEDEFAULT, 500, 360,
        NULL, NULL, hInstance, NULL);

    if (hwnd == NULL) {
        MessageBox(NULL, TEXT("Window Creation Failed!"), TEXT("Error!"),
            MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    ShowWindow(hwnd, iCmdShow);
    UpdateWindow(hwnd);

    // Step 3: The Message Loop
    while (GetMessage(&Msg, NULL, 0, 0) > 0) {

        TranslateMessage(&Msg);
        DispatchMessage(&Msg);
    }

    return (int)Msg.wParam;
}

// Step 4: the Window Procedure
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    HDC hdc;
    PAINTSTRUCT ps;
    RECT rect;
    TEXTMETRIC tm;
    SCROLLINFO si;
    POINT pt;
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
        gapbuffer.size = BUF_SIZE;
        gapbuffer.buffer = gapbuffer.txt_start;
        gapbuffer.buffer_index = 0;
        gapbuffer.buffer_end = gapbuffer.txt_start + BUF_SIZE - 1;


        ti.tm = tm;

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
            adjustCaretPos(&caret, gapbuffer, -1, 0, ti);
            break;
        case VK_END:
            adjustCaretPos(&caret, gapbuffer, -1, 0, ti);
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
            adjustCaretPos(&caret, gapbuffer, -1, 0, ti);
            break;
        case VK_RIGHT:
            adjustCaretPos(&caret, gapbuffer, 1, 0, ti);
            break;
        }
        break;
    }

    case WM_CHAR:
    {
        hdc = GetDC(hwnd);

        SelectObject(hdc, GetStockObject(SYSTEM_FIXED_FONT));
        SetBkMode(hdc, TRANSPARENT);

        switch (wParam) {
            case '\r':
                break;
            case '\t':
                break;
            case '\b':
                deleteChar(&gapbuffer);
                break;
            default:
                addBufferChar(wParam, &gapbuffer);
                break;
        }

        printBuffer(hwnd, hdc, gapbuffer, ti);

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

void addBufferChar(TCHAR ch, GapBuffer* gapbuffer) {
    
    if (gapbuffer->buffer > gapbuffer->buffer_end)
        resizeBuffer(gapbuffer);

    gapbuffer->buffer[0] = ch;
    gapbuffer->buffer++;

    gapbuffer->buffer_index++;
}

void deleteBufferChar(GapBuffer* gapbuffer) {

    if (gapbuffer->buffer == gapbuffer->txt_start)
        return;
    gapbuffer->buffer = gapbuffer->txt_start + gapbuffer->buffer_index-1;
    gapbuffer->buffer_index--;
    
    // remove character from screen


}

void adjustCaretPos(Caret* caret, GapBuffer gb, int x_change, int y_change, TextInfo ti) {

}

// first prints content before buffer, then in buffer, then after buffer in separate loops
void printBuffer(HWND hwnd, HDC hdc, GapBuffer gapbuffer, TextInfo ti) {

    TCHAR* szBuffer;
    int x = 0, y = 0;
    
    int max_char = ti.maxLineWidth / ti.tm.tmAveCharWidth;
    // prints each character in buffer
    for (int i = 0; i < gapbuffer.size; i++) {
        szBuffer = (TCHAR*)calloc(2, sizeof(TCHAR));

        if (!szBuffer) {
            free(szBuffer);
            continue;
        }
        // skip empty buffer
        if (i == gapbuffer.buffer_index) {
            i += gapbuffer.buffer_end - gapbuffer.buffer+1;
            continue;
        }
        TextOut(hdc, x, y, szBuffer, wsprintf(szBuffer, TEXT("%c"), gapbuffer.txt_start[i]));

        x += ti.tm.tmAveCharWidth;
        if (*szBuffer == '\n' || x >= ti.maxLineWidth) {
            y += ti.tm.tmHeight;
            x = 0;
        }
        
        free(szBuffer);
    }

}

// doubles buffer size and readjusts buffer pointers
int resizeBuffer(GapBuffer* gapbuffer) {

    TCHAR* temp = (TCHAR*)realloc(gapbuffer->txt_start, sizeof(TCHAR) * (gapbuffer->size + BUF_SIZE));
    if (!temp) {
        free(temp);
        return 0;
    }

    gapbuffer->txt_start = temp;
    gapbuffer->buffer = gapbuffer->txt_start + gapbuffer->size;
    gapbuffer->buffer_index = gapbuffer->size;
    gapbuffer->buffer_end = gapbuffer->buffer + BUF_SIZE;
    gapbuffer->size += BUF_SIZE;
    return 1;
}

void shifBuffertLeft(GapBuffer gapbuffer) {

    for (int i = BUF_SIZE+gapbuffer.buffer_index; i > gapbuffer.buffer_index; i--) {
        gapbuffer.buffer[i] = gapbuffer.buffer[i-1];
    }
}

void shiftBufferRight(GapBuffer gapbuffer) {

    for (int i = gapbuffer.buffer_index; i < BUF_SIZE + gapbuffer.buffer_index; i++) {
        gapbuffer.buffer[i] = gapbuffer.buffer[i+1];
    }
}
