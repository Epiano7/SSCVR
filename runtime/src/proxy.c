#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "openxr_bridge.h"
#include "geometry_math.h"
#include "game_build_layout.h"

#define GL_VENDOR 0x1F00
#define GL_RENDERER 0x1F01
#define GL_VERSION 0x1F02
#define GL_VIEWPORT 0x0BA2
#define GL_READ_BUFFER 0x0C02
#define GL_PACK_ALIGNMENT 0x0D05
#define GL_MODELVIEW_MATRIX 0x0BA6
#define GL_PROJECTION_MATRIX 0x0BA7
#define GL_MODELVIEW 0x1700
#define GL_PROJECTION 0x1701
#define GL_DEPTH_TEST 0x0B71
#define GL_BLEND 0x0BE2
#define GL_SCISSOR_TEST 0x0C11
#define GL_SCISSOR_BOX 0x0C10
#define GL_BACK 0x0405
#define GL_RGB 0x1907
#define GL_UNSIGNED_BYTE 0x1401
#define GL_FRAMEBUFFER 0x8D40
#define GL_DRAW_FRAMEBUFFER 0x8CA9
#define GL_READ_FRAMEBUFFER 0x8CA8
#define GL_DRAW_FRAMEBUFFER_BINDING 0x8CA6
#define GL_READ_FRAMEBUFFER_BINDING 0x8CAA
#define GL_SAMPLES 0x80A9
#define GL_COLOR_ATTACHMENT0 0x8CE0
#define GL_DEPTH_COMPONENT 0x1902
#define GL_DEPTH_BUFFER_BIT 0x00000100
#define GL_COLOR_BUFFER_BIT 0x00004000
#define GL_NEAREST 0x2600
#define GL_FLOAT 0x1406

typedef const unsigned char *(APIENTRY *PFNGLGETSTRING)(unsigned int);
typedef void (APIENTRY *PFNGLGETINTEGERV)(unsigned int, int *);
typedef void (APIENTRY *PFNGLGETFLOATV)(unsigned int, float *);
typedef unsigned char (APIENTRY *PFNGLISENABLED)(unsigned int);
typedef void (APIENTRY *PFNGLPIXELSTOREI)(unsigned int, int);
typedef void (APIENTRY *PFNGLREADBUFFER)(unsigned int);
typedef void (APIENTRY *PFNGLREADPIXELS)(int, int, int, int, unsigned int, unsigned int, void *);
typedef PROC (WINAPI *PFNWGLGETPROCADDRESS)(LPCSTR);
typedef HGLRC (WINAPI *PFNWGLCREATECONTEXTATTRIBSARB)(HDC, HGLRC, const int *);
typedef unsigned int (APIENTRY *PFNGLCREATESHADER)(unsigned int);
typedef unsigned int (APIENTRY *PFNGLCREATEPROGRAM)(void);
typedef void (APIENTRY *PFNGLSHADERSOURCE)(unsigned int, int, const char *const *, const int *);
typedef void (APIENTRY *PFNGLATTACHSHADER)(unsigned int, unsigned int);
typedef void (APIENTRY *PFNGLLINKPROGRAM)(unsigned int);
typedef void (APIENTRY *PFNGLUSEPROGRAM)(unsigned int);
typedef void (APIENTRY *PFNGLBINDFRAMEBUFFER)(unsigned int, unsigned int);
typedef void (APIENTRY *PFNGLDRAWARRAYS)(unsigned int, int, int);
typedef void (APIENTRY *PFNGLDRAWELEMENTS)(unsigned int, int, unsigned int, const void *);
typedef void (APIENTRY *PFNGLDRAWRANGEELEMENTS)(unsigned int, unsigned int, unsigned int, int, unsigned int, const void *);
typedef void (APIENTRY *PFNGLDRAWARRAYSINSTANCED)(unsigned int, int, int, int);
typedef void (APIENTRY *PFNGLDRAWELEMENTSINSTANCED)(unsigned int, int, unsigned int, const void *, int);
typedef void (APIENTRY *PFNGLDRAWELEMENTSBASEVERTEX)(unsigned int, int, unsigned int, const void *, int);
typedef void (APIENTRY *PFNGLDRAWRANGEELEMENTSBASEVERTEX)(unsigned int, unsigned int, unsigned int, int, unsigned int, const void *, int);
typedef void (APIENTRY *PFNGLDRAWELEMENTSINSTANCEDBASEVERTEX)(unsigned int, int, unsigned int, const void *, int, int);
typedef void (APIENTRY *PFNGLMULTIDRAWARRAYS)(unsigned int, const int *, const int *, int);
typedef void (APIENTRY *PFNGLMULTIDRAWELEMENTS)(unsigned int, const int *, unsigned int, const void *const *, int);
typedef void (APIENTRY *PFNGLMULTIDRAWELEMENTSBASEVERTEX)(unsigned int, const int *, unsigned int, const void *const *, int, const int *);
typedef void (APIENTRY *PFNGLBLITFRAMEBUFFER)(int, int, int, int, int, int, int, int, unsigned int, unsigned int);
typedef void (APIENTRY *PFNGLBEGIN)(unsigned int);
typedef void (APIENTRY *PFNGLEND)(void);
typedef void (APIENTRY *PFNGLCALLLIST)(unsigned int);
typedef void (APIENTRY *PFNGLCALLLISTS)(int, unsigned int, const void *);
typedef void (APIENTRY *PFNGLMATRIXMODE)(unsigned int);
typedef void (APIENTRY *PFNGLLOADIDENTITY)(void);
typedef void (APIENTRY *PFNGLLOADMATRIXF)(const float *);
typedef void (APIENTRY *PFNGLLOADMATRIXD)(const double *);
typedef void (APIENTRY *PFNGLMULTMATRIXF)(const float *);
typedef void (APIENTRY *PFNGLMULTMATRIXD)(const double *);
typedef void (APIENTRY *PFNGLFRUSTUM)(double, double, double, double, double, double);
typedef void (APIENTRY *PFNGLORTHO)(double, double, double, double, double, double);
typedef void (APIENTRY *PFNGLTRANSLATEF)(float, float, float);
typedef void (APIENTRY *PFNGLSCALEF)(float, float, float);
typedef void (APIENTRY *PFNGLVIEWPORT)(int, int, int, int);
typedef void (APIENTRY *PFNGLBINDRENDERBUFFER)(unsigned int, unsigned int);
typedef void (APIENTRY *PFNGLRENDERBUFFERSTORAGEMULTISAMPLE)(unsigned int, int, unsigned int, int, int);
typedef void (APIENTRY *PFNGLFRAMEBUFFERRENDERBUFFER)(unsigned int, unsigned int, unsigned int, unsigned int);
typedef void (APIENTRY *PFNGLBINDTEXTURE)(unsigned int, unsigned int);
typedef void (APIENTRY *PFNGLTEXIMAGE2DMULTISAMPLE)(unsigned int, int, unsigned int, int, int, unsigned char);
typedef void (APIENTRY *PFNGLFRAMEBUFFERTEXTURE2D)(unsigned int, unsigned int, unsigned int, unsigned int, int);
typedef void (APIENTRY *PFNGLBLENDFUNC)(unsigned int, unsigned int);
typedef void (APIENTRY *PFNGLBLENDFUNCSEPARATE)(unsigned int, unsigned int, unsigned int, unsigned int);
typedef void (APIENTRY *PFNGLBLENDEQUATION)(unsigned int);
typedef void (APIENTRY *PFNGLBLENDEQUATIONSEPARATE)(unsigned int, unsigned int);
typedef void (APIENTRY *PFNGLCLEAR)(unsigned int);
typedef BOOL (WINAPI *PFNWGLSWAPINTERVALEXT)(int);

static HMODULE g_real_gl;
static BOOL (WINAPI *g_real_swap_buffers)(HDC);
static PFNGLGETSTRING g_gl_get_string;
static PFNGLGETINTEGERV g_gl_get_integerv;
static PFNGLGETFLOATV g_gl_get_floatv;
static PFNGLISENABLED g_gl_is_enabled;
static PFNGLPIXELSTOREI g_gl_pixel_store_i;
static PFNGLREADBUFFER g_gl_read_buffer;
static PFNGLREADPIXELS g_gl_read_pixels;
static PFNWGLGETPROCADDRESS g_real_wgl_get_proc_address;
static PFNWGLCREATECONTEXTATTRIBSARB g_real_wgl_create_context_attribs;
static PFNWGLSWAPINTERVALEXT g_real_wgl_swap_interval;
static LONG g_vr_swap_interval_logged;
static PFNGLCREATESHADER g_real_gl_create_shader;
static PFNGLCREATEPROGRAM g_real_gl_create_program;
static PFNGLSHADERSOURCE g_real_gl_shader_source;
static PFNGLATTACHSHADER g_real_gl_attach_shader;
static PFNGLLINKPROGRAM g_real_gl_link_program;
static PFNGLUSEPROGRAM g_real_gl_use_program;
static PFNGLBINDFRAMEBUFFER g_real_gl_bind_framebuffer;
static PFNGLDRAWARRAYS g_real_gl_draw_arrays;
static PFNGLDRAWELEMENTS g_real_gl_draw_elements;
static PFNGLDRAWRANGEELEMENTS g_real_gl_draw_range_elements;
static PFNGLDRAWARRAYSINSTANCED g_real_gl_draw_arrays_instanced;
static PFNGLDRAWELEMENTSINSTANCED g_real_gl_draw_elements_instanced;
static PFNGLDRAWELEMENTSBASEVERTEX g_real_gl_draw_elements_base_vertex;
static PFNGLDRAWRANGEELEMENTSBASEVERTEX g_real_gl_draw_range_elements_base_vertex;
static PFNGLDRAWELEMENTSINSTANCEDBASEVERTEX g_real_gl_draw_elements_instanced_base_vertex;
static PFNGLMULTIDRAWARRAYS g_real_gl_multi_draw_arrays;
static PFNGLMULTIDRAWELEMENTS g_real_gl_multi_draw_elements;
static PFNGLMULTIDRAWELEMENTSBASEVERTEX g_real_gl_multi_draw_elements_base_vertex;
static PFNGLBLITFRAMEBUFFER g_real_gl_blit_framebuffer;
static PFNGLBEGIN g_real_gl_begin;
static PFNGLEND g_real_gl_end;
static PFNGLCALLLIST g_real_gl_call_list;
static PFNGLCALLLISTS g_real_gl_call_lists;
static PFNGLMATRIXMODE g_real_gl_matrix_mode;
static PFNGLLOADIDENTITY g_real_gl_load_identity;
static PFNGLLOADMATRIXF g_real_gl_load_matrix_f;
static PFNGLLOADMATRIXD g_real_gl_load_matrix_d;
static PFNGLMULTMATRIXF g_real_gl_mult_matrix_f;
static PFNGLMULTMATRIXD g_real_gl_mult_matrix_d;
static PFNGLFRUSTUM g_real_gl_frustum;
static PFNGLORTHO g_real_gl_ortho;
static PFNGLTRANSLATEF g_real_gl_translate_f;
static PFNGLSCALEF g_real_gl_scale_f;
static PFNGLVIEWPORT g_real_gl_viewport;
static PFNGLBINDRENDERBUFFER g_real_gl_bind_renderbuffer;
static PFNGLRENDERBUFFERSTORAGEMULTISAMPLE g_real_gl_renderbuffer_storage_multisample;
static PFNGLFRAMEBUFFERRENDERBUFFER g_real_gl_framebuffer_renderbuffer;
static PFNGLBINDTEXTURE g_real_gl_bind_texture;
static PFNGLTEXIMAGE2DMULTISAMPLE g_real_gl_tex_image_2d_multisample;
static PFNGLFRAMEBUFFERTEXTURE2D g_real_gl_framebuffer_texture_2d;
static PFNGLBLENDFUNC g_real_gl_blend_func;
static PFNGLBLENDFUNCSEPARATE g_real_gl_blend_func_separate;
static PFNGLBLENDEQUATION g_real_gl_blend_equation;
static PFNGLBLENDEQUATIONSEPARATE g_real_gl_blend_equation_separate;
static PFNGLCLEAR g_real_gl_clear;
static char g_directory[MAX_PATH];
static LONG g_frame_count;
static LONG g_initialized;
static unsigned int g_current_program;
static unsigned int g_draw_framebuffer;
static unsigned int g_read_framebuffer;

typedef struct {
    unsigned int program;
    unsigned int framebuffer;
    unsigned int texture_2d;
    unsigned int object_id;
    unsigned int mode;
    int count;
    char kind;
    unsigned char depth_test;
    unsigned char blend;
    unsigned char scissor_test;
    int viewport[4];
    int scissor[4];
    float projection[16];
    float modelview[16];
} DrawEvent;

static DrawEvent g_draw_events[8192];
static LONG g_draw_event_count;
static volatile LONG g_draw_capture_active;
static int g_test_held_key;
static ULONGLONG g_test_key_release_tick;

typedef struct {
    unsigned int shader;
    unsigned int type;
} ShaderType;

static ShaderType g_shader_types[256];
static LONG g_shader_type_count;
static int g_debug_skip_mode;
static unsigned int g_matrix_mode = GL_MODELVIEW;
static LONG g_projection_trace_count;
static int g_overscan_enabled;
static LONG g_overscan_logged;
static unsigned int g_bound_renderbuffer;
static unsigned int g_bound_texture_2d;
static unsigned int g_bound_texture_2d_multisample;
static unsigned int g_draw_object_id;
static LONG g_world_capture_frame = -1;
static LONG g_geometry_blit_trace_count;
static LONG g_geometry_bind_trace_count;
static LONG g_world_draw_trace_count;
static LONGLONG g_perf_render_ticks;
static LONGLONG g_perf_depth_ticks;
static LONGLONG g_perf_mirror_ticks;
static LONGLONG g_perf_capture_ticks;
static LONGLONG g_perf_total_ticks;
static LONGLONG g_perf_total_max_ticks;
static LONGLONG g_perf_frequency;
static LONG g_perf_eye_samples;
static LONG g_perf_slow_eye_samples;
static volatile LONG g_interface_draw_calls_current;
static volatile LONG64 g_interface_vertices_current;
static int g_projection_is_orthographic;
static int g_indicator_hide_follow_draws;
static int g_indicator_hide_matrix_active;
static float g_indicator_hide_saved_matrix[16];
static LONG g_indicator_shared_logged;
static int g_singleplayer_guard_enabled;
static int (WSAAPI *g_real_connect)(SOCKET, const struct sockaddr *, int);
static int (WSAAPI *g_real_sendto)(SOCKET, const char *, int, int, const struct sockaddr *, int);
static int (WSAAPI *g_real_wsa_send_to)(SOCKET, LPWSABUF, DWORD, LPDWORD, DWORD,
                                        const struct sockaddr *, int, LPWSAOVERLAPPED,
                                        LPWSAOVERLAPPED_COMPLETION_ROUTINE);
static LONG g_blocked_network_calls;
static int g_vr_runtime_disabled;
static ULONGLONG g_vr_window_resize_tick;
static HWND g_game_window;
static int g_geometry_hook_requested;
static LONG g_geometry_hook_attempted;
static int g_geometry_eye = -1;
/* The gameplay hook returns before the game appends full-screen interfaces.
   The entire appended pass is redirected to a transparent offscreen target,
   so its authored color and coverage are retained without any world pixels. */
static int g_interface_alpha_capture_active;
static int g_full_interface_capture_requested;
static int g_pending_eye_view_offset;
static float g_pending_eye_convergence;
static LONG g_eye_transform_trace_count;
static float g_world_eye_separation = 120.0f;
static float g_world_convergence_distance = 8000.0f;
static int g_auto_convergence = 1;
static float g_geometry_vertical_fov_scale = 1.98f;
static float g_depth_exaggeration = 1.0f;
static float g_head_translation_scale = 80.0f;
static float g_peripheral_cull_scale = 1.0f;
static float g_peripheral_cull_target = 1.0f;
static float g_peripheral_cull_minimum = 1.0f;
static float g_tabletop_pitch_degrees = 20.0f;
static float g_tabletop_horizontal_offset;
static float g_tabletop_vertical_offset = -320.0f;
static float g_tabletop_pivot_distance = 3360.0f;
static float g_hud_safe_scale_x = 0.66f;
static float g_hud_safe_scale_y = 0.400f;
static int g_geometry_seen_perspective;
static int g_geometry_hud_base_captured;
static double g_geometry_projection_aspect;
static LONG g_runtime_fov_logged;
static LONG g_hud_safe_logged;
static LONG g_hud_optical_center_logged;
static int g_hud_preview_enabled;
static LONG g_geometry_camera_trace_count;
static float g_head_translation_x;
static float g_head_translation_y;
static float g_geometry_pair_head_view[16] = {
    1, 0, 0, 0, 0, 1, 0, 0,
    0, 0, 1, 0, 0, 0, 0, 1
};
static LONG g_geometry_pair_head_logged;
static LONG g_gameplay_render_reentry;
static LONG g_geometry_first_entry_logged;
static LONG g_geometry_first_pass_logged;
static LONG g_geometry_first_capture_logged;
static LONG g_geometry_alternating_eye;
static int g_geometry_has_presented_eye;
static ULONGLONG g_geometry_last_render_tick;
/* During a full interface, render the ordinary game camera once and capture
   its world immediately before the first orthographic HUD pass. This gives
   the VR menu compositor a clean subtraction baseline while keeping the
   desktop window stable instead of alternating left/right eye cameras. */
static int g_menu_baseline_pending;
static int g_menu_baseline_captured;
static LONG g_menu_baseline_logged;
typedef void (*PFNGAMEPLAYRENDER)(void);
static PFNGAMEPLAYRENDER g_original_gameplay_render;
static const VrGameBuildLayout *g_game_build_layout;
typedef void (*PFNWORLDDRAW)(uint64_t render_target, void *focus_position);
static PFNWORLDDRAW g_original_world_draw;
static LONG g_peripheral_cull_logged;
static WNDPROC g_original_game_window_proc;
static HWND g_camera_input_window;
static int g_camera_drag_active;
static int g_camera_ignore_warp_move;
static POINT g_camera_drag_anchor;
static ULONGLONG g_camera_save_due;
static int g_cursor_clip_active;
static int g_cursor_clip_suspended;
static RECT g_cursor_clip_rectangle;
static void log_line(const char *format, ...);

static void release_game_cursor_clip(const char *reason)
{
    if (!g_cursor_clip_active) return;
    ClipCursor(NULL);
    g_cursor_clip_active = 0;
    if (reason) log_line("Gameplay cursor released: %s", reason);
}

static void update_game_cursor_clip(int gameplay_active, int interface_light)
{
    HWND window = g_game_window;
    if (!gameplay_active) {
        g_cursor_clip_suspended = 0;
        release_game_cursor_clip("menu or gameplay transition");
        return;
    }
    if (!window || !IsWindow(window) || GetForegroundWindow() != window) {
        release_game_cursor_clip("game window not focused");
        return;
    }
    if (g_cursor_clip_suspended || !interface_light) {
        release_game_cursor_clip(g_cursor_clip_suspended ? "Escape/menu requested" : "full-screen interface active");
        return;
    }
    RECT client;
    POINT top_left, bottom_right;
    if (!GetClientRect(window, &client)) return;
    top_left.x = client.left;
    top_left.y = client.top;
    bottom_right.x = client.right;
    bottom_right.y = client.bottom;
    if (!ClientToScreen(window, &top_left) || !ClientToScreen(window, &bottom_right) ||
        bottom_right.x <= top_left.x || bottom_right.y <= top_left.y) return;
    RECT screen = {top_left.x, top_left.y, bottom_right.x, bottom_right.y};
    if (!g_cursor_clip_active || memcmp(&screen, &g_cursor_clip_rectangle, sizeof(screen)) != 0) {
        if (ClipCursor(&screen)) {
            g_cursor_clip_rectangle = screen;
            if (!g_cursor_clip_active)
                log_line("Gameplay cursor confined to focused client area");
            g_cursor_clip_active = 1;
        }
    }
}

static void clamp_camera_tuning(void)
{
    if (g_tabletop_pitch_degrees < 10.0f) g_tabletop_pitch_degrees = 10.0f;
    if (g_tabletop_pitch_degrees > 65.0f) g_tabletop_pitch_degrees = 65.0f;
    if (g_tabletop_horizontal_offset < -2400.0f) g_tabletop_horizontal_offset = -2400.0f;
    if (g_tabletop_horizontal_offset > 2400.0f) g_tabletop_horizontal_offset = 2400.0f;
    if (g_tabletop_vertical_offset < -1600.0f) g_tabletop_vertical_offset = -1600.0f;
    if (g_tabletop_vertical_offset > 1000.0f) g_tabletop_vertical_offset = 1000.0f;
    if (g_geometry_vertical_fov_scale < 1.0f) g_geometry_vertical_fov_scale = 1.0f;
    if (g_geometry_vertical_fov_scale > 2.5f) g_geometry_vertical_fov_scale = 2.5f;
}

static void show_camera_tuning_in_title(const char *state)
{
    if (!g_camera_input_window) return;
    char title[512];
    snprintf(title, sizeof(title),
             "Skillshot City VR | Camera %s | angle %.0f deg | zoom %.2f | pan %.0f, %.0f | Ctrl+Shift+Alt: mouse pan, wheel zoom; Ctrl+Alt+Home reset",
             state ? state : "adjust", g_tabletop_pitch_degrees,
             g_geometry_vertical_fov_scale, g_tabletop_horizontal_offset,
             g_tabletop_vertical_offset);
    SetWindowTextA(g_camera_input_window, title);
}

static int save_camera_tuning(void)
{
    char path[MAX_PATH];
    snprintf(path, sizeof(path), "%s\\SkillshotCityVR-camera-live.ini", g_directory);
    FILE *file = fopen(path, "w");
    if (!file) return 0;
    fprintf(file, "# User-saved isolated VR camera\n");
    fprintf(file, "vertical_fov_scale=%.3f\n", g_geometry_vertical_fov_scale);
    fprintf(file, "tabletop_pitch_degrees=%.3f\n", g_tabletop_pitch_degrees);
    fprintf(file, "tabletop_horizontal_offset=%.3f\n", g_tabletop_horizontal_offset);
    fprintf(file, "tabletop_vertical_offset=%.3f\n", g_tabletop_vertical_offset);
    fprintf(file, "tabletop_pivot_distance=%.3f\n", g_tabletop_pivot_distance);
    fclose(file);
    return 1;
}

static void load_camera_tuning(void)
{
    char path[MAX_PATH];
    snprintf(path, sizeof(path), "%s\\SkillshotCityVR-camera-live.ini", g_directory);
    FILE *file = fopen(path, "r");
    if (!file) return;
    char line[256];
    while (fgets(line, sizeof(line), file)) {
        char key[64] = {0};
        float value = 0.0f;
        if (sscanf(line, " %63[^=]=%f", key, &value) != 2) continue;
        size_t length = strlen(key);
        while (length && (key[length - 1] == ' ' || key[length - 1] == '\t'))
            key[--length] = '\0';
        if (strcmp(key, "vertical_fov_scale") == 0 && value >= 1.0f && value <= 2.5f)
            g_geometry_vertical_fov_scale = value;
        if (strcmp(key, "tabletop_pitch_degrees") == 0 && value >= 10.0f && value <= 65.0f)
            g_tabletop_pitch_degrees = value;
        if (strcmp(key, "tabletop_horizontal_offset") == 0 && value >= -2400.0f && value <= 2400.0f)
            g_tabletop_horizontal_offset = value;
        if (strcmp(key, "tabletop_vertical_offset") == 0 && value >= -1200.0f && value <= 600.0f)
            g_tabletop_vertical_offset = value;
        if (strcmp(key, "tabletop_pivot_distance") == 0 && value >= 500.0f && value <= 10000.0f)
            g_tabletop_pivot_distance = value;
    }
    fclose(file);
    clamp_camera_tuning();
}

static void camera_tuning_changed(void)
{
    clamp_camera_tuning();
    g_camera_save_due = GetTickCount64() + 350;
    show_camera_tuning_in_title("adjusting");
}

static void finish_camera_chord(void)
{
    if (!g_camera_drag_active) return;
    g_camera_drag_active = 0;
    g_camera_ignore_warp_move = 0;
    if (save_camera_tuning()) {
        g_camera_save_due = 0;
        show_camera_tuning_in_title("saved");
        log_line("Saved chord-only comfort camera: pitch=%.1f pan=%.1f,%.1f zoom=%.2f",
                 g_tabletop_pitch_degrees, g_tabletop_horizontal_offset,
                 g_tabletop_vertical_offset, g_geometry_vertical_fov_scale);
    }
}

static LRESULT CALLBACK camera_window_proc(HWND window, UINT message,
                                              WPARAM w_param, LPARAM l_param)
{
    if (message == WM_KEYDOWN && w_param == VK_ESCAPE && !(l_param & (1L << 30))) {
        g_cursor_clip_suspended = !g_cursor_clip_suspended;
        if (g_cursor_clip_suspended) release_game_cursor_clip("Escape pressed");
    }
    if (message == WM_KILLFOCUS || (message == WM_ACTIVATEAPP && !w_param))
        release_game_cursor_clip("game window lost focus");
    int camera_chord = (GetKeyState(VK_CONTROL) & 0x8000) &&
                       (GetKeyState(VK_SHIFT) & 0x8000) &&
                       (GetKeyState(VK_MENU) & 0x8000);
    if (message == WM_MOUSEMOVE && camera_chord && !g_camera_drag_active) {
        g_camera_drag_active = 1;
        g_camera_drag_anchor.x = (short)LOWORD(l_param);
        g_camera_drag_anchor.y = (short)HIWORD(l_param);
        show_camera_tuning_in_title("adjusting");
        return 0;
    }
    if ((message == WM_MOUSEMOVE && g_camera_drag_active && !camera_chord) ||
        (message == WM_KEYUP && g_camera_drag_active &&
         (w_param == VK_CONTROL || w_param == VK_SHIFT || w_param == VK_MENU)))
        finish_camera_chord();
    if (message == WM_MOUSEMOVE && g_camera_drag_active) {
        if (g_camera_ignore_warp_move) {
            g_camera_ignore_warp_move = 0;
            return 0;
        }
        POINT current = {(short)LOWORD(l_param), (short)HIWORD(l_param)};
        int dx = current.x - g_camera_drag_anchor.x;
        int dy = current.y - g_camera_drag_anchor.y;
        if (dx || dy) {
            g_tabletop_horizontal_offset += (float)dx * 3.0f;
            g_tabletop_vertical_offset -= (float)dy * 3.0f;
            camera_tuning_changed();
            POINT anchor_screen = g_camera_drag_anchor;
            ClientToScreen(window, &anchor_screen);
            g_camera_ignore_warp_move = 1;
            SetCursorPos(anchor_screen.x, anchor_screen.y);
        }
        return 0;
    }
    if (message == WM_MOUSEWHEEL && camera_chord) {
        float steps = (float)(short)HIWORD(w_param) / (float)WHEEL_DELTA;
        g_geometry_vertical_fov_scale -= steps * 0.05f;
        camera_tuning_changed();
        return 0;
    }
    return CallWindowProcA(g_original_game_window_proc, window, message, w_param, l_param);
}

static void ensure_camera_input_window(HWND window)
{
    if (!window || window == g_camera_input_window || !g_geometry_hook_requested) return;
    SetLastError(0);
    LONG_PTR previous = SetWindowLongPtrA(window, GWLP_WNDPROC,
                                          (LONG_PTR)camera_window_proc);
    if (!previous && GetLastError() != 0) {
        log_line("Comfort camera input could not attach: Windows error %lu", GetLastError());
        return;
    }
    g_original_game_window_proc = (WNDPROC)previous;
    g_camera_input_window = window;
    log_line("Chord-only camera controls attached: Ctrl+Shift+Alt + mouse pans; chord+wheel zooms; Ctrl+Alt arrows adjust angle; Ctrl+Alt+Home resets; changes auto-save; middle mouse untouched");
}

static void poll_tabletop_tuning(void)
{
    if (!(GetAsyncKeyState(VK_CONTROL) & 0x8000) ||
        !(GetAsyncKeyState(VK_MENU) & 0x8000)) return;
    int changed = 0;
    if (GetAsyncKeyState(VK_UP) & 1) {
        g_tabletop_pitch_degrees += 2.0f;
        changed = 1;
    }
    if (GetAsyncKeyState(VK_DOWN) & 1) {
        g_tabletop_pitch_degrees -= 2.0f;
        changed = 1;
    }
    if (GetAsyncKeyState(VK_PRIOR) & 1) {
        g_tabletop_vertical_offset += 80.0f;
        changed = 1;
    }
    if (GetAsyncKeyState(VK_NEXT) & 1) {
        g_tabletop_vertical_offset -= 80.0f;
        changed = 1;
    }
    if ((GetAsyncKeyState(VK_ADD) & 1) ||
        (GetAsyncKeyState(VK_OEM_PLUS) & 1)) {
        g_geometry_vertical_fov_scale -= 0.05f;
        changed = 1;
    }
    if ((GetAsyncKeyState(VK_SUBTRACT) & 1) ||
        (GetAsyncKeyState(VK_OEM_MINUS) & 1)) {
        g_geometry_vertical_fov_scale += 0.05f;
        changed = 1;
    }
    if (GetAsyncKeyState(VK_HOME) & 1) {
        g_tabletop_pitch_degrees = 20.0f;
        g_tabletop_horizontal_offset = 0.0f;
        g_tabletop_vertical_offset = -320.0f;
        g_geometry_vertical_fov_scale = 1.65f;
        g_tabletop_pivot_distance = 3360.0f;
        changed = 1;
    }
    clamp_camera_tuning();
    if (GetAsyncKeyState(VK_END) & 1) {
        log_line(save_camera_tuning()
                     ? "Saved VR camera: pitch=%.1f vertical=%.1f zoom=%.2f"
                     : "Could not save VR camera: pitch=%.1f vertical=%.1f zoom=%.2f",
                 g_tabletop_pitch_degrees, g_tabletop_vertical_offset,
                 g_geometry_vertical_fov_scale);
    }
    if (changed)
        log_line("Tabletop tuning: pitch=%.1f vertical=%.1f zoom=%.2f "
                 "(Ctrl+Alt arrows/PageUp/PageDown/Plus/Minus; End saves)",
                 g_tabletop_pitch_degrees, g_tabletop_vertical_offset,
                 g_geometry_vertical_fov_scale);
}

static int patch_gameplay_render(void);
static int patch_world_draw(void);

static int select_game_build_layout(void)
{
    if (g_game_build_layout) return 1;
    unsigned char *base = (unsigned char *)GetModuleHandleW(NULL);
    if (!base) return 0;

    const IMAGE_DOS_HEADER *dos = (const IMAGE_DOS_HEADER *)base;
    if (dos->e_magic != IMAGE_DOS_SIGNATURE || dos->e_lfanew <= 0) {
        log_line("Game build refused: invalid executable image");
        return 0;
    }
    const IMAGE_NT_HEADERS64 *nt =
        (const IMAGE_NT_HEADERS64 *)(base + (size_t)dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE ||
        nt->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
        log_line("Game build refused: invalid 64-bit PE headers");
        return 0;
    }

    g_game_build_layout = vr_find_game_memory_layout(
        base, (size_t)nt->OptionalHeader.SizeOfImage);
    if (!g_game_build_layout) {
        log_line("Game build refused: no supported render-hook signatures matched");
        return 0;
    }
    log_line("Selected game layout: %s (gameplay=0x%zX world=0x%zX culling=0x%zX)",
             g_game_build_layout->name,
             g_game_build_layout->gameplay_render_rva,
             g_game_build_layout->world_draw_rva,
             g_game_build_layout->culling_bounds_rva);
    return 1;
}

static void build_path(char *destination, size_t capacity, const char *filename)
{
    snprintf(destination, capacity, "%s\\%s", g_directory, filename);
}

static void log_line(const char *format, ...)
{
    char path[MAX_PATH];
    build_path(path, sizeof(path), "SkillshotVR.log");
    FILE *file = fopen(path, "a");
    if (!file) return;

    SYSTEMTIME now;
    GetLocalTime(&now);
    fprintf(file, "%02u:%02u:%02u.%03u ", now.wHour, now.wMinute, now.wSecond, now.wMilliseconds);
    va_list arguments;
    va_start(arguments, format);
    vfprintf(file, format, arguments);
    va_end(arguments);
    fputc('\n', file);
    fclose(file);
}

static int endpoint_is_loopback(const struct sockaddr *address, int address_length)
{
    if (!address || address_length < (int)sizeof(address->sa_family)) return 0;
    if (address->sa_family == AF_INET && address_length >= (int)sizeof(struct sockaddr_in)) {
        const struct sockaddr_in *ipv4 = (const struct sockaddr_in *)address;
        return (ntohl(ipv4->sin_addr.s_addr) >> 24) == 127;
    }
    if (address->sa_family == AF_INET6 && address_length >= (int)sizeof(struct sockaddr_in6)) {
        const struct sockaddr_in6 *ipv6 = (const struct sockaddr_in6 *)address;
        static const unsigned char loopback[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};
        if (memcmp(ipv6->sin6_addr.s6_addr, loopback, sizeof(loopback)) == 0) return 1;
        if (IN6_IS_ADDR_V4MAPPED(&ipv6->sin6_addr)) {
            return ipv6->sin6_addr.s6_addr[12] == 127;
        }
    }
    return 0;
}

static void note_blocked_endpoint(const char *operation, const struct sockaddr *address)
{
    LONG count = InterlockedIncrement(&g_blocked_network_calls);
    if (count <= 12) {
        log_line("Single-player guard blocked remote %s (family=%d, count=%ld)", operation,
                 address ? address->sa_family : -1, count);
    }
}

static int WSAAPI guarded_connect(SOCKET socket_handle, const struct sockaddr *address, int address_length)
{
    if (!endpoint_is_loopback(address, address_length)) {
        note_blocked_endpoint("connect", address);
        WSASetLastError(WSAENETUNREACH);
        return SOCKET_ERROR;
    }
    return g_real_connect ? g_real_connect(socket_handle, address, address_length) : SOCKET_ERROR;
}

static int WSAAPI guarded_sendto(SOCKET socket_handle, const char *buffer, int length, int flags,
                                 const struct sockaddr *address, int address_length)
{
    if (address && !endpoint_is_loopback(address, address_length)) {
        note_blocked_endpoint("sendto", address);
        WSASetLastError(WSAENETUNREACH);
        return SOCKET_ERROR;
    }
    return g_real_sendto
        ? g_real_sendto(socket_handle, buffer, length, flags, address, address_length)
        : SOCKET_ERROR;
}

static int WSAAPI guarded_wsa_send_to(SOCKET socket_handle, LPWSABUF buffers, DWORD buffer_count,
                                      LPDWORD bytes_sent, DWORD flags, const struct sockaddr *address,
                                      int address_length, LPWSAOVERLAPPED overlapped,
                                      LPWSAOVERLAPPED_COMPLETION_ROUTINE completion)
{
    if (address && !endpoint_is_loopback(address, address_length)) {
        note_blocked_endpoint("WSASendTo", address);
        WSASetLastError(WSAENETUNREACH);
        return SOCKET_ERROR;
    }
    return g_real_wsa_send_to
        ? g_real_wsa_send_to(socket_handle, buffers, buffer_count, bytes_sent, flags, address,
                             address_length, overlapped, completion)
        : SOCKET_ERROR;
}

static FARPROC real_gl_proc(const char *name)
{
    if (!g_real_gl) g_real_gl = GetModuleHandleA("opengl32_system.dll");
    if (!g_real_gl) g_real_gl = LoadLibraryA("opengl32_system.dll");
    return g_real_gl ? GetProcAddress(g_real_gl, name) : NULL;
}

static void initialize_gl_diagnostics(void)
{
    if (InterlockedCompareExchange(&g_initialized, 1, 0) != 0) return;
    g_gl_get_string = (PFNGLGETSTRING)real_gl_proc("glGetString");
    g_gl_get_integerv = (PFNGLGETINTEGERV)real_gl_proc("glGetIntegerv");
    g_gl_get_floatv = (PFNGLGETFLOATV)real_gl_proc("glGetFloatv");
    g_gl_is_enabled = (PFNGLISENABLED)real_gl_proc("glIsEnabled");
    g_gl_pixel_store_i = (PFNGLPIXELSTOREI)real_gl_proc("glPixelStorei");
    g_gl_read_buffer = (PFNGLREADBUFFER)real_gl_proc("glReadBuffer");
    g_gl_read_pixels = (PFNGLREADPIXELS)real_gl_proc("glReadPixels");
    g_real_wgl_get_proc_address = (PFNWGLGETPROCADDRESS)real_gl_proc("wglGetProcAddress");
    g_real_gl_draw_arrays = (PFNGLDRAWARRAYS)real_gl_proc("glDrawArrays");
    g_real_gl_draw_elements = (PFNGLDRAWELEMENTS)real_gl_proc("glDrawElements");
    g_real_gl_blend_func = (PFNGLBLENDFUNC)real_gl_proc("glBlendFunc");
    g_real_gl_clear = (PFNGLCLEAR)real_gl_proc("glClear");
    if (g_real_wgl_get_proc_address) {
        g_real_gl_blend_func_separate = (PFNGLBLENDFUNCSEPARATE)
            g_real_wgl_get_proc_address("glBlendFuncSeparate");
        g_real_gl_blend_equation = (PFNGLBLENDEQUATION)
            g_real_wgl_get_proc_address("glBlendEquation");
        g_real_gl_blend_equation_separate = (PFNGLBLENDEQUATIONSEPARATE)
            g_real_wgl_get_proc_address("glBlendEquationSeparate");
    }

    const char *vendor = g_gl_get_string ? (const char *)g_gl_get_string(GL_VENDOR) : NULL;
    const char *renderer = g_gl_get_string ? (const char *)g_gl_get_string(GL_RENDERER) : NULL;
    const char *version = g_gl_get_string ? (const char *)g_gl_get_string(GL_VERSION) : NULL;
    int viewport[4] = {0};
    if (g_gl_get_integerv) g_gl_get_integerv(GL_VIEWPORT, viewport);
    log_line("OpenGL initialized: vendor='%s' renderer='%s' version='%s' viewport=%d,%d %dx%d",
             vendor ? vendor : "?", renderer ? renderer : "?", version ? version : "?",
             viewport[0], viewport[1], viewport[2], viewport[3]);
}

static void record_draw(char kind, unsigned int mode, int count)
{
    if (g_projection_is_orthographic) {
        InterlockedIncrement(&g_interface_draw_calls_current);
        InterlockedAdd64(&g_interface_vertices_current, count > 0 ? count : 1);
    }
    if (g_frame_count != 119 && !g_draw_capture_active) return;
    LONG index = InterlockedIncrement(&g_draw_event_count) - 1;
    if (index < 0 || index >= (LONG)(sizeof(g_draw_events) / sizeof(g_draw_events[0]))) return;
    g_draw_events[index].program = g_current_program;
    g_draw_events[index].framebuffer = g_draw_framebuffer;
    int bound_texture = (int)g_bound_texture_2d;
    if (g_gl_get_integerv)
        g_gl_get_integerv(0x8069u /* GL_TEXTURE_BINDING_2D */, &bound_texture);
    g_draw_events[index].texture_2d = (unsigned int)bound_texture;
    g_draw_events[index].object_id = g_draw_object_id;
    g_draw_events[index].mode = mode;
    g_draw_events[index].count = count;
    g_draw_events[index].kind = kind;
    g_draw_events[index].depth_test = g_gl_is_enabled ? g_gl_is_enabled(GL_DEPTH_TEST) : 0;
    g_draw_events[index].blend = g_gl_is_enabled ? g_gl_is_enabled(GL_BLEND) : 0;
    g_draw_events[index].scissor_test = g_gl_is_enabled
        ? g_gl_is_enabled(GL_SCISSOR_TEST) : 0;
    if (g_gl_get_integerv) {
        g_gl_get_integerv(GL_VIEWPORT, g_draw_events[index].viewport);
        g_gl_get_integerv(GL_SCISSOR_BOX, g_draw_events[index].scissor);
    }
    if (g_gl_get_floatv) {
        g_gl_get_floatv(GL_PROJECTION_MATRIX, g_draw_events[index].projection);
        g_gl_get_floatv(GL_MODELVIEW_MATRIX, g_draw_events[index].modelview);
    }
}

static int should_skip_current_draw(void)
{
    if (g_frame_count != 119) return 0;
    if (g_debug_skip_mode == 1) return g_current_program == 4 || g_current_program == 28;
    if (g_debug_skip_mode == 2) return g_current_program == 40 || g_current_program == 43 || g_current_program == 79;
    return 0;
}

static int should_trace_projection(void)
{
    if (g_matrix_mode != GL_PROJECTION ||
        !(g_frame_count <= 3 || (g_frame_count >= 118 && g_frame_count <= 121))) return 0;
    return InterlockedIncrement(&g_projection_trace_count) <= 160;
}

static int is_perspective_f(const float *matrix)
{
    return matrix && matrix[15] > -0.0001f && matrix[15] < 0.0001f &&
           (matrix[11] < -0.5f || matrix[11] > 0.5f);
}

static int is_perspective_d(const double *matrix)
{
    return matrix && matrix[15] > -0.0001 && matrix[15] < 0.0001 &&
           (matrix[11] < -0.5 || matrix[11] > 0.5);
}

static void log_overscan_once(float original_x, float original_y)
{
    if (InterlockedCompareExchange(&g_overscan_logged, 1, 0) == 0)
        log_line("VR expanded camera active: projection scale %.6g,%.6g -> %.6g,%.6g",
                 original_x, original_y, original_x * 0.82f, original_y * 0.82f);
}

__declspec(dllexport) void APIENTRY glMatrixMode(unsigned int mode)
{
    g_matrix_mode = mode;
    if (!g_real_gl_matrix_mode) g_real_gl_matrix_mode = (PFNGLMATRIXMODE)real_gl_proc("glMatrixMode");
    if (g_real_gl_matrix_mode) g_real_gl_matrix_mode(mode);
}

__declspec(dllexport) void APIENTRY glLoadIdentity(void)
{
    if (!g_real_gl_load_identity) g_real_gl_load_identity = (PFNGLLOADIDENTITY)real_gl_proc("glLoadIdentity");
    if (should_trace_projection()) log_line("Projection trace frame=%ld LoadIdentity", g_frame_count + 1);
    if (g_real_gl_load_identity) g_real_gl_load_identity();
    if (g_pending_eye_view_offset && g_geometry_eye >= 0 &&
        g_matrix_mode == GL_MODELVIEW && !g_projection_is_orthographic) {
        if (!g_real_gl_mult_matrix_f)
            g_real_gl_mult_matrix_f = (PFNGLMULTMATRIXF)real_gl_proc("glMultMatrixf");
        if (g_real_gl_mult_matrix_f) {
            float eye_view[16];
            memcpy(eye_view, g_geometry_pair_head_view, sizeof(eye_view));
            /* Eye offset is in the live camera's local X axis. The complete
               head view already contains inverse orientation and XYZ motion. */
            eye_view[12] += (g_geometry_eye == 0 ? 0.5f : -0.5f) *
                            g_world_eye_separation;
            g_real_gl_mult_matrix_f(eye_view);
            g_pending_eye_view_offset = 0;
        }
    }
}

__declspec(dllexport) void APIENTRY glLoadMatrixf(const float *matrix)
{
    if (!g_real_gl_load_matrix_f) g_real_gl_load_matrix_f = (PFNGLLOADMATRIXF)real_gl_proc("glLoadMatrixf");
    if (matrix && g_matrix_mode == GL_PROJECTION)
        g_projection_is_orthographic = !is_perspective_f(matrix);
    if (matrix && should_trace_projection())
        log_line("Projection trace frame=%ld LoadMatrixf p00=%.9g p11=%.9g p22=%.9g p23=%.9g p32=%.9g p33=%.9g",
                 g_frame_count + 1, matrix[0], matrix[5], matrix[10], matrix[11], matrix[14], matrix[15]);
    if (matrix && g_pending_eye_view_offset && g_geometry_eye >= 0 &&
        g_matrix_mode == GL_MODELVIEW && !g_projection_is_orthographic) {
        float shifted[16];
        memcpy(shifted, matrix, sizeof(shifted));
        shifted[12] += (g_geometry_eye == 0 ? 0.5f : -0.5f) * g_world_eye_separation - g_head_translation_x;
        shifted[13] -= g_head_translation_y;
        g_pending_eye_view_offset = 0;
        if (g_real_gl_load_matrix_f) g_real_gl_load_matrix_f(shifted);
    } else if (g_overscan_enabled && openxr_bridge_should_expand_world() &&
        g_matrix_mode == GL_PROJECTION && is_perspective_f(matrix)) {
        float widened[16];
        memcpy(widened, matrix, sizeof(widened));
        widened[0] *= 0.82f;
        widened[5] *= 0.82f;
        log_overscan_once(matrix[0], matrix[5]);
        if (g_real_gl_load_matrix_f) g_real_gl_load_matrix_f(widened);
    } else if (g_real_gl_load_matrix_f) g_real_gl_load_matrix_f(matrix);
}

__declspec(dllexport) void APIENTRY glLoadMatrixd(const double *matrix)
{
    if (!g_real_gl_load_matrix_d) g_real_gl_load_matrix_d = (PFNGLLOADMATRIXD)real_gl_proc("glLoadMatrixd");
    if (matrix && g_matrix_mode == GL_PROJECTION)
        g_projection_is_orthographic = !is_perspective_d(matrix);
    if (matrix && should_trace_projection())
        log_line("Projection trace frame=%ld LoadMatrixd p00=%.9g p11=%.9g p22=%.9g p23=%.9g p32=%.9g p33=%.9g",
                 g_frame_count + 1, matrix[0], matrix[5], matrix[10], matrix[11], matrix[14], matrix[15]);
    if (matrix && g_pending_eye_view_offset && g_geometry_eye >= 0 &&
        g_matrix_mode == GL_MODELVIEW && !g_projection_is_orthographic) {
        double shifted[16];
        memcpy(shifted, matrix, sizeof(shifted));
        shifted[12] += (g_geometry_eye == 0 ? 0.5 : -0.5) * g_world_eye_separation - g_head_translation_x;
        shifted[13] -= g_head_translation_y;
        g_pending_eye_view_offset = 0;
        if (g_real_gl_load_matrix_d) g_real_gl_load_matrix_d(shifted);
    } else if (g_overscan_enabled && openxr_bridge_should_expand_world() &&
        g_matrix_mode == GL_PROJECTION && is_perspective_d(matrix)) {
        double widened[16];
        memcpy(widened, matrix, sizeof(widened));
        widened[0] *= 0.82;
        widened[5] *= 0.82;
        log_overscan_once((float)matrix[0], (float)matrix[5]);
        if (g_real_gl_load_matrix_d) g_real_gl_load_matrix_d(widened);
    } else if (g_real_gl_load_matrix_d) g_real_gl_load_matrix_d(matrix);
}

__declspec(dllexport) void APIENTRY glMultMatrixf(const float *matrix)
{
    if (!g_real_gl_mult_matrix_f) g_real_gl_mult_matrix_f = (PFNGLMULTMATRIXF)real_gl_proc("glMultMatrixf");
    if (matrix && g_matrix_mode == GL_PROJECTION)
        g_projection_is_orthographic = !is_perspective_f(matrix);
    if (matrix && should_trace_projection())
        log_line("Projection trace frame=%ld MultMatrixf p00=%.9g p11=%.9g p22=%.9g p23=%.9g p32=%.9g p33=%.9g",
                 g_frame_count + 1, matrix[0], matrix[5], matrix[10], matrix[11], matrix[14], matrix[15]);
    if (g_overscan_enabled && openxr_bridge_should_expand_world() &&
        g_matrix_mode == GL_PROJECTION && is_perspective_f(matrix)) {
        float widened[16];
        memcpy(widened, matrix, sizeof(widened));
        widened[0] *= 0.82f;
        widened[5] *= 0.82f;
        log_overscan_once(matrix[0], matrix[5]);
        if (g_real_gl_mult_matrix_f) g_real_gl_mult_matrix_f(widened);
    } else if (g_real_gl_mult_matrix_f) g_real_gl_mult_matrix_f(matrix);
}

__declspec(dllexport) void APIENTRY glMultMatrixd(const double *matrix)
{
    if (!g_real_gl_mult_matrix_d) g_real_gl_mult_matrix_d = (PFNGLMULTMATRIXD)real_gl_proc("glMultMatrixd");
    if (matrix && g_matrix_mode == GL_PROJECTION)
        g_projection_is_orthographic = !is_perspective_d(matrix);
    if (matrix && should_trace_projection())
        log_line("Projection trace frame=%ld MultMatrixd p00=%.9g p11=%.9g p22=%.9g p23=%.9g p32=%.9g p33=%.9g",
                 g_frame_count + 1, matrix[0], matrix[5], matrix[10], matrix[11], matrix[14], matrix[15]);
    if (g_overscan_enabled && openxr_bridge_should_expand_world() &&
        g_matrix_mode == GL_PROJECTION && is_perspective_d(matrix)) {
        double widened[16];
        memcpy(widened, matrix, sizeof(widened));
        widened[0] *= 0.82;
        widened[5] *= 0.82;
        log_overscan_once((float)matrix[0], (float)matrix[5]);
        if (g_real_gl_mult_matrix_d) g_real_gl_mult_matrix_d(widened);
    } else if (g_real_gl_mult_matrix_d) g_real_gl_mult_matrix_d(matrix);
}

__declspec(dllexport) void APIENTRY glFrustum(double left, double right, double bottom, double top,
                                               double near_value, double far_value)
{
    g_projection_is_orthographic = 0;
    if ((g_geometry_eye >= 0 || g_hud_preview_enabled ||
         g_menu_baseline_pending) &&
        g_matrix_mode == GL_PROJECTION)
        g_geometry_seen_perspective = 1;
    if (g_geometry_eye >= 0) g_pending_eye_view_offset = 0;
    if (!g_real_gl_frustum) g_real_gl_frustum = (PFNGLFRUSTUM)real_gl_proc("glFrustum");
    if (should_trace_projection())
        log_line("Projection trace frame=%ld Frustum l=%.9g r=%.9g b=%.9g t=%.9g n=%.9g f=%.9g",
                 g_frame_count + 1, left, right, bottom, top, near_value, far_value);
    if (g_geometry_eye >= 0 && g_matrix_mode == GL_PROJECTION) {
        /* Skillshot loads its view matrix before defining the perspective
           frustum. Apply the physical eye camera to that already-live matrix;
           waiting for a later model-view load only produced a flat off-axis
           image shift because no later load occurs for the world pass. */
        if (!g_real_gl_matrix_mode)
            g_real_gl_matrix_mode = (PFNGLMATRIXMODE)real_gl_proc("glMatrixMode");
        if (!g_real_gl_load_matrix_f)
            g_real_gl_load_matrix_f = (PFNGLLOADMATRIXF)real_gl_proc("glLoadMatrixf");
        double render_left = left, render_right = right;
        double adjusted_bottom, adjusted_top;
        float runtime_left = 0.0f, runtime_right = 0.0f;
        float runtime_up = 0.0f, runtime_down = 0.0f;
        double runtime_bounds[4];
        int runtime_fov_active =
            openxr_bridge_get_runtime_fov(g_geometry_eye, &runtime_left,
                                          &runtime_right, &runtime_up,
                                          &runtime_down) &&
            vr_runtime_fov_to_reversed_frustum_d(
                runtime_left, runtime_right, runtime_up, runtime_down,
                near_value, runtime_bounds);
        if (runtime_fov_active) {
            render_left = runtime_bounds[0];
            render_right = runtime_bounds[1];
            adjusted_bottom = runtime_bounds[2];
            adjusted_top = runtime_bounds[3];
            if (InterlockedCompareExchange(&g_runtime_fov_logged, 1, 0) == 0)
                log_line("Quest lens FOV rendered natively: game %.2f/%.2f %.2f/%.2f -> runtime %.2f/%.2f %.2f/%.2f deg",
                         atan(left / near_value) * 57.2957795,
                         atan(right / near_value) * 57.2957795,
                         atan(fabs(top / near_value)) * 57.2957795,
                         -atan(fabs(bottom / near_value)) * 57.2957795,
                         runtime_left * 57.2957795f,
                         runtime_right * 57.2957795f,
                         runtime_up * 57.2957795f,
                         runtime_down * 57.2957795f);
        } else {
            double center = (bottom + top) * 0.5;
            double half = (top - bottom) * 0.5 * g_geometry_vertical_fov_scale;
            adjusted_bottom = center - half;
            adjusted_top = center + half;
        }
        double effective_convergence = g_world_convergence_distance;
        if (g_gl_get_floatv) {
            float view[16] = {0};
            g_gl_get_floatv(GL_MODELVIEW_MATRIX, view);
            float camera_distance = view[14] < 0.0f ? -view[14] : view[14];
            if (g_auto_convergence && camera_distance >= 500.0f && camera_distance <= 50000.0f)
                effective_convergence = camera_distance;
        }
        g_pending_eye_convergence = (float)effective_convergence;
        g_pending_eye_view_offset = 1;
        double eye_view_translation = (g_geometry_eye == 0 ? 0.5 : -0.5) * g_world_eye_separation;
        if (effective_convergence < 500.0) effective_convergence = 500.0;
        double convergence_shift = eye_view_translation * near_value / effective_convergence;
        /* Calibration used to crop and translate the completed eye texture.
           Render the same optical-centre adjustment as a true off-axis
           frustum instead. This makes the image rays exactly match the FOV
           submitted to OpenXR and avoids timewarp bending at oblique angles. */
        double alignment_shift = vr_frustum_alignment_shift_d(
            render_left, render_right,
            (double)openxr_bridge_get_geometry_alignment(g_geometry_eye));
        double shift = convergence_shift + alignment_shift;
        if (fabs(adjusted_top - adjusted_bottom) > 0.000001)
            g_geometry_projection_aspect =
                fabs((render_right - render_left) /
                     (adjusted_top - adjusted_bottom));
        openxr_bridge_set_geometry_fov(
            g_geometry_eye,
            (float)atan((render_left + shift) / near_value),
            (float)atan((render_right + shift) / near_value),
            (float)atan(fabs(adjusted_top / near_value)),
            (float)-atan(fabs(adjusted_bottom / near_value)));
        if (g_real_gl_frustum)
            g_real_gl_frustum(render_left + shift,
                              render_right + shift,
                              adjusted_bottom, adjusted_top,
                              near_value, far_value);
    } else if (g_overscan_enabled && openxr_bridge_should_expand_world() &&
               g_matrix_mode == GL_PROJECTION) {
        log_overscan_once((float)(2.0 * near_value / (right - left)),
                          (float)(2.0 * near_value / (top - bottom)));
        if (g_real_gl_frustum) g_real_gl_frustum(left / 0.82, right / 0.82,
                                                 bottom / 0.82, top / 0.82,
                                                 near_value, far_value);
    } else if (g_real_gl_frustum) {
        g_real_gl_frustum(left, right, bottom, top, near_value, far_value);
    }
}

__declspec(dllexport) void APIENTRY glTranslatef(float x, float y, float z)
{
    if (!g_real_gl_translate_f)
        g_real_gl_translate_f = (PFNGLTRANSLATEF)real_gl_proc("glTranslatef");
    if (!g_real_gl_scale_f)
        g_real_gl_scale_f = (PFNGLSCALEF)real_gl_proc("glScalef");
    if (!g_real_gl_translate_f) return;
    if (g_pending_eye_view_offset && g_geometry_eye >= 0 &&
        g_matrix_mode == GL_MODELVIEW && !g_projection_is_orthographic) {
        float eye_translation = (g_geometry_eye == 0 ? 0.5f : -0.5f) *
                                g_world_eye_separation - g_head_translation_x;
        float depth_translation = g_pending_eye_convergence * (g_depth_exaggeration - 1.0f);
        g_real_gl_translate_f(eye_translation, -g_head_translation_y, depth_translation);
        if (g_real_gl_scale_f && fabsf(g_depth_exaggeration - 1.0f) >= 0.00001f)
            g_real_gl_scale_f(1.0f, 1.0f, g_depth_exaggeration);
        g_pending_eye_view_offset = 0;
        LONG trace = InterlockedIncrement(&g_eye_transform_trace_count);
        if (trace <= 8)
            log_line("Late eye transform eye=%d convergence=%.3f eye-x=%.3f depth-z=%.3f before game Translatef(%.3f,%.3f,%.3f)",
                     g_geometry_eye, g_pending_eye_convergence, eye_translation,
                     depth_translation, x, y, z);
    }
    g_real_gl_translate_f(x, y, z);
}

__declspec(dllexport) void APIENTRY glScalef(float x, float y, float z)
{
    if (!g_real_gl_scale_f) g_real_gl_scale_f = (PFNGLSCALEF)real_gl_proc("glScalef");
    if (g_real_gl_scale_f) g_real_gl_scale_f(x, y, z);
}

__declspec(dllexport) void APIENTRY glOrtho(double left, double right, double bottom, double top,
                                             double near_value, double far_value)
{
    g_projection_is_orthographic = 1;
    if (g_matrix_mode == GL_PROJECTION) {
    }
    if (!g_real_gl_ortho) g_real_gl_ortho = (PFNGLORTHO)real_gl_proc("glOrtho");
    if (should_trace_projection())
        log_line("Projection trace frame=%ld Ortho l=%.9g r=%.9g b=%.9g t=%.9g n=%.9g f=%.9g",
                 g_frame_count + 1, left, right, bottom, top, near_value, far_value);
    if (g_menu_baseline_pending && g_geometry_seen_perspective &&
        !g_menu_baseline_captured) {
        /* Capture framebuffer zero before the first orthographic interface
           draw. The completed backbuffer later contains the exact same mono
           world plus Tab/Escape/level-up UI, so their difference contains no
           second camera image and the ordinary window never eye-flickers. */
        g_menu_baseline_captured = openxr_bridge_capture_native_mirror(0);
        if (g_menu_baseline_captured &&
            InterlockedCompareExchange(&g_menu_baseline_logged, 1, 0) == 0)
            log_line("Stable mono interface baseline captured before orthographic HUD");
    }
    /* Gameplay's fixed HUD is authored for a 16:9 canvas but is drawn into a
       near-square eye target.  Only the orthographic pass following the world
       perspective is inset.  Player names, crosshair, Start Game and other
       world-attached labels remain in the perspective pass.  Derive the Y
       scale from the live eye resolution because Link and the simulator expose
       slightly different eye aspects. */
    if ((g_geometry_eye >= 0 || g_hud_preview_enabled) &&
        g_geometry_seen_perspective && g_matrix_mode == GL_PROJECTION) {
        double bounds[4] = {left, right, bottom, top};
        double effective_scale_y = g_hud_safe_scale_y;
        int eye_width = 0, eye_height = 0;
        if (openxr_bridge_get_eye_size(&eye_width, &eye_height)) {
            double presentation_aspect = g_geometry_projection_aspect > 0.0
                ? g_geometry_projection_aspect
                : (double)eye_width / (double)eye_height;
            double aspect_scale_y = vr_hud_scale_y_for_eye(
                g_hud_safe_scale_x, presentation_aspect, 1.0, 16.0 / 9.0);
            if (aspect_scale_y > 0.0) effective_scale_y = aspect_scale_y;
        }
        if (g_geometry_eye >= 0 && !g_geometry_hud_base_captured &&
            openxr_bridge_capture_hud_base(
                g_geometry_eye, g_hud_safe_scale_x,
                (float)effective_scale_y)) {
            g_geometry_hud_base_captured = 1;
            if (InterlockedCompareExchange(&g_hud_safe_logged, 1, 0) == 0)
                log_line("Native-detail fixed HUD compositor active: source=full 16:9 safe=%.3f,%.3f eye=%dx%d",
                         g_hud_safe_scale_x, (float)effective_scale_y,
                         eye_width, eye_height);
        }
        /* Tab and Escape are drawn inside the gameplay root. Begin the exact
           transparent target immediately after preserving the perspective
           world, before the first orthographic interface draw can enter an
           eye texture. */
        if (g_geometry_eye >= 0 && g_full_interface_capture_requested &&
            g_geometry_hud_base_captured &&
            !g_interface_alpha_capture_active) {
            g_interface_alpha_capture_active =
                openxr_bridge_begin_interface_alpha_capture();
            if (g_interface_alpha_capture_active) {
                static LONG early_interface_capture_logged;
                if (InterlockedCompareExchange(
                        &early_interface_capture_logged, 1, 0) == 0)
                    log_line("Exact full-interface capture begins before the first orthographic UI draw");
            }
        }
        if (!g_geometry_hud_base_captured) {
            /* Safe fallback for runtimes without the additional interop
               surface: retain the proven in-render HUD inset. */
            vr_inset_ortho_d(bounds, g_hud_safe_scale_x, effective_scale_y);
            left = bounds[0]; right = bounds[1];
            bottom = bounds[2]; top = bounds[3];
        }
        if (g_geometry_eye >= 0 && !g_geometry_hud_base_captured) {
            float fov_left, fov_right, fov_up, fov_down, optical_center;
            if (openxr_bridge_get_runtime_fov(
                    g_geometry_eye, &fov_left, &fov_right, &fov_up, &fov_down) &&
                vr_projection_optical_center_x_f(
                    fov_left, fov_right, &optical_center)) {
                double horizontal_shift =
                    (0.5 - (double)optical_center) * (right - left);
                left += horizontal_shift;
                right += horizontal_shift;
                if (InterlockedCompareExchange(
                        &g_hud_optical_center_logged, 1, 0) == 0)
                    log_line("VR fixed HUD centered on asymmetric eye optics: eye=%d center=%.4f shift=%.3f",
                             g_geometry_eye, optical_center, horizontal_shift);
            }
        }
        if (!g_geometry_hud_base_captured &&
            InterlockedCompareExchange(&g_hud_safe_logged, 1, 0) == 0)
            log_line("VR fixed HUD 16:9 safe-area active: x=%.3f y=%.3f eye=%dx%d projection-aspect=%.3f",
                     g_hud_safe_scale_x, (float)effective_scale_y,
                     eye_width, eye_height, (float)g_geometry_projection_aspect);
    }
    if (g_real_gl_ortho) g_real_gl_ortho(left, right, bottom, top, near_value, far_value);
}

__declspec(dllexport) void APIENTRY glViewport(int x, int y, int width, int height)
{
    if (!g_real_gl_viewport) g_real_gl_viewport = (PFNGLVIEWPORT)real_gl_proc("glViewport");
    if (g_frame_count <= 3 && InterlockedIncrement(&g_projection_trace_count) <= 160)
        log_line("Viewport trace frame=%ld framebuffer=%u rect=%d,%d %dx%d", g_frame_count + 1,
                 g_draw_framebuffer, x, y, width, height);
    if (g_real_gl_viewport) g_real_gl_viewport(x, y, width, height);
}

__declspec(dllexport) void APIENTRY glBegin(unsigned int mode)
{
    g_indicator_hide_matrix_active = 0;
    if (g_indicator_hide_follow_draws > 0 && g_geometry_eye == 1 &&
        g_projection_is_orthographic && g_matrix_mode == GL_MODELVIEW &&
        g_gl_get_floatv) {
        if (!g_real_gl_load_matrix_f)
            g_real_gl_load_matrix_f = (PFNGLLOADMATRIXF)real_gl_proc("glLoadMatrixf");
        g_gl_get_floatv(GL_MODELVIEW_MATRIX, g_indicator_hide_saved_matrix);
        if (g_real_gl_load_matrix_f) {
            float hidden[16];
            memcpy(hidden, g_indicator_hide_saved_matrix, sizeof(hidden));
            hidden[12] += 1000000.0f;
            g_real_gl_load_matrix_f(hidden);
            g_indicator_hide_matrix_active = 1;
        }
        --g_indicator_hide_follow_draws;
    }
    record_draw('L', mode, 0);
    if (!g_real_gl_begin) g_real_gl_begin = (PFNGLBEGIN)real_gl_proc("glBegin");
    if (g_real_gl_begin) g_real_gl_begin(mode);
}

__declspec(dllexport) void APIENTRY glEnd(void)
{
    if (!g_real_gl_end) g_real_gl_end = (PFNGLEND)real_gl_proc("glEnd");
    if (g_real_gl_end) g_real_gl_end();
    if (g_indicator_hide_matrix_active) {
        if (!g_real_gl_load_matrix_f)
            g_real_gl_load_matrix_f = (PFNGLLOADMATRIXF)real_gl_proc("glLoadMatrixf");
        if (g_real_gl_load_matrix_f)
            g_real_gl_load_matrix_f(g_indicator_hide_saved_matrix);
        g_indicator_hide_matrix_active = 0;
    }
}

__declspec(dllexport) void APIENTRY glCallList(unsigned int list)
{
    g_draw_object_id = list;
    record_draw('C', 0, 1);
    g_draw_object_id = 0;
    if (!g_real_gl_call_list) g_real_gl_call_list = (PFNGLCALLLIST)real_gl_proc("glCallList");
    if (!g_real_gl_call_list || (g_frame_count == 119 && g_debug_skip_mode == 3)) return;

    /* Direction arrows are an exact fixed-HUD display-list signature. Draw
       the authored object and its two immediate label draws only into eye 0;
       the HUD resolver inserts those identical pixels into both eyes. */
    int bound_texture = (int)g_bound_texture_2d;
    if (g_gl_get_integerv)
        g_gl_get_integerv(0x8069u /* GL_TEXTURE_BINDING_2D */, &bound_texture);
    int indicator_signature =
        g_geometry_eye >= 0 && g_projection_is_orthographic &&
        g_matrix_mode == GL_MODELVIEW && g_current_program == 88u &&
        list == 9u && bound_texture == 9;
    if (indicator_signature && g_geometry_eye == 1) {
        g_indicator_hide_follow_draws = 2;
        if (InterlockedCompareExchange(&g_indicator_shared_logged, 1, 0) == 0)
            log_line("Target indicators flattened through shared eye-0 HUD source (program=88 texture=9 list=9)");
        return;
    }
    g_real_gl_call_list(list);
}

__declspec(dllexport) void APIENTRY glCallLists(int count, unsigned int type, const void *lists)
{
    record_draw('T', type, count);
    if (!g_real_gl_call_lists) g_real_gl_call_lists = (PFNGLCALLLISTS)real_gl_proc("glCallLists");
    if (g_real_gl_call_lists && !(g_frame_count == 119 && g_debug_skip_mode == 3)) g_real_gl_call_lists(count, type, lists);
}

static void dump_draw_events(LONG frame)
{
    char filename[64];
    snprintf(filename, sizeof(filename), "SkillshotVR-frame-%06ld-draws.csv", frame);
    char path[MAX_PATH];
    build_path(path, sizeof(path), filename);
    FILE *file = fopen(path, "w");
    if (!file) return;
    fprintf(file, "index,kind,program,framebuffer,texture_2d,object_id,mode,count,depth_test,blend,scissor_test,viewport_x,viewport_y,viewport_w,viewport_h,scissor_x,scissor_y,scissor_w,scissor_h");
    for (int element = 0; element < 16; ++element)
        fprintf(file, ",p%d", element);
    for (int element = 0; element < 16; ++element)
        fprintf(file, ",mv%d", element);
    fputc('\n', file);
    LONG count = g_draw_event_count;
    if (count > (LONG)(sizeof(g_draw_events) / sizeof(g_draw_events[0]))) {
        count = (LONG)(sizeof(g_draw_events) / sizeof(g_draw_events[0]));
    }
    for (LONG index = 0; index < count; ++index) {
        DrawEvent *event = &g_draw_events[index];
        fprintf(file, "%ld,%c,%u,%u,%u,%u,0x%X,%d,%u,%u,%u,%d,%d,%d,%d,%d,%d,%d,%d",
                index, event->kind, event->program, event->framebuffer, event->texture_2d,
                event->object_id, event->mode, event->count,
                event->depth_test, event->blend, event->scissor_test,
                event->viewport[0], event->viewport[1], event->viewport[2],
                event->viewport[3], event->scissor[0], event->scissor[1],
                event->scissor[2], event->scissor[3]);
        for (int element = 0; element < 16; ++element)
            fprintf(file, ",%.9g", event->projection[element]);
        for (int element = 0; element < 16; ++element)
            fprintf(file, ",%.9g", event->modelview[element]);
        fputc('\n', file);
    }
    fclose(file);
    log_line("Captured %ld draw events for frame %ld to %s", count, frame, filename);
}

static int capture_bmp_named(const char *filename, LONG frame)
{
    if (!g_gl_get_integerv || !g_gl_read_pixels || !g_gl_pixel_store_i || !g_gl_read_buffer) return 0;
    int viewport[4] = {0};
    int old_pack = 4;
    int old_read_buffer = GL_BACK;
    g_gl_get_integerv(GL_VIEWPORT, viewport);
    g_gl_get_integerv(GL_PACK_ALIGNMENT, &old_pack);
    g_gl_get_integerv(GL_READ_BUFFER, &old_read_buffer);
    int width = viewport[2];
    int height = viewport[3];
    if (width <= 0 || height <= 0 || width > 16384 || height > 16384) return 0;

    size_t source_stride = (size_t)width * 3;
    size_t destination_stride = (source_stride + 3u) & ~3u;
    size_t source_size = source_stride * (size_t)height;
    size_t destination_size = destination_stride * (size_t)height;
    unsigned char *rgb = (unsigned char *)malloc(source_size);
    unsigned char *bgr = (unsigned char *)calloc(1, destination_size);
    if (!rgb || !bgr) {
        free(rgb);
        free(bgr);
        return 0;
    }

    g_gl_read_buffer(g_read_framebuffer ? GL_COLOR_ATTACHMENT0 : GL_BACK);
    g_gl_pixel_store_i(GL_PACK_ALIGNMENT, 1);
    g_gl_read_pixels(viewport[0], viewport[1], width, height, GL_RGB, GL_UNSIGNED_BYTE, rgb);
    g_gl_pixel_store_i(GL_PACK_ALIGNMENT, old_pack);
    g_gl_read_buffer((unsigned int)old_read_buffer);

    for (int y = 0; y < height; ++y) {
        const unsigned char *source = rgb + (size_t)y * source_stride;
        unsigned char *destination = bgr + (size_t)y * destination_stride;
        for (int x = 0; x < width; ++x) {
            destination[x * 3 + 0] = source[x * 3 + 2];
            destination[x * 3 + 1] = source[x * 3 + 1];
            destination[x * 3 + 2] = source[x * 3 + 0];
        }
    }

    char path[MAX_PATH];
    build_path(path, sizeof(path), filename);
    FILE *file = fopen(path, "wb");
    if (!file) {
        free(rgb);
        free(bgr);
        return 0;
    }

#pragma pack(push, 1)
    typedef struct {
        uint16_t type;
        uint32_t size;
        uint16_t reserved1;
        uint16_t reserved2;
        uint32_t offset;
    } BmpFileHeader;
#pragma pack(pop)
    BITMAPINFOHEADER info = {0};
    BmpFileHeader header = {0};
    header.type = 0x4D42;
    header.offset = sizeof(header) + sizeof(info);
    header.size = header.offset + (uint32_t)destination_size;
    info.biSize = sizeof(info);
    info.biWidth = width;
    info.biHeight = height;
    info.biPlanes = 1;
    info.biBitCount = 24;
    info.biCompression = BI_RGB;
    info.biSizeImage = (DWORD)destination_size;
    fwrite(&header, sizeof(header), 1, file);
    fwrite(&info, sizeof(info), 1, file);
    fwrite(bgr, destination_size, 1, file);
    fclose(file);
    free(rgb);
    free(bgr);
    log_line("Captured frame %ld to %s (%dx%d)", frame, filename, width, height);
    return 1;
}

static int capture_bmp(LONG frame)
{
    char filename[64];
    snprintf(filename, sizeof(filename), "SkillshotVR-frame-%06ld.bmp", frame);
    return capture_bmp_named(filename, frame);
}

static void capture_depth_buffer(LONG frame)
{
    if (!g_gl_get_integerv || !g_gl_read_pixels || !g_gl_pixel_store_i) return;
    int viewport[4] = {0};
    int old_pack = 4;
    g_gl_get_integerv(GL_VIEWPORT, viewport);
    g_gl_get_integerv(GL_PACK_ALIGNMENT, &old_pack);
    int width = viewport[2];
    int height = viewport[3];
    if (width <= 0 || height <= 0 || width > 16384 || height > 16384) return;
    size_t count = (size_t)width * (size_t)height;
    float *depth = (float *)malloc(count * sizeof(float));
    if (!depth) return;
    g_gl_pixel_store_i(GL_PACK_ALIGNMENT, 1);
    g_gl_read_pixels(viewport[0], viewport[1], width, height, GL_DEPTH_COMPONENT, GL_FLOAT, depth);
    g_gl_pixel_store_i(GL_PACK_ALIGNMENT, old_pack);

    float minimum = 1.0f;
    float maximum = 0.0f;
    size_t non_far = 0;
    for (size_t index = 0; index < count; ++index) {
        if (depth[index] < minimum) minimum = depth[index];
        if (depth[index] > maximum) maximum = depth[index];
        if (depth[index] < 0.999999f) ++non_far;
    }
    char filename[64];
    snprintf(filename, sizeof(filename), "SkillshotVR-frame-%06ld.depth32f", frame);
    char path[MAX_PATH];
    build_path(path, sizeof(path), filename);
    FILE *file = fopen(path, "wb");
    if (file) {
        uint32_t dimensions[2] = {(uint32_t)width, (uint32_t)height};
        fwrite(dimensions, sizeof(dimensions), 1, file);
        fwrite(depth, sizeof(float), count, file);
        fclose(file);
        log_line("Captured depth frame %ld to %s (%dx%d min=%.9g max=%.9g nonFar=%zu)",
                 frame, filename, width, height, minimum, maximum, non_far);
    }
    free(depth);
}

static void capture_stage_if_needed(void)
{
    if (g_frame_count != 119) return;
    LONG completed = g_draw_event_count;
    if (completed == 603 || completed == 1016 || completed == 1180 || completed == 1191) {
        char filename[80];
        snprintf(filename, sizeof(filename), "SkillshotVR-frame-000120-stage-%04ld.bmp", completed);
        capture_bmp_named(filename, 120);
    }
}

static BOOL WINAPI hooked_swap_buffers(HDC device_context)
{
    initialize_gl_diagnostics();
    HWND swap_window = WindowFromDC(device_context);
    if (swap_window && IsWindow(swap_window)) {
        g_game_window = swap_window;
        ensure_camera_input_window(swap_window);
    }
    if (g_camera_save_due && GetTickCount64() >= g_camera_save_due &&
        !g_camera_drag_active) {
        if (save_camera_tuning()) show_camera_tuning_in_title("saved");
        g_camera_save_due = 0;
    }
    if (!g_vr_runtime_disabled && g_gl_get_integerv) {
        /* Keep a 1920x1080 source: materially sharper text and desktop output
           than the rejected 1728 target, while retaining most of its stereo
           raster savings versus the game's 2560-wide maximum. Account for Windows DPI
           virtualization by converting physical GL pixels to client units. */
        HWND window = g_game_window;
        int viewport[4] = {0};
        RECT client = {0};
        g_gl_get_integerv(GL_VIEWPORT, viewport);
        ULONGLONG now = GetTickCount64();
        if (window && GetClientRect(window, &client) && viewport[2] > 1920 &&
            now - g_vr_window_resize_tick >= 2000) {
            g_vr_window_resize_tick = now;
            int client_width = client.right - client.left;
            int client_height = client.bottom - client.top;
            int target_client_width = MulDiv(1920, client_width, viewport[2]);
            int target_client_height = MulDiv(1080, client_height, viewport[3]);
            RECT target = {0, 0, target_client_width, target_client_height};
            DWORD style = (DWORD)GetWindowLongPtrA(window, GWL_STYLE);
            DWORD ex_style = (DWORD)GetWindowLongPtrA(window, GWL_EXSTYLE);
            AdjustWindowRectEx(&target, style, FALSE, ex_style);
            ShowWindow(window, SW_RESTORE);
            if (SetWindowPos(window, NULL, 0, 0, target.right - target.left,
                             target.bottom - target.top,
                             SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED)) {
                log_line("VR source window resized from GL %dx%d to 1920x1080 balanced-quality target",
                         viewport[2], viewport[3]);
            }
        }
    }
    LONG frame = InterlockedIncrement(&g_frame_count);
    if (InterlockedExchange(&g_draw_capture_active, 0)) {
        dump_draw_events(frame);
        char capture_name[80];
        snprintf(capture_name, sizeof(capture_name),
                 "SkillshotVR-frame-%06ld-interface.bmp", frame);
        capture_bmp_named(capture_name, frame);
    }
    LONG interface_draws = InterlockedExchange(&g_interface_draw_calls_current, 0);
    LONG64 interface_vertices = InterlockedExchange64(&g_interface_vertices_current, 0);
    if (!g_vr_runtime_disabled) {
        ULONGLONG geometry_age = g_geometry_last_render_tick
            ? GetTickCount64() - g_geometry_last_render_tick : ~(ULONGLONG)0;
        int geometry_active = geometry_age <= 250;
        /* Escape is a persistent toggle in this game's UI; Tab is held for its
           scoreboard. Feed those exact states to the menu classifier instead
           of relying only on a draw-count heuristic. Update the classifier
           before selecting/capturing this frame's presentation source. */
        int explicit_full_interface = g_cursor_clip_suspended ||
            (GetAsyncKeyState(VK_TAB) & 0x8000);
        int classified_draws = explicit_full_interface ? 1000 : (int)interface_draws;
        openxr_bridge_set_interface_load(classified_draws,
                                         (long long)interface_vertices);
        int presentation_active = openxr_bridge_set_geometry_active(geometry_active);
        update_game_cursor_clip(presentation_active, interface_draws <= 180);
        if (g_geometry_has_presented_eye && !presentation_active)
            openxr_bridge_capture_flat_frame();
        openxr_bridge_finish_interface_alpha_capture(
            presentation_active && openxr_bridge_interface_heavy());
        g_interface_alpha_capture_active = 0;
        g_full_interface_capture_requested = 0;
        openxr_bridge_tick(g_directory, device_context);
        if (g_geometry_hook_requested && openxr_bridge_geometry_ready() &&
            InterlockedCompareExchange(&g_geometry_hook_attempted, 1, 0) == 0) {
            int cull_hook = patch_world_draw();
            int geometry_hook = patch_gameplay_render();
            log_line("True-geometry hooks gameplay=%s peripheral-cull=%s at safe frame boundary eye-separation=%.3f",
                     geometry_hook ? "installed" : "not installed",
                     cull_hook ? "installed" : "not installed", g_world_eye_separation);
        }
        if (g_geometry_has_presented_eye && presentation_active)
            openxr_bridge_present_mirror_eye(0, g_geometry_vertical_fov_scale);
        openxr_bridge_present_desktop_interface();
    }
    /* Explicit marker-driven controls support supervised, repeatable tests of
       the isolated copy. Nothing is injected unless the harness writes one of
       these one-shot files after positively verifying the Singleplayer UI. */
    if ((frame % 15) == 0) {
        if (g_test_held_key && GetTickCount64() >= g_test_key_release_tick) {
            if (g_game_window)
                PostMessageA(g_game_window, WM_KEYUP, (WPARAM)g_test_held_key, 0xC0000001);
            log_line("Background test released held key virtual-key=%d", g_test_held_key);
            g_test_held_key = 0;
            g_test_key_release_tick = 0;
        }
        char quit_path[MAX_PATH];
        build_path(quit_path, sizeof(quit_path), "SkillshotVR-test-quit.txt");
        if (GetFileAttributesA(quit_path) != INVALID_FILE_ATTRIBUTES && g_game_window) {
            PostMessageA(g_game_window, WM_CLOSE, 0, 0);
            DeleteFileA(quit_path);
            log_line("Background test requested a clean game-window close");
        }
        char click_path[MAX_PATH];
        build_path(click_path, sizeof(click_path), "SkillshotVR-test-click.txt");
        FILE *click_file = fopen(click_path, "r");
        if (click_file) {
            /* A visually selected "Singleplayer" tab can still lead into the
               network BR matchmaking flow. Until the game exposes a reliable
               internal local-session discriminator, reject all synthetic
               pointer activation instead of trusting screen coordinates. */
            fclose(click_file);
            DeleteFileA(click_path);
            log_line("Safety guard rejected background test click; local singleplayer state is not provable");
        }
        char key_path[MAX_PATH];
        build_path(key_path, sizeof(key_path), "SkillshotVR-test-key.txt");
        FILE *key_file = fopen(key_path, "r");
        if (key_file) {
            int consumed = 0;
            int virtual_key = 0;
            if (fscanf(key_file, "%d", &virtual_key) == 1 &&
                virtual_key > 0 && virtual_key < 256) {
                HWND window = g_game_window;
                if (window && !IsIconic(window)) {
                    PostMessageA(window, WM_KEYDOWN, (WPARAM)virtual_key, 1);
                    PostMessageA(window, WM_KEYUP, (WPARAM)virtual_key, 0xC0000001);
                    log_line("Background test key virtual-key=%d (focus unchanged)", virtual_key);
                    consumed = 1;
                } else {
                    log_line("Background test key deferred: game window unavailable or minimized");
                }
            }
            fclose(key_file);
            if (consumed) DeleteFileA(key_path);
        }
        char hold_path[MAX_PATH];
        build_path(hold_path, sizeof(hold_path), "SkillshotVR-test-hold.txt");
        FILE *hold_file = fopen(hold_path, "r");
        if (hold_file) {
            int consumed = 0;
            int virtual_key = 0, duration_ms = 0;
            if (fscanf(hold_file, "%d %d", &virtual_key, &duration_ms) == 2 &&
                virtual_key > 0 && virtual_key < 256 &&
                duration_ms >= 50 && duration_ms <= 15000) {
                HWND window = g_game_window;
                if (window && !IsIconic(window)) {
                    if (g_test_held_key)
                        PostMessageA(window, WM_KEYUP, (WPARAM)g_test_held_key, 0xC0000001);
                    g_test_held_key = virtual_key;
                    g_test_key_release_tick = GetTickCount64() + (ULONGLONG)duration_ms;
                    PostMessageA(window, WM_KEYDOWN, (WPARAM)virtual_key, 1);
                    log_line("Background test holding virtual-key=%d for %dms (focus unchanged)",
                             virtual_key, duration_ms);
                    consumed = 1;
                }
            }
            fclose(hold_file);
            if (consumed) DeleteFileA(hold_path);
        }
    }
    if ((frame % 15) == 0) {
        char draw_marker[MAX_PATH];
        build_path(draw_marker, sizeof(draw_marker), "SkillshotVR-draw-capture.txt");
        if (GetFileAttributesA(draw_marker) != INVALID_FILE_ATTRIBUTES) {
            InterlockedExchange(&g_draw_event_count, 0);
            InterlockedExchange(&g_draw_capture_active, 1);
            DeleteFileA(draw_marker);
            log_line("Armed synchronized interface draw trace for frame %ld", frame + 1);
        }
        char live_marker[MAX_PATH];
        build_path(live_marker, sizeof(live_marker), "SkillshotVR-live-capture.txt");
        if (GetFileAttributesA(live_marker) != INVALID_FILE_ATTRIBUTES) {
            capture_bmp_named("SkillshotVR-live.bmp", frame);
            DeleteFileA(live_marker);
        }
    }
    /* Poll this every presented game frame.  The old 15-frame sampling window
       missed ordinary Shift+1 taps, especially while Tab or Escape changed
       the render cadence, which made physical UI diagnostics appear to have
       succeeded without recording the state the tester actually saw. */
    static int headset_capture_key_down;
    SHORT capture_one_state = GetAsyncKeyState('1');
    int headset_capture_key =
        (GetAsyncKeyState(VK_SHIFT) & 0x8000) &&
        (capture_one_state & 0x8001);
    if (headset_capture_key && !headset_capture_key_down) {
        openxr_bridge_request_fresh_geometry_proof();
        char headset_marker[MAX_PATH];
        build_path(headset_marker, sizeof(headset_marker),
                   "SkillshotVR-headset-capture-request.txt");
        FILE *request = fopen(headset_marker, "wb");
        if (request) {
            fputs("capture both compositor eyes\n", request);
            fclose(request);
            log_line("Shift+1 requested a two-eye headset compositor capture");
        }
    }
    headset_capture_key_down =
        (GetAsyncKeyState(VK_SHIFT) & 0x8000) &&
        (GetAsyncKeyState('1') & 0x8000);
    if (frame == 120) {
        capture_bmp(frame);
        dump_draw_events(frame);
    } else if (GetAsyncKeyState(VK_F10) & 1) {
        capture_bmp(frame);
    }
    if (frame % 300 == 0 && g_gl_get_integerv) {
        int viewport[4] = {0};
        g_gl_get_integerv(GL_VIEWPORT, viewport);
        log_line("Frame %ld viewport=%d,%d %dx%d", frame, viewport[0], viewport[1], viewport[2], viewport[3]);
        log_line("OpenXR bridge: %s", openxr_bridge_status());
    }
    if (frame % 60 == 0)
        log_line("Interface load frame=%ld draws=%ld vertices=%lld", frame, interface_draws,
                 (long long)interface_vertices);
    return g_real_swap_buffers ? g_real_swap_buffers(device_context) : FALSE;
}

__declspec(dllexport) void APIENTRY glDrawArrays(unsigned int mode, int first, int count)
{
    record_draw('A', mode, count);
    if (!g_real_gl_draw_arrays) g_real_gl_draw_arrays = (PFNGLDRAWARRAYS)real_gl_proc("glDrawArrays");
    if (g_real_gl_draw_arrays && !should_skip_current_draw()) {
        g_real_gl_draw_arrays(mode, first, count);
        if (g_interface_alpha_capture_active && g_projection_is_orthographic) {
            for (int pass = 0; pass < 2; ++pass) {
                if (openxr_bridge_begin_interface_alpha_draw(pass)) {
                    g_real_gl_draw_arrays(mode, first, count);
                    openxr_bridge_end_interface_alpha_draw();
                }
            }
        }
    }
    capture_stage_if_needed();
}

__declspec(dllexport) void APIENTRY glBlendFunc(unsigned int source,
                                                unsigned int destination)
{
    if (!g_real_gl_blend_func)
        g_real_gl_blend_func = (PFNGLBLENDFUNC)real_gl_proc("glBlendFunc");
    /* Preserve each requested RGB blend mode, while transparent interface
       capture always accumulates source-over coverage in the alpha channel. */
    if (g_interface_alpha_capture_active && g_real_gl_blend_func_separate) {
        g_real_gl_blend_func_separate(source, destination,
                                      1u /* GL_ONE */,
                                      0x0303u /* GL_ONE_MINUS_SRC_ALPHA */);
    } else if (g_real_gl_blend_func) {
        g_real_gl_blend_func(source, destination);
    }
}

static void APIENTRY hooked_gl_blend_func_separate(unsigned int source_rgb,
                                                    unsigned int destination_rgb,
                                                    unsigned int source_alpha,
                                                    unsigned int destination_alpha)
{
    if (!g_real_gl_blend_func_separate) return;
    if (g_interface_alpha_capture_active) {
        source_alpha = 1u; /* GL_ONE */
        destination_alpha = 0x0303u; /* GL_ONE_MINUS_SRC_ALPHA */
    }
    g_real_gl_blend_func_separate(source_rgb, destination_rgb,
                                  source_alpha, destination_alpha);
}

static void APIENTRY hooked_gl_blend_equation(unsigned int mode)
{
    if (g_interface_alpha_capture_active && g_real_gl_blend_equation_separate)
        g_real_gl_blend_equation_separate(mode, 0x8006u /* GL_FUNC_ADD */);
    else if (g_real_gl_blend_equation)
        g_real_gl_blend_equation(mode);
}

static void APIENTRY hooked_gl_blend_equation_separate(unsigned int mode_rgb,
                                                        unsigned int mode_alpha)
{
    if (!g_real_gl_blend_equation_separate) return;
    if (g_interface_alpha_capture_active)
        mode_alpha = 0x8006u; /* GL_FUNC_ADD */
    g_real_gl_blend_equation_separate(mode_rgb, mode_alpha);
}

__declspec(dllexport) void APIENTRY glClear(unsigned int mask)
{
    if (!g_real_gl_clear)
        g_real_gl_clear = (PFNGLCLEAR)real_gl_proc("glClear");
    /* The clean world was captured before this appended pass. A later game
       backbuffer clear is presentation plumbing, not authored interface, and
       must never turn the transparent UI surface into a second world panel. */
    if (g_interface_alpha_capture_active)
        mask &= ~GL_COLOR_BUFFER_BIT;
    if (g_real_gl_clear && mask) g_real_gl_clear(mask);
}

__declspec(dllexport) void APIENTRY glDrawElements(unsigned int mode, int count, unsigned int type, const void *indices)
{
    record_draw('E', mode, count);
    if (!g_real_gl_draw_elements) g_real_gl_draw_elements = (PFNGLDRAWELEMENTS)real_gl_proc("glDrawElements");
    if (g_real_gl_draw_elements && !should_skip_current_draw()) {
        g_real_gl_draw_elements(mode, count, type, indices);
        if (g_interface_alpha_capture_active && g_projection_is_orthographic) {
            for (int pass = 0; pass < 2; ++pass) {
                if (openxr_bridge_begin_interface_alpha_draw(pass)) {
                    g_real_gl_draw_elements(mode, count, type, indices);
                    openxr_bridge_end_interface_alpha_draw();
                }
            }
        }
    }
    capture_stage_if_needed();
}

static int patch_swap_buffers_import(void)
{
    unsigned char *base = (unsigned char *)GetModuleHandleW(NULL);
    if (!base) return 0;
    IMAGE_DOS_HEADER *dos = (IMAGE_DOS_HEADER *)base;
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) return 0;
    IMAGE_NT_HEADERS64 *nt = (IMAGE_NT_HEADERS64 *)(base + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE) return 0;
    IMAGE_DATA_DIRECTORY imports = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    if (!imports.VirtualAddress) return 0;

    IMAGE_IMPORT_DESCRIPTOR *descriptor = (IMAGE_IMPORT_DESCRIPTOR *)(base + imports.VirtualAddress);
    for (; descriptor->Name; ++descriptor) {
        const char *library = (const char *)(base + descriptor->Name);
        if (_stricmp(library, "GDI32.dll") != 0) continue;
        IMAGE_THUNK_DATA64 *names = descriptor->OriginalFirstThunk
            ? (IMAGE_THUNK_DATA64 *)(base + descriptor->OriginalFirstThunk)
            : (IMAGE_THUNK_DATA64 *)(base + descriptor->FirstThunk);
        IMAGE_THUNK_DATA64 *addresses = (IMAGE_THUNK_DATA64 *)(base + descriptor->FirstThunk);
        for (; names->u1.AddressOfData; ++names, ++addresses) {
            if (IMAGE_SNAP_BY_ORDINAL64(names->u1.Ordinal)) continue;
            IMAGE_IMPORT_BY_NAME *import = (IMAGE_IMPORT_BY_NAME *)(base + names->u1.AddressOfData);
            if (strcmp((const char *)import->Name, "SwapBuffers") != 0) continue;
            DWORD old_protection;
            if (!VirtualProtect(&addresses->u1.Function, sizeof(uintptr_t), PAGE_READWRITE, &old_protection)) return 0;
            g_real_swap_buffers = (BOOL (WINAPI *)(HDC))(uintptr_t)addresses->u1.Function;
            addresses->u1.Function = (ULONGLONG)(uintptr_t)&hooked_swap_buffers;
            VirtualProtect(&addresses->u1.Function, sizeof(uintptr_t), old_protection, &old_protection);
            FlushInstructionCache(GetCurrentProcess(), &addresses->u1.Function, sizeof(uintptr_t));
            return 1;
        }
    }
    return 0;
}

static int patch_network_guard_imports(void)
{
    unsigned char *base = (unsigned char *)GetModuleHandleW(NULL);
    if (!base) return 0;
    IMAGE_DOS_HEADER *dos = (IMAGE_DOS_HEADER *)base;
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) return 0;
    IMAGE_NT_HEADERS64 *nt = (IMAGE_NT_HEADERS64 *)(base + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE) return 0;
    IMAGE_DATA_DIRECTORY imports = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    if (!imports.VirtualAddress) return 0;

    int patched = 0;
    IMAGE_IMPORT_DESCRIPTOR *descriptor = (IMAGE_IMPORT_DESCRIPTOR *)(base + imports.VirtualAddress);
    for (; descriptor->Name; ++descriptor) {
        const char *library = (const char *)(base + descriptor->Name);
        if (_stricmp(library, "WS2_32.dll") != 0) continue;
        IMAGE_THUNK_DATA64 *names = descriptor->OriginalFirstThunk
            ? (IMAGE_THUNK_DATA64 *)(base + descriptor->OriginalFirstThunk)
            : (IMAGE_THUNK_DATA64 *)(base + descriptor->FirstThunk);
        IMAGE_THUNK_DATA64 *addresses = (IMAGE_THUNK_DATA64 *)(base + descriptor->FirstThunk);
        for (; names->u1.AddressOfData; ++names, ++addresses) {
            ULONGLONG replacement = 0;
            if (IMAGE_SNAP_BY_ORDINAL64(names->u1.Ordinal)) {
                WORD ordinal = (WORD)IMAGE_ORDINAL64(names->u1.Ordinal);
                if (ordinal == 4) {
                    g_real_connect = (void *)(uintptr_t)addresses->u1.Function;
                    replacement = (ULONGLONG)(uintptr_t)&guarded_connect;
                } else if (ordinal == 20) {
                    g_real_sendto = (void *)(uintptr_t)addresses->u1.Function;
                    replacement = (ULONGLONG)(uintptr_t)&guarded_sendto;
                }
            } else {
                IMAGE_IMPORT_BY_NAME *import = (IMAGE_IMPORT_BY_NAME *)(base + names->u1.AddressOfData);
                if (strcmp((const char *)import->Name, "connect") == 0) {
                    g_real_connect = (void *)(uintptr_t)addresses->u1.Function;
                    replacement = (ULONGLONG)(uintptr_t)&guarded_connect;
                } else if (strcmp((const char *)import->Name, "sendto") == 0) {
                    g_real_sendto = (void *)(uintptr_t)addresses->u1.Function;
                    replacement = (ULONGLONG)(uintptr_t)&guarded_sendto;
                } else if (strcmp((const char *)import->Name, "WSASendTo") == 0) {
                    g_real_wsa_send_to = (void *)(uintptr_t)addresses->u1.Function;
                    replacement = (ULONGLONG)(uintptr_t)&guarded_wsa_send_to;
                }
            }
            if (!replacement) continue;
            DWORD old_protection;
            if (!VirtualProtect(&addresses->u1.Function, sizeof(uintptr_t), PAGE_READWRITE,
                                &old_protection)) continue;
            addresses->u1.Function = replacement;
            VirtualProtect(&addresses->u1.Function, sizeof(uintptr_t), old_protection,
                           &old_protection);
            FlushInstructionCache(GetCurrentProcess(), &addresses->u1.Function, sizeof(uintptr_t));
            patched++;
        }
    }
    return patched;
}

static void hooked_gameplay_render(void)
{
    if (!g_original_gameplay_render) return;
    if (!openxr_bridge_geometry_ready() ||
        InterlockedCompareExchange(&g_gameplay_render_reentry, 1, 0) != 0) {
        g_original_gameplay_render();
        return;
    }
    if (InterlockedCompareExchange(&g_geometry_first_entry_logged, 1, 0) == 0)
        log_line("True-geometry hook entered gameplay renderer");
    g_geometry_last_render_tick = GetTickCount64();
    /* Tab is held and Escape is toggled before the renderer starts, so mark
       those explicit interfaces immediately rather than waiting until their
       completed draw count reaches SwapBuffers. Draw-count-only interfaces
       (level-up/results) enter this path on their second settled frame. */
    int explicit_full_interface = g_cursor_clip_suspended ||
        (GetAsyncKeyState(VK_TAB) & 0x8000);
    g_full_interface_capture_requested = explicit_full_interface;
    if (explicit_full_interface)
        openxr_bridge_set_interface_load(1000, 0);
    /* The complete gameplay root updates camera, visibility, audio, and frame
       state, so it cannot safely execute twice for one game frame. Render one
       genuine eye per frame and retain the other eye's preceding frame. */
    int eye = (int)(InterlockedIncrement(&g_geometry_alternating_eye) & 1);
    /* Alternating-eye rendering spans two game frames. Sample positional lean
       once at the start of the pair and reuse it for both eyes; otherwise even
       normal head motion gives the eyes different camera centers and creates
       temporal double edges. The sequence starts with eye 1, then eye 0. */
    if (eye == 1) {
        static const float identity[16] = {
            1, 0, 0, 0, 0, 1, 0, 0,
            0, 0, 1, 0, 0, 0, 0, 1
        };
        float head_view[16], tabletop_view[16];
        poll_tabletop_tuning();
        if (!openxr_bridge_get_head_view_delta(g_head_translation_scale,
                                               g_world_eye_separation,
                                               head_view))
            memcpy(head_view, identity, sizeof(identity));
        g_head_translation_x = head_view[12];
        g_head_translation_y = head_view[13];
        vr_build_tabletop_view_f(g_tabletop_pitch_degrees, g_tabletop_horizontal_offset,
                                 g_tabletop_vertical_offset,
                                 g_tabletop_pivot_distance, tabletop_view);
        vr_multiply_matrix_f(head_view, tabletop_view, g_geometry_pair_head_view);
    }
    if (InterlockedCompareExchange(&g_geometry_pair_head_logged, 1, 0) == 0) {
        log_line("Tabletop stereo pair freezes one full head pose matrix "
                 "(translation=%.3f %.3f %.3f basisZ=%.3f %.3f %.3f)",
                 g_geometry_pair_head_view[12], g_geometry_pair_head_view[13],
                 g_geometry_pair_head_view[14], g_geometry_pair_head_view[8],
                 g_geometry_pair_head_view[9], g_geometry_pair_head_view[10]);
    }
    g_geometry_eye = eye;
    g_indicator_hide_follow_draws = 0;
    g_indicator_hide_matrix_active = 0;
    g_geometry_seen_perspective = 0;
    g_geometry_hud_base_captured = 0;
    g_pending_eye_view_offset = 0;
    g_menu_baseline_pending = 0;
    g_menu_baseline_captured = 0;
    LARGE_INTEGER total_start, render_finished, mirror_finished, capture_finished;
    QueryPerformanceCounter(&total_start);
    g_original_gameplay_render();
    QueryPerformanceCounter(&render_finished);
    g_menu_baseline_pending = 0;
    QueryPerformanceCounter(&mirror_finished);
    if (InterlockedCompareExchange(&g_geometry_first_pass_logged, 1, 0) == 0)
        log_line("True-geometry alternating eye render returned (eye=%d)", eye);
    /* Preserve the clean stereo eye before the game appends Tab/Escape or
       another full interface. The UI is captured independently below. */
    openxr_bridge_capture_eye(eye);
    if (!g_interface_alpha_capture_active)
        g_interface_alpha_capture_active =
            openxr_bridge_begin_interface_alpha_capture();
    QueryPerformanceCounter(&capture_finished);
    LONGLONG render_ticks = render_finished.QuadPart - total_start.QuadPart;
    LONGLONG mirror_ticks = mirror_finished.QuadPart - render_finished.QuadPart;
    LONGLONG capture_ticks = capture_finished.QuadPart - mirror_finished.QuadPart;
    LONGLONG total_ticks = capture_finished.QuadPart - total_start.QuadPart;
    g_perf_render_ticks += render_ticks;
    g_perf_mirror_ticks += mirror_ticks;
    g_perf_capture_ticks += capture_ticks;
    g_perf_total_ticks += total_ticks;
    if (total_ticks > g_perf_total_max_ticks) g_perf_total_max_ticks = total_ticks;
    if (!g_perf_frequency) {
        LARGE_INTEGER frequency;
        if (QueryPerformanceFrequency(&frequency)) g_perf_frequency = frequency.QuadPart;
    }
    if (g_perf_frequency && total_ticks * 10000 > g_perf_frequency * 65)
        ++g_perf_slow_eye_samples;
    if (++g_perf_eye_samples >= 240) {
        LARGE_INTEGER frequency;
        QueryPerformanceFrequency(&frequency);
        double scale = frequency.QuadPart > 0 ? 1000.0 / (double)frequency.QuadPart : 0.0;
        double average_total_ms = (double)g_perf_total_ticks * scale / g_perf_eye_samples;
        double slow_percent = 100.0 * (double)g_perf_slow_eye_samples / g_perf_eye_samples;
        log_line("VR eye timing avg render=%.3fms depth=%.3fms mirror=%.3fms capture=%.3fms total=%.3fms max=%.3fms samples=%ld",
                 (double)g_perf_render_ticks * scale / g_perf_eye_samples,
                 (double)g_perf_depth_ticks * scale / g_perf_eye_samples,
                 (double)g_perf_mirror_ticks * scale / g_perf_eye_samples,
                 (double)g_perf_capture_ticks * scale / g_perf_eye_samples,
                 average_total_ms,
                 (double)g_perf_total_max_ticks * scale, g_perf_eye_samples);
        float previous_cull_scale = g_peripheral_cull_scale;
        if ((average_total_ms > 6.0 || slow_percent > 5.0) &&
            g_peripheral_cull_scale > g_peripheral_cull_minimum + 0.001f) {
            g_peripheral_cull_scale -= 0.05f;
            if (g_peripheral_cull_scale < g_peripheral_cull_minimum)
                g_peripheral_cull_scale = g_peripheral_cull_minimum;
        } else if (average_total_ms < 5.0 && slow_percent < 1.0 &&
                   g_peripheral_cull_scale < g_peripheral_cull_target - 0.001f) {
            g_peripheral_cull_scale += 0.025f;
            if (g_peripheral_cull_scale > g_peripheral_cull_target)
                g_peripheral_cull_scale = g_peripheral_cull_target;
        }
        if (g_peripheral_cull_scale != previous_cull_scale)
            log_line("Adaptive peripheral rendering %.3fx -> %.3fx (eye avg %.3fms, slow %.1f%%, range %.3f-%.3f)",
                     previous_cull_scale, g_peripheral_cull_scale,
                     average_total_ms, slow_percent,
                     g_peripheral_cull_minimum, g_peripheral_cull_target);
        g_perf_render_ticks = g_perf_depth_ticks = g_perf_mirror_ticks = 0;
        g_perf_capture_ticks = g_perf_total_ticks = g_perf_total_max_ticks = 0;
        g_perf_eye_samples = g_perf_slow_eye_samples = 0;
    }
    if (eye == 0) g_geometry_has_presented_eye = 1;
    if (InterlockedCompareExchange(&g_geometry_first_capture_logged, 1, 0) == 0)
        log_line("True-geometry alternating eye capture returned (eye=%d)", eye);
    g_geometry_eye = -1;
    g_pending_eye_view_offset = 0;
    InterlockedExchange(&g_gameplay_render_reentry, 0);
}

static void hooked_world_draw(uint64_t render_target, void *focus_position)
{
    if (!g_original_world_draw) return;
    if (g_geometry_eye < 0 || g_peripheral_cull_scale <= 1.0001f) {
        g_original_world_draw(render_target, focus_position);
        return;
    }

    unsigned char *base = (unsigned char *)GetModuleHandleW(NULL);
    size_t culling_rva = g_game_build_layout->culling_bounds_rva;
    volatile float *minimum_x = (volatile float *)(base + culling_rva);
    volatile float *minimum_y = (volatile float *)(base + culling_rva + 4);
    volatile float *width = (volatile float *)(base + culling_rva + 8);
    volatile float *height = (volatile float *)(base + culling_rva + 12);
    float saved_x = *minimum_x, saved_y = *minimum_y;
    float saved_width = *width, saved_height = *height;
    int valid = isfinite(saved_x) && isfinite(saved_y) &&
                isfinite(saved_width) && isfinite(saved_height) &&
                saved_width > 1.0f && saved_height > 1.0f &&
                saved_width < 1000000.0f && saved_height < 1000000.0f;
    if (valid) {
        float center_x = saved_x + saved_width * 0.5f;
        float center_y = saved_y + saved_height * 0.5f;
        float expanded_width = saved_width * g_peripheral_cull_scale;
        float expanded_height = saved_height * g_peripheral_cull_scale;
        *minimum_x = center_x - expanded_width * 0.5f;
        *minimum_y = center_y - expanded_height * 0.5f;
        *width = expanded_width;
        *height = expanded_height;
        if (InterlockedCompareExchange(&g_peripheral_cull_logged, 1, 0) == 0)
            log_line("VR visual culling expanded %.2fx: %.1fx%.1f -> %.1fx%.1f",
                     g_peripheral_cull_scale, saved_width, saved_height,
                     expanded_width, expanded_height);
    }

    LONG trace = InterlockedIncrement(&g_world_draw_trace_count);
    int before_draw = (int)g_draw_framebuffer;
    int before_read = (int)g_read_framebuffer;
    int before_viewport[4] = {0};
    int before_samples = 0;
    if (trace <= 16 && g_gl_get_integerv) {
        g_gl_get_integerv(GL_VIEWPORT, before_viewport);
        g_gl_get_integerv(GL_SAMPLES, &before_samples);
    }
    g_original_world_draw(render_target, focus_position);
    /* Skillshot City draws the actual map directly into the default
       framebuffer.  Capture its depth here, while the world depth attachment
       is still intact and before the gameplay HUD is drawn over the color
       image.  The older resolve heuristic waited for a nonzero read FBO, but
       this renderer never has one at the world boundary. */
    if (g_geometry_eye >= 0 && g_gl_get_integerv) {
        int world_viewport[4] = {0};
        g_gl_get_integerv(GL_VIEWPORT, world_viewport);
        if (world_viewport[2] > 0 && world_viewport[3] > 0) {
            LARGE_INTEGER depth_start, depth_finished;
            QueryPerformanceCounter(&depth_start);
            openxr_bridge_capture_world(
                world_viewport[0], world_viewport[1],
                world_viewport[0] + world_viewport[2],
                world_viewport[1] + world_viewport[3], g_geometry_eye);
            QueryPerformanceCounter(&depth_finished);
            g_perf_depth_ticks += depth_finished.QuadPart - depth_start.QuadPart;
        }
    }
    if (trace <= 16) {
        int after_viewport[4] = {0};
        int after_samples = 0;
        if (g_gl_get_integerv) {
            g_gl_get_integerv(GL_VIEWPORT, after_viewport);
            g_gl_get_integerv(GL_SAMPLES, &after_samples);
        }
        log_line("World draw trace eye=%d target=0x%llX focus=%p beforeFBO=%d/%d vp=%dx%d samples=%d afterFBO=%u/%u vp=%dx%d samples=%d",
                 g_geometry_eye, (unsigned long long)render_target, focus_position,
                 before_read, before_draw, before_viewport[2], before_viewport[3],
                 before_samples, g_read_framebuffer, g_draw_framebuffer,
                 after_viewport[2], after_viewport[3], after_samples);
    }
    if (valid) {
        *minimum_x = saved_x;
        *minimum_y = saved_y;
        *width = saved_width;
        *height = saved_height;
    }
}

static int patch_world_draw(void)
{
    unsigned char *base = (unsigned char *)GetModuleHandleW(NULL);
    if (!base || !select_game_build_layout()) return 0;
    unsigned char *target = base + g_game_build_layout->world_draw_rva;
    if (memcmp(target, vr_world_draw_prologue,
               sizeof(vr_world_draw_prologue)) != 0) {
        log_line("Peripheral-cull hook refused: unsupported world-draw prologue");
        return 0;
    }
    unsigned char *trampoline = (unsigned char *)VirtualAlloc(NULL, 64,
        MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!trampoline) return 0;
    memcpy(trampoline, vr_world_draw_prologue, sizeof(vr_world_draw_prologue));
    unsigned char return_jump[14] = {0xff, 0x25, 0, 0, 0, 0};
    *(uint64_t *)(return_jump + 6) =
        (uint64_t)(uintptr_t)(target + sizeof(vr_world_draw_prologue));
    memcpy(trampoline + sizeof(vr_world_draw_prologue),
           return_jump, sizeof(return_jump));
    g_original_world_draw = (PFNWORLDDRAW)(uintptr_t)trampoline;

    unsigned char hook[18];
    memset(hook, 0x90, sizeof(hook));
    hook[0] = 0x48;
    hook[1] = 0xb8;
    *(uint64_t *)(hook + 2) = (uint64_t)(uintptr_t)&hooked_world_draw;
    hook[10] = 0xff;
    hook[11] = 0xe0;
    DWORD old_protection;
    if (!VirtualProtect(target, sizeof(hook), PAGE_EXECUTE_READWRITE, &old_protection)) {
        VirtualFree(trampoline, 0, MEM_RELEASE);
        g_original_world_draw = NULL;
        return 0;
    }
    memcpy(target, hook, sizeof(hook));
    VirtualProtect(target, sizeof(hook), old_protection, &old_protection);
    FlushInstructionCache(GetCurrentProcess(), target, sizeof(hook));
    return 1;
}

static int patch_gameplay_render(void)
{
    unsigned char *base = (unsigned char *)GetModuleHandleW(NULL);
    if (!base || !select_game_build_layout()) return 0;
    unsigned char *target = base + g_game_build_layout->gameplay_render_rva;
    if (memcmp(target, vr_gameplay_render_prologue,
               sizeof(vr_gameplay_render_prologue)) != 0) {
        log_line("Geometry hook refused: unsupported gameplay-render prologue");
        return 0;
    }
    unsigned char *trampoline = (unsigned char *)VirtualAlloc(NULL, 32, MEM_COMMIT | MEM_RESERVE,
                                                               PAGE_EXECUTE_READWRITE);
    if (!trampoline) return 0;
    memcpy(trampoline, vr_gameplay_render_prologue,
           sizeof(vr_gameplay_render_prologue));
    /* RIP-relative indirect jump preserves RAX. The displaced prologue's first
       instruction stores the entry stack pointer in RAX and later code needs it. */
    unsigned char return_jump[14] = {0xff, 0x25, 0, 0, 0, 0};
    *(uint64_t *)(return_jump + 6) =
        (uint64_t)(uintptr_t)(target + sizeof(vr_gameplay_render_prologue));
    memcpy(trampoline + sizeof(vr_gameplay_render_prologue),
           return_jump, sizeof(return_jump));
    g_original_gameplay_render = (PFNGAMEPLAYRENDER)(uintptr_t)trampoline;

    unsigned char hook_jump[12] = {0x48, 0xb8};
    *(uint64_t *)(hook_jump + 2) = (uint64_t)(uintptr_t)&hooked_gameplay_render;
    hook_jump[10] = 0xff;
    hook_jump[11] = 0xe0;
    DWORD old_protection;
    if (!VirtualProtect(target, sizeof(hook_jump), PAGE_EXECUTE_READWRITE, &old_protection)) return 0;
    memcpy(target, hook_jump, sizeof(hook_jump));
    VirtualProtect(target, sizeof(hook_jump), old_protection, &old_protection);
    FlushInstructionCache(GetCurrentProcess(), target, sizeof(hook_jump));
    return 1;
}

static DWORD WINAPI deferred_install(LPVOID unused)
{
    (void)unused;
    Sleep(100);
    int patched = patch_swap_buffers_import();
    log_line("SkillshotVR passive proxy loaded; deferred SwapBuffers hook=%s", patched ? "installed" : "not found");
    if (g_singleplayer_guard_enabled) {
        int network_patches = patch_network_guard_imports();
        log_line("Single-player network guard active; loopback-only patches=%d", network_patches);
    }
    if (g_geometry_hook_requested && !g_vr_runtime_disabled)
        log_line("True-geometry gameplay hook deferred until OpenXR is ready at a frame boundary");
    return 0;
}

static HGLRC WINAPI hooked_wgl_create_context_attribs(HDC dc, HGLRC shared, const int *attributes)
{
    char description[512] = {0};
    size_t used = 0;
    if (attributes) {
        for (int pair = 0; pair < 24 && attributes[pair * 2] != 0; ++pair) {
            int written = snprintf(description + used, sizeof(description) - used,
                                   "%s0x%X=%d", pair ? "," : "", attributes[pair * 2], attributes[pair * 2 + 1]);
            if (written < 0 || (size_t)written >= sizeof(description) - used) break;
            used += (size_t)written;
        }
    }
    log_line("wglCreateContextAttribsARB requested [%s]", description);
    return g_real_wgl_create_context_attribs ? g_real_wgl_create_context_attribs(dc, shared, attributes) : NULL;
}

static unsigned int shader_type_for(unsigned int shader)
{
    LONG count = g_shader_type_count;
    if (count > (LONG)(sizeof(g_shader_types) / sizeof(g_shader_types[0]))) {
        count = (LONG)(sizeof(g_shader_types) / sizeof(g_shader_types[0]));
    }
    for (LONG index = 0; index < count; ++index) {
        if (g_shader_types[index].shader == shader) return g_shader_types[index].type;
    }
    return 0;
}

static unsigned int APIENTRY hooked_gl_create_shader(unsigned int type)
{
    unsigned int shader = g_real_gl_create_shader ? g_real_gl_create_shader(type) : 0;
    LONG index = InterlockedIncrement(&g_shader_type_count) - 1;
    if (index >= 0 && index < (LONG)(sizeof(g_shader_types) / sizeof(g_shader_types[0]))) {
        g_shader_types[index].shader = shader;
        g_shader_types[index].type = type;
    }
    log_line("Created shader %u type=0x%X", shader, type);
    return shader;
}

static unsigned int APIENTRY hooked_gl_create_program(void)
{
    unsigned int program = g_real_gl_create_program ? g_real_gl_create_program() : 0;
    log_line("Created program %u", program);
    return program;
}

static void APIENTRY hooked_gl_shader_source(unsigned int shader, int count, const char *const *strings, const int *lengths)
{
    char filename[96];
    snprintf(filename, sizeof(filename), "SkillshotVR-shader-%u-type-%X.glsl", shader, shader_type_for(shader));
    char path[MAX_PATH];
    build_path(path, sizeof(path), filename);
    FILE *file = fopen(path, "wb");
    if (file) {
        for (int index = 0; index < count; ++index) {
            if (!strings[index]) continue;
            size_t length = lengths && lengths[index] >= 0 ? (size_t)lengths[index] : strlen(strings[index]);
            fwrite(strings[index], length, 1, file);
        }
        fclose(file);
        log_line("Captured shader source %u to %s", shader, filename);
    }
    if (g_real_gl_shader_source) g_real_gl_shader_source(shader, count, strings, lengths);
}

static void APIENTRY hooked_gl_attach_shader(unsigned int program, unsigned int shader)
{
    log_line("Attach shader %u to program %u", shader, program);
    if (g_real_gl_attach_shader) g_real_gl_attach_shader(program, shader);
}

static void APIENTRY hooked_gl_link_program(unsigned int program)
{
    log_line("Link program %u", program);
    if (g_real_gl_link_program) g_real_gl_link_program(program);
}

static void APIENTRY hooked_gl_use_program(unsigned int program)
{
    g_current_program = program;
    if (g_real_gl_use_program) g_real_gl_use_program(program);
}

static void APIENTRY hooked_gl_bind_framebuffer(unsigned int target, unsigned int framebuffer)
{
    if (g_geometry_eye >= 0 && InterlockedIncrement(&g_geometry_bind_trace_count) <= 64)
        log_line("Geometry framebuffer bind eye=%d target=0x%X framebuffer=%u previous=%u/%u",
                 g_geometry_eye, target, framebuffer,
                 g_read_framebuffer, g_draw_framebuffer);
    if (target == GL_FRAMEBUFFER || target == GL_DRAW_FRAMEBUFFER) g_draw_framebuffer = framebuffer;
    if (target == GL_FRAMEBUFFER || target == GL_READ_FRAMEBUFFER) g_read_framebuffer = framebuffer;
    unsigned int actual_framebuffer = framebuffer;
    if (g_interface_alpha_capture_active && framebuffer == 0 &&
        (target == GL_FRAMEBUFFER || target == GL_DRAW_FRAMEBUFFER)) {
        unsigned int capture = openxr_bridge_interface_alpha_framebuffer();
        if (capture) actual_framebuffer = capture;
    }
    if (g_real_gl_bind_framebuffer)
        g_real_gl_bind_framebuffer(target, actual_framebuffer);
}

static void APIENTRY hooked_gl_bind_renderbuffer(unsigned int target, unsigned int renderbuffer)
{
    g_bound_renderbuffer = renderbuffer;
    if (g_real_gl_bind_renderbuffer) g_real_gl_bind_renderbuffer(target, renderbuffer);
}

static void APIENTRY hooked_gl_renderbuffer_storage_multisample(unsigned int target, int samples,
                                                                 unsigned int internal_format,
                                                                 int width, int height)
{
    int vr_samples = g_geometry_hook_requested && samples > 2 ? 2 : samples;
    log_line("Renderbuffer storage id=%u target=0x%X samples=%d->%d format=0x%X size=%dx%d",
             g_bound_renderbuffer, target, samples, vr_samples, internal_format, width, height);
    if (g_real_gl_renderbuffer_storage_multisample)
        g_real_gl_renderbuffer_storage_multisample(target, vr_samples, internal_format, width, height);
}

static void APIENTRY hooked_gl_framebuffer_renderbuffer(unsigned int target, unsigned int attachment,
                                                         unsigned int renderbuffer_target,
                                                         unsigned int renderbuffer)
{
    log_line("Framebuffer renderbuffer draw=%u read=%u target=0x%X attachment=0x%X rbTarget=0x%X rb=%u",
             g_draw_framebuffer, g_read_framebuffer, target, attachment,
             renderbuffer_target, renderbuffer);
    if (g_real_gl_framebuffer_renderbuffer)
        g_real_gl_framebuffer_renderbuffer(target, attachment, renderbuffer_target, renderbuffer);
}

__declspec(dllexport) void APIENTRY glBindTexture(unsigned int target, unsigned int texture)
{
    if (target == 0x0DE1u) g_bound_texture_2d = texture;
    if (target == 0x9100u) g_bound_texture_2d_multisample = texture;
    if (!g_real_gl_bind_texture) g_real_gl_bind_texture = (PFNGLBINDTEXTURE)real_gl_proc("glBindTexture");
    if (g_real_gl_bind_texture) g_real_gl_bind_texture(target, texture);
}

static void APIENTRY hooked_gl_tex_image_2d_multisample(unsigned int target, int samples,
                                                         unsigned int internal_format,
                                                         int width, int height, unsigned char fixed)
{
    int vr_samples = g_geometry_hook_requested && samples > 2 ? 2 : samples;
    log_line("Multisample texture id=%u target=0x%X samples=%d->%d format=0x%X size=%dx%d fixed=%u",
             g_bound_texture_2d_multisample, target, samples, vr_samples,
             internal_format, width, height, fixed);
    if (g_real_gl_tex_image_2d_multisample)
        g_real_gl_tex_image_2d_multisample(target, vr_samples, internal_format, width, height, fixed);
}

static void APIENTRY hooked_gl_framebuffer_texture_2d(unsigned int target, unsigned int attachment,
                                                       unsigned int texture_target,
                                                       unsigned int texture, int level)
{
    if (g_frame_count < 5)
        log_line("Framebuffer texture draw=%u read=%u target=0x%X attachment=0x%X texTarget=0x%X tex=%u level=%d",
                 g_draw_framebuffer, g_read_framebuffer, target, attachment, texture_target, texture, level);
    if (g_real_gl_framebuffer_texture_2d)
        g_real_gl_framebuffer_texture_2d(target, attachment, texture_target, texture, level);
}

static void APIENTRY hooked_gl_blit_framebuffer(int source_x0, int source_y0, int source_x1, int source_y1,
                                                int destination_x0, int destination_y0, int destination_x1, int destination_y1,
                                                unsigned int mask, unsigned int filter)
{
    record_draw('B', mask, (source_x1 - source_x0) * (source_y1 - source_y0));
    int source_width = abs(source_x1 - source_x0);
    int source_height = abs(source_y1 - source_y0);
    int destination_width = abs(destination_x1 - destination_x0);
    int destination_height = abs(destination_y1 - destination_y0);
    int viewport[4] = {0};
    if (g_geometry_eye >= 0 && g_gl_get_integerv)
        g_gl_get_integerv(GL_VIEWPORT, viewport);
    int gameplay_resolve = g_geometry_eye >= 0 && g_read_framebuffer != 0 &&
        g_draw_framebuffer == 0 && (mask & GL_COLOR_BUFFER_BIT) &&
        source_width == viewport[2] && source_height == viewport[3] &&
        destination_width == viewport[2] && destination_height == viewport[3];
    LONG geometry_blit_trace = g_geometry_eye >= 0
        ? InterlockedIncrement(&g_geometry_blit_trace_count) : 0;
    if (geometry_blit_trace > 0 && geometry_blit_trace <= 32)
        log_line("Geometry blit trace eye=%d readFBO=%u drawFBO=%u viewport=%dx%d source=%d,%d-%d,%d destination=%d,%d-%d,%d mask=0x%X filter=0x%X",
                 g_geometry_eye, g_read_framebuffer, g_draw_framebuffer,
                 viewport[2], viewport[3], source_x0, source_y0, source_x1, source_y1,
                 destination_x0, destination_y0, destination_x1, destination_y1,
                 mask, filter);
    if (gameplay_resolve && geometry_blit_trace <= 32)
        log_line("Gameplay resolve candidate eye=%d readFBO=%u drawFBO=%u source=%dx%d destination=%dx%d mask=0x%X filter=0x%X",
                 g_geometry_eye, g_read_framebuffer, g_draw_framebuffer,
                 source_width, source_height, destination_width, destination_height,
                 mask, filter);
    if (g_frame_count >= 100 && g_world_capture_frame != g_frame_count && gameplay_resolve &&
        openxr_bridge_capture_world(source_x0, source_y0, source_x1, source_y1,
                                    g_geometry_eye)) {
        g_world_capture_frame = g_frame_count;
    }
    if (g_frame_count == 119) {
        /* The game's scene framebuffer is multisampled, so direct glReadPixels on
           its depth attachment is not useful. Resolve depth into the default
           framebuffer for this single diagnostic frame, read it there, then let
           the game's normal color resolve continue unchanged. */
        g_real_gl_blit_framebuffer(source_x0, source_y0, source_x1, source_y1,
                                   destination_x0, destination_y0, destination_x1, destination_y1,
                                   GL_DEPTH_BUFFER_BIT, GL_NEAREST);
        if (g_real_gl_bind_framebuffer) {
            unsigned int previous_read = g_read_framebuffer;
            g_real_gl_bind_framebuffer(GL_READ_FRAMEBUFFER, 0);
            g_read_framebuffer = 0;
            capture_depth_buffer(120);
            g_real_gl_bind_framebuffer(GL_READ_FRAMEBUFFER, previous_read);
            g_read_framebuffer = previous_read;
        }
        log_line("Frame 120 blit readFBO=%u drawFBO=%u source=%d,%d-%d,%d destination=%d,%d-%d,%d mask=0x%X filter=0x%X",
                 g_read_framebuffer, g_draw_framebuffer, source_x0, source_y0, source_x1, source_y1,
                 destination_x0, destination_y0, destination_x1, destination_y1, mask, filter);
    }
    g_real_gl_blit_framebuffer(source_x0, source_y0, source_x1, source_y1,
                               destination_x0, destination_y0, destination_x1, destination_y1, mask, filter);
    if (g_frame_count == 119 && g_real_gl_bind_framebuffer) {
        unsigned int previous_read = g_read_framebuffer;
        g_real_gl_bind_framebuffer(GL_READ_FRAMEBUFFER, 0);
        g_read_framebuffer = 0;
        capture_bmp_named("SkillshotVR-frame-000120-world-only.bmp", 120);
        g_real_gl_bind_framebuffer(GL_READ_FRAMEBUFFER, previous_read);
        g_read_framebuffer = previous_read;
    }
}

static void APIENTRY hooked_gl_draw_arrays_instanced(unsigned int mode, int first, int count, int instances)
{
    record_draw('I', mode, count * instances);
    g_real_gl_draw_arrays_instanced(mode, first, count, instances);
    if (g_interface_alpha_capture_active && g_projection_is_orthographic)
        for (int pass = 0; pass < 2; ++pass)
            if (openxr_bridge_begin_interface_alpha_draw(pass)) {
                g_real_gl_draw_arrays_instanced(mode, first, count, instances);
                openxr_bridge_end_interface_alpha_draw();
            }
}

static void APIENTRY hooked_gl_draw_elements_instanced(unsigned int mode, int count, unsigned int type, const void *indices, int instances)
{
    record_draw('J', mode, count * instances);
    g_real_gl_draw_elements_instanced(mode, count, type, indices, instances);
    if (g_interface_alpha_capture_active && g_projection_is_orthographic)
        for (int pass = 0; pass < 2; ++pass)
            if (openxr_bridge_begin_interface_alpha_draw(pass)) {
                g_real_gl_draw_elements_instanced(mode, count, type, indices, instances);
                openxr_bridge_end_interface_alpha_draw();
            }
}

static void APIENTRY hooked_gl_draw_range_elements(unsigned int mode, unsigned int start, unsigned int end, int count, unsigned int type, const void *indices)
{
    record_draw('G', mode, count);
    g_real_gl_draw_range_elements(mode, start, end, count, type, indices);
    if (g_interface_alpha_capture_active && g_projection_is_orthographic)
        for (int pass = 0; pass < 2; ++pass)
            if (openxr_bridge_begin_interface_alpha_draw(pass)) {
                g_real_gl_draw_range_elements(mode, start, end, count, type, indices);
                openxr_bridge_end_interface_alpha_draw();
            }
}

static void APIENTRY hooked_gl_draw_elements_base_vertex(unsigned int mode, int count, unsigned int type, const void *indices, int base_vertex)
{
    record_draw('V', mode, count);
    g_real_gl_draw_elements_base_vertex(mode, count, type, indices, base_vertex);
    if (g_interface_alpha_capture_active && g_projection_is_orthographic)
        for (int pass = 0; pass < 2; ++pass)
            if (openxr_bridge_begin_interface_alpha_draw(pass)) {
                g_real_gl_draw_elements_base_vertex(mode, count, type, indices, base_vertex);
                openxr_bridge_end_interface_alpha_draw();
            }
}

static void APIENTRY hooked_gl_draw_range_elements_base_vertex(unsigned int mode, unsigned int start, unsigned int end, int count, unsigned int type, const void *indices, int base_vertex)
{
    record_draw('R', mode, count);
    g_real_gl_draw_range_elements_base_vertex(mode, start, end, count, type, indices, base_vertex);
    if (g_interface_alpha_capture_active && g_projection_is_orthographic)
        for (int pass = 0; pass < 2; ++pass)
            if (openxr_bridge_begin_interface_alpha_draw(pass)) {
                g_real_gl_draw_range_elements_base_vertex(mode, start, end, count, type, indices, base_vertex);
                openxr_bridge_end_interface_alpha_draw();
            }
}

static void APIENTRY hooked_gl_draw_elements_instanced_base_vertex(unsigned int mode, int count, unsigned int type, const void *indices, int instances, int base_vertex)
{
    record_draw('K', mode, count * instances);
    g_real_gl_draw_elements_instanced_base_vertex(mode, count, type, indices, instances, base_vertex);
    if (g_interface_alpha_capture_active && g_projection_is_orthographic)
        for (int pass = 0; pass < 2; ++pass)
            if (openxr_bridge_begin_interface_alpha_draw(pass)) {
                g_real_gl_draw_elements_instanced_base_vertex(mode, count, type, indices, instances, base_vertex);
                openxr_bridge_end_interface_alpha_draw();
            }
}

static void APIENTRY hooked_gl_multi_draw_arrays(unsigned int mode, const int *first, const int *counts, int draw_count)
{
    int total = 0;
    for (int index = 0; index < draw_count; ++index) total += counts[index];
    record_draw('M', mode, total);
    g_real_gl_multi_draw_arrays(mode, first, counts, draw_count);
    if (g_interface_alpha_capture_active && g_projection_is_orthographic)
        for (int pass = 0; pass < 2; ++pass)
            if (openxr_bridge_begin_interface_alpha_draw(pass)) {
                g_real_gl_multi_draw_arrays(mode, first, counts, draw_count);
                openxr_bridge_end_interface_alpha_draw();
            }
}

static void APIENTRY hooked_gl_multi_draw_elements(unsigned int mode, const int *counts, unsigned int type, const void *const *indices, int draw_count)
{
    int total = 0;
    for (int index = 0; index < draw_count; ++index) total += counts[index];
    record_draw('N', mode, total);
    g_real_gl_multi_draw_elements(mode, counts, type, indices, draw_count);
    if (g_interface_alpha_capture_active && g_projection_is_orthographic)
        for (int pass = 0; pass < 2; ++pass)
            if (openxr_bridge_begin_interface_alpha_draw(pass)) {
                g_real_gl_multi_draw_elements(mode, counts, type, indices, draw_count);
                openxr_bridge_end_interface_alpha_draw();
            }
}

static void APIENTRY hooked_gl_multi_draw_elements_base_vertex(unsigned int mode, const int *counts, unsigned int type, const void *const *indices, int draw_count, const int *base_vertices)
{
    int total = 0;
    for (int index = 0; index < draw_count; ++index) total += counts[index];
    record_draw('W', mode, total);
    g_real_gl_multi_draw_elements_base_vertex(mode, counts, type, indices, draw_count, base_vertices);
    if (g_interface_alpha_capture_active && g_projection_is_orthographic)
        for (int pass = 0; pass < 2; ++pass)
            if (openxr_bridge_begin_interface_alpha_draw(pass)) {
                g_real_gl_multi_draw_elements_base_vertex(mode, counts, type, indices, draw_count, base_vertices);
                openxr_bridge_end_interface_alpha_draw();
            }
}

static BOOL WINAPI hooked_wgl_swap_interval(int interval)
{
    if (!g_real_wgl_swap_interval) return FALSE;
    /* Alternating-eye geometry needs two game renders for every coherent
       stereo pair.  If desktop VSync clamps the game to the headset cadence,
       a 72 Hz session can only receive about 36 fresh pairs.  The OpenXR
       thread already owns headset pacing, so leave the ordinary game window
       uncapped in VR while preserving the user's setting in standard mode. */
    int applied = g_vr_runtime_disabled ? interval : 0;
    if (!g_vr_runtime_disabled &&
        InterlockedCompareExchange(&g_vr_swap_interval_logged, 1, 0) == 0)
        log_line("VR render pacing disabled desktop swap interval (requested=%d applied=0)",
                 interval);
    return g_real_wgl_swap_interval(applied);
}

__declspec(dllexport) PROC WINAPI wglGetProcAddress(LPCSTR name)
{
    if (!g_real_wgl_get_proc_address) {
        g_real_wgl_get_proc_address = (PFNWGLGETPROCADDRESS)real_gl_proc("wglGetProcAddress");
    }
    if (!g_real_wgl_get_proc_address) return NULL;
    PROC result = g_real_wgl_get_proc_address(name);
    if (name && result) log_line("Requested GL procedure: %s -> %p", name, (void *)result);
    if (name && strcmp(name, "wglCreateContextAttribsARB") == 0 && result) {
        g_real_wgl_create_context_attribs = (PFNWGLCREATECONTEXTATTRIBSARB)result;
        return (PROC)&hooked_wgl_create_context_attribs;
    }
    if (name && strcmp(name, "wglSwapIntervalEXT") == 0 && result) {
        g_real_wgl_swap_interval = (PFNWGLSWAPINTERVALEXT)result;
        return (PROC)&hooked_wgl_swap_interval;
    }
    if (name && strcmp(name, "glCreateShader") == 0 && result) {
        g_real_gl_create_shader = (PFNGLCREATESHADER)result;
        return (PROC)&hooked_gl_create_shader;
    }
    if (name && strcmp(name, "glCreateProgram") == 0 && result) {
        g_real_gl_create_program = (PFNGLCREATEPROGRAM)result;
        return (PROC)&hooked_gl_create_program;
    }
    if (name && strcmp(name, "glShaderSource") == 0 && result) {
        g_real_gl_shader_source = (PFNGLSHADERSOURCE)result;
        return (PROC)&hooked_gl_shader_source;
    }
    if (name && strcmp(name, "glAttachShader") == 0 && result) {
        g_real_gl_attach_shader = (PFNGLATTACHSHADER)result;
        return (PROC)&hooked_gl_attach_shader;
    }
    if (name && strcmp(name, "glLinkProgram") == 0 && result) {
        g_real_gl_link_program = (PFNGLLINKPROGRAM)result;
        return (PROC)&hooked_gl_link_program;
    }
    if (name && strcmp(name, "glUseProgram") == 0 && result) {
        g_real_gl_use_program = (PFNGLUSEPROGRAM)result;
        return (PROC)&hooked_gl_use_program;
    }
    if (name && strcmp(name, "glBindFramebuffer") == 0 && result) {
        g_real_gl_bind_framebuffer = (PFNGLBINDFRAMEBUFFER)result;
        return (PROC)&hooked_gl_bind_framebuffer;
    }
    if (name && strcmp(name, "glBlendFuncSeparate") == 0 && result) {
        g_real_gl_blend_func_separate = (PFNGLBLENDFUNCSEPARATE)result;
        return (PROC)&hooked_gl_blend_func_separate;
    }
    if (name && strcmp(name, "glBlendEquation") == 0 && result) {
        g_real_gl_blend_equation = (PFNGLBLENDEQUATION)result;
        return (PROC)&hooked_gl_blend_equation;
    }
    if (name && strcmp(name, "glBlendEquationSeparate") == 0 && result) {
        g_real_gl_blend_equation_separate = (PFNGLBLENDEQUATIONSEPARATE)result;
        return (PROC)&hooked_gl_blend_equation_separate;
    }
    if (name && strcmp(name, "glBindRenderbuffer") == 0 && result) {
        g_real_gl_bind_renderbuffer = (PFNGLBINDRENDERBUFFER)result;
        return (PROC)&hooked_gl_bind_renderbuffer;
    }
    if (name && strcmp(name, "glRenderbufferStorageMultisample") == 0 && result) {
        g_real_gl_renderbuffer_storage_multisample = (PFNGLRENDERBUFFERSTORAGEMULTISAMPLE)result;
        return (PROC)&hooked_gl_renderbuffer_storage_multisample;
    }
    if (name && strcmp(name, "glFramebufferRenderbuffer") == 0 && result) {
        g_real_gl_framebuffer_renderbuffer = (PFNGLFRAMEBUFFERRENDERBUFFER)result;
        return (PROC)&hooked_gl_framebuffer_renderbuffer;
    }
    if (name && strcmp(name, "glTexImage2DMultisample") == 0 && result) {
        g_real_gl_tex_image_2d_multisample = (PFNGLTEXIMAGE2DMULTISAMPLE)result;
        return (PROC)&hooked_gl_tex_image_2d_multisample;
    }
    if (name && strcmp(name, "glFramebufferTexture2D") == 0 && result) {
        g_real_gl_framebuffer_texture_2d = (PFNGLFRAMEBUFFERTEXTURE2D)result;
        return (PROC)&hooked_gl_framebuffer_texture_2d;
    }
    if (name && strcmp(name, "glDrawArrays") == 0 && result) {
        g_real_gl_draw_arrays = (PFNGLDRAWARRAYS)result;
        return (PROC)&glDrawArrays;
    }
    if (name && strcmp(name, "glDrawElements") == 0 && result) {
        g_real_gl_draw_elements = (PFNGLDRAWELEMENTS)result;
        return (PROC)&glDrawElements;
    }
    if (name && strcmp(name, "glDrawRangeElements") == 0 && result) {
        g_real_gl_draw_range_elements = (PFNGLDRAWRANGEELEMENTS)result;
        return (PROC)&hooked_gl_draw_range_elements;
    }
    if (name && strcmp(name, "glBegin") == 0 && result) {
        g_real_gl_begin = (PFNGLBEGIN)result;
        return (PROC)&glBegin;
    }
    if (name && strcmp(name, "glEnd") == 0 && result) {
        g_real_gl_end = (PFNGLEND)result;
        return (PROC)&glEnd;
    }
    if (name && strcmp(name, "glCallList") == 0 && result) {
        g_real_gl_call_list = (PFNGLCALLLIST)result;
        return (PROC)&glCallList;
    }
    if (name && strcmp(name, "glCallLists") == 0 && result) {
        g_real_gl_call_lists = (PFNGLCALLLISTS)result;
        return (PROC)&glCallLists;
    }
    if (name && strcmp(name, "glDrawArraysInstanced") == 0 && result) {
        g_real_gl_draw_arrays_instanced = (PFNGLDRAWARRAYSINSTANCED)result;
        return (PROC)&hooked_gl_draw_arrays_instanced;
    }
    if (name && strcmp(name, "glDrawElementsInstanced") == 0 && result) {
        g_real_gl_draw_elements_instanced = (PFNGLDRAWELEMENTSINSTANCED)result;
        return (PROC)&hooked_gl_draw_elements_instanced;
    }
    if (name && strcmp(name, "glDrawElementsBaseVertex") == 0 && result) {
        g_real_gl_draw_elements_base_vertex = (PFNGLDRAWELEMENTSBASEVERTEX)result;
        return (PROC)&hooked_gl_draw_elements_base_vertex;
    }
    if (name && strcmp(name, "glDrawRangeElementsBaseVertex") == 0 && result) {
        g_real_gl_draw_range_elements_base_vertex = (PFNGLDRAWRANGEELEMENTSBASEVERTEX)result;
        return (PROC)&hooked_gl_draw_range_elements_base_vertex;
    }
    if (name && strcmp(name, "glDrawElementsInstancedBaseVertex") == 0 && result) {
        g_real_gl_draw_elements_instanced_base_vertex = (PFNGLDRAWELEMENTSINSTANCEDBASEVERTEX)result;
        return (PROC)&hooked_gl_draw_elements_instanced_base_vertex;
    }
    if (name && strcmp(name, "glMultiDrawArrays") == 0 && result) {
        g_real_gl_multi_draw_arrays = (PFNGLMULTIDRAWARRAYS)result;
        return (PROC)&hooked_gl_multi_draw_arrays;
    }
    if (name && strcmp(name, "glMultiDrawElements") == 0 && result) {
        g_real_gl_multi_draw_elements = (PFNGLMULTIDRAWELEMENTS)result;
        return (PROC)&hooked_gl_multi_draw_elements;
    }
    if (name && strcmp(name, "glMultiDrawElementsBaseVertex") == 0 && result) {
        g_real_gl_multi_draw_elements_base_vertex = (PFNGLMULTIDRAWELEMENTSBASEVERTEX)result;
        return (PROC)&hooked_gl_multi_draw_elements_base_vertex;
    }
    if (name && strcmp(name, "glBlitFramebuffer") == 0 && result) {
        g_real_gl_blit_framebuffer = (PFNGLBLITFRAMEBUFFER)result;
        return (PROC)&hooked_gl_blit_framebuffer;
    }
    return result;
}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved)
{
    (void)reserved;
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(instance);
        GetModuleFileNameA(instance, g_directory, sizeof(g_directory));
        char *separator = strrchr(g_directory, '\\');
        if (separator) *separator = '\0';
        char overscan_path[MAX_PATH];
        build_path(overscan_path, sizeof(overscan_path), "SkillshotVR-overscan-test.txt");
        g_overscan_enabled = GetFileAttributesA(overscan_path) != INVALID_FILE_ATTRIBUTES;
        char guard_path[MAX_PATH];
        build_path(guard_path, sizeof(guard_path), "SkillshotVR-singleplayer-guard.txt");
        g_singleplayer_guard_enabled = GetFileAttributesA(guard_path) != INVALID_FILE_ATTRIBUTES;
        char geometry_path[MAX_PATH];
        build_path(geometry_path, sizeof(geometry_path), "SkillshotCityVR-geometry-stereo.txt");
        g_geometry_hook_requested = GetFileAttributesA(geometry_path) != INVALID_FILE_ATTRIBUTES;
        char geometry_config_path[MAX_PATH];
        build_path(geometry_config_path, sizeof(geometry_config_path), "SkillshotCityVR-geometry.ini");
        FILE *geometry_config = fopen(geometry_config_path, "r");
        if (geometry_config) {
            char line[256];
            while (fgets(line, sizeof(line), geometry_config)) {
                char *cursor = line;
                while (*cursor == ' ' || *cursor == '\t') ++cursor;
                if (*cursor == '\0' || *cursor == '\r' || *cursor == '\n' ||
                    *cursor == '#' || *cursor == ';' ||
                    (cursor[0] == '/' && cursor[1] == '/')) continue;
                char key[64] = {0};
                float value = 0.0f;
                if (sscanf(cursor, "%63[^=]=%f", key, &value) != 2) continue;
                size_t key_length = strlen(key);
                while (key_length > 0 &&
                       (key[key_length - 1] == ' ' || key[key_length - 1] == '\t'))
                    key[--key_length] = '\0';
                if (strcmp(key, "world_eye_separation") == 0 && value >= 0.0f && value <= 400.0f)
                    g_world_eye_separation = value;
                if (strcmp(key, "world_convergence_distance") == 0 && value >= 100.0f && value <= 5000.0f)
                    g_world_convergence_distance = value;
                if (strcmp(key, "auto_convergence") == 0)
                    g_auto_convergence = value >= 0.5f;
                if (strcmp(key, "vertical_fov_scale") == 0 && value >= 1.0f && value <= 2.5f)
                    g_geometry_vertical_fov_scale = value;
                if (strcmp(key, "depth_exaggeration") == 0 && value >= 1.0f && value <= 6.0f)
                    g_depth_exaggeration = value;
                if (strcmp(key, "head_translation_scale") == 0 && value >= 0.0f && value <= 200.0f)
                    g_head_translation_scale = value;
                if (strcmp(key, "peripheral_cull_scale") == 0 && value >= 1.0f && value <= 4.0f)
                    g_peripheral_cull_scale = value;
                if (strcmp(key, "tabletop_pitch_degrees") == 0 && value >= 10.0f && value <= 65.0f)
                    g_tabletop_pitch_degrees = value;
                if (strcmp(key, "tabletop_horizontal_offset") == 0 && value >= -2400.0f && value <= 2400.0f)
                    g_tabletop_horizontal_offset = value;
                if (strcmp(key, "tabletop_vertical_offset") == 0 && value >= -1200.0f && value <= 600.0f)
                    g_tabletop_vertical_offset = value;
                if (strcmp(key, "tabletop_pivot_distance") == 0 && value >= 500.0f && value <= 10000.0f)
                    g_tabletop_pivot_distance = value;
                if (strcmp(key, "hud_safe_scale_x") == 0 && value >= 0.35f && value <= 1.0f)
                    g_hud_safe_scale_x = value;
                if (strcmp(key, "hud_safe_scale_y") == 0 && value >= 0.30f && value <= 1.0f)
                    g_hud_safe_scale_y = value;
            }
            fclose(geometry_config);
        }
        g_peripheral_cull_target = g_peripheral_cull_scale;
        g_peripheral_cull_minimum = g_peripheral_cull_target > 1.30f
            ? 1.30f : g_peripheral_cull_target;
        load_camera_tuning();
        char runtime_mode[32] = {0};
        GetEnvironmentVariableA("SKILLSHOTCITYVR_MODE", runtime_mode, sizeof(runtime_mode));
        g_hud_preview_enabled = _stricmp(runtime_mode, "hud-preview") == 0;
        g_vr_runtime_disabled = _stricmp(runtime_mode, "standard") == 0 ||
                                g_hud_preview_enabled;
        char log_path[MAX_PATH];
        build_path(log_path, sizeof(log_path), "SkillshotVR.log");
        DeleteFileA(log_path);
        log_line("Geometry config loaded: separation=%.3f convergence=%.3f auto-convergence=%d vertical=%.3f depth=%.3f head-scale=%.3f cull-scale=%.3f adaptive-min=%.3f tabletop-pitch=%.1f tabletop-x=%.1f tabletop-y=%.1f pivot=%.1f hud-safe=%.3f,%.3f",
                 g_world_eye_separation, g_world_convergence_distance,
                 g_auto_convergence, g_geometry_vertical_fov_scale, g_depth_exaggeration,
                 g_head_translation_scale, g_peripheral_cull_scale,
                 g_peripheral_cull_minimum,
                 g_tabletop_pitch_degrees, g_tabletop_horizontal_offset,
                 g_tabletop_vertical_offset,
                 g_tabletop_pivot_distance, g_hud_safe_scale_x, g_hud_safe_scale_y);
        char debug_path[MAX_PATH];
        build_path(debug_path, sizeof(debug_path), "SkillshotVR-debug-skip.txt");
        FILE *debug_file = fopen(debug_path, "r");
        if (debug_file) {
            char mode[16] = {0};
            if (fgets(mode, sizeof(mode), debug_file)) {
                if (strncmp(mode, "head", 4) == 0) g_debug_skip_mode = 1;
                if (strncmp(mode, "tail", 4) == 0) g_debug_skip_mode = 2;
                if (strncmp(mode, "lists", 5) == 0) g_debug_skip_mode = 3;
            }
            fclose(debug_file);
        }
        HANDLE installer = CreateThread(NULL, 0, deferred_install, NULL, 0, NULL);
        if (installer) CloseHandle(installer);
    }
    return TRUE;
}
