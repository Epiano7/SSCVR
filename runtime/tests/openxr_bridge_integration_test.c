#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <GL/gl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "openxr_bridge.h"
#include "geometry_math.h"

static HGLRC (WINAPI *p_wglCreateContext)(HDC);
static BOOL (WINAPI *p_wglMakeCurrent)(HDC, HGLRC);
static BOOL (WINAPI *p_wglDeleteContext)(HGLRC);
static PROC (WINAPI *p_wglGetProcAddress)(LPCSTR);
static void (APIENTRY *p_glBindFramebuffer)(GLenum, GLuint);
static void (APIENTRY *p_glScissor)(GLint, GLint, GLsizei, GLsizei);
static GLboolean (APIENTRY *p_glIsEnabled)(GLenum);
static void (APIENTRY *p_glGenTextures)(GLsizei, GLuint *);
static void (APIENTRY *p_glBindTexture)(GLenum, GLuint);
static void (APIENTRY *p_glTexParameteri)(GLenum, GLenum, GLint);
static void (APIENTRY *p_glTexImage2D)(GLenum, GLint, GLint, GLsizei, GLsizei,
                                     GLint, GLenum, GLenum, const void *);
static void (APIENTRY *p_glTexCoord2f)(GLfloat, GLfloat);
static GLuint replay_texture;
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
static void (APIENTRY *p_glColorMask)(GLboolean, GLboolean, GLboolean, GLboolean);
static void (APIENTRY *p_glGetIntegerv)(GLenum, GLint *);
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
    LOAD_GL(wglGetProcAddress); LOAD_GL(glScissor); LOAD_GL(glIsEnabled);
    LOAD_GL(glGenTextures); LOAD_GL(glBindTexture); LOAD_GL(glTexParameteri);
    LOAD_GL(glTexImage2D); LOAD_GL(glTexCoord2f);
    LOAD_GL(glViewport); LOAD_GL(glClearColor); LOAD_GL(glClear);
    LOAD_GL(glMatrixMode); LOAD_GL(glLoadIdentity); LOAD_GL(glFrustum);
    LOAD_GL(glTranslatef); LOAD_GL(glBegin); LOAD_GL(glColor3f);
    LOAD_GL(glColor4f); LOAD_GL(glBlendFunc);
    LOAD_GL(glColorMask); LOAD_GL(glGetIntegerv);
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

static void draw_center_announcement_test(int eye)
{
    /* Reproduce SSC's ordinary center announcements: an authored translucent
       bar is drawn after the pre-HUD world capture but before the completed
       eye capture.  The deliberately different scenery patch under each eye
       makes accidental source-eye gameplay transfer directly observable. */
    p_glMatrixMode(GL_PROJECTION);
    p_glLoadIdentity();
    p_glOrtho(0.0, 960.0, 540.0, 0.0, -1.0, 1.0);
    p_glMatrixMode(GL_MODELVIEW);
    p_glLoadIdentity();
    p_glDisable(GL_DEPTH_TEST);
    p_glDisable(GL_TEXTURE_2D);
    p_glDisable(GL_BLEND);
    p_glColor3f(eye ? 0.72f : 0.08f, eye ? 0.08f : 0.58f,
                eye ? 0.22f : 0.82f);
    p_glBegin(GL_QUADS);
    p_glVertex2f(250.0f, 210.0f); p_glVertex2f(710.0f, 210.0f);
    p_glVertex2f(710.0f, 330.0f); p_glVertex2f(250.0f, 330.0f);
    p_glEnd();
}

static void draw_center_announcement(void)
{
    p_glMatrixMode(GL_PROJECTION);
    p_glLoadIdentity();
    p_glOrtho(0.0, 960.0, 540.0, 0.0, -1.0, 1.0);
    p_glMatrixMode(GL_MODELVIEW);
    p_glLoadIdentity();
    p_glDisable(GL_DEPTH_TEST);
    p_glDisable(GL_TEXTURE_2D);
    p_glEnable(GL_BLEND);
    p_glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    p_glColor4f(0.035f, 0.055f, 0.085f, 0.72f);
    p_glBegin(GL_QUADS);
    p_glVertex2f(275.0f, 230.0f); p_glVertex2f(685.0f, 230.0f);
    p_glVertex2f(685.0f, 310.0f); p_glVertex2f(275.0f, 310.0f);
    p_glEnd();
    /* Opaque blocks stand in for announcement text. They must align exactly,
       while the translucent bar must retain each eye's own world beneath it. */
    p_glColor4f(0.95f, 0.93f, 0.82f, 1.0f);
    p_glBegin(GL_QUADS);
    p_glVertex2f(405.0f, 258.0f); p_glVertex2f(555.0f, 258.0f);
    p_glVertex2f(555.0f, 282.0f); p_glVertex2f(405.0f, 282.0f);
    p_glEnd();
    p_glDisable(GL_BLEND);
    p_glEnable(GL_DEPTH_TEST);
}

static void draw_interface(int frame)
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
    if (replay_texture) {
        /* Local-only captured game UI is already premultiplied. Replay its
           exact RGB and separately captured coverage, without game assets in
           the source tree or a live game/matchmaking session. */
        p_glEnable(GL_TEXTURE_2D);
        p_glBindTexture(GL_TEXTURE_2D, replay_texture);
        p_glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        p_glColor4f(1, 1, 1, 1);
        p_glBegin(GL_QUADS);
        p_glTexCoord2f(0, 0); p_glVertex2f(0, 0);
        p_glTexCoord2f(1, 0); p_glVertex2f(960, 0);
        p_glTexCoord2f(1, 1); p_glVertex2f(960, 540);
        p_glTexCoord2f(0, 1); p_glVertex2f(0, 540);
        p_glEnd();
        p_glDisable(GL_TEXTURE_2D);
        p_glDisable(GL_BLEND);
        p_glEnable(GL_DEPTH_TEST);
        return;
    }
    /* A translucent panel plus an opaque control exercises accumulated alpha
       and overlapping UI, while the untouched background must remain exactly
       transparent. */
    p_glColor4f(0.08f, 0.34f, 0.86f, 0.55f);
    p_glBegin(GL_QUADS);
    p_glVertex2f(250.0f, 145.0f); p_glVertex2f(710.0f, 145.0f);
    p_glVertex2f(710.0f, 395.0f); p_glVertex2f(250.0f, 395.0f);
    p_glEnd();
    int second = frame >= 1400;
    if (second) p_glColor4f(0.12f, 0.84f, 0.24f, 1.0f);
    else p_glColor4f(0.92f, 0.16f, 0.12f, 1.0f);
    p_glBegin(GL_QUADS);
    p_glVertex2f(400.0f, 235.0f); p_glVertex2f(560.0f, 235.0f);
    p_glVertex2f(560.0f, 305.0f); p_glVertex2f(400.0f, 305.0f);
    p_glEnd();
    /* An opaque marker near an authored edge detects clipped/scaled menus. */
    p_glColor4f(0.85f, 0.65f, 0.10f, 1.0f);
    p_glBegin(GL_QUADS);
    p_glVertex2f(24, 24); p_glVertex2f(64, 24);
    p_glVertex2f(64, 64); p_glVertex2f(24, 64);
    p_glEnd();
    p_glDisable(GL_BLEND);
    p_glEnable(GL_DEPTH_TEST);
}

static unsigned char *read_proof_pixels(const char *path, int *width, int *height)
{
    FILE *file = fopen(path, "rb");
    BITMAPFILEHEADER header;
    BITMAPINFOHEADER info;
    if (!file) return NULL;
    if (fread(&header, sizeof(header), 1, file) != 1 ||
        fread(&info, sizeof(info), 1, file) != 1 ||
        header.bfType != 0x4D42 || info.biSize != sizeof(info) ||
        info.biBitCount != 32 || info.biCompression != BI_RGB ||
        info.biWidth <= 0 || info.biWidth > 8192 ||
        info.biHeight <= 0 || info.biHeight > 8192) {
        fclose(file); return NULL;
    }
    size_t bytes = (size_t)info.biWidth * (size_t)info.biHeight * 4;
    unsigned char *pixels = malloc(bytes);
    if (!pixels || fseek(file, header.bfOffBits, SEEK_SET) != 0 ||
        fread(pixels, bytes, 1, file) != 1) {
        free(pixels); fclose(file); return NULL;
    }
    fclose(file);
    *width = info.biWidth; *height = info.biHeight;
    return pixels;
}

static int load_replay(const char *directory)
{
    char path[MAX_PATH], line[512];
    int x = 0, y = 0, width = 0, height = 0;
    marker_path(path, sizeof(path), directory, "SkillshotCityVR-stereo-diagnostics.txt");
    FILE *report = fopen(path, "rb");
    if (!report) return 0;
    while (fgets(line, sizeof(line), report)) {
        if (sscanf(line, "interface_rect=%d,%d %dx%d", &x, &y, &width, &height) == 4)
            break;
    }
    fclose(report);
    int rgb_width = 0, rgb_height = 0, mask_width = 0, mask_height = 0;
    marker_path(path, sizeof(path), directory, "SkillshotCityVR-interface-alpha.bmp");
    unsigned char *rgb = read_proof_pixels(path, &rgb_width, &rgb_height);
    marker_path(path, sizeof(path), directory, "SkillshotCityVR-interface-alpha-mask.bmp");
    unsigned char *mask = read_proof_pixels(path, &mask_width, &mask_height);
    if (!rgb || !mask || rgb_width != mask_width || rgb_height != mask_height ||
        x < 0 || y < 0 || width <= 0 || height <= 0 ||
        width > rgb_width || height > rgb_height ||
        x > rgb_width - width || y > rgb_height - height) {
        free(rgb); free(mask); return 0;
    }
    unsigned char *rgba = malloc((size_t)width * height * 4);
    if (!rgba) { free(rgb); free(mask); return 0; }
    for (int row = 0; row < height; ++row) {
        for (int column = 0; column < width; ++column) {
            size_t source = ((size_t)(rgb_height - 1 - y - row) * rgb_width + x + column) * 4;
            size_t target = ((size_t)row * width + column) * 4;
            rgba[target] = rgb[source + 2];
            rgba[target + 1] = rgb[source + 1];
            rgba[target + 2] = rgb[source];
            rgba[target + 3] = mask[source];
        }
    }
    p_glGenTextures(1, &replay_texture);
    p_glBindTexture(GL_TEXTURE_2D, replay_texture);
    p_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    p_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    p_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, 0x812F /* CLAMP_TO_EDGE */);
    p_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, 0x812F);
    p_glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0,
                   GL_RGBA, GL_UNSIGNED_BYTE, rgba);
    p_glBindTexture(GL_TEXTURE_2D, 0);
    free(rgba); free(rgb); free(mask);
    return replay_texture != 0;
}

/* Read the actual transparent capture, preserving the game's read target.
   Clearing only an inherited scissor rectangle leaves last frame's menu here. */
static int capture_is_clear(void)
{
    GLint previous_read = 0;
    unsigned char pixel[4] = {255, 255, 255, 255};
    p_glGetIntegerv(0x8CAA /* GL_READ_FRAMEBUFFER_BINDING */, &previous_read);
    p_glBindFramebuffer(0x8CA8 /* GL_READ_FRAMEBUFFER */,
                        openxr_bridge_interface_alpha_framebuffer());
    p_glReadPixels(480, 270, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
    p_glBindFramebuffer(0x8CA8, (GLuint)previous_read);
    return pixel[0] == 0 && pixel[1] == 0 && pixel[2] == 0 && pixel[3] == 0;
}

static int archive_proof(const char *base, const char *name)
{
    const char *files[] = {
        "SkillshotCityVR-geometry-left.bmp",
        "SkillshotCityVR-geometry-right.bmp",
        "SkillshotCityVR-final-interface-left.bmp",
        "SkillshotCityVR-final-interface-right.bmp",
        "SkillshotCityVR-presented-left.bmp",
        "SkillshotCityVR-presented-right.bmp",
        "SkillshotCityVR-hud-base-left.bmp",
        "SkillshotCityVR-hud-base-right.bmp",
        "SkillshotCityVR-interface-alpha.bmp",
        "SkillshotCityVR-interface-alpha-mask.bmp"
    };
    char directory[MAX_PATH], source[MAX_PATH], destination[MAX_PATH];
    marker_path(directory, sizeof(directory), base, name);
    CreateDirectoryA(directory, NULL);
    for (size_t i = 0; i < sizeof(files) / sizeof(files[0]); ++i) {
        marker_path(source, sizeof(source), base, files[i]);
        marker_path(destination, sizeof(destination), directory, files[i]);
        int copied = 0;
        for (int attempt = 0; attempt < 500 && !copied; ++attempt) {
            copied = CopyFileA(source, destination, FALSE);
            if (!copied) Sleep(10);
        }
        if (!copied) return 0;
    }
    return 1;
}

static int archive_center_proof(const char *base)
{
    const char *files[] = {
        "SkillshotCityVR-geometry-left.bmp",
        "SkillshotCityVR-geometry-right.bmp",
        "SkillshotCityVR-presented-left.bmp",
        "SkillshotCityVR-presented-right.bmp",
        "SkillshotCityVR-hud-base-left.bmp",
        "SkillshotCityVR-hud-base-right.bmp"
    };
    char directory[MAX_PATH], source[MAX_PATH], destination[MAX_PATH];
    marker_path(directory, sizeof(directory), base, "proof-center");
    CreateDirectoryA(directory, NULL);
    for (size_t i = 0; i < sizeof(files) / sizeof(files[0]); ++i) {
        marker_path(source, sizeof(source), base, files[i]);
        marker_path(destination, sizeof(destination), directory, files[i]);
        int copied = 0;
        for (int attempt = 0; attempt < 50 && !copied; ++attempt) {
            copied = CopyFileA(source, destination, FALSE);
            if (!copied) Sleep(10);
        }
        if (!copied) return 0;
    }
    return 1;
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
    p_glBindFramebuffer = (void *)p_wglGetProcAddress("glBindFramebuffer");
    if (!p_glBindFramebuffer) return 11;
    const char *replay_directory = getenv("SKILLSHOTVR_INTEGRATION_UI_REPLAY");
    if (replay_directory && replay_directory[0] && !load_replay(replay_directory)) return 12;
    if (!getenv("SKILLSHOTVR_INTEGRATION_HIDDEN")) ShowWindow(window, SW_SHOWNA);

    int published_pairs = 0;
    int menu_frames = 0;
    int alpha_mask_failures = 0;
    int capture_clear_failures = 0;
    int scissor_restore_failures = 0;
    int capture_begin_failures = 0;
    int menu_publish_failures = 0;
    int proof_archive_failures = 0;
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
            int center_announcement = frame >= 500 && frame < 900;
            if (center_announcement) draw_center_announcement_test(eye);
            openxr_bridge_capture_native_mirror(1);
            int eye_width = 0, eye_height = 0;
            openxr_bridge_get_eye_size(&eye_width, &eye_height);
            float hud_scale_y = (float)vr_hud_scale_y_for_eye(
                0.66, (double)eye_width, (double)eye_height, 16.0 / 9.0);
            openxr_bridge_capture_hud_base(eye, 0.66f, hud_scale_y);
            if (center_announcement) draw_center_announcement();
            int captured = openxr_bridge_capture_eye(eye);
            if (captured && eye == 0) ++published_pairs;
            p_glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            /* SSC disables destination alpha writes for its ordinary window.
               Reproduce that state so an RGB-only invisible menu cannot pass. */
            p_glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);
            p_glScissor(8, 8, 16, 16);
            p_glEnable(GL_SCISSOR_TEST);
            if (!openxr_bridge_begin_interface_alpha_capture())
                ++capture_begin_failures;
            else if (!capture_is_clear()) ++capture_clear_failures;
            if (!p_glIsEnabled(GL_SCISSOR_TEST)) ++scissor_restore_failures;
            p_glDisable(GL_SCISSOR_TEST);
            GLint capture_mask[4];
            p_glGetIntegerv(GL_COLOR_WRITEMASK, capture_mask);
            if (!capture_mask[3]) ++alpha_mask_failures;
            int heavy = frame >= 1000 && (((frame - 1000) / 100) & 1) == 0;
            if (frame == 1000) {
                char proof_marker[MAX_PATH];
                openxr_bridge_request_fresh_geometry_proof();
                marker_path(proof_marker, sizeof(proof_marker), base,
                            "SkillshotCityVR-geometry-proof.txt");
                write_marker(proof_marker, "capture completed interface eyes\n");
            }
            if (frame == 600) {
                char proof_marker[MAX_PATH];
                openxr_bridge_request_fresh_geometry_proof();
                marker_path(proof_marker, sizeof(proof_marker), base,
                            "SkillshotCityVR-geometry-proof.txt");
                write_marker(proof_marker,
                             "capture ordinary center announcement pair\n");
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
                draw_interface(frame);
                ++menu_frames;
            }
            /* Internal transfer must ignore the game's current clip while
               leaving it enabled for the next authored draw. */
            p_glEnable(GL_SCISSOR_TEST);
            if (!openxr_bridge_finish_interface_alpha_capture(heavy) && heavy)
                ++menu_publish_failures;
            if (!p_glIsEnabled(GL_SCISSOR_TEST)) ++scissor_restore_failures;
            p_glDisable(GL_SCISSOR_TEST);
            p_glGetIntegerv(GL_COLOR_WRITEMASK, capture_mask);
            if (capture_mask[3]) ++alpha_mask_failures;
            /* Exercise the real round-complete handoff: geometry disappears,
               the completed desktop frame is captured, and OpenXR must use a
               single flat quad rather than projecting identical pixels from
               two eye poses. */
            int round_transition = frame >= 2050 && frame < 2120;
            openxr_bridge_set_geometry_active(!round_transition);
            if (round_transition) openxr_bridge_capture_flat_frame();
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
        if (frame == 800 && !archive_center_proof(base))
            ++proof_archive_failures;
        if (frame == 1200 && !archive_proof(base, "proof-first"))
            ++proof_archive_failures;
        if (frame == 1600 && !archive_proof(base, "proof-second"))
            ++proof_archive_failures;
        Sleep(10);
    }

    int passed = published_pairs >= frame_limit / 4 && menu_frames >= 100 &&
        alpha_mask_failures == 0 && capture_clear_failures == 0 &&
        scissor_restore_failures == 0 && capture_begin_failures == 0 &&
        menu_publish_failures == 0 && proof_archive_failures == 0;
    DeleteFileA(openxr_marker);
    DeleteFileA(geometry_marker);
    printf("integration published_pairs=%d menu_frames=%d status=%s\n",
           published_pairs, menu_frames, openxr_bridge_status());
    fflush(stdout);
    int shutdown_once = openxr_bridge_shutdown();
    int shutdown_twice = openxr_bridge_shutdown();
    printf("integration shutdown_once=%d shutdown_twice=%d status=%s\n",
           shutdown_once, shutdown_twice, openxr_bridge_status());
    fflush(stdout);
    if (!shutdown_once || !shutdown_twice) passed = 0;
    char result_path[MAX_PATH];
    marker_path(result_path, sizeof(result_path), base, "integration-result.txt");
    FILE *result = fopen(result_path, "wb");
    if (result) {
        fprintf(result, "passed=%d published_pairs=%d menu_frames=%d alpha_mask_failures=%d "
                "capture_clear_failures=%d scissor_restore_failures=%d "
                "capture_begin_failures=%d menu_publish_failures=%d "
                "proof_archive_failures=%d shutdown_once=%d shutdown_twice=%d status=%s\n",
                passed, published_pairs, menu_frames, alpha_mask_failures,
                capture_clear_failures, scissor_restore_failures,
                capture_begin_failures, menu_publish_failures,
                proof_archive_failures, shutdown_once, shutdown_twice,
                openxr_bridge_status());
        fclose(result);
    }
    p_wglMakeCurrent(NULL, NULL);
    p_wglDeleteContext(context);
    ReleaseDC(window, dc);
    DestroyWindow(window);
    return passed ? 0 : 8;
}
