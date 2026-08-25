#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <GL/gl.h>

static LRESULT CALLBACK window_proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
    return DefWindowProcW(window, message, wparam, lparam);
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE previous, LPSTR command_line, int show)
{
    (void)previous; (void)command_line; (void)show;
    WNDCLASSW window_class = {0};
    window_class.lpfnWndProc = window_proc;
    window_class.hInstance = instance;
    window_class.lpszClassName = L"SkillshotVRProxyTest";
    RegisterClassW(&window_class);
    HWND window = CreateWindowW(window_class.lpszClassName, L"SkillshotVR proxy test", WS_OVERLAPPEDWINDOW,
                                100, 100, 640, 480, NULL, NULL, instance, NULL);
    HDC dc = GetDC(window);
    PIXELFORMATDESCRIPTOR format = {0};
    format.nSize = sizeof(format);
    format.nVersion = 1;
    format.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    format.iPixelType = PFD_TYPE_RGBA;
    format.cColorBits = 24;
    format.cDepthBits = 24;
    format.iLayerType = PFD_MAIN_PLANE;
    int pixel_format = ChoosePixelFormat(dc, &format);
    SetPixelFormat(dc, pixel_format, &format);
    HGLRC context = wglCreateContext(dc);
    wglMakeCurrent(dc, context);
    ShowWindow(window, SW_SHOW);

    for (int frame = 0; frame < 130; ++frame) {
        MSG message;
        while (PeekMessageW(&message, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
        glViewport(0, 0, 624, 441);
        glClearColor(0.05f, 0.08f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glBegin(GL_TRIANGLES);
        glColor3f(1.0f, 0.2f, 0.1f); glVertex2f(-0.7f, -0.6f);
        glColor3f(0.1f, 1.0f, 0.3f); glVertex2f(0.7f, -0.6f);
        glColor3f(0.2f, 0.4f, 1.0f); glVertex2f(0.0f, 0.7f);
        glEnd();
        SwapBuffers(dc);
        Sleep(5);
    }

    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(context);
    ReleaseDC(window, dc);
    DestroyWindow(window);
    return 0;
}
