#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <string.h>

static HWND g_mirror_window;
static const char *g_target_executable = "OculusMirror.exe";
static const char *g_target_title;

static int basename_equals(const char *path, const char *name)
{
    const char *base = strrchr(path, '\\');
    return _stricmp(base ? base + 1 : path, name) == 0;
}

static BOOL CALLBACK find_mirror(HWND window, LPARAM unused)
{
    (void)unused;
    DWORD process_id = 0;
    GetWindowThreadProcessId(window, &process_id);
    HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, process_id);
    if (!process) return TRUE;
    char path[1024]; DWORD length = sizeof(path);
    int match = QueryFullProcessImageNameA(process, 0, path, &length) &&
                basename_equals(path, g_target_executable);
    CloseHandle(process);
    if (!match) return TRUE;
    if (g_target_title) {
        char title[512] = {0};
        GetWindowTextA(window, title, sizeof(title));
        if (!strstr(title, g_target_title)) return TRUE;
    }
    RECT client;
    if (!GetClientRect(window, &client) || client.right <= 1 || client.bottom <= 1)
        return TRUE;
    g_mirror_window = window;
    return FALSE;
}

static int save_client_bmp(HWND window, const char *path)
{
    RECT client;
    if (!GetClientRect(window, &client)) return 0;
    int width = client.right - client.left;
    int height = client.bottom - client.top;
    if (width <= 0 || height <= 0) return 0;

    HDC source = GetDC(window);
    HDC memory = CreateCompatibleDC(source);
    BITMAPINFO info = {0};
    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = width;
    info.bmiHeader.biHeight = -height;
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;
    void *pixels = NULL;
    HBITMAP bitmap = CreateDIBSection(source, &info, DIB_RGB_COLORS, &pixels, NULL, 0);
    if (!source || !memory || !bitmap || !pixels) {
        if (bitmap) DeleteObject(bitmap);
        if (memory) DeleteDC(memory);
        if (source) ReleaseDC(window, source);
        return 0;
    }
    HGDIOBJ previous = SelectObject(memory, bitmap);
    BOOL captured = PrintWindow(window, memory, 3 /* client-only + full-content */);
    if (!captured) captured = BitBlt(memory, 0, 0, width, height, source, 0, 0, SRCCOPY);

    int ok = 0;
    if (captured) {
        FILE *file = fopen(path, "wb");
        if (file) {
            DWORD pixel_bytes = (DWORD)width * (DWORD)height * 4;
            BITMAPFILEHEADER header = {0};
            header.bfType = 0x4d42;
            header.bfOffBits = sizeof(header) + sizeof(BITMAPINFOHEADER);
            header.bfSize = header.bfOffBits + pixel_bytes;
            info.bmiHeader.biSizeImage = pixel_bytes;
            ok = fwrite(&header, 1, sizeof(header), file) == sizeof(header) &&
                 fwrite(&info.bmiHeader, 1, sizeof(info.bmiHeader), file) == sizeof(info.bmiHeader) &&
                 fwrite(pixels, 1, pixel_bytes, file) == pixel_bytes;
            fclose(file);
        }
    }
    SelectObject(memory, previous);
    DeleteObject(bitmap);
    DeleteDC(memory);
    ReleaseDC(window, source);
    return ok;
}

int main(int argc, char **argv)
{
    if (argc < 2 || argc > 4) {
        fprintf(stderr, "usage: headset_view_capture output.bmp [window-process.exe] [title-substring]\n");
        return 2;
    }
    if (argc >= 3) g_target_executable = argv[2];
    if (argc == 4) g_target_title = argv[3];
    EnumWindows(find_mirror, 0);
    if (!g_mirror_window) {
        fprintf(stderr, "%s does not have a capturable window.\n", g_target_executable);
        return 3;
    }
    if (!save_client_bmp(g_mirror_window, argv[1])) {
        fprintf(stderr, "Could not capture the Meta headset mirror.\n");
        return 4;
    }
    printf("Captured headset compositor view to %s\n", argv[1]);
    return 0;
}
