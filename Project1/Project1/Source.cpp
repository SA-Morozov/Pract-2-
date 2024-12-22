#include <windows.h>
#include <commdlg.h>
#include <vector>
#include <string>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

struct ImageData {
    int width;
    int height;
    std::vector<BYTE> pixels;
    BITMAPINFO bmpInfo;
};

ImageData loadedImage;
COLORREF bgColor = RGB(255, 255, 255);

bool LoadImageFromFile(const wchar_t* filePath, ImageData& image) {
    FILE* file;
    if (_wfopen_s(&file, filePath, L"rb") != 0) return false;

    BITMAPFILEHEADER bmpFileHeader;
    fread(&bmpFileHeader, sizeof(BITMAPFILEHEADER), 1, file);

    BITMAPINFOHEADER bmpInfoHeader;
    fread(&bmpInfoHeader, sizeof(BITMAPINFOHEADER), 1, file);

    if (bmpInfoHeader.biCompression != BI_RGB) {
        fclose(file);
        return false;
    }

    image.width = bmpInfoHeader.biWidth;
    image.height = abs(bmpInfoHeader.biHeight);
    int rowSize = ((image.width * 3 + 3) & ~3);

    image.pixels.resize(rowSize * image.height);
    image.bmpInfo.bmiHeader = bmpInfoHeader;

    fseek(file, bmpFileHeader.bfOffBits, SEEK_SET);
    fread(image.pixels.data(), 1, image.pixels.size(), file);
    fclose(file);

    bgColor = RGB(
        image.pixels[2],
        image.pixels[1],
        image.pixels[0]
    );

    return true;
}

void OpenImageDialog(HWND hwnd) {
    OPENFILENAME ofn = { 0 };
    wchar_t fileName[MAX_PATH] = { 0 };

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFile = fileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = L"Bitmap Files\0*.bmp\0";
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

    if (GetOpenFileName(&ofn)) {
        if (LoadImageFromFile(fileName, loadedImage)) {
            InvalidateRect(hwnd, nullptr, TRUE); // Перерисовать окно
        }
    }
}

void DrawBackground(HDC hdc, const RECT& rect) {
    int tileSize = 15;
    for (int y = 0; y < rect.bottom; y += tileSize) {
        for (int x = 0; x < rect.right; x += tileSize) {
            HBRUSH brush = CreateSolidBrush((x / tileSize + y / tileSize) % 2 == 0 ? RGB(230, 230, 230) : RGB(200, 200, 200));
            RECT tileRect = { x, y, x + tileSize, y + tileSize };
            FillRect(hdc, &tileRect, brush);
            DeleteObject(brush);
        }
    }
}

void RenderImage(HDC hdc, const RECT& rect) {
    if (loadedImage.pixels.empty()) return;

    int rowSize = ((loadedImage.width * 3 + 3) & ~3);

    // Левый нижний угол
    int startX = 0; // Левый край
    int startY = rect.bottom - loadedImage.height; // Нижний край

    for (int y = 0; y < loadedImage.height; ++y) {
        int sourceY = loadedImage.height - 1 - y; // Отображаем строки в обратном порядке
        for (int x = 0; x < loadedImage.width; ++x) {
            BYTE* pixel = &loadedImage.pixels[(sourceY * rowSize) + (x * 3)];
            COLORREF color = RGB(pixel[2], pixel[1], pixel[0]);

            if (color != bgColor) {
                SetPixel(hdc, startX + x, startY + y, color);
            }
        }
    }
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int nCmdShow) {
    const wchar_t className[] = L"ImageLoaderWindow";

    WNDCLASS wc = { };
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = className;

    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        0, className, L"Image Loader - Fixed",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 600, 600,
        NULL, NULL, hInstance, NULL
    );

    if (!hwnd) return 0;

    ShowWindow(hwnd, nCmdShow);

    MSG msg = { };
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    static HWND loadButton;

    switch (uMsg) {
    case WM_CREATE: {
        loadButton = CreateWindow(
            L"BUTTON", L"Open Image",
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            20, 20, 120, 40,
            hwnd, (HMENU)1, (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL
        );
        break;
    }
    case WM_COMMAND:
        if (LOWORD(wParam) == 1) {
            OpenImageDialog(hwnd);
        }
        break;
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        RECT clientRect;
        GetClientRect(hwnd, &clientRect);

        // Удаляем старый холст (обеспечивает очистку)
        FillRect(hdc, &clientRect, (HBRUSH)GetStockObject(WHITE_BRUSH));

        // Рисуем шахматный фон
        DrawBackground(hdc, clientRect);

        // Рендерим изображение поверх
        RenderImage(hdc, clientRect);

        EndPaint(hwnd, &ps);
        break;
    }
    case WM_SIZE: {
        // Уведомляем систему, что нужно обновить весь клиентский регион
        InvalidateRect(hwnd, nullptr, TRUE);
        break;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}
