#include <windows.h>
#include <stdio.h>
#include <math.h>

#include "DEVCAPS.h"


LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void printChar(HDC hdc, TCHAR ch, int* line, int* col, int maxWidth, int maxChar, int charHeight);
void deleteChar(HWND hwnd,HDC hdc, int* line, int* col, int maxWidth, int maxChar, int charHeight);


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
    wclass.hbrBackground = (HBRUSH)(COLOR_WINDOW);
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

    return Msg.wParam;
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
    static int vScrollPos, hScrollPos;
    static int doc_row, doc_col;
    TCHAR szBuffer[10];

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

        doc_row = 0;
        doc_col = 0;

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
        switch (wParam) {
        case VK_HOME:
            SendMessage(hwnd, WM_HSCROLL, SB_TOP, 0);
            break;
        case VK_END:
            SendMessage(hwnd, WM_HSCROLL, SB_BOTTOM, 0);
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
            SendMessage(hwnd, WM_HSCROLL, SB_LINELEFT, 0);
            break;
        case VK_RIGHT:
            SendMessage(hwnd, WM_HSCROLL, SB_LINERIGHT, 0);
            break;
        }
        break;
    }

    case WM_CHAR:
    {
        switch (wParam) {
            case '\r':

                break;
            case '\t':
                break;
            case '\b':
                hdc = GetDC(hwnd);
                deleteChar(hwnd, hdc, &doc_row, &doc_col, cxClient, cxChar, cyChar);
                ReleaseDC(hwnd, hdc);
                break;
            default:
                hdc = GetDC(hwnd);
                printChar(hdc, wParam, &doc_row, &doc_col, cxClient, cxChar, cyChar);
                ReleaseDC(hwnd, hdc);
                break;
        }
        break;
    }

    case WM_CLOSE:
        DestroyWindow(hwnd);
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

void printChar(HDC hdc, TCHAR ch, int* line, int* col, int maxWidth, int charWidth, int charHeight) {

    TCHAR szBuffer[2];
    TextOut(hdc, *col, *line, szBuffer, wsprintf(szBuffer, TEXT("%c"), ch));

    (*col)+=charWidth;
    if (*col > maxWidth-charWidth) {
        (*line)+=charHeight;
        *col = 0;
    }
}

void deleteChar(HWND hwnd, HDC hdc, int* line, int* col, int maxWidth, int charWidth, int charHeight) {

    RECT rect;

    (*col)-=charWidth;

    if (*col < 0) {
        (*line) -= charHeight;
        (*col) = maxWidth-charWidth;
    }
    if (*line < 0) {
        *line = 0;
        *col = 0;
    }

    SetRect(&rect, *col, *line, *col + charWidth, *line + charHeight);
    InvalidateRect(hwnd, &rect, TRUE);

}