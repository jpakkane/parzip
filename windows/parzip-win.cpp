/*
 * Copyright (C) 2016-2019 Jussi Pakkanen.
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of version 3, or (at your option) any later version,
 * of the GNU General Public License as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <commctrl.h>
#include <cstdio>
#include <windows.h>

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

void add_item(HWND hwndTV);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, PSTR, int iCmdShow) {
    static TCHAR name[] = TEXT("Parzip");
    RECT rcClient; // dimensions of client area
    HWND hwnd, hwndtv;
    MSG msg;
    WNDCLASS wndclass;

    InitCommonControls();

    wndclass.style = CS_HREDRAW | CS_VREDRAW;
    wndclass.lpfnWndProc = WndProc;
    wndclass.cbClsExtra = 0;
    wndclass.cbWndExtra = 0;
    wndclass.hInstance = hInstance;
    wndclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
    wndclass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wndclass.lpszMenuName = NULL;
    wndclass.lpszClassName = name;

    if (!RegisterClass(&wndclass)) {
        MessageBox(NULL, TEXT("Could not initialize wndclass"), name, MB_ICONERROR);
        return 0;
    }

    hwnd = CreateWindow(name, TEXT("Parallel unzipper"), WS_OVERLAPPEDWINDOW, CW_USEDEFAULT,
                        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInstance, NULL);

    GetClientRect(hwnd, &rcClient);
    hwndtv = CreateWindowEx(0, WC_TREEVIEW, TEXT("Tree view"), WS_CHILD, 0, 0, rcClient.right,
                            rcClient.bottom, hwnd, NULL, hInstance, NULL);

    ShowWindow(hwndtv, iCmdShow);
    UpdateWindow(hwndtv);
    ShowWindow(hwnd, iCmdShow);
    UpdateWindow(hwnd);
    add_item(hwndtv);
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return msg.wParam;
}

void add_item(HWND hwndTV) {
    static char msg[] = "This is text inside a treeview.";
    TVITEM tvi;
    TVINSERTSTRUCT tvins;
    static HTREEITEM hPrev = (HTREEITEM)TVI_FIRST;

    tvi.mask = TVIF_TEXT;

    // Set the text of the item.
    tvi.pszText = msg;
    tvi.cchTextMax = sizeof(tvi.pszText) / sizeof(tvi.pszText[0]);

    tvins.item = tvi;
    tvins.hInsertAfter = TVI_ROOT;
    tvins.hParent = TVI_ROOT;

    // Add the item to the tree-view control.
    hPrev = (HTREEITEM)SendMessage(hwndTV, TVM_INSERTITEM, 0, (LPARAM)(LPTVINSERTSTRUCT)&tvins);

    if (hPrev == NULL) {
        MessageBox(NULL, TEXT("Inserting failed."), NULL, MB_ICONERROR);
        return;
    }
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
    switch (message) {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hwnd, message, wparam, lparam);
}
