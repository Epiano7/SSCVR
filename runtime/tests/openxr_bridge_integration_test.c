#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <GL/gl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "openxr_bridge.h"

static HGLRC (WINAPI *p_wglCreateContext)(HDC);
static BOOL (WINAPI *p_wglMakeCurrent)(HDC, HGLRC);
static BOOL (WINAPI *p_wglDeleteContext)(HGLRC);
static void (APIENTRY *p_glViewport)(GLint, GLint, GLsizei, GLsizei);
static void (APIENTRY *p_glClearColor)(GLclampf, GLclampf, GLclampf, GLclampf);
static void (APIENTRY *p_glClear)(GLbitfield);
static void (APIENTRY *p_glMatrixMode)(GLenum);
static void (APIENTRY *p_glLoadIdentity)(void);
static void (APIENTRY *p_glFrustum)(GLdouble, GLdouble, GLdouble, GLdouble,
                                    GLdouble, GLdouble);
static void (APIENTRY *p_glTranslatef)(GLfloat, GLfloat, GLfloat);
static void (APIENTRY *p_glBegin)(GLenum);
static void (APIENTRY *p_glColor3f)(GLfloat, GLfloat, GLfloat);
static void (APIENTRY *p_glColor4f)(GLfloat, GLfloat, GLfloat, GLfloat);
static void (APIENTRY *p_glBlendFunc)(GLenum, GLenum);
static void (APIENTRY *p_glVertex3f)(GLfloat, GLfloat, GLfloat);
static void (APIENTRY *p_glVertex2f)(GLfloat, GLfloat);
static void (APIENTRY *p_glEnd)(void);
static void (APIENTRY *p_glOrtho)(GLdouble, GLdouble, GLdouble, GLdouble,
                                  GLdouble, GLdouble);
static void (APIENTRY *p_glDisable)(GLenum);
static void (APIENTRY *p_glEnable)(GLenum);
static void (APIENTRY *p_glPolygonMode)(GLenum, GLenum);
static void (APIENTRY *p_glReadBuffer)(GLenum);
static void (APIENTRY *p_glPixelStorei)(GLenum, GLint);
static void (APIENTRY *p_glReadPixels)(GLint, GLint, GLsizei, GLsizei,
                                       GLenum, GLenum, void *);

static int load_system_gl(void)
{
    HMODULE module = LoadLibraryA("opengl32_system.dll");
    if (!module) return 0;
#define LOAD_GL(name) do { \
    p_##name = (void *)GetProcAddress(module, #name); \
    if (!p_##name) return 0; \
} while (0)
    LOAD_GL(wglCreateContext); LOAD_GL(wglMakeCurrent); LOAD_GL(wglDeleteContext);
    LOAD_GL(glViewport); LOAD_GL(glClearColor); LOAD_GL(glClear);
    LOAD_GL(glMatrixMode); LOAD_GL(glLoadIdentity); LOAD_GL(glFrustum);
    LOAD_GL(glTranslatef); LOAD_GL(glBegin); LOAD_GL(glColor3f);
    LOAD_GL(glColor4f); LOAD_GL(glBlendFunc);
    LOAD_GL(glVertex3f); LOAD_GL(glVertex2f); LOAD_GL(glEnd);
    LOAD_GL(glOrtho); LOAD_GL(glDisable); LOAD_GL(glEnable);
    LOAD_GL(glPolygonMode);
    LOAD_GL(glReadBuffer); LOAD_GL(glPixelStorei); LOAD_GL(glReadPixels);
#undef LOAD_GL
    return 1;
}

static LRESULT CALLBACK integration_window_proc(HWND window, UINT message,
                                                 WPARAM wparam, LPARAM lparam)
{
    return DefWindowProcW(window, message, wparam, lparam);
}

static void marker_path(char *output, size_t capacity, const char *base,
                        const char *name)
{
    snprintf(output, capacity, "%s\\%s", base, name);
}

static int write_marker(const char *path, const char *contents)
{
    FILE *file = fopen(path, "wb");
    if (!file) return 0;
    fputs(contents, file);
    fclose(file);
    return 1;
}

static int capture_desktop_bmp(const char *path, int width, int height)
{
    size_t pixel_bytes = (size_t)width * (size_t)height * 4u;
    unsigned char *pixels = (unsigned char *)malloc(pixel_bytes);
    if (!pixels) return 0;
    p_glReadBuffer(GL_BACK);
    p_glPixelStorei(GL_PACK_ALIGNMENT, 1);
    p_glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    for (size_t offset = 0; offset < pixel_bytes; offset += 4u) {
        unsigned char red = pixels[offset];
        pixels[offset] = pixels[offset + 2u];
        pixels[offset + 2u] = red;
    }
    BITMAPFILEHEADER file_header = {0};
    BITMAPINFOHEADER info_header = {0};
    file_header.bfType = 0x4D42;
    file_header.bfOffBits = sizeof(file_header) + sizeof(info_header);
    file_header.bfSize = file_header.bfOffBits + (DWORD)pixel_bytes;
    info_header.biSize = sizeof(info_header);
    info_header.biWidth = width;
    info_header.biHeight = height;
    info_header.biPlanes = 1;
    info_header.biBitCount = 32;
    info_header.biCompression = BI_RGB;
    info_header.biSizeImage = (DWORD)pixel_bytes;
    FILE *file = fopen(path, "wb");
    int written = file &&
        fwrite(&file_header, sizeof(file_header), 1, file) == 1 &&
        fwrite(&info_header, sizeof(info_header), 1, file) == 1 &&
        fwrite(pixels, pixel_bytes, 1, file) == 1;
    if (file) fclose(file);
    free(pixels);
    return written;
}

static void draw_world(int frame, int eye)
{
    float pulse = 0.15f + 0.05f * sinf((float)frame * 0.025f);
    p_glViewport(0, 0, 960, 540);
    p_glClearColor(0.015f, 0.035f + pulse, 0.08f, 1.0f);
    p_glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    p_glMatrixMode(GL_PROJECTION);
    p_glLoadIdentity();
    p_glFrustum(-0.10, 0.10, -0.06, 0.06, 0.10, 100.0);
    p_glMatrixMode(GL_MODELVIEW);
    p_glLoadIdentity();
    p_glTranslatef(eye ? -0.025f : 0.025f, 0.0f, -2.5f);
    p_glBegin(GL_TRIANGLES);
    p_glColor3f(0.08f, 0.65f, 1.0f); p_glVertex3f(-0.8f, -0.55f, 0.0f);
    p_glColor3f(0.1f, 1.0f, 0.35f); p_glVertex3f(0.8f, -0.55f, 0.0f);
    p_glColor3f(1.0f, 0.35f, 0.08f); p_glVertex3f(0.0f, 0.70f, -0.35f);
    p_glEnd();
}

static void draw_interface(void)
{
    p_glMatrixMode(GL_PROJECTION);
    p_glLoadIdentity();
    p_glOrtho(0.0, 960.0, 540.0, 0.0, -1.0, 1.0);
    p_glMatrixMode(GL_MODELVIEW);
    p_glLoadIdentity();
    p_glDisable(GL_DEPTH_TEST);
    p_glDisable(GL_SCISSOR_TEST);
    p_glDisable(GL_TEXTURE_2D);
    p_glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    p_glEnable(GL_BLEND);
    /* A translucent panel plus an opaque control exercises accumulated alpha
       and overlapping UI, while the untouched background must remain exactly
       transparent. */
    p_glColor4f(0.08f, 0.34f, 0.86f, 0.55f);
    p_glBegin(GL_QUADS);
    p_glVertex2f(250.0f, 145.0f); p_glVertex2f(710.0f, 145.0f);
    p_glVertex2f(710.0f, 395.0f); p_glVertex2f(250.0f, 395.0f);
    p_glEnd();
    p_glColor4f(0.92f, 0.16f, 0.12f, 1.0f);
    p_glBegin(GL_QUADS);
    p_glVertex2f(400.0f, 235.0f); p_glVertex2f(560.0f, 235.0f);
    p_glVertex2f(560.0f, 305.0f); p_glVertex2f(400.0f, 305.0f);
    p_glEnd();
    p_glDisable(GL_BLEND);
    p_glEnable(GL_DEPTH_TEST);
}

int main(void)
{
    char executable[MAX_PATH], base[MAX_PATH];
    DWORD length = GetModuleFileNameA(NULL, executable, MAX_PATH);
    if (!length || length >= MAX_PATH) return 2;
    snprintf(base, sizeof(base), "%s", executable);
    char *slash = strrchr(base, '\\');
    if (!slash) return 3;
    *slash = '\0';

    char openxr_marker[MAX_PATH], geometry_marker[MAX_PATH];
    char log_directory[MAX_PATH], game_log[MAX_PATH];
    marker_path(openxr_marker, sizeof(openxr_marker), base,
                "SkillshotVR-openxr.txt");
    marker_path(geometry_marker, sizeof(geometry_marker), base,
                "SkillshotCityVR-geometry-stereo.txt");
    marker_path(log_directory, sizeof(log_directory), base, "data");
    CreateDirectoryA(log_directory, NULL);
    marker_path(game_log, sizeof(game_log), log_directory, "log.txt");
    if (!write_marker(openxr_marker, "integration test\n") ||
        !write_marker(geometry_marker, "integration test\n") ||
        !write_marker(game_log, "[0] Loading Map: maps/integration_test\n"))
        return 4;

    if (!load_system_gl()) return 10;
    HINSTANCE instance = GetModuleHandleW(NULL);
    WNDCLASSW window_class = {0};
    window_class.lpfnWndProc = integration_window_proc;
    window_class.hInstance = instance;
    window_class.lpszClassName = L"SkillshotVRBridgeIntegration";
    RegisterClassW(&window_class);
    HWND window = CreateWindowW(window_class.lpszClassName,
        L"Skillshot VR bridge integration", WS_OVERLAPPEDWINDOW,
        100, 100, 976, 579, NULL, NULL, instance, NULL);
    if (!window) return 5;
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
    if (!pixel_format || !SetPixelFormat(dc, pixel_format, &format)) return 6;
    HGLRC context = p_wglCreateContext(dc);
    if (!context || !p_wglMakeCurrent(dc, context)) return 7;
    ShowWindow(window, SW_SHOWNA);

    int published_pairs = 0;
    int menu_frames = 0;
    int frame_limit = 2400;
    {
        const char *requested_frames = getenv("SKILLSHOTVR_INTEGRATION_FRAMES");
        if (requested_frames && requested_frames[0]) {
            char *end = NULL;
            long parsed = strtol(requested_frames, &end, 10);
            if (end && *end == '\0' && parsed >= 2400 && parsed <= 120000) {
                frame_limit = (int)parsed;
            }
        }
    }
    /* Seven full menu enter/exit cycles after gameplay becomes active catch
       stale overlays, state leaks, and transition-only corruption. Longer
       unattended soak runs can request additional cycles through the bounded
       SKILLSHOTVR_INTEGRATION_FRAMES environment variable. */
    for (int frame = 0; frame < frame_limit; ++frame) {
        MSG message;
        while (PeekMessageW(&message, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
        openxr_bridge_tick(base, dc);
        if (openxr_bridge_geometry_ready()) {
            int eye = frame & 1;
            float head_view[16];
            openxr_bridge_get_head_view_delta(80.0f, 5.2f, head_view);
            openxr_bridge_set_geometry_fov(eye, -0.7853982f, 0.7853982f,
                                           0.5404195f, -0.5404195f);
            draw_world(frame, eye);
            openxr_bridge_capture_native_mirror(1);
            openxr_bridge_capture_hud_base(eye, 0.66f, 0.40f);
            int captured = openxr_bridge_capture_eye(eye);
            if (captured && eye == 0) ++published_pairs;
            p_glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            openxr_bridge_begin_interface_alpha_capture();
            int heavy = frame >= 1000 && (((frame - 1000) / 100) & 1) == 0;
            if (frame == 1000) {
                char proof_marker[MAX_PATH];
                openxr_bridge_request_fresh_geometry_proof();
                marker_path(proof_marker, sizeof(proof_marker), base,
                            "SkillshotCityVR-geometry-proof.txt");
                write_marker(proof_marker, "capture completed interface eyes\n");
            }
            if (frame == 1400) {
                char proof_marker[MAX_PATH];
                openxr_bridge_request_fresh_geometry_proof();
                marker_path(proof_marker, sizeof(proof_marker), base,
                            "SkillshotCityVR-geometry-proof.txt");
                write_marker(proof_marker,
                             "capture second completed interface proof set\n");
            }
            openxr_bridge_set_interface_load(heavy ? 1000 : 120,
                                             heavy ? 80000 : 5000);
            if (heavy) {
                draw_interface();
                ++menu_frames;
            }
            openxr_bridge_finish_interface_alpha_capture(heavy);
            openxr_bridge_set_geometry_active(1);
            openxr_bridge_present_mirror_eye(0, 1.0f);
            openxr_bridge_present_desktop_interface();
            if (frame == 999) {
                char desktop_path[MAX_PATH];
                marker_path(desktop_path, sizeof(desktop_path), base,
                            "SkillshotCityVR-desktop-world.bmp");
                capture_desktop_bmp(desktop_path, 960, 540);
            }
            if (frame == 1000) {
                char desktop_path[MAX_PATH];
                marker_path(desktop_path, sizeof(desktop_path), base,
                            "SkillshotCityVR-desktop-interface.bmp");
                capture_desktop_bmp(desktop_path, 960, 540);
            }
        }
        SwapBuffers(dc);
        Sleep(10);
    }

    int passed = published_pairs >= 50 && menu_frames >= 100;
    DeleteFileA(openxr_marker);
    DeleteFileA(geometry_marker);
    char result_path[MAX_PATH];
    marker_path(result_path, sizeof(result_path), base, "integration-result.txt");
    FILE *result = fopen(result_path, "wb");
    if (result) {
        fprintf(result, "published_pairs=%d menu_frames=%d status=%s\n",
                published_pairs, menu_frames, openxr_bridge_status());
        fclose(result);
    }
    printf("integration published_pairs=%d menu_frames=%d status=%s\n",
           published_pairs, menu_frames, openxr_bridge_status());
    fflush(stdout);
    /* The product process owns the bridge for its whole lifetime. This test
       deliberately has no runtime teardown API, so terminate after recording
       the result instead of destroying GL while the XR frame thread is live. */
    ExitProcess(passed ? 0 : 8);
    return passed ? 0 : 8;
}
