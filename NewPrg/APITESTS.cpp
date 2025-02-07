#include <windows.h>
#include <stdio.h>
#include <math.h>
#include <time.h>

#include "DEVCAPS.h"


LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
RECT* randRect(HWND);
void PrintRect(HWND hwnd, RECT* rect);


const TCHAR g_szClassName[] = TEXT("APITESTS");
int cxClient, cyClient;


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow) {

    srand(time(NULL));
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
        WS_OVERLAPPEDWINDOW,
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
    while (TRUE) {

        if (PeekMessage(&Msg, NULL, 0, 0, PM_REMOVE)) {
            if (Msg.message == WM_QUIT)
                break;

            TranslateMessage(&Msg);
            DispatchMessage(&Msg);
        }
        else {
            
            PrintRect(hwnd, randRect(hwnd));
        }

    }

    return Msg.wParam;
}

// Step 4: the Window Procedure
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {

    RECT rect;
    PAINTSTRUCT ps;
    switch (msg) {

    case WM_SIZE:
    {
        cxClient = LOWORD(lParam);
        cyClient = HIWORD(lParam);
        return 0;
    }

    case WM_PAINT:

        BeginPaint(hwnd, &ps);
        PrintRect(hwnd, randRect(hwnd));
        EndPaint(hwnd, &ps);
        break;

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


RECT* randRect(HWND hwnd) {

    RECT rect;
    
    int width = rand() % cxClient;
    int height = rand() % cyClient;
    int left = rand() % (cxClient-width);
    int right = left+width;
    int top = rand() % (cyClient-height);
    int bottom = top+height;
    HDC hdc = GetDC(hwnd);

    SetRect(&rect, left, top, right, bottom);

    ReleaseDC(hwnd, hdc);

    return &rect;

}

void PrintRect(HWND hwnd, RECT* rect) {

    HDC hdc = GetDC(hwnd);
    HBRUSH hbrush1 = CreateSolidBrush(RGB(rand() % 255, rand() % 255, rand() % 255));

    FillRect(hdc, rect, hbrush1);

    ReleaseDC(hwnd, hdc);
    DeleteObject(hbrush1);
}