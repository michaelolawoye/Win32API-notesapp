#include <windows.h>
#include <stdio.h>
#include <math.h>

#include "DEVCAPS.h"


LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

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

        return 0;
    }
    
    case WM_PAINT:
    {
        HPEN dotPen = CreatePen(PS_DOT, 0, RGB(0, 255, 0));
        HPEN dashPen = CreatePen(PS_DASH, 0, RGB(200, 200, 0));
        LOGBRUSH logBrush = {BS_SOLID, RGB(0, 0, 0), NULL};
        HBRUSH exBrush = CreateBrushIndirect(&logBrush);
        int xborder = cxClient / 10;
        int yborder = cyClient / 10;

        hdc = BeginPaint(hwnd, &ps);
        
        SelectObject(hdc, dashPen);


        Rectangle(hdc, xborder, yborder, cxClient-xborder, cyClient - yborder);

        SelectObject(hdc, dotPen);

        MoveToEx(hdc, xborder, yborder, NULL);
        Chord(hdc, xborder-cxClient, yborder, cxClient - xborder, 2 * (cyClient - yborder),
            cxClient-xborder, cyClient-yborder, xborder, yborder);

        EndPaint(hwnd, &ps);

        DeleteObject(dotPen);
        DeleteObject(dashPen);
        DeleteObject(exBrush);
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

    case WM_HSCROLL:

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