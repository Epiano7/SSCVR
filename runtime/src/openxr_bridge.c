#define WIN32_LEAN_AND_MEAN
#define COBJMACROS
#define XR_USE_PLATFORM_WIN32
#define XR_USE_GRAPHICS_API_D3D11
#include <windows.h>
#include <d3d11.h>
#include <d3d11_4.h>
#include <d3dcompiler.h>
#include <dxgi1_2.h>
#include <GL/gl.h>
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "openxr_bridge.h"
#include "geometry_math.h"

#define EYE_COUNT 2
#define PAIR_BUFFER_COUNT 3
#define MAX_SWAPCHAIN_IMAGES 8
#define GL_READ_FRAMEBUFFER_BINDING 0x8CAA
#define GL_DRAW_FRAMEBUFFER_BINDING 0x8CA6
#define GL_READ_FRAMEBUFFER 0x8CA8
#define GL_DRAW_FRAMEBUFFER 0x8CA9
#define GL_COLOR_ATTACHMENT0 0x8CE0
#define GL_COLOR_CLEAR_VALUE 0x0C22
#define GL_DEPTH_ATTACHMENT 0x8D00
#define GL_DEPTH_STENCIL_ATTACHMENT 0x821A
#define GL_FRAMEBUFFER_COMPLETE 0x8CD5
#define GL_COLOR_BUFFER_BIT 0x00004000
#define GL_DEPTH_BUFFER_BIT 0x00000100
#define GL_STENCIL_BUFFER_BIT 0x00000400
#define GL_NEAREST 0x2600
#define GL_LINEAR 0x2601
#define GL_RGBA 0x1908
#define GL_UNSIGNED_BYTE 0x1401
#define GL_TEXTURE_MIN_FILTER 0x2801
#define GL_TEXTURE_MAG_FILTER 0x2800
#define GL_TEXTURE_BINDING_2D 0x8069
#define GL_BLEND_DST_RGB 0x80C8
#define GL_BLEND_SRC_RGB 0x80C9
#define GL_BLEND_DST_ALPHA 0x80CA
#define GL_BLEND_SRC_ALPHA 0x80CB
#define GL_ACTIVE_TEXTURE 0x84E0
#define GL_TEXTURE0 0x84C0
#define GL_CURRENT_PROGRAM 0x8B8D
#define GL_MATRIX_MODE 0x0BA0
#define GL_CURRENT_COLOR 0x0B00
#define GL_BLEND_EQUATION_RGB 0x8009
#define GL_BLEND_EQUATION_ALPHA 0x883D
#define GL_FUNC_ADD 0x8006
#define GL_TEXTURE_ENV 0x2300
#define GL_TEXTURE_ENV_MODE 0x2200
#define GL_REPLACE 0x1E01
#define GL_ONE 1
#define GL_ONE_MINUS_SRC_ALPHA 0x0303
#define GL_SAMPLES 0x80A9
#define GL_NONE 0
#define GL_TEXTURE_2D_MULTISAMPLE 0x9100
#define WGL_ACCESS_READ_WRITE_NV 0x0001
#define WGL_ACCESS_WRITE_DISCARD_NV 0x0002

typedef HANDLE (WINAPI *PFNWGLDXOPENDEVICENV)(void *dx_device);
typedef BOOL (WINAPI *PFNWGLDXCLOSEDEVICENV)(HANDLE device);
typedef HANDLE (WINAPI *PFNWGLDXREGISTEROBJECTNV)(HANDLE device, void *dx_object,
                                                  GLuint name, GLenum type, GLenum access);
typedef BOOL (WINAPI *PFNWGLDXUNREGISTEROBJECTNV)(HANDLE device, HANDLE object);
typedef BOOL (WINAPI *PFNWGLDXLOCKOBJECTSNV)(HANDLE device, GLint count, HANDLE *objects);
typedef BOOL (WINAPI *PFNWGLDXUNLOCKOBJECTSNV)(HANDLE device, GLint count, HANDLE *objects);
typedef void (APIENTRY *PFNGLGENFRAMEBUFFERS)(GLsizei count, GLuint *framebuffers);
typedef void (APIENTRY *PFNGLBINDFRAMEBUFFER)(GLenum target, GLuint framebuffer);
typedef void (APIENTRY *PFNGLFRAMEBUFFERTEXTURE2D)(GLenum target, GLenum attachment,
                                                   GLenum texture_target, GLuint texture, GLint level);
typedef GLenum (APIENTRY *PFNGLCHECKFRAMEBUFFERSTATUS)(GLenum target);
typedef void (APIENTRY *PFNGLBLITFRAMEBUFFER)(GLint source_x0, GLint source_y0,
                                              GLint source_x1, GLint source_y1,
                                              GLint target_x0, GLint target_y0,
                                              GLint target_x1, GLint target_y1,
                                              GLbitfield mask, GLenum filter);
typedef PROC (WINAPI *PFNWGLGETPROCADDRESSRAW)(LPCSTR name);
typedef void (APIENTRY *PFNGLGETINTEGERVRAW)(GLenum name, GLint *value);
typedef void (APIENTRY *PFNGLGENTEXTURESRAW)(GLsizei count, GLuint *textures);
typedef GLenum (APIENTRY *PFNGLGETERRORRAW)(void);
typedef void (APIENTRY *PFNGLFINISHRAW)(void);
typedef void (APIENTRY *PFNGLDRAWBUFFERRAW)(GLenum buffer);
typedef void (APIENTRY *PFNGLBINDTEXTURERAW)(GLenum target, GLuint texture);
typedef void (APIENTRY *PFNGLTEXIMAGE2DRAW)(GLenum target, GLint level, GLint internal_format,
                                            GLsizei width, GLsizei height, GLint border,
                                            GLenum format, GLenum type, const void *pixels);
typedef void (APIENTRY *PFNGLTEXPARAMETERIRAW)(GLenum target, GLenum name, GLint value);
typedef void (APIENTRY *PFNGLGETFLOATVRAW)(GLenum name, GLfloat *value);
typedef void (APIENTRY *PFNGLCLEARCOLORRAW)(GLfloat red, GLfloat green,
                                            GLfloat blue, GLfloat alpha);
typedef void (APIENTRY *PFNGLCLEARRAW)(GLbitfield mask);
typedef void (APIENTRY *PFNGLBLENDFUNCSEPARATERAW)(GLenum source_rgb,
                                                    GLenum destination_rgb,
                                                    GLenum source_alpha,
                                                    GLenum destination_alpha);
typedef GLboolean (APIENTRY *PFNGLISENABLEDRAW)(GLenum capability);
typedef void (APIENTRY *PFNGLENABLEDISABLERAW)(GLenum capability);
typedef void (APIENTRY *PFNGLUSEPROGRAMRAW)(GLuint program);
typedef void (APIENTRY *PFNGLACTIVETEXTURERAW)(GLenum texture);
typedef void (APIENTRY *PFNGLMATRIXMODERAW)(GLenum mode);
typedef void (APIENTRY *PFNGLPUSHPOPMATRIXRAW)(void);
typedef void (APIENTRY *PFNGLLOADIDENTITYRAW)(void);
typedef void (APIENTRY *PFNGLORTHORAW)(GLdouble left, GLdouble right,
                                       GLdouble bottom, GLdouble top,
                                       GLdouble near_value, GLdouble far_value);
typedef void (APIENTRY *PFNGLCOLOR4FRAW)(GLfloat red, GLfloat green,
                                         GLfloat blue, GLfloat alpha);
typedef void (APIENTRY *PFNGLBEGINRAW)(GLenum mode);
typedef void (APIENTRY *PFNGLENDRAW)(void);
typedef void (APIENTRY *PFNGLTEXCOORD2FRAW)(GLfloat s, GLfloat t);
typedef void (APIENTRY *PFNGLVERTEX2FRAW)(GLfloat x, GLfloat y);
typedef void (APIENTRY *PFNGLBLENDEQUATIONSEPARATERAW)(GLenum rgb, GLenum alpha);
typedef void (APIENTRY *PFNGLGETTEXENVIVRAW)(GLenum target, GLenum name, GLint *value);
typedef void (APIENTRY *PFNGLTEXENVIRAW)(GLenum target, GLenum name, GLint value);

typedef struct EyeSwapchain {
    XrSwapchain handle;
    int32_t width;
    int32_t height;
    uint32_t image_count;
    XrSwapchainImageD3D11KHR images[MAX_SWAPCHAIN_IMAGES];
    ID3D11RenderTargetView *targets[MAX_SWAPCHAIN_IMAGES];
} EyeSwapchain;

typedef struct DepthSwapchain {
    XrSwapchain handle;
    int32_t width;
    int32_t height;
    uint32_t image_count;
    XrSwapchainImageD3D11KHR images[MAX_SWAPCHAIN_IMAGES];
    ID3D11DepthStencilView *targets[MAX_SWAPCHAIN_IMAGES];
} DepthSwapchain;

static HMODULE g_loader;
static XrInstance g_instance = XR_NULL_HANDLE;
static XrSystemId g_system = XR_NULL_SYSTEM_ID;
static XrSession g_session = XR_NULL_HANDLE;
static XrSpace g_space = XR_NULL_HANDLE;
static XrSpace g_view_space = XR_NULL_HANDLE;
static ID3D11Device *g_device;
static ID3D11DeviceContext *g_context;
static ID3D11Multithread *g_d3d_multithread;
static ID3D11Query *g_pair_copy_fence;
static LONG g_pair_copy_fence_pending;
static EyeSwapchain g_eyes[EYE_COUNT];
/* Full-screen in-round interfaces are submitted separately from the world.
   Keeping this surface transparent prevents Escape/Tab from replacing the
   stereo projection with a flat copy of the game. */
static EyeSwapchain g_interface_layer;
static DepthSwapchain g_depth_eyes[EYE_COUNT];
static XrView g_views[EYE_COUNT];
static SRWLOCK g_view_lock = SRWLOCK_INIT;
static XrViewConfigurationView g_view_config[EYE_COUNT];
static XrSessionState g_session_state = XR_SESSION_STATE_UNKNOWN;
static int g_enabled;
static int g_initialized;
static int g_running;
static int g_failed;
static unsigned long g_submitted_frames;
static LONG g_frame_thread_started;
static HANDLE g_frame_thread;
static LONG g_shutdown_requested;
static LONG g_shutdown_started;
static LONG g_shutdown_complete;
static ULONGLONG g_session_stopped_tick;
static int g_session_stall_logged;
static ULONGLONG g_last_slow_frame_log;
static ULONGLONG g_cadence_window_start;
static unsigned long g_cadence_submitted_start;
static LONG g_cadence_pair_start;
static LONG g_cadence_eye_start[EYE_COUNT];
static int g_live_pose_diagnostic;
static int g_display_refresh_extension_enabled;
static int g_composition_depth_extension_enabled;
static float g_requested_refresh_rate = 72.0f;
static int64_t g_swapchain_format;
static int64_t g_depth_swapchain_format;
static ID3D11Texture2D *g_interop_textures[EYE_COUNT];
static ID3D11Texture2D *g_hud_base_interop_textures[EYE_COUNT];
static ID3D11ShaderResourceView *g_hud_base_interop_views[EYE_COUNT];
static ID3D11Texture2D *g_present_pair_textures[PAIR_BUFFER_COUNT][EYE_COUNT];
static ID3D11RenderTargetView *g_present_pair_targets[PAIR_BUFFER_COUNT][EYE_COUNT];
static ID3D11ShaderResourceView *g_present_pair_views[PAIR_BUFFER_COUNT][EYE_COUNT];
static LONG g_present_pair_generation;
static LONG g_present_pair_index = -1;
static LONG g_submit_pair_index = -1;
static SRWLOCK g_interop_capture_lock = SRWLOCK_INIT;
static LONG g_interop_lock_failures;
static LONG g_interop_unlock_failures;
static LONG g_eye_capture_generation[EYE_COUNT];
static XrPosef g_pending_pair_pose[EYE_COUNT];
static XrFovf g_pending_pair_fov[EYE_COUNT];
static ULONGLONG g_pending_pair_pose_tick;
static int g_geometry_fov_valid[EYE_COUNT];
static int g_geometry_fov_logged;
static int g_pending_pair_pose_valid;
static XrPosef g_present_pair_pose[PAIR_BUFFER_COUNT][EYE_COUNT];
static XrFovf g_present_pair_fov[PAIR_BUFFER_COUNT][EYE_COUNT];
static ULONGLONG g_present_pair_pose_tick[PAIR_BUFFER_COUNT];
static ULONGLONG g_present_pair_publish_tick[PAIR_BUFFER_COUNT];
static XrRect2Di g_geometry_eye_rect[EYE_COUNT];
static XrRect2Di g_present_pair_rect[PAIR_BUFFER_COUNT][EYE_COUNT];
static int g_present_pair_pose_valid[PAIR_BUFFER_COUNT];
static int g_present_pair_completed_interface[PAIR_BUFFER_COUNT];
static int g_completed_interface_eye_ready[EYE_COUNT];
static ID3D11Texture2D *g_geometry_eye_depth_textures[EYE_COUNT];
static ID3D11ShaderResourceView *g_geometry_eye_depth_views[EYE_COUNT];
static int g_geometry_eye_depth_ready[EYE_COUNT];
static ID3D11Texture2D *g_present_pair_depth_textures[PAIR_BUFFER_COUNT][EYE_COUNT];
static ID3D11ShaderResourceView *g_present_pair_depth_views[PAIR_BUFFER_COUNT][EYE_COUNT];
static int g_present_pair_depth_valid[PAIR_BUFFER_COUNT];
static ID3D11ShaderResourceView *g_interop_views[EYE_COUNT];
static HANDLE g_interop_device;
static HANDLE g_interop_objects[EYE_COUNT];
static GLuint g_interop_gl_textures[EYE_COUNT];
static HANDLE g_hud_base_interop_objects[EYE_COUNT];
static GLuint g_hud_base_interop_gl_textures[EYE_COUNT];
static ID3D11Texture2D *g_menu_interop_textures[2];
static ID3D11ShaderResourceView *g_menu_interop_views[2];
static HANDLE g_menu_interop_objects[2];
static GLuint g_menu_interop_gl_textures[2];
static int g_menu_overlay_ready;
static int g_menu_overlay_exact_alpha;
static GLuint g_interface_alpha_textures[2];
static GLuint g_interface_alpha_framebuffer;
static int g_interface_alpha_width;
static int g_interface_alpha_height;
static int g_interface_alpha_active;
static int g_interface_alpha_draws;
static int g_interface_alpha_desktop_ready;
static GLint g_interface_alpha_saved_draw_framebuffer;
static GLint g_interface_alpha_saved_blend[4];
static GLint g_interface_alpha_saved_color_mask[4];
static GLint g_interface_alpha_saved_blend_equation[2];
static GLuint g_interop_framebuffer;
static GLuint g_hud_base_source_textures[EYE_COUNT];
static GLuint g_hud_base_source_framebuffer;
static int g_hud_base_source_width;
static int g_hud_base_source_height;
static int g_hud_base_source_ready[EYE_COUNT];
static int g_hud_base_interop_ready[EYE_COUNT];
static float g_native_hud_safe_scale_x = 0.66f;
static float g_native_hud_safe_scale_y = 0.40f;
static int g_native_hud_logged;
static int g_gl_interop_ready;
static int g_geometry_stereo_requested;
static int g_geometry_eye_ready[EYE_COUNT];
static volatile LONG g_geometry_active;
static ULONGLONG g_flat_capture_tick;
static int g_geometry_activity_logged = -1;
static int g_geometry_capture_logged;
static int g_projection_rect_logged;
static GLuint g_native_mirror_texture;
static GLuint g_native_mirror_framebuffer;
static int g_native_mirror_width;
static int g_native_mirror_height;
static int g_native_mirror_ready;
static GLuint g_desktop_mirror_texture;
static GLuint g_desktop_mirror_framebuffer;
static int g_desktop_mirror_width;
static int g_desktop_mirror_height;
static int g_desktop_mirror_ready;
static ID3D11Texture2D *g_world_color_texture;
static ID3D11Texture2D *g_world_depth_texture;
static ID3D11Texture2D *g_world_depth_shader_texture;
static ID3D11ShaderResourceView *g_world_color_view;
static ID3D11ShaderResourceView *g_world_depth_view;
static HANDLE g_world_interop_objects[2];
static GLuint g_world_gl_textures[2];
static GLuint g_world_color_framebuffer;
static GLuint g_world_depth_framebuffer;
static int g_world_width;
static int g_world_height;
static int g_world_samples;
static int g_world_capture_ready;
static int g_world_capture_logged;
static int g_world_depth_validated;
static int g_stereo_enabled;
static ID3D11VertexShader *g_stereo_vertex_shader;
static ID3D11PixelShader *g_stereo_pixel_shader;
static ID3D11Buffer *g_stereo_constants;
static ID3D11SamplerState *g_stereo_sampler;
static ID3D11RasterizerState *g_stereo_rasterizer;
static ID3D11VertexShader *g_pair_sharpen_vertex_shader;
static ID3D11PixelShader *g_pair_sharpen_pixel_shader;
static ID3D11PixelShader *g_menu_overlay_pixel_shader;
static ID3D11BlendState *g_menu_overlay_blend;
static ID3D11Buffer *g_pair_sharpen_constants;
static ID3D11SamplerState *g_pair_sharpen_sampler;
static ID3D11RasterizerState *g_pair_sharpen_rasterizer;
static int g_pair_sharpen_logged;
static int g_menu_overlay_logged;
static ID3D11VertexShader *g_depth_copy_vertex_shader;
static ID3D11PixelShader *g_depth_copy_pixel_shader;
static ID3D11Buffer *g_depth_copy_constants;
static ID3D11DepthStencilState *g_depth_copy_state;
static int g_depth_copy_logged;
static int g_depth_submission_logged;
static int g_stereo_logged;
static int g_stereo_eye_status[EYE_COUNT];
static int g_eye_used_depth_stereo[EYE_COUNT];
static int g_projection_anchor_valid;
static XrPosef g_projection_anchor_pose[EYE_COUNT];
static XrFovf g_projection_anchor_fov[EYE_COUNT];
static int g_projection_layer_logged;
static int g_quad_layer_logged;
static char g_game_log_path[MAX_PATH];
static char g_last_map_line[512];
static char g_last_round_complete_line[512];
static ULONGLONG g_scene_check_tick;
static ULONGLONG g_depth_scene_ready_tick;
static ULONGLONG g_depth_scene_active_tick;
static int g_flat_scene_mode = 1;
static int g_scene_mode_announced;
static int g_interface_draw_calls;
static long long g_interface_vertices;
static int g_interface_heavy = 1;
static int g_interface_light_frames;
static int g_interface_heavy_candidate_frames;
static int g_interface_mode_initialized;
static ULONGLONG g_interface_heavy_since_tick;
static int g_content_x;
static int g_content_y;
static int g_content_width;
static int g_content_height;
/* The projection/desktop path updates g_content_* whenever a fresh eye is
   captured.  The transparent interface is published on the game thread and
   consumed later by the XR thread, so sharing that rectangle made the XR
   quad sample the subsequently-written world rectangle instead of the UI.
   Keep the completed interface's source rectangle stable until the next
   interface publication. */
static int g_interface_content_x;
static int g_interface_content_y;
static int g_interface_content_width;
static int g_interface_content_height;
static int g_interface_present_rect[EYE_COUNT][4];
static HWND g_game_window;
static HWND g_logged_game_window;
static HCURSOR g_cursor_handle;
static UINT g_cursor_width;
static UINT g_cursor_height;
static UINT g_cursor_hotspot_x;
static UINT g_cursor_hotspot_y;
static ID3D11Texture2D *g_cursor_texture;
static ID3D11ShaderResourceView *g_cursor_view;
static ID3D11VertexShader *g_cursor_vertex_shader;
static ID3D11PixelShader *g_cursor_pixel_shader;
static ID3D11Buffer *g_cursor_constants;
static ID3D11SamplerState *g_cursor_sampler;
static ID3D11BlendState *g_cursor_blend;
static ID3D11RasterizerState *g_cursor_rasterizer;
static int g_cursor_logged;
static int g_cursor_proof_written;
static float g_display_width = 2.8f;
static float g_display_distance = 2.0f;
static char g_cursor_proof_marker[MAX_PATH];
static char g_cursor_proof_path[MAX_PATH];
static char g_menu_complete_proof_path[MAX_PATH];
static char g_menu_baseline_proof_path[MAX_PATH];
static char g_stereo_proof_marker[MAX_PATH];
static char g_stereo_proof_path[EYE_COUNT][MAX_PATH];
static int g_stereo_proof_written[EYE_COUNT];
static char g_geometry_proof_marker[MAX_PATH];
static char g_geometry_proof_path[EYE_COUNT][MAX_PATH];
static int g_geometry_proof_written[EYE_COUNT];
static char g_geometry_presented_proof_path[EYE_COUNT][MAX_PATH];
static int g_geometry_presented_proof_written[EYE_COUNT];
static char g_geometry_final_interface_proof_path[EYE_COUNT][MAX_PATH];
static int g_geometry_final_interface_proof_written[EYE_COUNT];
static char g_stereo_diagnostic_path[5][MAX_PATH];
static char g_stereo_diagnostic_report_path[MAX_PATH];
static LONG g_stereo_diagnostic_started;
static char g_geometry_hud_base_proof_path[EYE_COUNT][MAX_PATH];
static int g_geometry_hud_base_proof_written[EYE_COUNT];
static char g_interface_alpha_proof_path[MAX_PATH];
static int g_interface_alpha_proof_written;
static char g_interface_alpha_mask_proof_path[MAX_PATH];
static int g_interface_alpha_mask_proof_written;
static char g_tabletop_recenter_marker[MAX_PATH];
static char g_pose_trace_marker[MAX_PATH];
static char g_pose_trace_path[MAX_PATH];
static FILE *g_pose_trace_file;
static unsigned long g_pose_trace_rows;
static int g_tabletop_motion_logged;
static int g_geometry_ipd_logged;
static float g_stereo_strength;
static float g_eye_alignment;
static int g_calibration_keys_enabled;
static char g_config_path[MAX_PATH];
static char g_calibration_live_path[MAX_PATH];
static PFNWGLDXOPENDEVICENV p_wglDXOpenDeviceNV;
static PFNWGLDXCLOSEDEVICENV p_wglDXCloseDeviceNV;
static PFNWGLDXREGISTEROBJECTNV p_wglDXRegisterObjectNV;
static PFNWGLDXUNREGISTEROBJECTNV p_wglDXUnregisterObjectNV;
static PFNWGLDXLOCKOBJECTSNV p_wglDXLockObjectsNV;
static PFNWGLDXUNLOCKOBJECTSNV p_wglDXUnlockObjectsNV;
static PFNGLGENFRAMEBUFFERS p_glGenFramebuffers;
static PFNGLBINDFRAMEBUFFER p_glBindFramebuffer;
static PFNGLFRAMEBUFFERTEXTURE2D p_glFramebufferTexture2D;
static PFNGLCHECKFRAMEBUFFERSTATUS p_glCheckFramebufferStatus;
static PFNGLBLITFRAMEBUFFER p_glBlitFramebuffer;
static PFNGLGETINTEGERVRAW p_glGetIntegerv;
static PFNGLGENTEXTURESRAW p_glGenTextures;
static PFNGLGETERRORRAW p_glGetError;
static PFNGLFINISHRAW p_glFinish;
static PFNGLDRAWBUFFERRAW p_glDrawBuffer;
static PFNGLBINDTEXTURERAW p_glBindTextureRaw;
static PFNGLTEXIMAGE2DRAW p_glTexImage2DRaw;
static PFNGLTEXPARAMETERIRAW p_glTexParameteriRaw;
static PFNGLGETFLOATVRAW p_glGetFloatvRaw;
static PFNGLCLEARCOLORRAW p_glClearColorRaw;
static PFNGLCLEARRAW p_glClearRaw;
static void (APIENTRY *p_glColorMaskRaw)(GLboolean, GLboolean, GLboolean, GLboolean);
static PFNGLBLENDFUNCSEPARATERAW p_glBlendFuncSeparateRaw;
static PFNGLISENABLEDRAW p_glIsEnabledRaw;
static PFNGLENABLEDISABLERAW p_glEnableRaw;
static PFNGLENABLEDISABLERAW p_glDisableRaw;
static PFNGLUSEPROGRAMRAW p_glUseProgramRaw;
static PFNGLACTIVETEXTURERAW p_glActiveTextureRaw;
static PFNGLMATRIXMODERAW p_glMatrixModeRaw;
static PFNGLPUSHPOPMATRIXRAW p_glPushMatrixRaw;
static PFNGLPUSHPOPMATRIXRAW p_glPopMatrixRaw;
static PFNGLLOADIDENTITYRAW p_glLoadIdentityRaw;
static PFNGLORTHORAW p_glOrthoRaw;
static PFNGLCOLOR4FRAW p_glColor4fRaw;
static PFNGLBEGINRAW p_glBeginRaw;
static PFNGLENDRAW p_glEndRaw;
static PFNGLTEXCOORD2FRAW p_glTexCoord2fRaw;
static PFNGLVERTEX2FRAW p_glVertex2fRaw;
static PFNGLBLENDEQUATIONSEPARATERAW p_glBlendEquationSeparateRaw;
static PFNGLGETTEXENVIVRAW p_glGetTexEnvivRaw;
static PFNGLTEXENVIRAW p_glTexEnviRaw;
static char g_log_path[MAX_PATH];
static char g_status[160] = "disabled";
static void xr_log(const char *format, ...);
static void capture_composite_proof(ID3D11Texture2D *source,
                                    const char *marker,
                                    const char *path, int *written,
                                    const char *label);
static void start_stereo_diagnostic_if_ready(void);

static void load_display_config(const char *base_directory)
{
    snprintf(g_config_path, sizeof(g_config_path), "%s\\SkillshotVR-config.txt", base_directory);
    snprintf(g_calibration_live_path, sizeof(g_calibration_live_path),
             "%s\\SkillshotVR-calibration-live.txt", base_directory);
    FILE *file = fopen(g_config_path, "r");
    if (!file) return;
    char line[160];
    while (fgets(line, sizeof(line), file)) {
        char *equals = strchr(line, '=');
        if (!equals) continue;
        *equals++ = '\0';
        float value = strtof(equals, NULL);
        if (strcmp(line, "display_width") == 0 && value >= 1.5f && value <= 4.0f)
            g_display_width = value;
        else if (strcmp(line, "display_distance") == 0 && value >= 1.0f && value <= 4.0f)
            g_display_distance = value;
        else if (strcmp(line, "stereo_strength") == 0 && value >= -8.0f && value <= 8.0f)
            g_stereo_strength = value;
        else if (strcmp(line, "eye_alignment") == 0 && value >= -0.30f && value <= 0.30f)
            g_eye_alignment = value;
    }
    fclose(file);
}

static void write_calibration_value(const char *state)
{
    FILE *file = fopen(g_calibration_live_path, "w");
    if (file) {
        fprintf(file, "eye_alignment=%.4f\nstereo_strength=%.4f\nstate=%s\n",
                g_eye_alignment, g_stereo_strength, state);
        fclose(file);
    }
}

static void save_display_config(void)
{
    FILE *file = fopen(g_config_path, "w");
    if (!file) {
        xr_log("ERROR Could not save stereo calibration to %s", g_config_path);
        return;
    }
    fprintf(file, "# Skillshot City VR display and stereo calibration.\n");
    fprintf(file, "display_width=%.2f\n", g_display_width);
    fprintf(file, "display_distance=%.2f\n", g_display_distance);
    fprintf(file, "eye_alignment=%.4f\n", g_eye_alignment);
    fprintf(file, "stereo_strength=%.4f\n", g_stereo_strength);
    fclose(file);
    write_calibration_value("saved");
    xr_log("Eye alignment saved: %.4f (depth strength %.4f)",
           g_eye_alignment, g_stereo_strength);
}

static void update_stereo_calibration(int active)
{
    if (!active || !g_calibration_keys_enabled) return;
    /* Native geometry and flat presentation now share one explicit horizontal
       alignment. Require modifiers so ordinary gameplay arrows remain free. */
    if (!(GetAsyncKeyState(VK_CONTROL) & 0x8000) ||
        !(GetAsyncKeyState(VK_MENU) & 0x8000)) return;
    float previous = g_eye_alignment;
    if (GetAsyncKeyState(VK_LEFT) & 1) g_eye_alignment -= 0.005f;
    if (GetAsyncKeyState(VK_RIGHT) & 1) g_eye_alignment += 0.005f;
    if (g_eye_alignment < -0.30f) g_eye_alignment = -0.30f;
    if (g_eye_alignment > 0.30f) g_eye_alignment = 0.30f;
    if (g_eye_alignment != previous) {
        char title[160];
        snprintf(title, sizeof(title),
                 "Skillshot City VR - native eye alignment %.4f (Ctrl+Alt Left/Right)",
                 g_eye_alignment);
        if (g_game_window) SetWindowTextA(g_game_window, title);
        write_calibration_value("live");
        xr_log("Eye alignment changed: %.4f", g_eye_alignment);
    }
    if (GetAsyncKeyState(VK_F8) & 1) save_display_config();
}

static PFN_xrGetInstanceProcAddr p_xrGetInstanceProcAddr;
static PFN_xrEnumerateInstanceExtensionProperties p_xrEnumerateInstanceExtensionProperties;
static PFN_xrCreateInstance p_xrCreateInstance;
static PFN_xrDestroyInstance p_xrDestroyInstance;
static PFN_xrGetSystem p_xrGetSystem;
static PFN_xrGetSystemProperties p_xrGetSystemProperties;
static PFN_xrEnumerateViewConfigurationViews p_xrEnumerateViewConfigurationViews;
static PFN_xrCreateSession p_xrCreateSession;
static PFN_xrDestroySession p_xrDestroySession;
static PFN_xrCreateReferenceSpace p_xrCreateReferenceSpace;
static PFN_xrDestroySpace p_xrDestroySpace;
static PFN_xrEnumerateSwapchainFormats p_xrEnumerateSwapchainFormats;
static PFN_xrCreateSwapchain p_xrCreateSwapchain;
static PFN_xrDestroySwapchain p_xrDestroySwapchain;
static PFN_xrEnumerateSwapchainImages p_xrEnumerateSwapchainImages;
static PFN_xrAcquireSwapchainImage p_xrAcquireSwapchainImage;
static PFN_xrWaitSwapchainImage p_xrWaitSwapchainImage;
static PFN_xrReleaseSwapchainImage p_xrReleaseSwapchainImage;
static PFN_xrPollEvent p_xrPollEvent;
static PFN_xrBeginSession p_xrBeginSession;
static PFN_xrEndSession p_xrEndSession;
static PFN_xrRequestExitSession p_xrRequestExitSession;
static PFN_xrWaitFrame p_xrWaitFrame;
static PFN_xrBeginFrame p_xrBeginFrame;
static PFN_xrEndFrame p_xrEndFrame;
static PFN_xrLocateViews p_xrLocateViews;
static PFN_xrGetD3D11GraphicsRequirementsKHR p_xrGetD3D11GraphicsRequirementsKHR;
static PFN_xrEnumerateDisplayRefreshRatesFB p_xrEnumerateDisplayRefreshRatesFB;
static PFN_xrRequestDisplayRefreshRateFB p_xrRequestDisplayRefreshRateFB;

static void xr_log(const char *format, ...)
{
    FILE *file = g_log_path[0] ? fopen(g_log_path, "a") : NULL;
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

static void update_pose_trace(XrTime predicted_display_time, LONG pair_index,
                              int projection_active, int interface_layer_active,
                              const XrCompositionLayerProjectionView *views)
{
    int requested = g_pose_trace_marker[0] &&
        GetFileAttributesA(g_pose_trace_marker) != INVALID_FILE_ATTRIBUTES;
    if (!requested) {
        if (g_pose_trace_file) {
            fflush(g_pose_trace_file);
            fclose(g_pose_trace_file);
            g_pose_trace_file = NULL;
            xr_log("Per-frame pose trace stopped after %lu rows", g_pose_trace_rows);
        }
        return;
    }
    if (!g_pose_trace_file) {
        g_pose_trace_file = fopen(g_pose_trace_path, "w");
        if (!g_pose_trace_file) return;
        setvbuf(g_pose_trace_file, NULL, _IOFBF, 65536);
        fprintf(g_pose_trace_file,
                "frame,predicted_ns,tick_ms,pair,generation,projection,interface_heavy,"
                "interface_layer,live_px,live_py,live_pz,live_qx,live_qy,live_qz,live_qw,"
                "render_px,render_py,render_pz,render_qx,render_qy,render_qz,render_qw,"
                "submit_px,submit_py,submit_pz,submit_qx,submit_qy,submit_qz,submit_qw,"
                "visual_render_qx,visual_render_qy,visual_render_qz,visual_render_qw,"
                "visual_live_qx,visual_live_qy,visual_live_qz,visual_live_qw\n");
        g_pose_trace_rows = 0;
        xr_log("Per-frame pose trace started: %s", g_pose_trace_path);
    }
    XrPosef render = {{0, 0, 0, 1}, {0, 0, 0}};
    if (pair_index >= 0 && pair_index < PAIR_BUFFER_COUNT &&
        g_present_pair_pose_valid[pair_index])
        render = g_present_pair_pose[pair_index][0];
    XrPosef live = g_views[0].pose;
    XrPosef submitted = views ? views[0].pose : live;
    float anchor_orientation[4] = {0, 0, 0, 1};
    if (g_projection_anchor_valid) {
        anchor_orientation[0] = g_projection_anchor_pose[0].orientation.x;
        anchor_orientation[1] = g_projection_anchor_pose[0].orientation.y;
        anchor_orientation[2] = g_projection_anchor_pose[0].orientation.z;
        anchor_orientation[3] = g_projection_anchor_pose[0].orientation.w;
    }
    float render_orientation[4] = {
        render.orientation.x, render.orientation.y,
        render.orientation.z, render.orientation.w
    };
    float live_orientation[4] = {
        live.orientation.x, live.orientation.y,
        live.orientation.z, live.orientation.w
    };
    float visual_render[4], visual_live[4];
    vr_render_head_orientation_about_anchor_f(
        anchor_orientation, render_orientation, visual_render);
    vr_visual_head_orientation_about_anchor_f(
        anchor_orientation, live_orientation, visual_live);
    fprintf(g_pose_trace_file,
            "%lu,%lld,%llu,%ld,%ld,%d,%d,%d,"
            "%.7g,%.7g,%.7g,%.7g,%.7g,%.7g,%.7g,"
            "%.7g,%.7g,%.7g,%.7g,%.7g,%.7g,%.7g,"
            "%.7g,%.7g,%.7g,%.7g,%.7g,%.7g,%.7g,"
            "%.7g,%.7g,%.7g,%.7g,%.7g,%.7g,%.7g,%.7g\n",
            g_submitted_frames + 1, (long long)predicted_display_time,
            (unsigned long long)GetTickCount64(), pair_index,
            InterlockedCompareExchange(&g_present_pair_generation, 0, 0),
            projection_active, g_interface_heavy, interface_layer_active,
            live.position.x, live.position.y, live.position.z,
            live.orientation.x, live.orientation.y, live.orientation.z,
            live.orientation.w,
            render.position.x, render.position.y, render.position.z,
            render.orientation.x, render.orientation.y, render.orientation.z,
            render.orientation.w,
            submitted.position.x, submitted.position.y, submitted.position.z,
            submitted.orientation.x, submitted.orientation.y,
            submitted.orientation.z, submitted.orientation.w,
            visual_render[0], visual_render[1], visual_render[2], visual_render[3],
            visual_live[0], visual_live[1], visual_live[2], visual_live[3]);
    if ((++g_pose_trace_rows % 72) == 0) fflush(g_pose_trace_file);
}

static void fail(const char *step, XrResult result)
{
    snprintf(g_status, sizeof(g_status), "failed: %s (%d)", step, (int)result);
    xr_log("ERROR %s: XrResult=%d", step, (int)result);
    g_failed = 1;
}

static FARPROC loader_proc(const char *name)
{
    return g_loader ? GetProcAddress(g_loader, name) : NULL;
}

#define LOAD_CORE(name) do { \
    p_##name = (PFN_##name)loader_proc(#name); \
    if (!p_##name) { fail("missing " #name, XR_ERROR_FUNCTION_UNSUPPORTED); return 0; } \
} while (0)

static int load_openxr(const char *base_directory)
{
    char path[MAX_PATH];
    snprintf(path, sizeof(path), "%s\\openxr_loader.dll", base_directory);
    g_loader = LoadLibraryA(path);
    if (!g_loader) {
        snprintf(g_status, sizeof(g_status), "failed: openxr_loader.dll (%lu)", GetLastError());
        xr_log("ERROR LoadLibrary('%s') failed: %lu", path, GetLastError());
        g_failed = 1;
        return 0;
    }
    LOAD_CORE(xrGetInstanceProcAddr);
    LOAD_CORE(xrEnumerateInstanceExtensionProperties);
    LOAD_CORE(xrCreateInstance);
    LOAD_CORE(xrDestroyInstance);
    LOAD_CORE(xrGetSystem);
    LOAD_CORE(xrGetSystemProperties);
    LOAD_CORE(xrEnumerateViewConfigurationViews);
    LOAD_CORE(xrCreateSession);
    LOAD_CORE(xrDestroySession);
    LOAD_CORE(xrCreateReferenceSpace);
    LOAD_CORE(xrDestroySpace);
    LOAD_CORE(xrEnumerateSwapchainFormats);
    LOAD_CORE(xrCreateSwapchain);
    LOAD_CORE(xrDestroySwapchain);
    LOAD_CORE(xrEnumerateSwapchainImages);
    LOAD_CORE(xrAcquireSwapchainImage);
    LOAD_CORE(xrWaitSwapchainImage);
    LOAD_CORE(xrReleaseSwapchainImage);
    LOAD_CORE(xrPollEvent);
    LOAD_CORE(xrBeginSession);
    LOAD_CORE(xrEndSession);
    LOAD_CORE(xrRequestExitSession);
    LOAD_CORE(xrWaitFrame);
    LOAD_CORE(xrBeginFrame);
    LOAD_CORE(xrEndFrame);
    LOAD_CORE(xrLocateViews);
    return 1;
}

static int instance_extension_available(const char *wanted)
{
    uint32_t count = 0;
    if (XR_FAILED(p_xrEnumerateInstanceExtensionProperties(NULL, 0, &count, NULL)) || !count)
        return 0;
    XrExtensionProperties *properties =
        (XrExtensionProperties *)calloc(count, sizeof(XrExtensionProperties));
    if (!properties) return 0;
    for (uint32_t index = 0; index < count; ++index)
        properties[index].type = XR_TYPE_EXTENSION_PROPERTIES;
    XrResult result = p_xrEnumerateInstanceExtensionProperties(NULL, count, &count, properties);
    int found = 0;
    if (XR_SUCCEEDED(result)) {
        for (uint32_t index = 0; index < count; ++index) {
            if (strcmp(properties[index].extensionName, wanted) == 0) {
                found = 1;
                break;
            }
        }
    }
    free(properties);
    return found;
}

static void request_stable_display_refresh_rate(void)
{
    if (!g_display_refresh_extension_enabled) {
        xr_log("90 Hz request unavailable: runtime lacks %s",
               XR_FB_DISPLAY_REFRESH_RATE_EXTENSION_NAME);
        return;
    }
    XrResult enumerate_result = p_xrGetInstanceProcAddr(
        g_instance, "xrEnumerateDisplayRefreshRatesFB",
        (PFN_xrVoidFunction *)&p_xrEnumerateDisplayRefreshRatesFB);
    XrResult request_proc_result = p_xrGetInstanceProcAddr(
        g_instance, "xrRequestDisplayRefreshRateFB",
        (PFN_xrVoidFunction *)&p_xrRequestDisplayRefreshRateFB);
    if (XR_FAILED(enumerate_result) || XR_FAILED(request_proc_result) ||
        !p_xrEnumerateDisplayRefreshRatesFB || !p_xrRequestDisplayRefreshRateFB) {
        xr_log("90 Hz request unavailable: refresh-rate entry points missing");
        return;
    }
    uint32_t count = 0;
    XrResult result = p_xrEnumerateDisplayRefreshRatesFB(g_session, 0, &count, NULL);
    if (XR_FAILED(result) || !count) {
        xr_log("90 Hz request unavailable: rate enumeration failed (%d)", (int)result);
        return;
    }
    float *rates = (float *)calloc(count, sizeof(float));
    if (!rates) return;
    result = p_xrEnumerateDisplayRefreshRatesFB(g_session, count, &count, rates);
    float selected = 0.0f;
    char rate_list[256] = {0};
    if (XR_SUCCEEDED(result)) {
        for (uint32_t index = 0; index < count; ++index) {
            char item[32];
            snprintf(item, sizeof(item), "%s%.2f", index ? ", " : "", rates[index]);
            strncat(rate_list, item, sizeof(rate_list) - strlen(rate_list) - 1);
            /* This two-pass legacy renderer cannot sustain a 120 Hz physical
               session.  Select only a comfort cadence that its dedicated XR
               thread can actually hold; otherwise leave the runtime alone and
               tell the launcher/user which Link setting must change. */
            if (rates[index] >= 71.0f && rates[index] <= 90.5f &&
                (selected == 0.0f || fabsf(rates[index] - g_requested_refresh_rate) <
                                     fabsf(selected - g_requested_refresh_rate)))
                selected = rates[index];
        }
        xr_log("Supported display cadences: %s Hz", rate_list[0] ? rate_list : "none");
    }
    free(rates);
    if (XR_FAILED(result) || selected == 0.0f) {
        xr_log("No supported 72-90 Hz cadence is exposed. Set Meta Link Graphics "
               "Preferences to 72, 80, or 90 Hz before physical play; 120 Hz "
               "forces unstable ASW for this two-pass renderer.");
        return;
    }
    result = p_xrRequestDisplayRefreshRateFB(g_session, selected);
    if (XR_SUCCEEDED(result))
        xr_log("Requested stable display cadence %.2f Hz (nearest safe rate to %.2f Hz)",
               selected, g_requested_refresh_rate);
    else
        xr_log("Display cadence request %.2f Hz failed (%d)", selected, (int)result);
}

static int create_d3d_device(const XrGraphicsRequirementsD3D11KHR *requirements)
{
    IDXGIFactory1 *factory = NULL;
    IDXGIAdapter1 *selected = NULL;
    if (SUCCEEDED(CreateDXGIFactory1(&IID_IDXGIFactory1, (void **)&factory))) {
        for (UINT index = 0; ; ++index) {
            IDXGIAdapter1 *adapter = NULL;
            if (IDXGIFactory1_EnumAdapters1(factory, index, &adapter) == DXGI_ERROR_NOT_FOUND) break;
            DXGI_ADAPTER_DESC1 description;
            memset(&description, 0, sizeof(description));
            IDXGIAdapter1_GetDesc1(adapter, &description);
            if (description.AdapterLuid.LowPart == requirements->adapterLuid.LowPart &&
                description.AdapterLuid.HighPart == requirements->adapterLuid.HighPart) {
                selected = adapter;
                break;
            }
            IDXGIAdapter1_Release(adapter);
        }
    }

    D3D_FEATURE_LEVEL chosen_level;
    HRESULT hr = D3D11CreateDevice((IDXGIAdapter *)selected,
                                   selected ? D3D_DRIVER_TYPE_UNKNOWN : D3D_DRIVER_TYPE_HARDWARE,
                                   NULL, D3D11_CREATE_DEVICE_BGRA_SUPPORT,
                                   NULL, 0, D3D11_SDK_VERSION,
                                   &g_device, &chosen_level, &g_context);
    if (selected) IDXGIAdapter1_Release(selected);
    if (factory) IDXGIFactory1_Release(factory);
    if (FAILED(hr)) {
        snprintf(g_status, sizeof(g_status), "failed: D3D11 device (0x%08lX)", (unsigned long)hr);
        xr_log("ERROR D3D11CreateDevice failed: 0x%08lX", (unsigned long)hr);
        g_failed = 1;
        return 0;
    }
    hr = ID3D11Device_QueryInterface(g_device, &IID_ID3D11Multithread,
                                     (void **)&g_d3d_multithread);
    if (SUCCEEDED(hr) && g_d3d_multithread) {
        ID3D11Multithread_SetMultithreadProtected(g_d3d_multithread, TRUE);
        xr_log("D3D11 multithread protection enabled for game/XR shared context");
    } else {
        xr_log("ERROR D3D11 multithread protection unavailable: 0x%08lX",
               (unsigned long)hr);
        return 0;
    }
    xr_log("D3D11 device created, feature level=0x%X", (unsigned int)chosen_level);
    return 1;
}

static int initialize_pair_sharpen_compositor(void)
{
    static const char *vertex_source =
        "struct Out { float4 position : SV_POSITION; float2 uv : TEXCOORD0; };"
        "Out main(uint id : SV_VertexID) {"
        " Out o; float2 p=float2((id<<1)&2,id&2); o.uv=p;"
        " o.position=float4(p*float2(2,-2)+float2(-1,1),0,1); return o; }";
    static const char *pixel_source =
        "cbuffer SharpenParams : register(b0) {"
        " float4 sharpen; float4 content; float4 safeArea; };"
        "Texture2D sourceColor : register(t0); Texture2D hudBase : register(t1);"
        "Texture2D sharedHud : register(t2); Texture2D sharedHudBase : register(t3);"
        "SamplerState linearSampler : register(s0);"
        "float4 main(float4 position:SV_POSITION,float2 uv:TEXCOORD0):SV_TARGET {"
        " float4 c=sourceColor.Sample(linearSampler,uv);"
        " float3 n=sourceColor.Sample(linearSampler,uv+float2(0,-sharpen.y)).rgb;"
        " float3 s=sourceColor.Sample(linearSampler,uv+float2(0, sharpen.y)).rgb;"
        " float3 e=sourceColor.Sample(linearSampler,uv+float2( sharpen.x,0)).rgb;"
        " float3 w=sourceColor.Sample(linearSampler,uv+float2(-sharpen.x,0)).rgb;"
        " float3 detail=c.rgb-(n+s+e+w)*0.25;"
        " float3 outputRgb=saturate(c.rgb+detail*0.10);"
        " if(sharpen.z>0.5) {"
        "  float4 baseHere=hudBase.Sample(linearSampler,uv);"
        "  float oldDiff=max(abs(c.r-baseHere.r),max(abs(c.g-baseHere.g),abs(c.b-baseHere.b)));"
        "  float topCenter=(uv.y<0.46&&uv.x>0.22&&uv.x<0.78)?1.0:0.0;"
        "  float midPrompt=(uv.y>0.38&&uv.y<0.68&&uv.x>0.22&&uv.x<0.78)?1.0:0.0;"
        /* HUD arrows can travel anywhere around the player/target.  Restricting
           the shared pass to edge rectangles left START GAME in the independent
           eye images and created distance-dependent double vision.  oldDiff
           already limits removal to pixels authored after the world baseline. */
        "  float fixedRegion=1.0;"
        /* Completed interfaces also contain a screen-wide dim pass.  Direct
           low-threshold subtraction classified that scaled world as HUD and
           copied it into the compact canvas.  Identify real menu material by
           chromatic tint or gradients that a scalar world fade cannot
           explain; ordinary HUD frames retain the physically safer v5 mask. */
        "  float originalStructure=0.0;"
        "  if(sharpen.w>0.5) {"
        "   float2 oex=float2(sharpen.x*8,0),oey=float2(0,sharpen.y*8);"
        "   float3 ocpx=sourceColor.Sample(linearSampler,uv+oex).rgb;"
        "   float3 ocnx=sourceColor.Sample(linearSampler,uv-oex).rgb;"
        "   float3 ocpy=sourceColor.Sample(linearSampler,uv+oey).rgb;"
        "   float3 ocny=sourceColor.Sample(linearSampler,uv-oey).rgb;"
        "   float3 obpx=hudBase.Sample(linearSampler,uv+oex).rgb;"
        "   float3 obnx=hudBase.Sample(linearSampler,uv-oex).rgb;"
        "   float3 obpy=hudBase.Sample(linearSampler,uv+oey).rgb;"
        "   float3 obny=hudBase.Sample(linearSampler,uv-oey).rgb;"
        "   float3 ocgx=ocpx-ocnx,ocgy=ocpy-ocny;"
        "   float3 obgx=obpx-obnx,obgy=obpy-obny;"
        "   float oldScale=clamp((dot(ocgx,obgx)+dot(ocgy,obgy))/"
        "     (dot(obgx,obgx)+dot(obgy,obgy)+0.00001),0.0,1.25);"
        "   float3 orx=ocgx-oldScale*obgx,ory=ocgy-oldScale*obgy;"
        "   float oldResidual=max(max(abs(orx.r),max(abs(orx.g),abs(orx.b))),"
        "                         max(abs(ory.r),max(abs(ory.g),abs(ory.b))));"
        "   float3 oldTint=c.rgb-oldScale*baseHere.rgb;"
        "   float oldChroma=max(oldTint.r,max(oldTint.g,oldTint.b))-"
        "                   min(oldTint.r,min(oldTint.g,oldTint.b));"
        "   originalStructure=max(smoothstep(0.035,0.100,oldResidual),"
        "                         smoothstep(0.025,0.065,oldChroma));"
        "  }"
        /* A center announcement is reconstructed later from the shared HUD.
           Remove its whole authored rectangle from this eye first. Replacing
           sourceColor with the same-eye hudBase is lossless where no overlay
           exists and prevents translucent flare pixels from retaining a copy
           of this eye's old map. */
        "  float centerOverlay=max(topCenter,midPrompt);"
        "  float oldMask=max((sharpen.w>0.5?originalStructure:"
        "    smoothstep(0.045,0.115,oldDiff))*fixedRegion,centerOverlay);"
        "  outputRgb=lerp(outputRgb,baseHere.rgb,oldMask);"
        "  float2 local=(uv-safeArea.xy)/safeArea.zw;"
        "  if(all(local>=0)&&all(local<=1)) {"
        "   float2 sourceUv=content.xy+local*content.zw;"
        "   float4 hud=sharedHud.Sample(linearSampler,sourceUv);"
        "   float4 hudUnder=sharedHudBase.Sample(linearSampler,sourceUv);"
        "   float3 hudDelta=hud.rgb-hudUnder.rgb;"
        "   float hudDiff=max(abs(hudDelta.r),max(abs(hudDelta.g),abs(hudDelta.b)));"
        "   float hudPositive=max(hudDelta.r,max(hudDelta.g,hudDelta.b));"
        "   float sourceCenter=(sourceUv.y<0.46&&sourceUv.x>0.22&&sourceUv.x<0.78)?1.0:0.0;"
        "   float sourceMidPrompt=(sourceUv.y>0.38&&sourceUv.y<0.68&&sourceUv.x>0.22&&sourceUv.x<0.78)?1.0:0.0;"
        "   float sourceFixed=1.0;"
        /* Center announcements and prompts often use a translucent surround.
           Transfer their bright authored pixels, not the recoloured world
           visible through that surround. Edge/bottom HUD keeps the existing
           exact difference mask because its opaque panels are stable. */
        "   float hudSignal=hudDiff;"
        "   float residual=0.0;"
        "   float edgeScale=1.0;"
        "   float3 menuColor=hud.rgb;"
        "   float centerOverlay=max(sourceCenter,sourceMidPrompt);"
        "   if(sharpen.w>0.5||centerOverlay>0.5) {"
        "    float2 ex=float2(sharpen.x*8,0),ey=float2(0,sharpen.y*8);"
        "    float3 cpx=sharedHud.Sample(linearSampler,sourceUv+ex).rgb;"
        "    float3 cnx=sharedHud.Sample(linearSampler,sourceUv-ex).rgb;"
        "    float3 cpy=sharedHud.Sample(linearSampler,sourceUv+ey).rgb;"
        "    float3 cny=sharedHud.Sample(linearSampler,sourceUv-ey).rgb;"
        "    float3 bpx=sharedHudBase.Sample(linearSampler,sourceUv+ex).rgb;"
        "    float3 bnx=sharedHudBase.Sample(linearSampler,sourceUv-ex).rgb;"
        "    float3 bpy=sharedHudBase.Sample(linearSampler,sourceUv+ey).rgb;"
        "    float3 bny=sharedHudBase.Sample(linearSampler,sourceUv-ey).rgb;"
        "    float3 cgx=cpx-cnx,cgy=cpy-cny,bgx=bpx-bnx,bgy=bpy-bny;"
        "    edgeScale=clamp((dot(cgx,bgx)+dot(cgy,bgy))/"
        "      (dot(bgx,bgx)+dot(bgy,bgy)+0.00001),0.0,1.25);"
        "    float3 rx=cgx-edgeScale*bgx,ry=cgy-edgeScale*bgy;"
        "    residual=max(max(abs(rx.r),max(abs(rx.g),abs(rx.b))),"
        "                 max(abs(ry.r),max(abs(ry.g),abs(ry.b))));"
        /* A translucent menu is locally C = k*B + tint.  Reconstructing k*B
           on a relocated safe canvas still exposes a differently sampled
           world as grain inside the panel. Use the recovered authored tint as
           an opaque comfort/readability panel; exact edges and glyphs are
           restored below. This contains no source-eye or destination-world
           image at all. */
        "    menuColor=saturate(hud.rgb-edgeScale*hudUnder.rgb);"
        "   }"
        "   float3 hn=sharedHud.Sample(linearSampler,sourceUv+float2(0,-sharpen.y)).rgb;"
        "   float3 hs=sharedHud.Sample(linearSampler,sourceUv+float2(0, sharpen.y)).rgb;"
        "   float3 he=sharedHud.Sample(linearSampler,sourceUv+float2( sharpen.x,0)).rgb;"
        "   float3 hw=sharedHud.Sample(linearSampler,sourceUv+float2(-sharpen.x,0)).rgb;"
        "   float3 hudDetail=hud.rgb-(hn+hs+he+hw)*0.25;"
        "   float3 authoredColor=saturate(hud.rgb+hudDetail*0.28);"
        "   float authoredMask=sharpen.w>0.5?smoothstep(0.035,0.100,residual):1.0;"
        "   float3 interfaceTint=hud.rgb-edgeScale*hudUnder.rgb;"
        "   float tintChroma=max(interfaceTint.r,max(interfaceTint.g,interfaceTint.b))-"
        "                    min(interfaceTint.r,min(interfaceTint.g,interfaceTint.b));"
        "   float interfaceStructure=max(authoredMask,smoothstep(0.025,0.065,tintChroma));"
        "   float hudMask=(sharpen.w>0.5?interfaceStructure:"
        "     smoothstep(0.040,0.105,hudSignal))*sourceFixed;"
        "   float3 resolvedHud=lerp(menuColor,authoredColor,authoredMask);"
        /* For ordinary center announcements, transfer the locally recovered
           attenuation and tint onto the destination eye instead of copying
           the already-composited source pixel. This preserves translucent
           bars and flares while making it mathematically impossible for the
           old source-eye gameplay image to appear inside them. */
        "   if(sharpen.w<0.5&&centerOverlay>0.5) {"
        /* A flat translucent panel has no local gradients, so edgeScale
           collapses to zero and the previous version treated the complete
           source-eye pixel as authored tint. Recover attenuation from the
           minimum colour ratio instead: positive overlay tint can only raise
           that ratio, so no source-eye spatial detail survives the transfer. */
        "    float3 safeUnder=max(hudUnder.rgb,float3(0.015,0.015,0.015));"
        "    float centerScale=clamp(min(hud.r/safeUnder.r,"
        "      min(hud.g/safeUnder.g,hud.b/safeUnder.b)),0.0,1.0);"
        "    float3 positiveTint=max(hud.rgb-centerScale*hudUnder.rgb,0.0);"
        "    float3 transferred=saturate(centerScale*outputRgb+positiveTint);"
        /* Bright glyphs and icons are effectively opaque. Preserve their
           authored colour exactly so text aligns and stays readable. */
        "    float centerBright=max(hud.r,max(hud.g,hud.b));"
        "    float centerOpaque=smoothstep(0.70,0.92,centerBright)*"
        "      smoothstep(0.15,0.40,hudDiff);"
        "    transferred=lerp(transferred,authoredColor,centerOpaque);"
        "    float transferSignal=max(hudSignal,max(residual,hudPositive));"
        "    float transferMask=smoothstep(0.010,0.045,transferSignal);"
        "    outputRgb=lerp(outputRgb,transferred,transferMask);"
        "   } else outputRgb=lerp(outputRgb,resolvedHud,hudMask);"
        "  }"
        " }"
        " return float4(outputRgb,c.a); }";
    ID3DBlob *vertex_blob = NULL;
    ID3DBlob *pixel_blob = NULL;
    ID3DBlob *errors = NULL;
    HRESULT hr = D3DCompile(vertex_source, strlen(vertex_source),
                            "SkillshotVRPairSharpenVS", NULL, NULL,
                            "main", "vs_4_0", 0, 0, &vertex_blob, &errors);
    if (FAILED(hr)) {
        xr_log("Pair sharpen vertex shader failed: %s",
               errors ? (char *)ID3D10Blob_GetBufferPointer(errors) : "unknown");
        if (errors) ID3D10Blob_Release(errors);
        return 0;
    }
    if (errors) { ID3D10Blob_Release(errors); errors = NULL; }
    hr = D3DCompile(pixel_source, strlen(pixel_source),
                    "SkillshotVRPairSharpenPS", NULL, NULL,
                    "main", "ps_4_0", 0, 0, &pixel_blob, &errors);
    if (FAILED(hr)) {
        xr_log("Pair sharpen pixel shader failed: %s",
               errors ? (char *)ID3D10Blob_GetBufferPointer(errors) : "unknown");
        if (errors) ID3D10Blob_Release(errors);
        ID3D10Blob_Release(vertex_blob);
        return 0;
    }
    if (errors) ID3D10Blob_Release(errors);
    hr = ID3D11Device_CreateVertexShader(
        g_device, ID3D10Blob_GetBufferPointer(vertex_blob),
        ID3D10Blob_GetBufferSize(vertex_blob), NULL, &g_pair_sharpen_vertex_shader);
    if (SUCCEEDED(hr))
        hr = ID3D11Device_CreatePixelShader(
            g_device, ID3D10Blob_GetBufferPointer(pixel_blob),
            ID3D10Blob_GetBufferSize(pixel_blob), NULL, &g_pair_sharpen_pixel_shader);
    ID3D10Blob_Release(vertex_blob);
    ID3D10Blob_Release(pixel_blob);
    if (FAILED(hr)) return 0;

    D3D11_BUFFER_DESC buffer_desc;
    memset(&buffer_desc, 0, sizeof(buffer_desc));
    buffer_desc.ByteWidth = 48;
    buffer_desc.Usage = D3D11_USAGE_DEFAULT;
    buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    if (FAILED(ID3D11Device_CreateBuffer(
            g_device, &buffer_desc, NULL, &g_pair_sharpen_constants))) return 0;
    D3D11_SAMPLER_DESC sampler_desc;
    memset(&sampler_desc, 0, sizeof(sampler_desc));
    sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampler_desc.MaxLOD = D3D11_FLOAT32_MAX;
    if (FAILED(ID3D11Device_CreateSamplerState(
            g_device, &sampler_desc, &g_pair_sharpen_sampler))) return 0;
    D3D11_RASTERIZER_DESC rasterizer_desc;
    memset(&rasterizer_desc, 0, sizeof(rasterizer_desc));
    rasterizer_desc.FillMode = D3D11_FILL_SOLID;
    rasterizer_desc.CullMode = D3D11_CULL_NONE;
    rasterizer_desc.DepthClipEnable = TRUE;
    if (FAILED(ID3D11Device_CreateRasterizerState(
            g_device, &rasterizer_desc, &g_pair_sharpen_rasterizer))) return 0;
    xr_log("Native-detail HUD separation and pair clarity resolve initialized");
    return 1;
}

static int initialize_menu_overlay_compositor(void)
{
    static const char *pixel_source =
        "cbuffer SharpenParams : register(b0) {"
        " float4 sharpen; float4 content; float4 safeArea; };"
        "Texture2D interfaceComposite : register(t0);"
        "Texture2D unusedTexture : register(t1);"
        "SamplerState linearSampler : register(s0);"
        "float4 main(float4 position:SV_POSITION,float2 uv:TEXCOORD0):SV_TARGET {"
        " float2 sourceUv=content.xy+uv*content.zw;"
        " float4 overlay=interfaceComposite.Sample(linearSampler,sourceUv);"
        /* The OpenGL capture target starts transparent and uses separate
           alpha accumulation. RGB is therefore premultiplied authored UI and
           A is exact accumulated coverage, with no world pixels involved. */
        " float rawAlpha=saturate(overlay.a);"
        " float3 straightColor=saturate(overlay.rgb/max(rawAlpha,0.002));"
        /* The legacy UI texture contains display-encoded RGB. An sRGB RTV
           encodes shader output again, so decode before hardware blending. */
        " if(sharpen.w>0.5) straightColor=lerp(straightColor/12.92,"
        "   pow((straightColor+0.055)/1.055,2.4),step(0.04045,straightColor));"
        " return float4(straightColor,rawAlpha); }";
    ID3DBlob *pixel_blob = NULL;
    ID3DBlob *errors = NULL;
    HRESULT hr = D3DCompile(pixel_source, strlen(pixel_source),
                            "SkillshotVRMenuOverlayPS", NULL, NULL,
                            "main", "ps_4_0", 0, 0, &pixel_blob, &errors);
    if (FAILED(hr)) {
        xr_log("Menu overlay pixel shader failed: %s",
               errors ? (char *)ID3D10Blob_GetBufferPointer(errors) : "unknown");
        if (errors) ID3D10Blob_Release(errors);
        return 0;
    }
    if (errors) ID3D10Blob_Release(errors);
    hr = ID3D11Device_CreatePixelShader(
        g_device, ID3D10Blob_GetBufferPointer(pixel_blob),
        ID3D10Blob_GetBufferSize(pixel_blob), NULL,
        &g_menu_overlay_pixel_shader);
    ID3D10Blob_Release(pixel_blob);
    if (FAILED(hr)) return 0;
    D3D11_BLEND_DESC blend_desc;
    memset(&blend_desc, 0, sizeof(blend_desc));
    blend_desc.RenderTarget[0].BlendEnable = TRUE;
    blend_desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blend_desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blend_desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blend_desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blend_desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
    blend_desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blend_desc.RenderTarget[0].RenderTargetWriteMask =
        D3D11_COLOR_WRITE_ENABLE_ALL;
    if (FAILED(ID3D11Device_CreateBlendState(
            g_device, &blend_desc, &g_menu_overlay_blend))) return 0;
    xr_log("Exact transparent-pass projection interface compositor initialized");
    return 1;
}

static int render_menu_overlay_layer(ID3D11RenderTargetView *target,
                                     ID3D11Texture2D *destination,
                                     int width, int height,
                                     int eye, int blend_over_world,
                                     XrRect2Di *present_rect)
{
    if (!target || !destination || width <= 0 || height <= 0 ||
        eye < 0 || eye >= EYE_COUNT ||
        !g_menu_overlay_ready || !g_menu_overlay_exact_alpha ||
        g_interface_content_width <= 0 || g_interface_content_height <= 0 ||
        !g_menu_overlay_pixel_shader || !g_menu_interop_views[0]) return 0;
    float constants[12] = {
        1.0f / (float)g_eyes[0].width,
        1.0f / (float)g_eyes[0].height, 0,
        (g_swapchain_format == DXGI_FORMAT_R8G8B8A8_UNORM_SRGB ||
         g_swapchain_format == DXGI_FORMAT_B8G8R8A8_UNORM_SRGB) ? 1.0f : 0.0f,
        (float)g_interface_content_x / (float)g_eyes[0].width,
        (float)g_interface_content_y / (float)g_eyes[0].height,
        (float)g_interface_content_width / (float)g_eyes[0].width,
        (float)g_interface_content_height / (float)g_eyes[0].height,
        0, 0, 1, 1
    };
    ID3D11DeviceContext_UpdateSubresource(
        g_context, (ID3D11Resource *)g_pair_sharpen_constants,
        0, NULL, constants, 0, 0);
    D3D11_VIEWPORT viewport = {
        0, 0, (float)width, (float)height, 0, 1
    };
    int destination_rect[4] = {0, 0, width, height};
    if (blend_over_world) {
        float optical_center = 0.5f;
        if (g_geometry_fov_valid[eye])
            vr_projection_optical_center_x_f(
                g_pending_pair_fov[eye].angleLeft,
                g_pending_pair_fov[eye].angleRight, &optical_center);
        if (!vr_safe_canvas_rect_for_eye(
                width, height, optical_center, g_native_hud_safe_scale_x,
                g_native_hud_safe_scale_y, destination_rect))
            return 0;
        viewport.TopLeftX = (float)destination_rect[0];
        viewport.TopLeftY = (float)destination_rect[1];
        viewport.Width = (float)destination_rect[2];
        viewport.Height = (float)destination_rect[3];
    }
    ID3D11ShaderResourceView *views[2] = {
        g_menu_interop_views[0], NULL
    };
    ID3D11DeviceContext_OMSetRenderTargets(g_context, 1, &target, NULL);
    ID3D11DeviceContext_OMSetBlendState(
        g_context, blend_over_world ? g_menu_overlay_blend : NULL,
        NULL, 0xFFFFFFFFu);
    ID3D11DeviceContext_RSSetState(g_context, g_pair_sharpen_rasterizer);
    ID3D11DeviceContext_RSSetViewports(g_context, 1, &viewport);
    ID3D11DeviceContext_IASetInputLayout(g_context, NULL);
    ID3D11DeviceContext_IASetPrimitiveTopology(
        g_context, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ID3D11DeviceContext_VSSetShader(
        g_context, g_pair_sharpen_vertex_shader, NULL, 0);
    ID3D11DeviceContext_PSSetShader(
        g_context, g_menu_overlay_pixel_shader, NULL, 0);
    ID3D11DeviceContext_PSSetConstantBuffers(
        g_context, 0, 1, &g_pair_sharpen_constants);
    ID3D11DeviceContext_PSSetShaderResources(g_context, 0, 2, views);
    ID3D11DeviceContext_PSSetSamplers(
        g_context, 0, 1, &g_pair_sharpen_sampler);
    ID3D11DeviceContext_Draw(g_context, 3, 0);
    ID3D11ShaderResourceView *empty[2] = {NULL, NULL};
    ID3D11DeviceContext_PSSetShaderResources(g_context, 0, 2, empty);
    ID3D11DeviceContext_OMSetRenderTargets(g_context, 0, NULL, NULL);
    memcpy(g_interface_present_rect[eye], destination_rect,
           sizeof(destination_rect));
    if (present_rect) {
        present_rect->offset.x = destination_rect[0];
        present_rect->offset.y = destination_rect[1];
        present_rect->extent.width = destination_rect[2];
        present_rect->extent.height = destination_rect[3];
    }
    if (!g_menu_overlay_logged) {
        xr_log("Pause/Tab/level-up UI composited into both projection eyes "
               "from source rect %d,%d %dx%d into HUD-safe rect %d,%d %dx%d "
               "(current projection rect %d,%d %dx%d)",
               g_interface_content_x, g_interface_content_y,
               g_interface_content_width, g_interface_content_height,
               destination_rect[0], destination_rect[1],
               destination_rect[2], destination_rect[3],
               g_content_x, g_content_y, g_content_width, g_content_height);
        g_menu_overlay_logged = 1;
    }
    return 1;
}

static int resolve_sharpened_pair_eye(int pair, int eye,
                                      int completed_interface)
{
    if (pair < 0 || pair >= PAIR_BUFFER_COUNT || eye < 0 || eye >= EYE_COUNT ||
        !g_pair_sharpen_vertex_shader || !g_pair_sharpen_pixel_shader ||
        !g_present_pair_targets[pair][eye] || !g_interop_views[eye]) return 0;
    float optical_center = 0.5f;
    if (g_geometry_fov_valid[eye])
        vr_projection_optical_center_x_f(
            g_pending_pair_fov[eye].angleLeft,
            g_pending_pair_fov[eye].angleRight, &optical_center);
    XrRect2Di rect = g_geometry_eye_rect[eye];
    float content_x = (float)rect.offset.x / (float)g_eyes[eye].width;
    float content_y = (float)rect.offset.y / (float)g_eyes[eye].height;
    float content_width = (float)rect.extent.width / (float)g_eyes[eye].width;
    float content_height = (float)rect.extent.height / (float)g_eyes[eye].height;
    if (content_width <= 0.0f || content_height <= 0.0f) {
        content_x = content_y = 0.0f;
        content_width = content_height = 1.0f;
    }
    float constants[12] = {
        1.0f / (float)g_eyes[eye].width,
        1.0f / (float)g_eyes[eye].height,
        (g_hud_base_interop_ready[eye] && g_hud_base_interop_ready[0])
            ? 1.0f : 0.0f, completed_interface ? 1.0f : 0.0f,
        content_x, content_y, content_width, content_height,
        optical_center - g_native_hud_safe_scale_x * 0.5f,
        0.5f - g_native_hud_safe_scale_y * 0.5f,
        g_native_hud_safe_scale_x, g_native_hud_safe_scale_y
    };
    ID3D11DeviceContext_UpdateSubresource(
        g_context, (ID3D11Resource *)g_pair_sharpen_constants,
        0, NULL, constants, 0, 0);
    D3D11_VIEWPORT viewport = {
        0, 0, (float)g_eyes[eye].width, (float)g_eyes[eye].height, 0, 1
    };
    ID3D11ShaderResourceView *views[4] = {
        g_interop_views[eye], g_hud_base_interop_views[eye],
        g_interop_views[0], g_hud_base_interop_views[0]
    };
    ID3D11DeviceContext_OMSetRenderTargets(
        g_context, 1, &g_present_pair_targets[pair][eye], NULL);
    ID3D11DeviceContext_OMSetBlendState(g_context, NULL, NULL, 0xFFFFFFFFu);
    ID3D11DeviceContext_RSSetState(g_context, g_pair_sharpen_rasterizer);
    ID3D11DeviceContext_RSSetViewports(g_context, 1, &viewport);
    ID3D11DeviceContext_IASetInputLayout(g_context, NULL);
    ID3D11DeviceContext_IASetPrimitiveTopology(
        g_context, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ID3D11DeviceContext_VSSetShader(
        g_context, g_pair_sharpen_vertex_shader, NULL, 0);
    ID3D11DeviceContext_PSSetShader(
        g_context, g_pair_sharpen_pixel_shader, NULL, 0);
    ID3D11DeviceContext_PSSetConstantBuffers(
        g_context, 0, 1, &g_pair_sharpen_constants);
    ID3D11DeviceContext_PSSetShaderResources(g_context, 0, 4, views);
    ID3D11DeviceContext_PSSetSamplers(
        g_context, 0, 1, &g_pair_sharpen_sampler);
    ID3D11DeviceContext_Draw(g_context, 3, 0);
    ID3D11ShaderResourceView *empty[4] = {NULL, NULL, NULL, NULL};
    ID3D11DeviceContext_PSSetShaderResources(g_context, 0, 4, empty);
    ID3D11DeviceContext_OMSetRenderTargets(g_context, 0, NULL, NULL);
    if (!g_pair_sharpen_logged) {
        xr_log("Fresh stereo pairs receive native-detail HUD separation and clarity resolve");
        g_pair_sharpen_logged = 1;
    }
    return 1;
}

static int initialize_gl_interop(void)
{
    HMODULE system_gl = GetModuleHandleA("opengl32_system.dll");
    PFNWGLGETPROCADDRESSRAW get_proc = system_gl
        ? (PFNWGLGETPROCADDRESSRAW)GetProcAddress(system_gl, "wglGetProcAddress") : NULL;
    p_glGetIntegerv = system_gl ? (PFNGLGETINTEGERVRAW)GetProcAddress(system_gl, "glGetIntegerv") : NULL;
    p_glGenTextures = system_gl ? (PFNGLGENTEXTURESRAW)GetProcAddress(system_gl, "glGenTextures") : NULL;
    p_glBindTextureRaw = system_gl ? (PFNGLBINDTEXTURERAW)GetProcAddress(system_gl, "glBindTexture") : NULL;
    p_glTexImage2DRaw = system_gl ? (PFNGLTEXIMAGE2DRAW)GetProcAddress(system_gl, "glTexImage2D") : NULL;
    p_glTexParameteriRaw = system_gl ? (PFNGLTEXPARAMETERIRAW)GetProcAddress(system_gl, "glTexParameteri") : NULL;
    p_glGetFloatvRaw = system_gl ? (PFNGLGETFLOATVRAW)GetProcAddress(system_gl, "glGetFloatv") : NULL;
    p_glClearColorRaw = system_gl ? (PFNGLCLEARCOLORRAW)GetProcAddress(system_gl, "glClearColor") : NULL;
    p_glClearRaw = system_gl ? (PFNGLCLEARRAW)GetProcAddress(system_gl, "glClear") : NULL;
    p_glColorMaskRaw = system_gl ? (void *)GetProcAddress(system_gl, "glColorMask") : NULL;
    p_glIsEnabledRaw = system_gl ? (PFNGLISENABLEDRAW)GetProcAddress(system_gl, "glIsEnabled") : NULL;
    p_glEnableRaw = system_gl ? (PFNGLENABLEDISABLERAW)GetProcAddress(system_gl, "glEnable") : NULL;
    p_glDisableRaw = system_gl ? (PFNGLENABLEDISABLERAW)GetProcAddress(system_gl, "glDisable") : NULL;
    p_glMatrixModeRaw = system_gl ? (PFNGLMATRIXMODERAW)GetProcAddress(system_gl, "glMatrixMode") : NULL;
    p_glPushMatrixRaw = system_gl ? (PFNGLPUSHPOPMATRIXRAW)GetProcAddress(system_gl, "glPushMatrix") : NULL;
    p_glPopMatrixRaw = system_gl ? (PFNGLPUSHPOPMATRIXRAW)GetProcAddress(system_gl, "glPopMatrix") : NULL;
    p_glLoadIdentityRaw = system_gl ? (PFNGLLOADIDENTITYRAW)GetProcAddress(system_gl, "glLoadIdentity") : NULL;
    p_glOrthoRaw = system_gl ? (PFNGLORTHORAW)GetProcAddress(system_gl, "glOrtho") : NULL;
    p_glColor4fRaw = system_gl ? (PFNGLCOLOR4FRAW)GetProcAddress(system_gl, "glColor4f") : NULL;
    p_glBeginRaw = system_gl ? (PFNGLBEGINRAW)GetProcAddress(system_gl, "glBegin") : NULL;
    p_glEndRaw = system_gl ? (PFNGLENDRAW)GetProcAddress(system_gl, "glEnd") : NULL;
    p_glTexCoord2fRaw = system_gl ? (PFNGLTEXCOORD2FRAW)GetProcAddress(system_gl, "glTexCoord2f") : NULL;
    p_glVertex2fRaw = system_gl ? (PFNGLVERTEX2FRAW)GetProcAddress(system_gl, "glVertex2f") : NULL;
    p_glGetTexEnvivRaw = system_gl ? (PFNGLGETTEXENVIVRAW)GetProcAddress(system_gl, "glGetTexEnviv") : NULL;
    p_glTexEnviRaw = system_gl ? (PFNGLTEXENVIRAW)GetProcAddress(system_gl, "glTexEnvi") : NULL;
    p_glGetError = system_gl ? (PFNGLGETERRORRAW)GetProcAddress(system_gl, "glGetError") : NULL;
    p_glFinish = system_gl ? (PFNGLFINISHRAW)GetProcAddress(system_gl, "glFinish") : NULL;
    p_glDrawBuffer = system_gl ? (PFNGLDRAWBUFFERRAW)GetProcAddress(system_gl, "glDrawBuffer") : NULL;
    if (!get_proc || !p_glGetIntegerv || !p_glGenTextures || !p_glFinish ||
        !p_glGetFloatvRaw || !p_glClearColorRaw || !p_glClearRaw) {
        xr_log("OpenGL/D3D interop unavailable: core OpenGL functions missing");
        return 0;
    }
#define GL_EXT(name, type) p_##name = (type)get_proc(#name)
    GL_EXT(wglDXOpenDeviceNV, PFNWGLDXOPENDEVICENV);
    GL_EXT(wglDXCloseDeviceNV, PFNWGLDXCLOSEDEVICENV);
    GL_EXT(wglDXRegisterObjectNV, PFNWGLDXREGISTEROBJECTNV);
    GL_EXT(wglDXUnregisterObjectNV, PFNWGLDXUNREGISTEROBJECTNV);
    GL_EXT(wglDXLockObjectsNV, PFNWGLDXLOCKOBJECTSNV);
    GL_EXT(wglDXUnlockObjectsNV, PFNWGLDXUNLOCKOBJECTSNV);
    GL_EXT(glGenFramebuffers, PFNGLGENFRAMEBUFFERS);
    GL_EXT(glBindFramebuffer, PFNGLBINDFRAMEBUFFER);
    GL_EXT(glFramebufferTexture2D, PFNGLFRAMEBUFFERTEXTURE2D);
    GL_EXT(glCheckFramebufferStatus, PFNGLCHECKFRAMEBUFFERSTATUS);
    GL_EXT(glBlitFramebuffer, PFNGLBLITFRAMEBUFFER);
    p_glUseProgramRaw = (PFNGLUSEPROGRAMRAW)get_proc("glUseProgram");
    p_glActiveTextureRaw = (PFNGLACTIVETEXTURERAW)get_proc("glActiveTexture");
    p_glBlendEquationSeparateRaw =
        (PFNGLBLENDEQUATIONSEPARATERAW)get_proc("glBlendEquationSeparate");
    p_glBlendFuncSeparateRaw =
        (PFNGLBLENDFUNCSEPARATERAW)get_proc("glBlendFuncSeparate");
#undef GL_EXT
    if (!p_wglDXOpenDeviceNV || !p_wglDXRegisterObjectNV || !p_wglDXLockObjectsNV ||
        !p_wglDXUnlockObjectsNV || !p_glGenFramebuffers || !p_glBindFramebuffer ||
        !p_glFramebufferTexture2D || !p_glCheckFramebufferStatus || !p_glBlitFramebuffer ||
        !p_glBlendFuncSeparateRaw || !p_glIsEnabledRaw || !p_glEnableRaw ||
        !p_glDisableRaw || !p_glUseProgramRaw || !p_glActiveTextureRaw ||
        !p_glMatrixModeRaw || !p_glPushMatrixRaw || !p_glPopMatrixRaw ||
        !p_glLoadIdentityRaw || !p_glOrthoRaw || !p_glColor4fRaw ||
        !p_glBeginRaw || !p_glEndRaw || !p_glTexCoord2fRaw || !p_glVertex2fRaw ||
        !p_glBlendEquationSeparateRaw || !p_glGetTexEnvivRaw || !p_glTexEnviRaw) {
        xr_log("OpenGL/D3D interop unavailable: WGL_NV_DX_interop2 is incomplete");
        return 0;
    }

    D3D11_TEXTURE2D_DESC texture_desc;
    memset(&texture_desc, 0, sizeof(texture_desc));
    texture_desc.Width = (UINT)g_eyes[0].width;
    texture_desc.Height = (UINT)g_eyes[0].height;
    texture_desc.MipLevels = 1;
    texture_desc.ArraySize = 1;
    texture_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texture_desc.SampleDesc.Count = 1;
    texture_desc.Usage = D3D11_USAGE_DEFAULT;
    texture_desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    texture_desc.MiscFlags = D3D11_RESOURCE_MISC_SHARED;
    for (uint32_t eye = 0; eye < EYE_COUNT; ++eye) {
        HRESULT hr = ID3D11Device_CreateTexture2D(g_device, &texture_desc, NULL, &g_interop_textures[eye]);
        if (FAILED(hr)) {
            xr_log("OpenGL/D3D interop texture %u failed: 0x%08lX", eye, (unsigned long)hr);
            return 0;
        }
        D3D11_TEXTURE2D_DESC pair_desc = texture_desc;
        pair_desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
        pair_desc.MiscFlags = 0;
        for (int pair = 0; pair < PAIR_BUFFER_COUNT; ++pair) {
            hr = ID3D11Device_CreateTexture2D(g_device, &pair_desc, NULL,
                                              &g_present_pair_textures[pair][eye]);
            if (FAILED(hr)) {
                xr_log("Coherent pair texture %d/%u failed: 0x%08lX", pair, eye,
                       (unsigned long)hr);
                return 0;
            }
            hr = ID3D11Device_CreateRenderTargetView(
                g_device, (ID3D11Resource *)g_present_pair_textures[pair][eye],
                NULL, &g_present_pair_targets[pair][eye]);
            if (FAILED(hr)) {
                xr_log("Coherent pair target %d/%u failed: 0x%08lX", pair, eye,
                       (unsigned long)hr);
                return 0;
            }
            hr = ID3D11Device_CreateShaderResourceView(
                g_device, (ID3D11Resource *)g_present_pair_textures[pair][eye],
                NULL, &g_present_pair_views[pair][eye]);
            if (FAILED(hr)) {
                xr_log("Coherent pair view %d/%u failed: 0x%08lX", pair, eye,
                       (unsigned long)hr);
                return 0;
            }
        }
        hr = ID3D11Device_CreateShaderResourceView(g_device,
                                                    (ID3D11Resource *)g_interop_textures[eye],
                                                    NULL, &g_interop_views[eye]);
        if (FAILED(hr)) {
            xr_log("OpenGL/D3D final-frame SRV %u failed: 0x%08lX", eye, (unsigned long)hr);
            return 0;
        }
        hr = ID3D11Device_CreateTexture2D(
            g_device, &texture_desc, NULL, &g_hud_base_interop_textures[eye]);
        if (FAILED(hr)) {
            xr_log("OpenGL/D3D HUD-base texture %u failed: 0x%08lX",
                   eye, (unsigned long)hr);
            return 0;
        }
        hr = ID3D11Device_CreateShaderResourceView(
            g_device, (ID3D11Resource *)g_hud_base_interop_textures[eye],
            NULL, &g_hud_base_interop_views[eye]);
        if (FAILED(hr)) {
            xr_log("OpenGL/D3D HUD-base SRV %u failed: 0x%08lX",
                   eye, (unsigned long)hr);
            return 0;
        }
    }
    if (!initialize_pair_sharpen_compositor())
        xr_log("Fresh-pair clarity resolve unavailable; using direct pair copies");
    if (!initialize_menu_overlay_compositor())
        xr_log("In-world menu composition unavailable; using flat-menu fallback");
    for (int frame = 0; frame < 2; ++frame) {
        HRESULT hr = ID3D11Device_CreateTexture2D(
            g_device, &texture_desc, NULL, &g_menu_interop_textures[frame]);
        if (FAILED(hr)) {
            xr_log("Menu interop texture %d failed: 0x%08lX", frame,
                   (unsigned long)hr);
            return 0;
        }
        hr = ID3D11Device_CreateShaderResourceView(
            g_device, (ID3D11Resource *)g_menu_interop_textures[frame],
            NULL, &g_menu_interop_views[frame]);
        if (FAILED(hr)) {
            xr_log("Menu interop view %d failed: 0x%08lX", frame,
                   (unsigned long)hr);
            return 0;
        }
    }
    g_interop_device = p_wglDXOpenDeviceNV(g_device);
    if (!g_interop_device) {
        xr_log("wglDXOpenDeviceNV failed: %lu", GetLastError());
        return 0;
    }
    p_glGenTextures(EYE_COUNT, g_interop_gl_textures);
    p_glGenTextures(EYE_COUNT, g_hud_base_interop_gl_textures);
    p_glGenTextures(2, g_menu_interop_gl_textures);
    p_glGenFramebuffers(1, &g_interop_framebuffer);
    for (uint32_t eye = 0; eye < EYE_COUNT; ++eye) {
        g_interop_objects[eye] = p_wglDXRegisterObjectNV(g_interop_device, g_interop_textures[eye],
                                                         g_interop_gl_textures[eye], GL_TEXTURE_2D,
                                                         WGL_ACCESS_WRITE_DISCARD_NV);
        if (!g_interop_objects[eye]) {
            xr_log("wglDXRegisterObjectNV eye %u failed: %lu", eye, GetLastError());
            return 0;
        }
        g_hud_base_interop_objects[eye] = p_wglDXRegisterObjectNV(
            g_interop_device, g_hud_base_interop_textures[eye],
            g_hud_base_interop_gl_textures[eye], GL_TEXTURE_2D,
            WGL_ACCESS_WRITE_DISCARD_NV);
        if (!g_hud_base_interop_objects[eye]) {
            xr_log("wglDXRegisterObjectNV HUD base eye %u failed: %lu",
                   eye, GetLastError());
            return 0;
        }
    }
    for (int frame = 0; frame < 2; ++frame) {
        g_menu_interop_objects[frame] = p_wglDXRegisterObjectNV(
            g_interop_device, g_menu_interop_textures[frame],
            g_menu_interop_gl_textures[frame], GL_TEXTURE_2D,
            WGL_ACCESS_WRITE_DISCARD_NV);
        if (!g_menu_interop_objects[frame]) {
            xr_log("wglDXRegisterObjectNV menu frame %d failed: %lu",
                   frame, GetLastError());
            return 0;
        }
    }
    D3D11_QUERY_DESC fence_desc;
    memset(&fence_desc, 0, sizeof(fence_desc));
    fence_desc.Query = D3D11_QUERY_EVENT;
    HRESULT fence_hr = ID3D11Device_CreateQuery(g_device, &fence_desc, &g_pair_copy_fence);
    if (FAILED(fence_hr)) {
        xr_log("Coherent-pair completion fence creation failed: 0x%08lX",
               (unsigned long)fence_hr);
        return 0;
    }
    g_gl_interop_ready = 1;
    xr_log("WGL_NV_DX_interop2 initialized for direct game-frame transfer");
    return 1;
}

static int wait_for_pair_copy_fence(void)
{
    if (!g_pair_copy_fence ||
        !InterlockedCompareExchange(&g_pair_copy_fence_pending, 0, 0)) return 1;
    ULONGLONG started = GetTickCount64();
    for (;;) {
        BOOL complete = FALSE;
        HRESULT hr = ID3D11DeviceContext_GetData(
            g_context, (ID3D11Asynchronous *)g_pair_copy_fence,
            &complete, sizeof(complete), 0);
        if (hr == S_OK && complete) {
            InterlockedExchange(&g_pair_copy_fence_pending, 0);
            return 1;
        }
        if (FAILED(hr)) {
            xr_log("Coherent-pair completion fence failed: 0x%08lX", (unsigned long)hr);
            InterlockedExchange(&g_pair_copy_fence_pending, 0);
            return 0;
        }
        if (GetTickCount64() - started >= 1000) {
            xr_log("Coherent-pair completion fence timed out after 1000ms");
            return 0;
        }
        SwitchToThread();
    }
}

/* Physical NVIDIA/Meta Link occasionally rejects a WGL interop ownership
   transition even after GL completion. The simulator never reproduced it.
   Keep retries short and bounded: a missed game-eye publication is preferable
   to blocking the 72-Hz OpenXR frame thread or permanently retaining a stale
   pair. Callers deliberately transfer one object at a time because physical
   Link also proved less reliable with multi-object lock arrays. */
static int lock_single_interop_object(HANDLE object, DWORD *last_error)
{
    if (last_error) *last_error = ERROR_SUCCESS;
    if (!object || !g_interop_device || !p_wglDXLockObjectsNV) return 0;
    for (int attempt = 0; attempt < 3; ++attempt) {
        SetLastError(ERROR_SUCCESS);
        if (p_wglDXLockObjectsNV(g_interop_device, 1, &object)) return 1;
        if (last_error) *last_error = GetLastError();
        if (p_glFinish) p_glFinish();
        SwitchToThread();
    }
    return 0;
}

static int unlock_single_interop_object(HANDLE object, DWORD *last_error)
{
    if (last_error) *last_error = ERROR_SUCCESS;
    if (!object || !g_interop_device || !p_wglDXUnlockObjectsNV) return 0;
    for (int attempt = 0; attempt < 3; ++attempt) {
        SetLastError(ERROR_SUCCESS);
        if (p_wglDXUnlockObjectsNV(g_interop_device, 1, &object)) return 1;
        if (last_error) *last_error = GetLastError();
        if (p_glFinish) p_glFinish();
        SwitchToThread();
    }
    return 0;
}

int openxr_bridge_begin_interface_alpha_capture(void)
{
    g_interface_alpha_desktop_ready = 0;
    if (!g_gl_interop_ready || !p_glGetIntegerv || !p_glGenTextures ||
        !p_glBindTextureRaw || !p_glTexImage2DRaw || !p_glTexParameteriRaw ||
        !p_glGenFramebuffers || !p_glBindFramebuffer ||
        !p_glFramebufferTexture2D || !p_glCheckFramebufferStatus ||
        !p_glGetFloatvRaw || !p_glClearColorRaw || !p_glClearRaw) return 0;
    GLint viewport[4] = {0};
    GLint previous_texture = 0;
    GLfloat previous_clear[4] = {0, 0, 0, 0};
    p_glGetIntegerv(GL_VIEWPORT, viewport);
    if (viewport[2] <= 0 || viewport[3] <= 0) return 0;
    p_glGetIntegerv(GL_TEXTURE_BINDING_2D, &previous_texture);
    p_glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING,
                    &g_interface_alpha_saved_draw_framebuffer);
    p_glGetIntegerv(GL_BLEND_SRC_RGB,
                    &g_interface_alpha_saved_blend[0]);
    p_glGetIntegerv(GL_BLEND_DST_RGB,
                    &g_interface_alpha_saved_blend[1]);
    p_glGetIntegerv(GL_BLEND_SRC_ALPHA,
                    &g_interface_alpha_saved_blend[2]);
    p_glGetIntegerv(GL_BLEND_DST_ALPHA,
                    &g_interface_alpha_saved_blend[3]);
    p_glGetIntegerv(GL_BLEND_EQUATION_RGB,
                    &g_interface_alpha_saved_blend_equation[0]);
    p_glGetIntegerv(GL_BLEND_EQUATION_ALPHA,
                    &g_interface_alpha_saved_blend_equation[1]);
    p_glGetFloatvRaw(GL_COLOR_CLEAR_VALUE, previous_clear);
    p_glGetIntegerv(GL_COLOR_WRITEMASK, g_interface_alpha_saved_color_mask);
    if (!g_interface_alpha_textures[0])
        p_glGenTextures(2, g_interface_alpha_textures);
    if (!g_interface_alpha_framebuffer)
        p_glGenFramebuffers(1, &g_interface_alpha_framebuffer);
    int resized = g_interface_alpha_width != viewport[2] ||
                  g_interface_alpha_height != viewport[3];
    p_glBindTextureRaw(GL_TEXTURE_2D, g_interface_alpha_textures[0]);
    p_glTexParameteriRaw(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    p_glTexParameteriRaw(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    if (resized || !g_interface_alpha_width || !g_interface_alpha_height)
        p_glTexImage2DRaw(GL_TEXTURE_2D, 0, GL_RGBA, viewport[2], viewport[3],
                          0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    p_glBindFramebuffer(GL_DRAW_FRAMEBUFFER, g_interface_alpha_framebuffer);
    p_glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                             GL_TEXTURE_2D, g_interface_alpha_textures[0], 0);
    int complete = p_glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) ==
                   GL_FRAMEBUFFER_COMPLETE;
    if (complete) {
        /* An authored UI clip must not limit clearing our private texture;
           otherwise pixels from the previous menu survive outside the clip. */
        GLboolean scissor_enabled = p_glIsEnabledRaw(GL_SCISSOR_TEST);
        p_glDisableRaw(GL_SCISSOR_TEST);
        if (p_glColorMaskRaw) p_glColorMaskRaw(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        p_glClearColorRaw(0.0f, 0.0f, 0.0f, 0.0f);
        p_glClearRaw(GL_COLOR_BUFFER_BIT);
        if (scissor_enabled) p_glEnableRaw(GL_SCISSOR_TEST);
        /* Preserve the game's RGB blend equation while accumulating alpha as
           ordinary source-over coverage. This makes the texture's RGB
           premultiplied and its A channel exact even for overlapping panels,
           antialiased glyphs, and translucent menu fills. */
        if (p_glColorMaskRaw) p_glColorMaskRaw(
            (GLboolean)g_interface_alpha_saved_color_mask[0],
            (GLboolean)g_interface_alpha_saved_color_mask[1],
            (GLboolean)g_interface_alpha_saved_color_mask[2], GL_TRUE);
        p_glBlendFuncSeparateRaw(
            (GLenum)g_interface_alpha_saved_blend[0],
            (GLenum)g_interface_alpha_saved_blend[1],
            GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        p_glBlendEquationSeparateRaw(
            (GLenum)g_interface_alpha_saved_blend_equation[0], GL_FUNC_ADD);
    }
    p_glBindTextureRaw(GL_TEXTURE_2D, (GLuint)previous_texture);
    p_glClearColorRaw(previous_clear[0], previous_clear[1],
                      previous_clear[2], previous_clear[3]);
    if (!complete) {
        p_glBindFramebuffer(
            GL_DRAW_FRAMEBUFFER,
            (GLuint)g_interface_alpha_saved_draw_framebuffer);
        g_interface_alpha_active = 0;
        g_menu_overlay_ready = 0;
        g_menu_overlay_exact_alpha = 0;
        return 0;
    }
    g_interface_alpha_width = viewport[2];
    g_interface_alpha_height = viewport[3];
    g_interface_alpha_draws = 0;
    g_interface_alpha_active = 1;
    return 1;
}

int openxr_bridge_begin_interface_alpha_draw(int pass)
{
    /* The original appended UI pass is redirected wholesale, which also
       captures legacy immediate-mode and display-list draws. Draw wrappers
       call this hook only to count activity; no draw replay is required. */
    if (!g_interface_alpha_active) return 0;
    if (pass == 0) ++g_interface_alpha_draws;
    return 0;
}

void openxr_bridge_end_interface_alpha_draw(void)
{
    /* No-op: the capture framebuffer stays bound until SwapBuffers so legacy
       glBegin/glEnd and display lists are included without replaying them. */
}

unsigned int openxr_bridge_interface_alpha_framebuffer(void)
{
    return g_interface_alpha_active ? g_interface_alpha_framebuffer : 0;
}

int openxr_bridge_finish_interface_alpha_capture(int publish)
{
    if (!g_interface_alpha_active) {
        if (!publish) {
            g_menu_overlay_ready = 0;
            g_menu_overlay_exact_alpha = 0;
        }
        return 0;
    }
    g_interface_alpha_active = 0;
    static int mask_logged;
    if (!mask_logged) {
        GLint mask[4];
        p_glGetIntegerv(GL_COLOR_WRITEMASK, mask);
        xr_log("Interface color-write mask saved=%d/%d/%d/%d final=%d/%d/%d/%d",
            g_interface_alpha_saved_color_mask[0], g_interface_alpha_saved_color_mask[1],
            g_interface_alpha_saved_color_mask[2], g_interface_alpha_saved_color_mask[3],
            mask[0], mask[1], mask[2], mask[3]);
        mask_logged = 1;
    }
    /* UI drawing is complete. Restore the game's framebuffer and complete
       blend state before the D3D copy or any later desktop presentation. */
    p_glBindFramebuffer(GL_DRAW_FRAMEBUFFER,
                        (GLuint)g_interface_alpha_saved_draw_framebuffer);
    if (p_glColorMaskRaw) p_glColorMaskRaw(
        (GLboolean)g_interface_alpha_saved_color_mask[0],
        (GLboolean)g_interface_alpha_saved_color_mask[1],
        (GLboolean)g_interface_alpha_saved_color_mask[2],
        (GLboolean)g_interface_alpha_saved_color_mask[3]);
    p_glBlendFuncSeparateRaw(
        (GLenum)g_interface_alpha_saved_blend[0],
        (GLenum)g_interface_alpha_saved_blend[1],
        (GLenum)g_interface_alpha_saved_blend[2],
        (GLenum)g_interface_alpha_saved_blend[3]);
    p_glBlendEquationSeparateRaw(
        (GLenum)g_interface_alpha_saved_blend_equation[0],
        (GLenum)g_interface_alpha_saved_blend_equation[1]);
    g_interface_alpha_desktop_ready = publish;
    if (!publish || !g_gl_interop_ready) {
        g_menu_overlay_ready = 0;
        g_menu_overlay_exact_alpha = 0;
        return 0;
    }
    AcquireSRWLockExclusive(&g_interop_capture_lock);
    HANDLE object = g_menu_interop_objects[0];
    DWORD lock_error = ERROR_SUCCESS;
    if (!lock_single_interop_object(object, &lock_error)) {
        g_menu_overlay_ready = 0;
        g_menu_overlay_exact_alpha = 0;
        static LONG menu_lock_failures;
        LONG failures = InterlockedIncrement(&menu_lock_failures);
        if (failures == 1 || (failures % 120) == 0)
            xr_log("Menu interop lock failed error=%lu failures=%ld",
                   lock_error, failures);
        ReleaseSRWLockExclusive(&g_interop_capture_lock);
        return 0;
    }
    GLint previous_read = 0, previous_draw = 0;
    GLfloat previous_clear[4] = {0, 0, 0, 0};
    p_glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &previous_read);
    p_glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &previous_draw);
    p_glGetFloatvRaw(GL_COLOR_CLEAR_VALUE, previous_clear);
    int target_width = g_eyes[0].width;
    int target_height = g_eyes[0].height;
    int fitted_width = target_width;
    int fitted_height = g_interface_alpha_width > 0
        ? (g_interface_alpha_height * target_width) /
          g_interface_alpha_width : target_height;
    if (fitted_height > target_height) {
        fitted_height = target_height;
        fitted_width = g_interface_alpha_height > 0
            ? (g_interface_alpha_width * target_height) /
              g_interface_alpha_height : target_width;
    }
    int x = (target_width - fitted_width) / 2;
    int y = (target_height - fitted_height) / 2;
    g_interface_content_x = x;
    g_interface_content_y = y;
    g_interface_content_width = fitted_width;
    g_interface_content_height = fitted_height;
    p_glBindFramebuffer(GL_READ_FRAMEBUFFER,
                        g_interface_alpha_framebuffer);
    p_glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                             GL_TEXTURE_2D,
                             g_interface_alpha_textures[0], 0);
    p_glBindFramebuffer(GL_DRAW_FRAMEBUFFER, g_interop_framebuffer);
    p_glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                             GL_TEXTURE_2D,
                             g_menu_interop_gl_textures[0], 0);
    int complete = p_glCheckFramebufferStatus(GL_READ_FRAMEBUFFER) ==
                       GL_FRAMEBUFFER_COMPLETE &&
                   p_glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) ==
                       GL_FRAMEBUFFER_COMPLETE;
    if (complete) {
        /* Both clear and blit obey scissor; clear also obeys the game's
           restored RGB/alpha write mask. Initialize all letterbox pixels and
           transfer the whole menu, then put the authored state back. */
        GLboolean scissor_enabled = p_glIsEnabledRaw(GL_SCISSOR_TEST);
        p_glDisableRaw(GL_SCISSOR_TEST);
        p_glColorMaskRaw(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        p_glClearColorRaw(0.0f, 0.0f, 0.0f, 0.0f);
        p_glClearRaw(GL_COLOR_BUFFER_BIT);
        p_glBlitFramebuffer(0, 0,
                            g_interface_alpha_width,
                            g_interface_alpha_height,
                            x, y + fitted_height,
                            x + fitted_width, y,
                            GL_COLOR_BUFFER_BIT, GL_LINEAR);
        p_glColorMaskRaw(
            (GLboolean)g_interface_alpha_saved_color_mask[0],
            (GLboolean)g_interface_alpha_saved_color_mask[1],
            (GLboolean)g_interface_alpha_saved_color_mask[2],
            (GLboolean)g_interface_alpha_saved_color_mask[3]);
        if (scissor_enabled) p_glEnableRaw(GL_SCISSOR_TEST);
    }
    p_glFinish();
    p_glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                             GL_TEXTURE_2D, 0, 0);
    p_glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                             GL_TEXTURE_2D, 0, 0);
    p_glClearColorRaw(previous_clear[0], previous_clear[1],
                      previous_clear[2], previous_clear[3]);
    p_glBindFramebuffer(GL_READ_FRAMEBUFFER, (GLuint)previous_read);
    p_glBindFramebuffer(GL_DRAW_FRAMEBUFFER, (GLuint)previous_draw);
    DWORD unlock_error = ERROR_SUCCESS;
    int unlocked = unlock_single_interop_object(object, &unlock_error);
    g_menu_overlay_ready = complete && unlocked;
    g_menu_overlay_exact_alpha = g_menu_overlay_ready;
    ReleaseSRWLockExclusive(&g_interop_capture_lock);
    if (!unlocked) {
        static LONG menu_unlock_failures;
        LONG failures = InterlockedIncrement(&menu_unlock_failures);
        if (failures == 1 || (failures % 120) == 0)
            xr_log("Menu interop unlock failed error=%lu failures=%ld",
                   unlock_error, failures);
    }
    if (g_menu_overlay_ready) {
        static LONG logged;
        if (InterlockedCompareExchange(&logged, 1, 0) == 0)
            xr_log("Exact transparent interface pass published (%d tracked draws)",
                   g_interface_alpha_draws);
        capture_composite_proof(
            g_menu_interop_textures[0], g_geometry_proof_marker,
            g_interface_alpha_proof_path,
            &g_interface_alpha_proof_written,
            "premultiplied transparent interface");
        capture_composite_proof(
            g_menu_interop_textures[0], g_geometry_proof_marker,
            g_interface_alpha_mask_proof_path,
            &g_interface_alpha_mask_proof_written,
            "transparent interface alpha channel");
    }
    return g_menu_overlay_ready;
}

static int initialize_stereo_compositor(void)
{
    static const char *vertex_source =
        "struct Out { float4 position : SV_POSITION; float2 uv : TEXCOORD0; };"
        "Out main(uint id : SV_VertexID) {"
        " Out o; float2 p=float2((id<<1)&2,id&2); o.uv=p;"
        " o.position=float4(p*float2(2,-2)+float2(-1,1),0,1); return o; }";
    static const char *pixel_source =
        "cbuffer StereoParams : register(b0) {"
        " float4 p0; float4 content; float4 dims; float4 alignment; };"
        "Texture2D worldColor : register(t0);"
        "Texture2DMS<float,4> worldDepth : register(t1);"
        "Texture2D finalColor : register(t2);"
        "SamplerState linearSampler : register(s0);"
        "float4 main(float4 position:SV_POSITION,float2 uv:TEXCOORD0):SV_TARGET {"
        " float2 alignedUv=float2(clamp(uv.x+alignment.x*p0.x,0.0,1.0),uv.y);"
        " float2 worldUv=saturate(alignedUv);"
        " int2 dp=int2(clamp(worldUv.x,0.0,0.999999)*dims.x,"
        "              clamp(1-worldUv.y,0.0,0.999999)*dims.y);"
        " float d=worldDepth.Load(dp,0);"
        " d=min(d,worldDepth.Load(dp,1)); d=min(d,worldDepth.Load(dp,2));"
        " d=min(d,worldDepth.Load(dp,3));"
        " float zndc=d*2-1; float linearZ=(2*p0.z*p0.w)/(p0.w+p0.z-zndc*(p0.w-p0.z));"
        " float disparity=p0.x*p0.y*(1/max(linearZ,p0.z)-1/dims.z);"
        " disparity=clamp(disparity,-0.006,0.006);"
        " float2 sourceUv=float2(clamp(worldUv.x+disparity,0.0,1.0),1-worldUv.y);"
        " float4 stereoPixel=worldColor.Sample(linearSampler,sourceUv);"
        " float3 outputRgb=pow(saturate(stereoPixel.rgb),2.2);"
        /* Expanded cameras can look beyond the finite map. Keep that guard
           band inside the immersive projection, but replace only genuinely
           empty far-plane black with a quiet environment instead of a void. */
        " float emptyColor=1-smoothstep(0.008,0.035,max(stereoPixel.r,max(stereoPixel.g,stereoPixel.b)));"
        " float emptyDepth=smoothstep(0.9997,0.99998,d);"
        " float emptyWorld=emptyColor*emptyDepth;"
        /* The top and bottom extremes are rotational guard bands. Some maps
           leave culled props there even when their clear color is not black,
           so dissolve those extremes before a head turn can expose them. */
        " float guardBand=max(1-smoothstep(0.08,0.17,uv.y),smoothstep(0.83,0.92,uv.y));"
        " emptyWorld=max(emptyWorld,guardBand);"
        " float horizon=1-abs(uv.y*2-1);"
        " float3 environment=lerp(float3(0.006,0.010,0.020),float3(0.022,0.035,0.055),"
        "                         0.30+0.70*horizon);"
        " outputRgb=lerp(outputRgb,environment,emptyWorld);"
        " float2 hudUv=(alignedUv-content.xy)/content.zw;"
        " bool topHud=hudUv.y<0.16&&(hudUv.x<0.25||hudUv.x>0.82);"
        " bool bottomHud=hudUv.y>0.78&&(hudUv.x<0.20||"
        "                (hudUv.x>0.34&&hudUv.x<0.68)||hudUv.x>0.82);"
        " if(all(hudUv>=0)&&all(hudUv<=1)&&(topHud||bottomHud)) {"
        /* finalColor is already letterboxed in the eye texture. uv addresses
           that texture; hudUv is only the corresponding 0..1 game coordinate. */
        "  float4 finalPixel=finalColor.Sample(linearSampler,alignedUv);"
        "  float4 baseWorld=worldColor.Sample(linearSampler,float2(hudUv.x,1-hudUv.y));"
        "  float difference=max(abs(finalPixel.r-baseWorld.r),"
        "                   max(abs(finalPixel.g-baseWorld.g),abs(finalPixel.b-baseWorld.b)));"
        "  float interfaceMask=smoothstep(dims.w,dims.w+0.04,difference);"
        "  outputRgb=lerp(outputRgb,pow(saturate(finalPixel.rgb),2.2),interfaceMask);"
        " }"
        " return float4(outputRgb,1); }";
    ID3DBlob *vertex_blob = NULL;
    ID3DBlob *pixel_blob = NULL;
    ID3DBlob *errors = NULL;
    HRESULT hr = D3DCompile(vertex_source, strlen(vertex_source), "SkillshotVRStereoVS", NULL, NULL,
                            "main", "vs_4_0", 0, 0, &vertex_blob, &errors);
    if (FAILED(hr)) {
        xr_log("Stereo vertex shader failed: %s", errors ? (char *)ID3D10Blob_GetBufferPointer(errors) : "unknown");
        if (errors) ID3D10Blob_Release(errors);
        return 0;
    }
    if (errors) { ID3D10Blob_Release(errors); errors = NULL; }
    hr = D3DCompile(pixel_source, strlen(pixel_source), "SkillshotVRStereoPS", NULL, NULL,
                    "main", "ps_4_0", 0, 0, &pixel_blob, &errors);
    if (FAILED(hr)) {
        xr_log("Stereo pixel shader failed: %s", errors ? (char *)ID3D10Blob_GetBufferPointer(errors) : "unknown");
        if (errors) ID3D10Blob_Release(errors);
        ID3D10Blob_Release(vertex_blob);
        return 0;
    }
    if (errors) ID3D10Blob_Release(errors);
    hr = ID3D11Device_CreateVertexShader(g_device, ID3D10Blob_GetBufferPointer(vertex_blob),
                                         ID3D10Blob_GetBufferSize(vertex_blob), NULL,
                                         &g_stereo_vertex_shader);
    if (SUCCEEDED(hr))
        hr = ID3D11Device_CreatePixelShader(g_device, ID3D10Blob_GetBufferPointer(pixel_blob),
                                            ID3D10Blob_GetBufferSize(pixel_blob), NULL,
                                            &g_stereo_pixel_shader);
    ID3D10Blob_Release(vertex_blob);
    ID3D10Blob_Release(pixel_blob);
    if (FAILED(hr)) return 0;
    D3D11_BUFFER_DESC buffer_desc;
    memset(&buffer_desc, 0, sizeof(buffer_desc));
    buffer_desc.ByteWidth = 64;
    buffer_desc.Usage = D3D11_USAGE_DEFAULT;
    buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    if (FAILED(ID3D11Device_CreateBuffer(g_device, &buffer_desc, NULL, &g_stereo_constants))) return 0;
    D3D11_SAMPLER_DESC sampler_desc;
    memset(&sampler_desc, 0, sizeof(sampler_desc));
    sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampler_desc.MaxLOD = D3D11_FLOAT32_MAX;
    if (FAILED(ID3D11Device_CreateSamplerState(g_device, &sampler_desc, &g_stereo_sampler))) return 0;
    D3D11_RASTERIZER_DESC rasterizer_desc;
    memset(&rasterizer_desc, 0, sizeof(rasterizer_desc));
    rasterizer_desc.FillMode = D3D11_FILL_SOLID;
    rasterizer_desc.CullMode = D3D11_CULL_NONE;
    rasterizer_desc.DepthClipEnable = TRUE;
    if (FAILED(ID3D11Device_CreateRasterizerState(g_device, &rasterizer_desc, &g_stereo_rasterizer))) return 0;
    xr_log("Depth stereo compositor initialized");
    return 1;
}

static int initialize_depth_copy_compositor(void)
{
    static const char *vertex_source =
        "struct Out { float4 position : SV_POSITION; float2 uv : TEXCOORD0; };"
        "Out main(uint id : SV_VertexID) {"
        " Out o; float2 p=float2((id<<1)&2,id&2); o.uv=p;"
        " o.position=float4(p*float2(2,-2)+float2(-1,1),0,1); return o; }";
    static const char *pixel_source =
        "cbuffer DepthParams : register(b0) { float4 p; };"
        "Texture2D<float> sourceDepth : register(t0);"
        "float main(float4 position:SV_POSITION,float2 uv:TEXCOORD0):SV_DEPTH {"
        " float sx=uv.x+p.x;"
        " if(sx<0 || sx>=1) return 1.0;"
        " int2 q=int2(clamp(sx,0.0,0.999999)*p.y,"
        "             clamp(1-uv.y,0.0,0.999999)*p.z);"
        " float d=sourceDepth.Load(int3(q,0));"
        " return saturate(d); }";
    ID3DBlob *vertex_blob = NULL, *pixel_blob = NULL, *errors = NULL;
    HRESULT hr = D3DCompile(vertex_source, strlen(vertex_source),
                            "SkillshotVRDepthCopyVS", NULL, NULL, "main", "vs_5_0",
                            D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, &vertex_blob, &errors);
    if (FAILED(hr)) {
        xr_log("Depth copy vertex shader failed: %s",
               errors ? (char *)ID3D10Blob_GetBufferPointer(errors) : "unknown");
        if (errors) ID3D10Blob_Release(errors);
        return 0;
    }
    if (errors) { ID3D10Blob_Release(errors); errors = NULL; }
    hr = D3DCompile(pixel_source, strlen(pixel_source),
                    "SkillshotVRDepthCopyPS", NULL, NULL, "main", "ps_5_0",
                    D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, &pixel_blob, &errors);
    if (FAILED(hr)) {
        xr_log("Depth copy pixel shader failed: %s",
               errors ? (char *)ID3D10Blob_GetBufferPointer(errors) : "unknown");
        if (errors) ID3D10Blob_Release(errors);
        ID3D10Blob_Release(vertex_blob);
        return 0;
    }
    if (errors) ID3D10Blob_Release(errors);
    hr = ID3D11Device_CreateVertexShader(
        g_device, ID3D10Blob_GetBufferPointer(vertex_blob),
        ID3D10Blob_GetBufferSize(vertex_blob), NULL, &g_depth_copy_vertex_shader);
    if (SUCCEEDED(hr))
        hr = ID3D11Device_CreatePixelShader(
            g_device, ID3D10Blob_GetBufferPointer(pixel_blob),
            ID3D10Blob_GetBufferSize(pixel_blob), NULL, &g_depth_copy_pixel_shader);
    ID3D10Blob_Release(vertex_blob);
    ID3D10Blob_Release(pixel_blob);
    if (FAILED(hr)) return 0;

    D3D11_BUFFER_DESC buffer_desc;
    memset(&buffer_desc, 0, sizeof(buffer_desc));
    buffer_desc.ByteWidth = 16;
    buffer_desc.Usage = D3D11_USAGE_DEFAULT;
    buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    if (FAILED(ID3D11Device_CreateBuffer(
            g_device, &buffer_desc, NULL, &g_depth_copy_constants))) return 0;

    D3D11_DEPTH_STENCIL_DESC depth_desc;
    memset(&depth_desc, 0, sizeof(depth_desc));
    depth_desc.DepthEnable = TRUE;
    depth_desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    depth_desc.DepthFunc = D3D11_COMPARISON_ALWAYS;
    if (FAILED(ID3D11Device_CreateDepthStencilState(
            g_device, &depth_desc, &g_depth_copy_state))) return 0;
    xr_log("Per-eye OpenXR depth copy compositor initialized");
    return 1;
}

static int copy_pair_depth_to_swapchain(uint32_t eye, LONG pair_index,
                                         ID3D11DepthStencilView *target)
{
    if (!g_composition_depth_extension_enabled || pair_index < 0 ||
        pair_index >= PAIR_BUFFER_COUNT || !g_present_pair_depth_valid[pair_index] ||
        !g_present_pair_depth_views[pair_index][eye] || !target ||
        !g_depth_copy_vertex_shader || !g_depth_copy_pixel_shader ||
        g_world_width <= 0 || g_world_height <= 0 || g_world_samples != 1) return 0;

    ID3D11DeviceContext_ClearDepthStencilView(
        g_context, target, D3D11_CLEAR_DEPTH, 1.0f, 0);
    float signed_offset = g_eye_alignment * (eye == 0 ? -1.0f : 1.0f);
    float constants[4] = {
        signed_offset, (float)g_world_width, (float)g_world_height,
        (float)g_world_samples
    };
    ID3D11DeviceContext_UpdateSubresource(
        g_context, (ID3D11Resource *)g_depth_copy_constants,
        0, NULL, constants, 0, 0);
    XrRect2Di rect = g_present_pair_rect[pair_index][eye];
    if (rect.extent.width <= 0 || rect.extent.height <= 0) {
        rect.offset.x = rect.offset.y = 0;
        rect.extent.width = g_depth_eyes[eye].width;
        rect.extent.height = g_depth_eyes[eye].height;
    }
    D3D11_VIEWPORT viewport = {
        (float)rect.offset.x, (float)rect.offset.y,
        (float)rect.extent.width, (float)rect.extent.height, 0, 1
    };
    ID3D11ShaderResourceView *view = g_present_pair_depth_views[pair_index][eye];
    ID3D11DeviceContext_OMSetRenderTargets(g_context, 0, NULL, target);
    ID3D11DeviceContext_OMSetDepthStencilState(g_context, g_depth_copy_state, 0);
    ID3D11DeviceContext_RSSetState(g_context, g_stereo_rasterizer);
    ID3D11DeviceContext_RSSetViewports(g_context, 1, &viewport);
    ID3D11DeviceContext_IASetInputLayout(g_context, NULL);
    ID3D11DeviceContext_IASetPrimitiveTopology(
        g_context, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ID3D11DeviceContext_VSSetShader(g_context, g_depth_copy_vertex_shader, NULL, 0);
    ID3D11DeviceContext_PSSetShader(g_context, g_depth_copy_pixel_shader, NULL, 0);
    ID3D11DeviceContext_PSSetConstantBuffers(
        g_context, 0, 1, &g_depth_copy_constants);
    ID3D11DeviceContext_PSSetShaderResources(g_context, 0, 1, &view);
    ID3D11DeviceContext_Draw(g_context, 3, 0);
    ID3D11ShaderResourceView *empty = NULL;
    ID3D11DeviceContext_PSSetShaderResources(g_context, 0, 1, &empty);
    ID3D11DeviceContext_OMSetRenderTargets(g_context, 0, NULL, NULL);
    ID3D11DeviceContext_OMSetDepthStencilState(g_context, NULL, 0);
    if (!g_depth_copy_logged) {
        xr_log("Synchronized game depth copied into OpenXR eye surfaces");
        g_depth_copy_logged = 1;
    }
    return 1;
}

static void update_scene_mode(void)
{
    ULONGLONG now = GetTickCount64();
    if (now < g_scene_check_tick) return;
    g_scene_check_tick = now + 1000;

    FILE *file = fopen(g_game_log_path, "rb");
    if (file) {
        char line[512];
        char latest_map[512] = {0};
        char latest_round_complete[512] = {0};
        /* Only recent transitions matter. Avoid rescanning an ever-growing
           gameplay log on the render thread once per second. */
        if (fseek(file, 0, SEEK_END) == 0) {
            long length = ftell(file);
            if (length > 65536) {
                fseek(file, -65536, SEEK_END);
                fgets(line, sizeof(line), file);
            } else {
                rewind(file);
            }
        }
        while (fgets(line, sizeof(line), file)) {
            if (strstr(line, "Loading Map:")) {
                snprintf(latest_map, sizeof(latest_map), "%s", line);
                latest_map[sizeof(latest_map) - 1] = '\0';
            }
            if (strstr(line, "Saving Log As BattleRoundComplete")) {
                snprintf(latest_round_complete, sizeof(latest_round_complete), "%s", line);
                latest_round_complete[sizeof(latest_round_complete) - 1] = '\0';
            }
        }
        fclose(file);
        if (latest_map[0] && strcmp(latest_map, g_last_map_line) != 0) {
            strncpy(g_last_map_line, latest_map, sizeof(g_last_map_line) - 1);
            g_last_map_line[sizeof(g_last_map_line) - 1] = '\0';
            if (strstr(latest_map, "/backgroundMaps/") ||
                strstr(latest_map, "\\backgroundMaps\\")) {
                g_depth_scene_ready_tick = 0;
                g_depth_scene_active_tick = 0;
                if (!g_flat_scene_mode || !g_scene_mode_announced)
                    xr_log("Scene mode: flat interface (background menu map)");
                g_flat_scene_mode = 1;
                g_scene_mode_announced = 1;
            } else {
                /* Keep loading and round-intro screens flat, then enable depth once
                   the newly loaded gameplay map has had time to become drawable. */
                g_depth_scene_ready_tick = now + 10000;
                g_depth_scene_active_tick = 0;
                if (!g_flat_scene_mode || !g_scene_mode_announced)
                    xr_log("Scene mode: flat interface (gameplay map loading)");
                g_flat_scene_mode = 1;
                g_scene_mode_announced = 1;
            }
        }
        if (latest_round_complete[0] &&
            strcmp(latest_round_complete, g_last_round_complete_line) != 0) {
            strncpy(g_last_round_complete_line, latest_round_complete,
                    sizeof(g_last_round_complete_line) - 1);
            g_last_round_complete_line[sizeof(g_last_round_complete_line) - 1] = '\0';
            /* Survival emits this BattleRoundComplete marker during live-play
               startup (immediately after its local player joins), so it cannot
               identify a results screen. Full-screen interface load already
               switches the submitted headset image to the completed flat
               backbuffer for pause/results, while a later background-map load
               identifies a genuine return to the main menu. */
            xr_log("Ignored ambiguous BattleRoundComplete marker; interface load controls pause/results presentation");
        }
    }
    if (g_flat_scene_mode && g_depth_scene_ready_tick && now >= g_depth_scene_ready_tick) {
        g_depth_scene_ready_tick = 0;
        g_flat_scene_mode = 0;
        g_depth_scene_active_tick = now;
        xr_log("Scene mode: depth stereo (gameplay map ready)");
    }
}

static int render_stereo_frame(uint32_t eye, ID3D11Texture2D *destination)
{
    update_scene_mode();
    if (g_flat_scene_mode) return 0;
    if (!g_stereo_enabled || !g_world_capture_ready || !g_world_color_view ||
        !g_world_depth_view || !g_interop_views[eye]) {
        if (g_stereo_enabled && g_world_capture_ready && !g_stereo_eye_status[eye]) {
            xr_log("Stereo eye %u unavailable: color=%p depth=%p final=%p", eye,
                   (void *)g_world_color_view, (void *)g_world_depth_view,
                   (void *)g_interop_views[eye]);
            g_stereo_eye_status[eye] = -1;
        }
        return 0;
    }
    D3D11_RENDER_TARGET_VIEW_DESC target_desc;
    memset(&target_desc, 0, sizeof(target_desc));
    target_desc.Format = (DXGI_FORMAT)g_swapchain_format;
    target_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
    target_desc.Texture2DArray.ArraySize = 1;
    ID3D11RenderTargetView *target = NULL;
    HRESULT target_hr = ID3D11Device_CreateRenderTargetView(g_device, (ID3D11Resource *)destination,
                                                             &target_desc, &target);
    if (FAILED(target_hr)) {
        if (!g_stereo_eye_status[eye])
            xr_log("Stereo eye %u render-target creation failed: 0x%08lX", eye,
                   (unsigned long)target_hr);
        g_stereo_eye_status[eye] = -1;
        return 0;
    }
    float constants[16] = {
        eye == 0 ? -1.0f : 1.0f, g_stereo_strength, 32.0f, 3797.10156f,
        (float)g_content_x / g_eyes[eye].width, (float)g_content_y / g_eyes[eye].height,
        (float)g_content_width / g_eyes[eye].width, (float)g_content_height / g_eyes[eye].height,
        (float)g_world_width, (float)g_world_height, 2000.0f, 0.10f,
        g_eye_alignment, 0.0f, 0.0f, 0.0f
    };
    ID3D11DeviceContext_UpdateSubresource(g_context, (ID3D11Resource *)g_stereo_constants,
                                           0, NULL, constants, 0, 0);
    D3D11_VIEWPORT viewport = {0, 0, (float)g_eyes[eye].width, (float)g_eyes[eye].height, 0, 1};
    ID3D11ShaderResourceView *views[3] = {g_world_color_view, g_world_depth_view, g_interop_views[eye]};
    ID3D11DeviceContext_OMSetRenderTargets(g_context, 1, &target, NULL);
    ID3D11DeviceContext_OMSetBlendState(g_context, NULL, NULL, 0xFFFFFFFFu);
    ID3D11DeviceContext_RSSetState(g_context, g_stereo_rasterizer);
    ID3D11DeviceContext_RSSetViewports(g_context, 1, &viewport);
    ID3D11DeviceContext_IASetInputLayout(g_context, NULL);
    ID3D11DeviceContext_IASetPrimitiveTopology(g_context, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ID3D11DeviceContext_VSSetShader(g_context, g_stereo_vertex_shader, NULL, 0);
    ID3D11DeviceContext_VSSetConstantBuffers(g_context, 0, 1, &g_stereo_constants);
    ID3D11DeviceContext_PSSetShader(g_context, g_stereo_pixel_shader, NULL, 0);
    ID3D11DeviceContext_PSSetConstantBuffers(g_context, 0, 1, &g_stereo_constants);
    ID3D11DeviceContext_PSSetShaderResources(g_context, 0, 3, views);
    ID3D11DeviceContext_PSSetSamplers(g_context, 0, 1, &g_stereo_sampler);
    ID3D11DeviceContext_Draw(g_context, 3, 0);
    ID3D11ShaderResourceView *empty[3] = {NULL, NULL, NULL};
    ID3D11DeviceContext_PSSetShaderResources(g_context, 0, 3, empty);
    ID3D11DeviceContext_OMSetRenderTargets(g_context, 0, NULL, NULL);
    ID3D11RenderTargetView_Release(target);
    if (g_stereo_eye_status[eye] != 1) {
        xr_log("Stereo eye %u rendered successfully", eye);
        g_stereo_eye_status[eye] = 1;
    }
    if (!g_stereo_logged) {
        xr_log("Live depth calibration active: strength=%.4f", g_stereo_strength);
        g_stereo_logged = 1;
    }
    return 1;
}

static int initialize_geometry_depth_storage(const D3D11_TEXTURE2D_DESC *source_desc)
{
    if (g_geometry_eye_depth_textures[0]) return 1;
    D3D11_TEXTURE2D_DESC desc = *source_desc;
    desc.Format = DXGI_FORMAT_R24G8_TYPELESS;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;

    D3D11_SHADER_RESOURCE_VIEW_DESC view_desc;
    memset(&view_desc, 0, sizeof(view_desc));
    view_desc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
    view_desc.ViewDimension = desc.SampleDesc.Count > 1
        ? D3D11_SRV_DIMENSION_TEXTURE2DMS : D3D11_SRV_DIMENSION_TEXTURE2D;
    if (desc.SampleDesc.Count == 1) view_desc.Texture2D.MipLevels = 1;

    for (int eye = 0; eye < EYE_COUNT; ++eye) {
        HRESULT hr = ID3D11Device_CreateTexture2D(
            g_device, &desc, NULL, &g_geometry_eye_depth_textures[eye]);
        if (FAILED(hr)) {
            xr_log("Geometry eye depth texture %d creation failed: 0x%08lX",
                   eye, (unsigned long)hr);
            return 0;
        }
        hr = ID3D11Device_CreateShaderResourceView(
            g_device, (ID3D11Resource *)g_geometry_eye_depth_textures[eye],
            &view_desc, &g_geometry_eye_depth_views[eye]);
        if (FAILED(hr)) {
            xr_log("Geometry eye depth SRV %d creation failed: 0x%08lX",
                   eye, (unsigned long)hr);
            return 0;
        }
    }
    for (int pair = 0; pair < PAIR_BUFFER_COUNT; ++pair) {
        for (int eye = 0; eye < EYE_COUNT; ++eye) {
            HRESULT hr = ID3D11Device_CreateTexture2D(
                g_device, &desc, NULL, &g_present_pair_depth_textures[pair][eye]);
            if (FAILED(hr)) {
                xr_log("Pair depth texture %d/%d creation failed: 0x%08lX",
                       pair, eye, (unsigned long)hr);
                return 0;
            }
            hr = ID3D11Device_CreateShaderResourceView(
                g_device, (ID3D11Resource *)g_present_pair_depth_textures[pair][eye],
                &view_desc, &g_present_pair_depth_views[pair][eye]);
            if (FAILED(hr)) {
                xr_log("Pair depth SRV %d/%d creation failed: 0x%08lX",
                       pair, eye, (unsigned long)hr);
                return 0;
            }
        }
    }
    xr_log("Synchronized per-eye depth storage allocated at %ux%u@%ux",
           desc.Width, desc.Height, desc.SampleDesc.Count);
    return 1;
}

static int initialize_world_capture(int width, int height, int samples)
{
    if (!g_gl_interop_ready || width <= 0 || height <= 0 || samples <= 0) return 0;
    if (g_world_color_texture && g_world_width == width && g_world_height == height &&
        g_world_samples == samples) return 1;
    if (g_world_color_texture) {
        xr_log("World capture changed from %dx%d@%dx to %dx%d@%dx; restart required",
               g_world_width, g_world_height, g_world_samples, width, height, samples);
        return 0;
    }

    D3D11_TEXTURE2D_DESC color_desc;
    memset(&color_desc, 0, sizeof(color_desc));
    color_desc.Width = (UINT)width;
    color_desc.Height = (UINT)height;
    color_desc.MipLevels = 1;
    color_desc.ArraySize = 1;
    color_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    color_desc.SampleDesc.Count = 1;
    color_desc.Usage = D3D11_USAGE_DEFAULT;
    color_desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    color_desc.MiscFlags = D3D11_RESOURCE_MISC_SHARED;
    HRESULT hr = ID3D11Device_CreateTexture2D(g_device, &color_desc, NULL, &g_world_color_texture);
    if (FAILED(hr)) {
        xr_log("World color texture creation failed: 0x%08lX", (unsigned long)hr);
        return 0;
    }

    D3D11_TEXTURE2D_DESC depth_desc = color_desc;
    depth_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depth_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    depth_desc.SampleDesc.Count = (UINT)samples;
    hr = ID3D11Device_CreateTexture2D(g_device, &depth_desc, NULL, &g_world_depth_texture);
    if (FAILED(hr)) {
        xr_log("World depth texture creation failed: 0x%08lX", (unsigned long)hr);
        return 0;
    }
    if (!initialize_geometry_depth_storage(&depth_desc)) return 0;

    hr = ID3D11Device_CreateShaderResourceView(g_device, (ID3D11Resource *)g_world_color_texture,
                                                NULL, &g_world_color_view);
    if (FAILED(hr)) {
        xr_log("World color SRV creation failed: 0x%08lX", (unsigned long)hr);
        return 0;
    }
    D3D11_TEXTURE2D_DESC shader_depth_desc = depth_desc;
    shader_depth_desc.Format = DXGI_FORMAT_R24G8_TYPELESS;
    shader_depth_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    shader_depth_desc.MiscFlags = 0;
    hr = ID3D11Device_CreateTexture2D(g_device, &shader_depth_desc, NULL,
                                      &g_world_depth_shader_texture);
    if (FAILED(hr)) {
        xr_log("Shader-readable depth texture creation failed: 0x%08lX", (unsigned long)hr);
        return 0;
    }
    D3D11_SHADER_RESOURCE_VIEW_DESC depth_view_desc;
    memset(&depth_view_desc, 0, sizeof(depth_view_desc));
    depth_view_desc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
    depth_view_desc.ViewDimension = samples > 1
        ? D3D11_SRV_DIMENSION_TEXTURE2DMS : D3D11_SRV_DIMENSION_TEXTURE2D;
    if (samples == 1) depth_view_desc.Texture2D.MipLevels = 1;
    hr = ID3D11Device_CreateShaderResourceView(g_device,
                                                (ID3D11Resource *)g_world_depth_shader_texture,
                                                &depth_view_desc, &g_world_depth_view);
    if (FAILED(hr)) {
        xr_log("World depth SRV creation failed: 0x%08lX", (unsigned long)hr);
        return 0;
    }

    p_glGenTextures(2, g_world_gl_textures);
    p_glGenFramebuffers(1, &g_world_color_framebuffer);
    p_glGenFramebuffers(1, &g_world_depth_framebuffer);
    g_world_interop_objects[0] = p_wglDXRegisterObjectNV(g_interop_device, g_world_color_texture,
                                                         g_world_gl_textures[0], GL_TEXTURE_2D,
                                                         WGL_ACCESS_WRITE_DISCARD_NV);
    g_world_interop_objects[1] = p_wglDXRegisterObjectNV(g_interop_device, g_world_depth_texture,
                                                         g_world_gl_textures[1], samples > 1
                                                             ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D,
                                                         WGL_ACCESS_WRITE_DISCARD_NV);
    if (!g_world_interop_objects[0] || !g_world_interop_objects[1]) {
        xr_log("World color/depth WGL registration failed: %lu", GetLastError());
        return 0;
    }
    g_world_width = width;
    g_world_height = height;
    g_world_samples = samples;
    xr_log("World color/depth interop allocated at %dx%d, depth samples=%d", width, height, samples);
    return 1;
}

static void validate_world_depth_once(void)
{
    if (g_world_depth_validated || !g_world_depth_texture) return;
    g_world_depth_validated = 1;
    D3D11_TEXTURE2D_DESC description;
    ID3D11Texture2D_GetDesc(g_world_depth_texture, &description);
    if (description.SampleDesc.Count > 1) {
        xr_log("Shared world depth is multisampled (%ux); numerical validation deferred to resolve shader",
               description.SampleDesc.Count);
        return;
    }
    description.Usage = D3D11_USAGE_STAGING;
    description.BindFlags = 0;
    description.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    description.MiscFlags = 0;
    ID3D11Texture2D *staging = NULL;
    HRESULT hr = ID3D11Device_CreateTexture2D(g_device, &description, NULL, &staging);
    if (FAILED(hr)) {
        xr_log("World depth validation staging creation failed: 0x%08lX", (unsigned long)hr);
        return;
    }
    ID3D11DeviceContext_CopyResource(g_context, (ID3D11Resource *)staging,
                                     (ID3D11Resource *)g_world_depth_texture);
    D3D11_MAPPED_SUBRESOURCE mapped;
    memset(&mapped, 0, sizeof(mapped));
    hr = ID3D11DeviceContext_Map(g_context, (ID3D11Resource *)staging, 0,
                                 D3D11_MAP_READ, 0, &mapped);
    if (SUCCEEDED(hr)) {
        float minimum = 1.0f;
        float maximum = 0.0f;
        size_t non_far = 0;
        for (int y = 0; y < g_world_height; ++y) {
            const uint32_t *row = (const uint32_t *)((const unsigned char *)mapped.pData +
                                                      (size_t)y * mapped.RowPitch);
            for (int x = 0; x < g_world_width; ++x) {
                float value = (float)(row[x] & 0x00FFFFFFu) / 16777215.0f;
                if (value < minimum) minimum = value;
                if (value > maximum) maximum = value;
                if (value < 0.999999f) ++non_far;
            }
        }
        ID3D11DeviceContext_Unmap(g_context, (ID3D11Resource *)staging, 0);
        xr_log("Shared world depth validated: min=%.9g max=%.9g nonFar=%llu/%llu",
               minimum, maximum, (unsigned long long)non_far,
               (unsigned long long)((size_t)g_world_width * g_world_height));
    } else {
        xr_log("World depth validation map failed: 0x%08lX", (unsigned long)hr);
    }
    ID3D11Texture2D_Release(staging);
}

int openxr_bridge_capture_world(int source_x0, int source_y0, int source_x1, int source_y1,
                                int eye)
{
    /* Native two-eye geometry also needs the game's world depth even when the
       retired screen-space stereo test compositor is disabled. */
    if (!g_stereo_enabled &&
        (!g_geometry_stereo_requested || !g_composition_depth_extension_enabled)) return 0;
    int width = source_x1 - source_x0;
    int height = source_y1 - source_y0;
    GLint previous_draw = 0;
    GLint previous_read = 0;
    GLint source_samples = 0;
    p_glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &previous_draw);
    p_glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &previous_read);
    p_glBindFramebuffer(GL_DRAW_FRAMEBUFFER, (GLuint)previous_read);
    p_glGetIntegerv(GL_SAMPLES, &source_samples);
    p_glBindFramebuffer(GL_DRAW_FRAMEBUFFER, (GLuint)previous_draw);
    /* GL reports zero samples for an ordinary single-sample default
       framebuffer. D3D texture descriptors express that same layout as a
       sample count of one. */
    if (source_samples < 1) source_samples = 1;
    if (!initialize_world_capture(width, height, source_samples)) return 0;
    HANDLE objects[2] = {g_world_interop_objects[0], g_world_interop_objects[1]};
    int capture_color = g_stereo_enabled || eye < 0;
    HANDLE *locked_objects = capture_color ? objects : &objects[1];
    GLint locked_object_count = capture_color ? 2 : 1;
    if (!p_wglDXLockObjectsNV(g_interop_device, locked_object_count, locked_objects)) {
        if (!g_world_capture_logged) xr_log("World capture lock failed: %lu", GetLastError());
        return 0;
    }
    GLint color_target_samples = source_samples;
    GLint depth_target_samples = 0;
    GLenum color_status = GL_FRAMEBUFFER_COMPLETE;
    GLenum color_error = GL_NO_ERROR;
    if (capture_color) {
        p_glBindFramebuffer(GL_DRAW_FRAMEBUFFER, g_world_color_framebuffer);
        p_glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                                 g_world_gl_textures[0], 0);
        color_status = p_glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);
        p_glGetIntegerv(GL_SAMPLES, &color_target_samples);
        if (p_glGetError) while (p_glGetError() != GL_NO_ERROR) {}
        if (color_status == GL_FRAMEBUFFER_COMPLETE) {
            p_glBlitFramebuffer(source_x0, source_y0, source_x1, source_y1,
                                0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
            color_error = p_glGetError ? p_glGetError() : GL_NO_ERROR;
        }
    }

    p_glBindFramebuffer(GL_DRAW_FRAMEBUFFER, g_world_depth_framebuffer);
    if (p_glDrawBuffer) p_glDrawBuffer(GL_NONE);
    p_glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                             source_samples > 1 ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D,
                             g_world_gl_textures[1], 0);
    GLenum depth_status = p_glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);
    p_glGetIntegerv(GL_SAMPLES, &depth_target_samples);
    if (p_glGetError) while (p_glGetError() != GL_NO_ERROR) {}
    GLenum depth_error = GL_NO_ERROR;
    if (depth_status == GL_FRAMEBUFFER_COMPLETE) {
        p_glBlitFramebuffer(source_x0, source_y0, source_x1, source_y1,
                            0, 0, width, height,
                            GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT, GL_NEAREST);
        depth_error = p_glGetError ? p_glGetError() : GL_NO_ERROR;
    }
    g_world_capture_ready = color_status == GL_FRAMEBUFFER_COMPLETE &&
                            depth_status == GL_FRAMEBUFFER_COMPLETE &&
                            color_error == GL_NO_ERROR && depth_error == GL_NO_ERROR;
    /* NVIDIA's WGL/D3D interop unlock is not a sufficient GL completion
       barrier for this depth blit. Returning ownership while it is queued
       eventually poisons later eye locks, so finish before detaching. */
    p_glFinish();
    p_glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                             GL_TEXTURE_2D, 0, 0);
    if (capture_color) {
        p_glBindFramebuffer(GL_DRAW_FRAMEBUFFER, g_world_color_framebuffer);
        p_glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                 GL_TEXTURE_2D, 0, 0);
    }
    p_glBindFramebuffer(GL_DRAW_FRAMEBUFFER, (GLuint)previous_draw);
    p_wglDXUnlockObjectsNV(g_interop_device, locked_object_count, locked_objects);
    /* The extra shader-readable copy only feeds the retired screen-space
       stereo compositor. Native geometry sends depth straight to the current
       eye, so copying the full buffer here was pure bandwidth and a possible
       cross-API synchronization point on every eye. */
    if (g_stereo_enabled)
        ID3D11DeviceContext_CopyResource(
            g_context, (ID3D11Resource *)g_world_depth_shader_texture,
            (ID3D11Resource *)g_world_depth_texture);
    if (g_world_capture_ready && eye >= 0 && eye < EYE_COUNT &&
        g_geometry_eye_depth_textures[eye]) {
        ID3D11DeviceContext_CopyResource(
            g_context, (ID3D11Resource *)g_geometry_eye_depth_textures[eye],
            (ID3D11Resource *)g_world_depth_texture);
        g_geometry_eye_depth_ready[eye] = 1;
    }
    validate_world_depth_once();
    if (!g_world_capture_logged) {
        xr_log("World capture %s: color=%s(fbo=0x%X err=0x%X %d->%d) depth(fbo=0x%X err=0x%X %d->%d)",
               g_world_capture_ready ? "active" : "failed",
               capture_color ? "copied " : "skipped ",
               color_status, color_error, source_samples, color_target_samples,
               depth_status, depth_error, source_samples, depth_target_samples);
        g_world_capture_logged = 1;
    }
    return g_world_capture_ready;
}

static int initialize_vr_cursor(void)
{
    static const char *vertex_source =
        "cbuffer CursorRect : register(b0) { float4 rect; };"
        "struct Out { float4 position : SV_POSITION; float2 uv : TEXCOORD0; };"
        "Out main(uint id : SV_VertexID) {"
        "  Out o; float2 uv=float2(id & 1, (id >> 1) & 1);"
        "  o.position=float4(lerp(rect.xy, rect.zw, uv), 0, 1); o.uv=uv; return o; }";
    static const char *pixel_source =
        "Texture2D cursorTexture : register(t0); SamplerState cursorSampler : register(s0);"
        "float4 main(float4 position : SV_POSITION, float2 uv : TEXCOORD0) : SV_TARGET {"
        "  float4 c=cursorTexture.Sample(cursorSampler, uv);"
        "  return float4(pow(saturate(c.rgb),2.2),c.a); }";
    ID3DBlob *vertex_blob = NULL;
    ID3DBlob *pixel_blob = NULL;
    ID3DBlob *errors = NULL;
    HRESULT hr = D3DCompile(vertex_source, strlen(vertex_source), "SkillshotVRCursorVS", NULL, NULL,
                            "main", "vs_4_0", 0, 0, &vertex_blob, &errors);
    if (FAILED(hr)) {
        xr_log("VR cursor vertex shader failed: %s", errors ? (char *)ID3D10Blob_GetBufferPointer(errors) : "unknown");
        if (errors) ID3D10Blob_Release(errors);
        return 0;
    }
    if (errors) { ID3D10Blob_Release(errors); errors = NULL; }
    hr = D3DCompile(pixel_source, strlen(pixel_source), "SkillshotVRCursorPS", NULL, NULL,
                    "main", "ps_4_0", 0, 0, &pixel_blob, &errors);
    if (FAILED(hr)) {
        xr_log("VR cursor pixel shader failed: %s", errors ? (char *)ID3D10Blob_GetBufferPointer(errors) : "unknown");
        if (errors) ID3D10Blob_Release(errors);
        ID3D10Blob_Release(vertex_blob);
        return 0;
    }
    if (errors) ID3D10Blob_Release(errors);
    hr = ID3D11Device_CreateVertexShader(g_device, ID3D10Blob_GetBufferPointer(vertex_blob),
                                         ID3D10Blob_GetBufferSize(vertex_blob), NULL,
                                         &g_cursor_vertex_shader);
    if (SUCCEEDED(hr))
        hr = ID3D11Device_CreatePixelShader(g_device, ID3D10Blob_GetBufferPointer(pixel_blob),
                                            ID3D10Blob_GetBufferSize(pixel_blob), NULL,
                                            &g_cursor_pixel_shader);
    ID3D10Blob_Release(vertex_blob);
    ID3D10Blob_Release(pixel_blob);
    if (FAILED(hr)) return 0;

    D3D11_BUFFER_DESC buffer_desc;
    memset(&buffer_desc, 0, sizeof(buffer_desc));
    buffer_desc.ByteWidth = 16;
    buffer_desc.Usage = D3D11_USAGE_DEFAULT;
    buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    if (FAILED(ID3D11Device_CreateBuffer(g_device, &buffer_desc, NULL, &g_cursor_constants))) return 0;

    D3D11_SAMPLER_DESC sampler_desc;
    memset(&sampler_desc, 0, sizeof(sampler_desc));
    sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampler_desc.MaxLOD = D3D11_FLOAT32_MAX;
    if (FAILED(ID3D11Device_CreateSamplerState(g_device, &sampler_desc, &g_cursor_sampler))) return 0;

    D3D11_BLEND_DESC blend_desc;
    memset(&blend_desc, 0, sizeof(blend_desc));
    blend_desc.RenderTarget[0].BlendEnable = TRUE;
    blend_desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blend_desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blend_desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blend_desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blend_desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
    blend_desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blend_desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    if (FAILED(ID3D11Device_CreateBlendState(g_device, &blend_desc, &g_cursor_blend))) return 0;

    D3D11_RASTERIZER_DESC rasterizer_desc;
    memset(&rasterizer_desc, 0, sizeof(rasterizer_desc));
    rasterizer_desc.FillMode = D3D11_FILL_SOLID;
    rasterizer_desc.CullMode = D3D11_CULL_NONE;
    rasterizer_desc.DepthClipEnable = TRUE;
    if (FAILED(ID3D11Device_CreateRasterizerState(g_device, &rasterizer_desc, &g_cursor_rasterizer))) return 0;
    xr_log("Native cursor compositor initialized");
    return 1;
}

static void destroy_cursor_bitmap(HBITMAP bitmap)
{
    if (bitmap) DeleteObject(bitmap);
}

static int capture_native_cursor(HCURSOR cursor)
{
    ICONINFO icon;
    memset(&icon, 0, sizeof(icon));
    if (!GetIconInfo(cursor, &icon)) return 0;
    BITMAP bitmap;
    memset(&bitmap, 0, sizeof(bitmap));
    HBITMAP dimension_bitmap = icon.hbmColor ? icon.hbmColor : icon.hbmMask;
    if (!dimension_bitmap || !GetObject(dimension_bitmap, sizeof(bitmap), &bitmap)) {
        destroy_cursor_bitmap(icon.hbmColor);
        destroy_cursor_bitmap(icon.hbmMask);
        return 0;
    }
    UINT width = (UINT)bitmap.bmWidth;
    UINT height = (UINT)(icon.hbmColor ? bitmap.bmHeight : bitmap.bmHeight / 2);
    if (!width || !height || width > 256 || height > 256) {
        destroy_cursor_bitmap(icon.hbmColor);
        destroy_cursor_bitmap(icon.hbmMask);
        return 0;
    }

    BITMAPINFO info;
    memset(&info, 0, sizeof(info));
    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = (LONG)width;
    info.bmiHeader.biHeight = -(LONG)height;
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;
    void *black_pixels = NULL;
    void *white_pixels = NULL;
    HDC screen = GetDC(NULL);
    HDC black_dc = CreateCompatibleDC(screen);
    HDC white_dc = CreateCompatibleDC(screen);
    HBITMAP black_bitmap = CreateDIBSection(screen, &info, DIB_RGB_COLORS, &black_pixels, NULL, 0);
    HBITMAP white_bitmap = CreateDIBSection(screen, &info, DIB_RGB_COLORS, &white_pixels, NULL, 0);
    HGDIOBJ old_black = black_bitmap ? SelectObject(black_dc, black_bitmap) : NULL;
    HGDIOBJ old_white = white_bitmap ? SelectObject(white_dc, white_bitmap) : NULL;
    int ok = black_bitmap && white_bitmap && black_pixels && white_pixels;
    if (ok) {
        PatBlt(black_dc, 0, 0, (int)width, (int)height, BLACKNESS);
        PatBlt(white_dc, 0, 0, (int)width, (int)height, WHITENESS);
        ok = DrawIconEx(black_dc, 0, 0, cursor, (int)width, (int)height, 0, NULL, DI_NORMAL) &&
             DrawIconEx(white_dc, 0, 0, cursor, (int)width, (int)height, 0, NULL, DI_NORMAL);
    }
    unsigned char *rgba = ok ? (unsigned char *)malloc((size_t)width * height * 4) : NULL;
    if (ok && rgba) {
        const unsigned char *black = (const unsigned char *)black_pixels;
        const unsigned char *white = (const unsigned char *)white_pixels;
        UINT visible_pixels = 0;
        int maximum_alpha = 0;
        for (UINT index = 0; index < width * height; ++index) {
            int difference_b = (int)white[index * 4] - (int)black[index * 4];
            int difference_g = (int)white[index * 4 + 1] - (int)black[index * 4 + 1];
            int difference_r = (int)white[index * 4 + 2] - (int)black[index * 4 + 2];
            int transparent = difference_r;
            if (difference_g > transparent) transparent = difference_g;
            if (difference_b > transparent) transparent = difference_b;
            if (transparent < 0) transparent = 0;
            if (transparent > 255) transparent = 255;
            int alpha = 255 - transparent;
            rgba[index * 4] = alpha ? (unsigned char)((black[index * 4 + 2] * 255 + alpha / 2) / alpha) : 0;
            rgba[index * 4 + 1] = alpha ? (unsigned char)((black[index * 4 + 1] * 255 + alpha / 2) / alpha) : 0;
            rgba[index * 4 + 2] = alpha ? (unsigned char)((black[index * 4] * 255 + alpha / 2) / alpha) : 0;
            rgba[index * 4 + 3] = (unsigned char)alpha;
            if (alpha) ++visible_pixels;
            if (alpha > maximum_alpha) maximum_alpha = alpha;
        }
        D3D11_TEXTURE2D_DESC texture_desc;
        memset(&texture_desc, 0, sizeof(texture_desc));
        texture_desc.Width = width;
        texture_desc.Height = height;
        texture_desc.MipLevels = 1;
        texture_desc.ArraySize = 1;
        texture_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        texture_desc.SampleDesc.Count = 1;
        texture_desc.Usage = D3D11_USAGE_DEFAULT;
        texture_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        D3D11_SUBRESOURCE_DATA initial = {rgba, width * 4, 0};
        ID3D11Texture2D *texture = NULL;
        ID3D11ShaderResourceView *view = NULL;
        ok = SUCCEEDED(ID3D11Device_CreateTexture2D(g_device, &texture_desc, &initial, &texture));
        if (ok) ok = SUCCEEDED(ID3D11Device_CreateShaderResourceView(g_device, (ID3D11Resource *)texture, NULL, &view));
        if (ok) {
            if (g_cursor_view) ID3D11ShaderResourceView_Release(g_cursor_view);
            if (g_cursor_texture) ID3D11Texture2D_Release(g_cursor_texture);
            g_cursor_texture = texture;
            g_cursor_view = view;
            g_cursor_handle = cursor;
            g_cursor_width = width;
            g_cursor_height = height;
            g_cursor_hotspot_x = icon.xHotspot;
            g_cursor_hotspot_y = icon.yHotspot;
            xr_log("Captured native cursor %ux%u hotspot=%u,%u visible=%u maxAlpha=%d",
                   width, height, g_cursor_hotspot_x, g_cursor_hotspot_y,
                   visible_pixels, maximum_alpha);
        } else {
            if (view) ID3D11ShaderResourceView_Release(view);
            if (texture) ID3D11Texture2D_Release(texture);
        }
    } else ok = 0;
    free(rgba);
    if (old_black) SelectObject(black_dc, old_black);
    if (old_white) SelectObject(white_dc, old_white);
    if (black_bitmap) DeleteObject(black_bitmap);
    if (white_bitmap) DeleteObject(white_bitmap);
    if (black_dc) DeleteDC(black_dc);
    if (white_dc) DeleteDC(white_dc);
    if (screen) ReleaseDC(NULL, screen);
    destroy_cursor_bitmap(icon.hbmColor);
    destroy_cursor_bitmap(icon.hbmMask);
    return ok;
}

static void capture_composite_proof(ID3D11Texture2D *source, const char *marker,
                                    const char *path, int *written, const char *label)
{
    if (GetFileAttributesA(marker) == INVALID_FILE_ATTRIBUTES) {
        *written = 0;
        return;
    }
    if (*written) return;
    *written = 1;
    D3D11_TEXTURE2D_DESC description;
    ID3D11Texture2D_GetDesc(source, &description);
    description.Usage = D3D11_USAGE_STAGING;
    description.BindFlags = 0;
    description.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    description.MiscFlags = 0;
    ID3D11Texture2D *staging = NULL;
    if (FAILED(ID3D11Device_CreateTexture2D(g_device, &description, NULL, &staging))) return;
    ID3D11DeviceContext_CopyResource(g_context, (ID3D11Resource *)staging, (ID3D11Resource *)source);
    ID3D11DeviceContext_Flush(g_context);
    D3D11_MAPPED_SUBRESOURCE mapped;
    memset(&mapped, 0, sizeof(mapped));
    if (FAILED(ID3D11DeviceContext_Map(g_context, (ID3D11Resource *)staging, 0,
                                       D3D11_MAP_READ, 0, &mapped))) {
        ID3D11Texture2D_Release(staging);
        return;
    }
    int alpha_channel = strstr(label, "alpha channel") != NULL;
    FILE *file = fopen(path, "wb");
    if (file) {
        BITMAPFILEHEADER file_header;
        BITMAPINFOHEADER info_header;
        memset(&file_header, 0, sizeof(file_header));
        memset(&info_header, 0, sizeof(info_header));
        UINT row_bytes = description.Width * 4;
        file_header.bfType = 0x4D42;
        file_header.bfOffBits = sizeof(file_header) + sizeof(info_header);
        file_header.bfSize = file_header.bfOffBits + row_bytes * description.Height;
        info_header.biSize = sizeof(info_header);
        info_header.biWidth = (LONG)description.Width;
        info_header.biHeight = (LONG)description.Height;
        info_header.biPlanes = 1;
        info_header.biBitCount = 32;
        info_header.biCompression = BI_RGB;
        fwrite(&file_header, sizeof(file_header), 1, file);
        fwrite(&info_header, sizeof(info_header), 1, file);
        unsigned char *row = (unsigned char *)malloc(row_bytes);
        if (row) {
            for (int y = (int)description.Height - 1; y >= 0; --y) {
                const unsigned char *source_row = (const unsigned char *)mapped.pData + (size_t)y * mapped.RowPitch;
                for (UINT x = 0; x < description.Width; ++x) {
                    if (alpha_channel) {
                        unsigned char alpha = source_row[x * 4 + 3];
                        row[x * 4] = alpha;
                        row[x * 4 + 1] = alpha;
                        row[x * 4 + 2] = alpha;
                    } else {
                        row[x * 4] = source_row[x * 4 + 2];
                        row[x * 4 + 1] = source_row[x * 4 + 1];
                        row[x * 4 + 2] = source_row[x * 4];
                    }
                    row[x * 4 + 3] = 255;
                }
                fwrite(row, row_bytes, 1, file);
            }
            free(row);
        }
        fclose(file);
        xr_log("Captured %s proof to %s", label, path);
    }
    ID3D11DeviceContext_Unmap(g_context, (ID3D11Resource *)staging, 0);
    ID3D11Texture2D_Release(staging);
}

typedef struct StereoDiagnosticBitmap {
    int width;
    int height;
    unsigned char *bgra;
} StereoDiagnosticBitmap;

static void free_stereo_diagnostic_bitmap(StereoDiagnosticBitmap *bitmap)
{
    if (!bitmap) return;
    free(bitmap->bgra);
    memset(bitmap, 0, sizeof(*bitmap));
}

static int load_stereo_diagnostic_bitmap(const char *path,
                                         StereoDiagnosticBitmap *bitmap)
{
    memset(bitmap, 0, sizeof(*bitmap));
    FILE *file = fopen(path, "rb");
    if (!file) return 0;
    BITMAPFILEHEADER file_header;
    BITMAPINFOHEADER info_header;
    int ok = fread(&file_header, sizeof(file_header), 1, file) == 1 &&
             fread(&info_header, sizeof(info_header), 1, file) == 1 &&
             file_header.bfType == 0x4D42 && info_header.biBitCount == 32 &&
             info_header.biCompression == BI_RGB && info_header.biWidth > 0 &&
             info_header.biHeight != 0;
    int height = info_header.biHeight < 0 ? -info_header.biHeight : info_header.biHeight;
    size_t bytes = ok ? (size_t)info_header.biWidth * (size_t)height * 4u : 0;
    unsigned char *pixels = bytes ? (unsigned char *)malloc(bytes) : NULL;
    if (!pixels || fseek(file, (long)file_header.bfOffBits, SEEK_SET) != 0) ok = 0;
    if (ok) {
        size_t row_bytes = (size_t)info_header.biWidth * 4u;
        for (int y = 0; y < height; ++y) {
            int source_y = info_header.biHeight > 0 ? height - 1 - y : y;
            if (fseek(file, (long)file_header.bfOffBits +
                            (long)((size_t)source_y * row_bytes), SEEK_SET) != 0 ||
                fread(pixels + (size_t)y * row_bytes, row_bytes, 1, file) != 1) {
                ok = 0;
                break;
            }
        }
    }
    fclose(file);
    if (!ok) { free(pixels); return 0; }
    bitmap->width = info_header.biWidth;
    bitmap->height = height;
    bitmap->bgra = pixels;
    return 1;
}

static int write_stereo_diagnostic_bitmap(const char *path, int width, int height,
                                           const unsigned char *top_down_bgra)
{
    if (!path || width <= 0 || height <= 0 || !top_down_bgra) return 0;
    FILE *file = fopen(path, "wb");
    if (!file) return 0;
    size_t row_bytes = (size_t)width * 4u;
    BITMAPFILEHEADER file_header;
    BITMAPINFOHEADER info_header;
    memset(&file_header, 0, sizeof(file_header));
    memset(&info_header, 0, sizeof(info_header));
    file_header.bfType = 0x4D42;
    file_header.bfOffBits = sizeof(file_header) + sizeof(info_header);
    file_header.bfSize = (DWORD)(file_header.bfOffBits + row_bytes * (size_t)height);
    info_header.biSize = sizeof(info_header);
    info_header.biWidth = width;
    info_header.biHeight = height;
    info_header.biPlanes = 1;
    info_header.biBitCount = 32;
    info_header.biCompression = BI_RGB;
    int ok = fwrite(&file_header, sizeof(file_header), 1, file) == 1 &&
             fwrite(&info_header, sizeof(info_header), 1, file) == 1;
    for (int y = height - 1; ok && y >= 0; --y)
        ok = fwrite(top_down_bgra + (size_t)y * row_bytes, row_bytes, 1, file) == 1;
    fclose(file);
    return ok;
}

static unsigned char clamp_byte(int value)
{
    return (unsigned char)(value < 0 ? 0 : value > 255 ? 255 : value);
}

static DWORD WINAPI stereo_diagnostic_worker(void *unused)
{
    (void)unused;
    StereoDiagnosticBitmap left = {0}, right = {0};
    if (!load_stereo_diagnostic_bitmap(
            g_geometry_final_interface_proof_path[0], &left) ||
        !load_stereo_diagnostic_bitmap(
            g_geometry_final_interface_proof_path[1], &right) ||
        left.width != right.width || left.height != right.height) {
        free_stereo_diagnostic_bitmap(&left);
        free_stereo_diagnostic_bitmap(&right);
        xr_log("Stereo diagnostic generation failed: final eye BMPs unavailable or mismatched");
        InterlockedExchange(&g_stereo_diagnostic_started, 0);
        return 0;
    }
    int width = left.width, height = left.height;
    size_t eye_bytes = (size_t)width * (size_t)height * 4u;
    size_t pair_bytes = eye_bytes * 2u;
    unsigned char *side = (unsigned char *)calloc(1, pair_bytes);
    unsigned char *overlay = (unsigned char *)calloc(1, eye_bytes);
    unsigned char *anaglyph = (unsigned char *)calloc(1, eye_bytes);
    unsigned char *difference = (unsigned char *)calloc(1, eye_bytes);
    unsigned char *lens = (unsigned char *)calloc(1, pair_bytes);
    if (!side || !overlay || !anaglyph || !difference || !lens) {
        free(side); free(overlay); free(anaglyph); free(difference); free(lens);
        free_stereo_diagnostic_bitmap(&left);
        free_stereo_diagnostic_bitmap(&right);
        xr_log("Stereo diagnostic generation failed: out of memory");
        InterlockedExchange(&g_stereo_diagnostic_started, 0);
        return 0;
    }
    unsigned long long total_difference = 0, equal_pixels = 0;
    unsigned int maximum_difference = 0;
    for (int y = 0; y < height; ++y) {
        unsigned char *side_row = side + (size_t)y * (size_t)width * 8u;
        memcpy(side_row, left.bgra + (size_t)y * (size_t)width * 4u,
               (size_t)width * 4u);
        memcpy(side_row + (size_t)width * 4u,
               right.bgra + (size_t)y * (size_t)width * 4u,
               (size_t)width * 4u);
    }
    for (size_t pixel = 0; pixel < (size_t)width * (size_t)height; ++pixel) {
        size_t at = pixel * 4u;
        int sum = 0;
        for (int channel = 0; channel < 3; ++channel) {
            int a = left.bgra[at + channel], b = right.bgra[at + channel];
            int delta = abs(a - b);
            sum += delta;
            overlay[at + channel] = (unsigned char)((a + b + 1) / 2);
            difference[at + channel] = clamp_byte(delta * 4);
        }
        overlay[at + 3] = anaglyph[at + 3] = difference[at + 3] = 255;
        anaglyph[at + 2] = left.bgra[at + 2];
        anaglyph[at + 1] = right.bgra[at + 1];
        anaglyph[at] = right.bgra[at];
        total_difference += (unsigned long long)sum;
        if ((unsigned int)sum > maximum_difference) maximum_difference = (unsigned int)sum;
        if (sum <= 3) ++equal_pixels;
    }
    /* This radial preview is deliberately diagnostic, not a Quest optical
       calibration. OpenXR does not expose Meta's private lens coefficients. */
    for (int eye = 0; eye < 2; ++eye) {
        const unsigned char *source = eye ? right.bgra : left.bgra;
        for (int y = 0; y < height; ++y) {
            float ny = ((float)y + 0.5f) * 2.0f / (float)height - 1.0f;
            for (int x = 0; x < width; ++x) {
                float nx = ((float)x + 0.5f) * 2.0f / (float)width - 1.0f;
                float radius2 = nx * nx + ny * ny;
                size_t target = ((size_t)y * (size_t)width * 2u +
                                 (size_t)eye * (size_t)width + (size_t)x) * 4u;
                if (radius2 > 1.0f) { lens[target + 3] = 255; continue; }
                float scale = 1.0f + 0.20f * radius2;
                int sx = (int)(((nx * scale + 1.0f) * 0.5f) * (float)width);
                int sy = (int)(((ny * scale + 1.0f) * 0.5f) * (float)height);
                if (sx < 0 || sx >= width || sy < 0 || sy >= height) {
                    lens[target + 3] = 255;
                    continue;
                }
                memcpy(lens + target,
                       source + ((size_t)sy * (size_t)width + (size_t)sx) * 4u, 4u);
                lens[target + 3] = 255;
            }
        }
    }
    int wrote = write_stereo_diagnostic_bitmap(g_stereo_diagnostic_path[0],
                                                width * 2, height, side) &&
                write_stereo_diagnostic_bitmap(g_stereo_diagnostic_path[1],
                                                width, height, overlay) &&
                write_stereo_diagnostic_bitmap(g_stereo_diagnostic_path[2],
                                                width, height, anaglyph) &&
                write_stereo_diagnostic_bitmap(g_stereo_diagnostic_path[3],
                                                width, height, difference) &&
                write_stereo_diagnostic_bitmap(g_stereo_diagnostic_path[4],
                                                width * 2, height, lens);
    FILE *report = fopen(g_stereo_diagnostic_report_path, "wb");
    if (report) {
        double pixels = (double)width * (double)height;
        fprintf(report,
            "source=final OpenXR projection textures before physical headset compositor\n"
            "dimensions_per_eye=%dx%d\n"
            "mean_absolute_rgb_difference=%.3f\n"
            "nearly_equal_pixels_percent=%.3f\n"
            "maximum_rgb_difference_sum=%u\n"
            "interface_rect=%d,%d %dx%d\n"
            "interface_present_left=%d,%d %dx%d\n"
            "interface_present_right=%d,%d %dx%d\n"
            "lens_preview=approximate radial visualization only; not Quest optical calibration\n"
            "interpretation=world differences are expected from stereo; head-locked UI edges should overlap in overlay/anaglyph\n",
            width, height, total_difference / (pixels * 3.0),
            equal_pixels * 100.0 / pixels, maximum_difference,
            g_interface_content_x, g_interface_content_y,
            g_interface_content_width, g_interface_content_height,
            g_interface_present_rect[0][0], g_interface_present_rect[0][1],
            g_interface_present_rect[0][2], g_interface_present_rect[0][3],
            g_interface_present_rect[1][0], g_interface_present_rect[1][1],
            g_interface_present_rect[1][2], g_interface_present_rect[1][3]);
        fclose(report);
    }
    free(side); free(overlay); free(anaglyph); free(difference); free(lens);
    free_stereo_diagnostic_bitmap(&left);
    free_stereo_diagnostic_bitmap(&right);
    xr_log(wrote ? "Generated Shift+1 stereo diagnostic image set"
                 : "Stereo diagnostic generation failed while writing images");
    InterlockedExchange(&g_stereo_diagnostic_started, wrote ? 2 : 0);
    return wrote ? 0 : 1;
}

static void start_stereo_diagnostic_if_ready(void)
{
    if (!g_geometry_final_interface_proof_written[0] ||
        !g_geometry_final_interface_proof_written[1] ||
        InterlockedCompareExchange(&g_stereo_diagnostic_started, 1, 0) != 0)
        return;
    HANDLE worker = CreateThread(NULL, 0, stereo_diagnostic_worker, NULL, 0, NULL);
    if (worker) CloseHandle(worker);
    else {
        InterlockedExchange(&g_stereo_diagnostic_started, 0);
        xr_log("Stereo diagnostic generation failed: worker could not start");
    }
}

void openxr_bridge_set_interface_load(int draw_calls, long long vertices)
{
    /* The launcher can arm a proof without invoking the in-process Shift+1
       handler. Reset completed flags whenever no marker is present so the
       next external marker cannot accidentally reuse the preceding set. */
    if (g_geometry_proof_marker[0] &&
        GetFileAttributesA(g_geometry_proof_marker) == INVALID_FILE_ATTRIBUTES) {
        for (int eye = 0; eye < EYE_COUNT; ++eye) {
            InterlockedExchange((volatile LONG *)&g_geometry_proof_written[eye], 0);
            InterlockedExchange((volatile LONG *)&g_geometry_presented_proof_written[eye], 0);
            InterlockedExchange((volatile LONG *)&g_geometry_final_interface_proof_written[eye], 0);
            InterlockedExchange((volatile LONG *)&g_geometry_hud_base_proof_written[eye], 0);
        }
        InterlockedExchange((volatile LONG *)&g_interface_alpha_proof_written, 0);
        InterlockedExchange((volatile LONG *)&g_interface_alpha_mask_proof_written, 0);
        InterlockedCompareExchange(&g_stereo_diagnostic_started, 0, 2);
    }
    g_interface_draw_calls = draw_calls;
    g_interface_vertices = vertices;
    int previous = g_interface_heavy;
    int gameplay_active =
        InterlockedCompareExchange(&g_geometry_active, 0, 0) != 0;
    /* Escape/Tab calls this once from the gameplay hook with a zero vertex
       count, before the appended interface begins. Keep that path immediate.
       In ordinary gameplay the native 2560x1441 HUD now measures roughly
       280-420 draws, with short combat spikes above 800. The old 250-draw
       cutoff consequently treated every gameplay frame as a full menu and
       blended its dark backdrop over both eyes. Non-keyboard overlays such as
       level-up/results persist, so require a short sustained high-load run
       while geometry is active instead of reacting to one combat frame. */
    /* The proxy uses a negative vertex count as an unambiguous Escape/Tab
       sentinel. Draw-count-only detection confused dense combat frames with
       menus, while the old zero-vertex convention was lost when the same
       frame's authored UI vertices were forwarded. Very dense persistent
       interfaces (results/level-up) still enter quickly without relying on a
       keyboard key. */
    int explicit_interface = vertices < 0;
    int enter_threshold = gameplay_active ? 700 : 250;
    int leave_threshold = gameplay_active ? 575 : 235;
    int enter_frames = gameplay_active ? (draw_calls >= 1000 ? 2 : 6) : 1;
    if (explicit_interface) {
        g_interface_heavy = 1;
        g_interface_light_frames = 0;
        g_interface_heavy_candidate_frames = 0;
    } else if (draw_calls >= enter_threshold) {
        g_interface_light_frames = 0;
        if (++g_interface_heavy_candidate_frames >= enter_frames) {
            g_interface_heavy = 1;
            g_interface_heavy_candidate_frames = 0;
        }
    } else if (draw_calls <= leave_threshold) {
        g_interface_heavy_candidate_frames = 0;
        if (++g_interface_light_frames >= 4) g_interface_heavy = 0;
    } else {
        g_interface_heavy_candidate_frames = 0;
        g_interface_light_frames = 0;
    }
    if (!previous && g_interface_heavy) {
        g_interface_heavy_since_tick = GetTickCount64();
        g_menu_overlay_ready = 0;
        g_menu_overlay_exact_alpha = 0;
    }
    if (g_interface_mode_initialized && previous != g_interface_heavy && g_log_path[0]) {
        xr_log("Interface mode: %s (draws=%d vertices=%lld)",
               g_interface_heavy
                   ? "clean stereo world plus exact transparent projection UI"
                   : "HUD overlay; depth world allowed",
               draw_calls, vertices);
    }
    g_interface_mode_initialized = 1;
}

int openxr_bridge_interface_heavy(void)
{
    return g_interface_heavy;
}

void openxr_bridge_request_fresh_geometry_proof(void)
{
    if (InterlockedCompareExchange(&g_stereo_diagnostic_started, 0, 0) == 1) {
        xr_log("Shift+1 capture ignored while the previous diagnostic set is writing");
        return;
    }
    InterlockedExchange(&g_stereo_diagnostic_started, 0);
    for (int output = 0; output < 5; ++output)
        if (g_stereo_diagnostic_path[output][0])
            DeleteFileA(g_stereo_diagnostic_path[output]);
    if (g_stereo_diagnostic_report_path[0])
        DeleteFileA(g_stereo_diagnostic_report_path);
    for (int eye = 0; eye < EYE_COUNT; ++eye) {
        InterlockedExchange((volatile LONG *)&g_geometry_proof_written[eye], 0);
        InterlockedExchange(
            (volatile LONG *)&g_geometry_presented_proof_written[eye], 0);
        InterlockedExchange(
            (volatile LONG *)&g_geometry_final_interface_proof_written[eye], 0);
        InterlockedExchange(
            (volatile LONG *)&g_geometry_hud_base_proof_written[eye], 0);
        if (g_geometry_proof_path[eye][0])
            DeleteFileA(g_geometry_proof_path[eye]);
        if (g_geometry_presented_proof_path[eye][0])
            DeleteFileA(g_geometry_presented_proof_path[eye]);
        if (g_geometry_final_interface_proof_path[eye][0])
            DeleteFileA(g_geometry_final_interface_proof_path[eye]);
        if (g_geometry_hud_base_proof_path[eye][0])
            DeleteFileA(g_geometry_hud_base_proof_path[eye]);
    }
    InterlockedExchange(
        (volatile LONG *)&g_interface_alpha_proof_written, 0);
    InterlockedExchange(
        (volatile LONG *)&g_interface_alpha_mask_proof_written, 0);
    if (g_interface_alpha_proof_path[0])
        DeleteFileA(g_interface_alpha_proof_path);
    if (g_interface_alpha_mask_proof_path[0])
        DeleteFileA(g_interface_alpha_mask_proof_path);
    /* Shift+1 must arm its own texture proofs, even with no observer script. */
    if (g_geometry_proof_marker[0]) {
        FILE *marker = fopen(g_geometry_proof_marker, "wb");
        if (marker) {
            fputs("fresh two-eye proof\n", marker);
            fclose(marker);
        }
    }
    xr_log("Fresh two-eye proof set armed for Shift+1");
}

static void capture_cursor_composite_proof(ID3D11Texture2D *source)
{
    /* Cursor proofs are a one-eye diagnostic, unlike the shared two-eye proof
       markers. Treat each marker creation as a fresh one-shot request and
       consume it after the image is written, so pause/menu captures can be
       repeated after a geometry session in the same process. */
    if (GetFileAttributesA(g_cursor_proof_marker) != INVALID_FILE_ATTRIBUTES) {
        g_cursor_proof_written = 0;
        if (g_menu_overlay_ready && g_menu_interop_textures[0] &&
            g_menu_interop_textures[1]) {
            int complete_written = 0, baseline_written = 0;
            capture_composite_proof(
                g_menu_interop_textures[0], g_cursor_proof_marker,
                g_menu_complete_proof_path, &complete_written,
                "completed menu backbuffer");
            capture_composite_proof(
                g_menu_interop_textures[1], g_cursor_proof_marker,
                g_menu_baseline_proof_path, &baseline_written,
                "pre-interface world baseline");
        }
    }
    capture_composite_proof(source, g_cursor_proof_marker, g_cursor_proof_path,
                            &g_cursor_proof_written, "composited cursor");
    if (g_cursor_proof_written)
        DeleteFileA(g_cursor_proof_marker);
}

static void draw_vr_cursor(ID3D11Texture2D *destination, int destination_width,
                           int destination_height, const XrRect2Di *content_rect)
{
    if (!g_game_window || !g_cursor_vertex_shader || !IsWindow(g_game_window)) return;
    CURSORINFO cursor_info;
    memset(&cursor_info, 0, sizeof(cursor_info));
    cursor_info.cbSize = sizeof(cursor_info);
    RECT client;
    if (!GetCursorInfo(&cursor_info) || !cursor_info.hCursor ||
        !ScreenToClient(g_game_window, &cursor_info.ptScreenPos) ||
        !GetClientRect(g_game_window, &client)) return;
    int client_width = client.right - client.left;
    int client_height = client.bottom - client.top;
    if (client_width <= 0 || client_height <= 0 || cursor_info.ptScreenPos.x < 0 ||
        cursor_info.ptScreenPos.y < 0 || cursor_info.ptScreenPos.x >= client_width ||
        cursor_info.ptScreenPos.y >= client_height) return;
    if (cursor_info.hCursor != g_cursor_handle) {
        /* GetCursorInfo is system-wide. Only accept a new shape while the game
           owns focus, otherwise an overlapping browser/tool window can leak its
           cursor into the headset image. Keep the last real game cursor while
           diagnostic tools are foreground. */
        if (GetForegroundWindow() == g_game_window) {
            if (!capture_native_cursor(cursor_info.hCursor)) return;
        } else if (!g_cursor_view) return;
    }
    if (!g_cursor_view || !g_cursor_width || !g_cursor_height) return;

    int content_x = content_rect ? content_rect->offset.x : g_content_x;
    int content_y = content_rect ? content_rect->offset.y : g_content_y;
    int content_width = content_rect ? content_rect->extent.width : g_content_width;
    int content_height = content_rect ? content_rect->extent.height : g_content_height;
    if (content_width <= 0 || content_height <= 0) return;
    float scale_x = (float)content_width / (float)client_width;
    float scale_y = (float)content_height / (float)client_height;
    float left = (float)content_x + cursor_info.ptScreenPos.x * scale_x - g_cursor_hotspot_x * scale_x;
    float top = (float)content_y + cursor_info.ptScreenPos.y * scale_y - g_cursor_hotspot_y * scale_y;
    float right = left + g_cursor_width * scale_x;
    float bottom = top + g_cursor_height * scale_y;
    float rect[4] = {
        left * 2.0f / destination_width - 1.0f,
        1.0f - top * 2.0f / destination_height,
        right * 2.0f / destination_width - 1.0f,
        1.0f - bottom * 2.0f / destination_height
    };
    ID3D11DeviceContext_UpdateSubresource(g_context, (ID3D11Resource *)g_cursor_constants, 0, NULL, rect, 0, 0);

    D3D11_RENDER_TARGET_VIEW_DESC target_desc;
    memset(&target_desc, 0, sizeof(target_desc));
    target_desc.Format = (DXGI_FORMAT)g_swapchain_format;
    target_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
    target_desc.Texture2DArray.ArraySize = 1;
    ID3D11RenderTargetView *target = NULL;
    if (FAILED(ID3D11Device_CreateRenderTargetView(g_device, (ID3D11Resource *)destination,
                                                   &target_desc, &target))) return;
    D3D11_VIEWPORT viewport = {0, 0, (float)destination_width, (float)destination_height, 0, 1};
    float blend_factor[4] = {0, 0, 0, 0};
    ID3D11DeviceContext_OMSetRenderTargets(g_context, 1, &target, NULL);
    ID3D11DeviceContext_OMSetBlendState(g_context, g_cursor_blend, blend_factor, 0xFFFFFFFFu);
    ID3D11DeviceContext_RSSetState(g_context, g_cursor_rasterizer);
    ID3D11DeviceContext_RSSetViewports(g_context, 1, &viewport);
    ID3D11DeviceContext_IASetInputLayout(g_context, NULL);
    ID3D11DeviceContext_IASetPrimitiveTopology(g_context, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    ID3D11DeviceContext_VSSetShader(g_context, g_cursor_vertex_shader, NULL, 0);
    ID3D11DeviceContext_VSSetConstantBuffers(g_context, 0, 1, &g_cursor_constants);
    ID3D11DeviceContext_PSSetShader(g_context, g_cursor_pixel_shader, NULL, 0);
    ID3D11DeviceContext_PSSetShaderResources(g_context, 0, 1, &g_cursor_view);
    ID3D11DeviceContext_PSSetSamplers(g_context, 0, 1, &g_cursor_sampler);
    ID3D11DeviceContext_Draw(g_context, 4, 0);
    ID3D11ShaderResourceView *empty_view = NULL;
    ID3D11DeviceContext_PSSetShaderResources(g_context, 0, 1, &empty_view);
    ID3D11DeviceContext_OMSetRenderTargets(g_context, 0, NULL, NULL);
    ID3D11RenderTargetView_Release(target);
    capture_cursor_composite_proof(destination);
    if (!g_cursor_logged) {
        xr_log("Exact native cursor active");
        g_cursor_logged = 1;
    }
}

int openxr_bridge_geometry_ready(void)
{
    return g_geometry_stereo_requested && g_gl_interop_ready;
}

int openxr_bridge_get_head_translation(float *x_metres, float *y_metres)
{
    if (!x_metres || !y_metres || !g_projection_anchor_valid) return 0;
    float current_x = (g_views[0].pose.position.x + g_views[1].pose.position.x) * 0.5f;
    float current_y = (g_views[0].pose.position.y + g_views[1].pose.position.y) * 0.5f;
    float anchor_x = g_projection_anchor_pose[0].position.x;
    float anchor_y = g_projection_anchor_pose[0].position.y;
    *x_metres = current_x - anchor_x;
    *y_metres = current_y - anchor_y;
    if (*x_metres > 0.12f) *x_metres = 0.12f;
    if (*x_metres < -0.12f) *x_metres = -0.12f;
    if (*y_metres > 0.10f) *y_metres = 0.10f;
    if (*y_metres < -0.10f) *y_metres = -0.10f;
    return 1;
}

int openxr_bridge_get_eye_size(int *width, int *height)
{
    if (!width || !height || g_eyes[0].width <= 0 || g_eyes[0].height <= 0)
        return 0;
    *width = g_eyes[0].width;
    *height = g_eyes[0].height;
    return 1;
}

int openxr_bridge_get_runtime_fov(int eye, float *angle_left,
                                  float *angle_right, float *angle_up,
                                  float *angle_down)
{
    if (eye < 0 || eye >= EYE_COUNT || !angle_left || !angle_right ||
        !angle_up || !angle_down)
        return 0;
    AcquireSRWLockShared(&g_view_lock);
    XrFovf fov = g_views[eye].fov;
    ReleaseSRWLockShared(&g_view_lock);
    if (!isfinite(fov.angleLeft) || !isfinite(fov.angleRight) ||
        !isfinite(fov.angleUp) || !isfinite(fov.angleDown) ||
        fov.angleRight <= fov.angleLeft || fov.angleUp <= 0.0f ||
        fov.angleDown >= 0.0f)
        return 0;
    *angle_left = fov.angleLeft;
    *angle_right = fov.angleRight;
    *angle_up = fov.angleUp;
    *angle_down = fov.angleDown;
    return 1;
}

int openxr_bridge_get_head_view_delta(float units_per_metre,
                                      float world_eye_separation,
                                      float matrix[16])
{
    if (!matrix || !g_projection_anchor_valid || units_per_metre <= 0.0f) return 0;
    AcquireSRWLockShared(&g_view_lock);
    float anchor_position[3] = {
        g_projection_anchor_pose[0].position.x,
        g_projection_anchor_pose[0].position.y,
        g_projection_anchor_pose[0].position.z
    };
    float current_position[3] = {
        (g_views[0].pose.position.x + g_views[1].pose.position.x) * 0.5f,
        (g_views[0].pose.position.y + g_views[1].pose.position.y) * 0.5f,
        (g_views[0].pose.position.z + g_views[1].pose.position.z) * 0.5f
    };
    /* Keep physical movement bounded during early comfort testing. */
    for (int axis = 0; axis < 3; ++axis) {
        float difference = current_position[axis] - anchor_position[axis];
        float limit = axis == 1 ? 0.12f : 0.15f;
        if (difference > limit) current_position[axis] = anchor_position[axis] + limit;
        if (difference < -limit) current_position[axis] = anchor_position[axis] - limit;
    }
    float anchor_orientation[4] = {
        g_projection_anchor_pose[0].orientation.x,
        g_projection_anchor_pose[0].orientation.y,
        g_projection_anchor_pose[0].orientation.z,
        g_projection_anchor_pose[0].orientation.w
    };
    float current_orientation[4] = {
        g_views[0].pose.orientation.x, g_views[0].pose.orientation.y,
        g_views[0].pose.orientation.z, g_views[0].pose.orientation.w
    };
    for (int eye = 0; eye < EYE_COUNT; ++eye) {
        /* Preserve the raw render-time pose. The XR frame thread combines it
           with the latest tracked pose to reflect only the incremental
           pitch/roll/Y reprojection delta used by the OpenGL camera. */
        g_pending_pair_pose[eye] = g_views[eye].pose;
        if (!g_geometry_fov_valid[eye]) g_pending_pair_fov[eye] = g_views[eye].fov;
    }
    float left_position[3] = {
        g_pending_pair_pose[0].position.x,
        g_pending_pair_pose[0].position.y,
        g_pending_pair_pose[0].position.z
    };
    float right_position[3] = {
        g_pending_pair_pose[1].position.x,
        g_pending_pair_pose[1].position.y,
        g_pending_pair_pose[1].position.z
    };
    float target_ipd = world_eye_separation / units_per_metre;
    if (vr_scale_stereo_eye_positions_f(left_position, right_position, target_ipd)) {
        g_pending_pair_pose[0].position.x = left_position[0];
        g_pending_pair_pose[0].position.y = left_position[1];
        g_pending_pair_pose[0].position.z = left_position[2];
        g_pending_pair_pose[1].position.x = right_position[0];
        g_pending_pair_pose[1].position.y = right_position[1];
        g_pending_pair_pose[1].position.z = right_position[2];
        if (!g_geometry_ipd_logged) {
            xr_log("Projection metadata IPD matched to game stereo: %.4fm", target_ipd);
            g_geometry_ipd_logged = 1;
        }
    }
    g_pending_pair_pose_valid = 1;
    g_pending_pair_pose_tick = GetTickCount64();
    ReleaseSRWLockShared(&g_view_lock);
    vr_reflect_pose_for_texture_y_flip_f(anchor_position, anchor_orientation);
    vr_reflect_pose_for_texture_y_flip_f(current_position, current_orientation);
    vr_build_head_view_delta_f(anchor_position, anchor_orientation, current_position,
                               current_orientation, units_per_metre, matrix);
    if (!g_tabletop_motion_logged &&
        (fabsf(matrix[12]) > 0.01f || fabsf(matrix[13]) > 0.01f ||
         fabsf(matrix[14]) > 0.01f || fabsf(matrix[0] - 1.0f) > 0.001f ||
         fabsf(matrix[5] - 1.0f) > 0.001f || fabsf(matrix[10] - 1.0f) > 0.001f)) {
        xr_log("Full-pose tabletop camera active: %.3f %.3f %.3f "
               "basisX %.3f %.3f %.3f basisZ %.3f %.3f %.3f",
               matrix[12], matrix[13], matrix[14], matrix[0], matrix[1], matrix[2],
               matrix[8], matrix[9], matrix[10]);
        g_tabletop_motion_logged = 1;
    }
    return 1;
}

void openxr_bridge_set_geometry_fov(int eye, float angle_left, float angle_right,
                                    float angle_up, float angle_down)
{
    if (eye < 0 || eye >= EYE_COUNT) return;
    g_pending_pair_fov[eye].angleLeft = angle_left;
    g_pending_pair_fov[eye].angleRight = angle_right;
    g_pending_pair_fov[eye].angleUp = angle_up;
    g_pending_pair_fov[eye].angleDown = angle_down;
    g_geometry_fov_valid[eye] = 1;
    if (!g_geometry_fov_logged && g_geometry_fov_valid[0] && g_geometry_fov_valid[1]) {
        xr_log("Exact game projection metadata active: left eye %.2f/%.2f/%.2f/%.2f deg",
               g_pending_pair_fov[0].angleLeft * 57.2957795f,
               g_pending_pair_fov[0].angleRight * 57.2957795f,
               g_pending_pair_fov[0].angleUp * 57.2957795f,
               g_pending_pair_fov[0].angleDown * 57.2957795f);
        g_geometry_fov_logged = 1;
    }
}

float openxr_bridge_get_geometry_alignment(int eye)
{
    if (eye < 0 || eye >= EYE_COUNT) return 0.0f;
    return g_eye_alignment * (eye == 0 ? -1.0f : 1.0f);
}

int openxr_bridge_capture_hud_base(int eye, float safe_scale_x,
                                   float safe_scale_y)
{
    if (eye < 0 || eye >= EYE_COUNT || !g_gl_interop_ready ||
        !p_glGetIntegerv || !p_glGenTextures || !p_glBindTextureRaw ||
        !p_glTexImage2DRaw || !p_glTexParameteriRaw ||
        !p_glBindFramebuffer || !p_glFramebufferTexture2D ||
        !p_glBlitFramebuffer) return 0;
    GLint viewport[4] = {0};
    GLint previous_read = 0, previous_draw = 0, previous_texture = 0;
    p_glGetIntegerv(GL_VIEWPORT, viewport);
    p_glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &previous_read);
    p_glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &previous_draw);
    p_glGetIntegerv(GL_TEXTURE_BINDING_2D, &previous_texture);
    if (viewport[2] <= 0 || viewport[3] <= 0) return 0;

    if (!g_hud_base_source_textures[0]) {
        p_glGenTextures(EYE_COUNT, g_hud_base_source_textures);
        p_glGenFramebuffers(1, &g_hud_base_source_framebuffer);
        for (int source_eye = 0; source_eye < EYE_COUNT; ++source_eye) {
            p_glBindTextureRaw(GL_TEXTURE_2D,
                               g_hud_base_source_textures[source_eye]);
            p_glTexParameteriRaw(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            p_glTexParameteriRaw(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            p_glTexImage2DRaw(GL_TEXTURE_2D, 0, GL_RGBA,
                              viewport[2], viewport[3], 0,
                              GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        }
        g_hud_base_source_width = viewport[2];
        g_hud_base_source_height = viewport[3];
    }
    p_glBindTextureRaw(GL_TEXTURE_2D, (GLuint)previous_texture);
    if (g_hud_base_source_width != viewport[2] ||
        g_hud_base_source_height != viewport[3]) return 0;

    p_glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    p_glBindFramebuffer(GL_DRAW_FRAMEBUFFER, g_hud_base_source_framebuffer);
    p_glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                             GL_TEXTURE_2D,
                             g_hud_base_source_textures[eye], 0);
    GLenum status = p_glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);
    if (status == GL_FRAMEBUFFER_COMPLETE) {
        p_glBlitFramebuffer(viewport[0], viewport[1],
                            viewport[0] + viewport[2],
                            viewport[1] + viewport[3],
                            0, 0, viewport[2], viewport[3],
                            GL_COLOR_BUFFER_BIT, GL_NEAREST);
        g_hud_base_source_ready[eye] = 1;
        if (safe_scale_x >= 0.35f && safe_scale_x <= 1.0f)
            g_native_hud_safe_scale_x = safe_scale_x;
        if (safe_scale_y >= 0.25f && safe_scale_y <= 1.0f)
            g_native_hud_safe_scale_y = safe_scale_y;
        if (!g_native_hud_logged) {
            xr_log("Native-detail HUD base preserved at %dx%d; safe canvas %.3fx%.3f",
                   viewport[2], viewport[3],
                   g_native_hud_safe_scale_x, g_native_hud_safe_scale_y);
            g_native_hud_logged = 1;
        }
    }
    p_glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                             GL_TEXTURE_2D, 0, 0);
    p_glBindFramebuffer(GL_READ_FRAMEBUFFER, (GLuint)previous_read);
    p_glBindFramebuffer(GL_DRAW_FRAMEBUFFER, (GLuint)previous_draw);
    return status == GL_FRAMEBUFFER_COMPLETE;
}

static int capture_eye_impl(int eye, int completed_interface)
{
    if (eye < 0 || eye >= EYE_COUNT || !openxr_bridge_geometry_ready()) return 0;
    /* Do not hold the game/XR interop lock while waiting for the prior GPU
       pair copy.  On Quest this wait can span one compositor interval; the XR
       frame thread then misses that interval even though it only needs a fast
       CopyResource.  Pair publication is single-threaded on the game render
       thread, while D3D11 multithread protection makes this fence query safe
       outside the short WGL ownership section. */
    if (!wait_for_pair_copy_fence()) {
        return 0;
    }
    AcquireSRWLockExclusive(&g_interop_capture_lock);
    HANDLE eye_object = g_interop_objects[eye];
    int want_hud_base = g_hud_base_source_ready[eye] ? 1 : 0;
    DWORD error = ERROR_SUCCESS;
    if (!lock_single_interop_object(eye_object, &error)) {
        LONG failures = InterlockedIncrement(&g_interop_lock_failures);
        if (failures == 1 || (failures % 240) == 0)
            xr_log("Eye interop lock failed eye=%d error=%lu failures=%ld",
                   eye, error, failures);
        ReleaseSRWLockExclusive(&g_interop_capture_lock);
        return 0;
    }

    GLint previous_read = 0;
    GLint previous_draw = 0;
    GLint viewport[4] = {0};
    p_glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &previous_read);
    p_glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &previous_draw);
    p_glGetIntegerv(GL_VIEWPORT, viewport);
    p_glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    p_glBindFramebuffer(GL_DRAW_FRAMEBUFFER, g_interop_framebuffer);
    p_glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                             g_interop_gl_textures[eye], 0);
    GLenum status = p_glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);
    int hud_base_copied = 0;
    if (status == GL_FRAMEBUFFER_COMPLETE) {
        int target_width = g_eyes[eye].width;
        int target_height = g_eyes[eye].height;
        int rectangle[4] = {0, 0, target_width, target_height};
        if (g_geometry_fov_valid[eye])
            vr_fit_projection_rect_f(target_width, target_height,
                                     g_pending_pair_fov[eye].angleLeft,
                                     g_pending_pair_fov[eye].angleRight,
                                     g_pending_pair_fov[eye].angleUp,
                                     g_pending_pair_fov[eye].angleDown,
                                     rectangle);
        int x = rectangle[0];
        int y = rectangle[1];
        int content_width = rectangle[2];
        int fitted_height = rectangle[3];
        g_geometry_eye_rect[eye].offset.x = x;
        g_geometry_eye_rect[eye].offset.y = y;
        g_geometry_eye_rect[eye].extent.width = content_width;
        g_geometry_eye_rect[eye].extent.height = fitted_height;
        if (!g_projection_rect_logged &&
            (content_width != target_width || fitted_height != target_height)) {
            xr_log("Projection image rectangle synchronized: eye=%d rect=%d,%d %dx%d texture=%dx%d",
                   eye, x, y, content_width, fitted_height, target_width, target_height);
            g_projection_rect_logged = 1;
        }
        g_content_x = x;
        g_content_y = y;
        g_content_width = content_width;
        g_content_height = fitted_height;
        p_glBlitFramebuffer(viewport[0], viewport[1], viewport[0] + viewport[2], viewport[1] + viewport[3],
                            x, y + fitted_height, x + content_width, y,
                            GL_COLOR_BUFFER_BIT, GL_LINEAR);
        /* The shared eye texture must be fully released by OpenGL before
           ownership returns to D3D/OpenXR. Without this completion barrier,
           NVIDIA can eventually reject every later WGL interop lock and the
           headset remains stuck on the last completed stereo pair. */
        p_glFinish();
    }
    /* An interop object may not remain attached to GL state when ownership is
       returned to D3D. Explicitly detach it before the WGL unlock. */
    p_glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                             GL_TEXTURE_2D, 0, 0);
    p_glBindFramebuffer(GL_READ_FRAMEBUFFER, (GLuint)previous_read);
    p_glBindFramebuffer(GL_DRAW_FRAMEBUFFER, (GLuint)previous_draw);
    int unlocked = unlock_single_interop_object(eye_object, &error);
    if (status != GL_FRAMEBUFFER_COMPLETE || !unlocked) {
        if (!unlocked) {
            LONG failures = InterlockedIncrement(&g_interop_unlock_failures);
            if (failures == 1 || (failures % 240) == 0)
                xr_log("Eye interop unlock failed eye=%d error=%lu failures=%ld",
                       eye, error, failures);
        }
        ReleaseSRWLockExclusive(&g_interop_capture_lock);
        return 0;
    }

    /* Transfer the fixed-HUD source under its own ownership interval. Physical
       Link eventually poisoned the whole interop device when the eye and HUD
       resources were locked/unlocked as one array. HUD failure is allowed to
       degrade for this pair without throwing away the valid world eye. */
    if (want_hud_base) {
        HANDLE hud_object = g_hud_base_interop_objects[eye];
        DWORD hud_error = ERROR_SUCCESS;
        if (lock_single_interop_object(hud_object, &hud_error)) {
            p_glBindFramebuffer(GL_READ_FRAMEBUFFER,
                                g_hud_base_source_framebuffer);
            p_glFramebufferTexture2D(
                GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                g_hud_base_source_textures[eye], 0);
            p_glBindFramebuffer(GL_DRAW_FRAMEBUFFER, g_interop_framebuffer);
            p_glFramebufferTexture2D(
                GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                g_hud_base_interop_gl_textures[eye], 0);
            if (p_glCheckFramebufferStatus(GL_READ_FRAMEBUFFER) ==
                    GL_FRAMEBUFFER_COMPLETE &&
                p_glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) ==
                    GL_FRAMEBUFFER_COMPLETE) {
                XrRect2Di rect = g_geometry_eye_rect[eye];
                p_glBlitFramebuffer(0, 0, g_hud_base_source_width,
                                    g_hud_base_source_height,
                                    rect.offset.x,
                                    rect.offset.y + rect.extent.height,
                                    rect.offset.x + rect.extent.width,
                                    rect.offset.y,
                                    GL_COLOR_BUFFER_BIT, GL_LINEAR);
                p_glFinish();
                hud_base_copied = 1;
            }
            p_glFramebufferTexture2D(
                GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                GL_TEXTURE_2D, 0, 0);
            p_glFramebufferTexture2D(
                GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                GL_TEXTURE_2D, 0, 0);
            p_glBindFramebuffer(GL_READ_FRAMEBUFFER, (GLuint)previous_read);
            p_glBindFramebuffer(GL_DRAW_FRAMEBUFFER, (GLuint)previous_draw);
            if (!unlock_single_interop_object(hud_object, &hud_error))
                hud_base_copied = 0;
        }
        if (!hud_base_copied) {
            static LONG hud_transfer_failures;
            LONG failures = InterlockedIncrement(&hud_transfer_failures);
            if (failures == 1 || (failures % 240) == 0)
                xr_log("HUD interop transfer failed eye=%d error=%lu failures=%ld; "
                       "world eye retained", eye, hud_error, failures);
        }
    }
    g_hud_base_interop_ready[eye] = hud_base_copied;
    g_completed_interface_eye_ready[eye] = completed_interface ? 1 : 0;
    g_geometry_eye_ready[eye] = 1;
    ++g_eye_capture_generation[eye];
    if (eye == 0 && g_geometry_eye_ready[1]) {
        /* Preserve every completed pair immediately. The earlier XR-thread
           sampler only had the interval between eye 0 finishing and the next
           eye 1 starting in which both interop textures still belonged to the
           same pair. At high game rates it missed more than half the genuine
           pairs. A GPU-side copy into a private triple buffer makes the pair
           durable without stalling either thread or synthesizing an eye. */
        LONG current = InterlockedCompareExchange(&g_present_pair_index, -1, -1);
        LONG submitted = InterlockedCompareExchange(&g_submit_pair_index, -1, -1);
        int next = current < 0 ? 0 : ((int)current + 1) % PAIR_BUFFER_COUNT;
        if (next == submitted) next = (next + 1) % PAIR_BUFFER_COUNT;
        int completed_pair = g_completed_interface_eye_ready[0] &&
                             g_completed_interface_eye_ready[1];
        if (completed_pair) {
            static LONG completed_pair_logged;
            if (InterlockedCompareExchange(&completed_pair_logged, 1, 0) == 0)
                xr_log("Exact completed stereo interface pair published into the shared HUD-safe canvas");
        }
        for (int pair_eye = 0; pair_eye < EYE_COUNT; ++pair_eye) {
            /* Completed eyes contain the exact game-authored interface, but
               submitting them directly as world stereo gives flat menus
               disparity and makes them fill the lens. The same-eye world base
               lets the HUD resolver remove each copy and insert eye 0 once in
               the established compact 16:9 safe canvas. */
            if (!resolve_sharpened_pair_eye(next, pair_eye, completed_pair)) {
                ID3D11DeviceContext_CopyResource(
                    g_context,
                    (ID3D11Resource *)g_present_pair_textures[next][pair_eye],
                    (ID3D11Resource *)g_interop_textures[pair_eye]);
            }
            if (g_hud_base_interop_ready[pair_eye]) {
                capture_composite_proof(
                    g_hud_base_interop_textures[pair_eye],
                    g_geometry_proof_marker,
                    g_geometry_hud_base_proof_path[pair_eye],
                    &g_geometry_hud_base_proof_written[pair_eye],
                    pair_eye == 0 ? "pre-HUD base left eye"
                                  : "pre-HUD base right eye");
            }
            /* Marker-gated proof of the actual resolved pair. This is taken at
               publication time, so menu/HUD verification works even when a
               physical headset is not actively presenting OpenXR frames. */
            capture_composite_proof(
                g_present_pair_textures[next][pair_eye],
                g_geometry_proof_marker,
                g_geometry_presented_proof_path[pair_eye],
                &g_geometry_presented_proof_written[pair_eye],
                pair_eye == 0 ? "resolved pair left eye"
                              : "resolved pair right eye");
            if (g_pending_pair_pose_valid) {
                g_present_pair_pose[next][pair_eye] = g_pending_pair_pose[pair_eye];
                g_present_pair_fov[next][pair_eye] = g_pending_pair_fov[pair_eye];
            }
            g_present_pair_rect[next][pair_eye] = g_geometry_eye_rect[pair_eye];
        }
        g_present_pair_pose_tick[next] = g_pending_pair_pose_tick;
        g_present_pair_publish_tick[next] = GetTickCount64();
        g_present_pair_depth_valid[next] =
            g_geometry_eye_depth_ready[0] && g_geometry_eye_depth_ready[1];
        if (g_present_pair_depth_valid[next]) {
            for (int pair_eye = 0; pair_eye < EYE_COUNT; ++pair_eye) {
                ID3D11DeviceContext_CopyResource(
                    g_context,
                    (ID3D11Resource *)g_present_pair_depth_textures[next][pair_eye],
                    (ID3D11Resource *)g_geometry_eye_depth_textures[pair_eye]);
            }
        }
        ID3D11DeviceContext_End(
            g_context, (ID3D11Asynchronous *)g_pair_copy_fence);
        ID3D11DeviceContext_Flush(g_context);
        InterlockedExchange(&g_pair_copy_fence_pending, 1);
        g_present_pair_pose_valid[next] = g_pending_pair_pose_valid;
        g_present_pair_completed_interface[next] = completed_pair;
        InterlockedExchange(&g_present_pair_index, next);
        InterlockedIncrement(&g_present_pair_generation);
    }
    ReleaseSRWLockExclusive(&g_interop_capture_lock);
    capture_composite_proof(g_interop_textures[eye], g_geometry_proof_marker,
                            g_geometry_proof_path[eye], &g_geometry_proof_written[eye],
                            eye == 0 ? "true-geometry left eye" : "true-geometry right eye");
    if (g_geometry_proof_written[0] && g_geometry_proof_written[1] &&
        g_geometry_presented_proof_written[0] &&
        g_geometry_presented_proof_written[1] &&
        g_geometry_final_interface_proof_written[0] &&
        g_geometry_final_interface_proof_written[1] &&
        g_geometry_hud_base_proof_written[0] &&
        g_geometry_hud_base_proof_written[1] &&
        g_interface_alpha_proof_written &&
        g_interface_alpha_mask_proof_written)
        DeleteFileA(g_geometry_proof_marker);
    if (!g_geometry_capture_logged && g_geometry_eye_ready[0] && g_geometry_eye_ready[1]) {
        xr_log("True-geometry per-eye capture active at %dx%d; projection rect %d,%d %dx%d",
               g_eyes[0].width, g_eyes[0].height,
               g_geometry_eye_rect[0].offset.x, g_geometry_eye_rect[0].offset.y,
               g_geometry_eye_rect[0].extent.width, g_geometry_eye_rect[0].extent.height);
        g_geometry_capture_logged = 1;
    }
    return 1;
}

int openxr_bridge_capture_eye(int eye)
{
    return capture_eye_impl(eye, 0);
}

int openxr_bridge_capture_completed_interface_eye(int eye)
{
    return capture_eye_impl(eye, 1);
}

int openxr_bridge_present_mirror_eye(int eye, float vertical_scale)
{
    if (g_desktop_mirror_ready) {
        GLint previous_read = 0, previous_draw = 0, viewport[4] = {0};
        p_glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &previous_read);
        p_glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &previous_draw);
        p_glGetIntegerv(GL_VIEWPORT, viewport);
        p_glBindFramebuffer(GL_READ_FRAMEBUFFER, g_desktop_mirror_framebuffer);
        p_glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        if (vertical_scale < 1.0f) vertical_scale = 1.0f;
        int source_height = (int)((float)g_desktop_mirror_height /
                                  vertical_scale + 0.5f);
        if (source_height < 1) source_height = 1;
        if (source_height > g_desktop_mirror_height)
            source_height = g_desktop_mirror_height;
        int mirror_y0 = (g_desktop_mirror_height - source_height) / 2;
        int mirror_y1 = mirror_y0 + source_height;
        p_glBlitFramebuffer(
            0, mirror_y0, g_desktop_mirror_width, mirror_y1,
            viewport[0], viewport[1], viewport[0] + viewport[2],
            viewport[1] + viewport[3], GL_COLOR_BUFFER_BIT, GL_LINEAR);
        p_glBindFramebuffer(GL_READ_FRAMEBUFFER, (GLuint)previous_read);
        p_glBindFramebuffer(GL_DRAW_FRAMEBUFFER, (GLuint)previous_draw);
        return 1;
    }
    /* The game renders alternating eyes. Prefer the requested retained eye so
       the desktop window does not flash between two cameras every frame. The
       native backbuffer remains a startup/menu fallback. */
    if (g_native_mirror_ready &&
        (eye < 0 || eye >= EYE_COUNT || !g_gl_interop_ready ||
         !g_geometry_eye_ready[eye] || g_content_width <= 0 ||
         g_content_height <= 0)) {
        GLint previous_read = 0, previous_draw = 0, viewport[4] = {0};
        p_glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &previous_read);
        p_glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &previous_draw);
        p_glGetIntegerv(GL_VIEWPORT, viewport);
        p_glBindFramebuffer(GL_READ_FRAMEBUFFER, g_native_mirror_framebuffer);
        p_glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        if (vertical_scale < 1.0f) vertical_scale = 1.0f;
        int source_height = (int)((float)g_native_mirror_height / vertical_scale + 0.5f);
        if (source_height < 1) source_height = 1;
        if (source_height > g_native_mirror_height) source_height = g_native_mirror_height;
        int mirror_y0 = (g_native_mirror_height - source_height) / 2;
        int mirror_y1 = mirror_y0 + source_height;
        p_glBlitFramebuffer(0, mirror_y0, g_native_mirror_width, mirror_y1,
                            viewport[0], viewport[1], viewport[0] + viewport[2],
                            viewport[1] + viewport[3], GL_COLOR_BUFFER_BIT, GL_LINEAR);
        p_glBindFramebuffer(GL_READ_FRAMEBUFFER, (GLuint)previous_read);
        p_glBindFramebuffer(GL_DRAW_FRAMEBUFFER, (GLuint)previous_draw);
        return 1;
    }
    if (eye < 0 || eye >= EYE_COUNT || !g_gl_interop_ready ||
        !g_geometry_eye_ready[eye] || g_content_width <= 0 || g_content_height <= 0)
        return 0;
    HANDLE object = g_interop_objects[eye];
    if (!p_wglDXLockObjectsNV(g_interop_device, 1, &object)) return 0;
    GLint previous_read = 0;
    GLint previous_draw = 0;
    GLint viewport[4] = {0};
    p_glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &previous_read);
    p_glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &previous_draw);
    p_glGetIntegerv(GL_VIEWPORT, viewport);
    p_glBindFramebuffer(GL_READ_FRAMEBUFFER, g_interop_framebuffer);
    p_glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                             g_interop_gl_textures[eye], 0);
    GLenum status = p_glCheckFramebufferStatus(GL_READ_FRAMEBUFFER);
    if (status == GL_FRAMEBUFFER_COMPLETE) {
        /* Capture into the shared eye reverses destination Y for D3D. Reverse
           it again here to restore the game's normal OpenGL window orientation. */
        p_glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        p_glBlitFramebuffer(g_content_x, g_content_y + g_content_height,
                            g_content_x + g_content_width, g_content_y,
                            viewport[0], viewport[1], viewport[0] + viewport[2],
                            viewport[1] + viewport[3], GL_COLOR_BUFFER_BIT, GL_LINEAR);
    }
    p_glFinish();
    p_glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                             GL_TEXTURE_2D, 0, 0);
    p_glBindFramebuffer(GL_READ_FRAMEBUFFER, (GLuint)previous_read);
    p_glBindFramebuffer(GL_DRAW_FRAMEBUFFER, (GLuint)previous_draw);
    int unlocked = p_wglDXUnlockObjectsNV(g_interop_device, 1, &object);
    return status == GL_FRAMEBUFFER_COMPLETE && unlocked;
}

int openxr_bridge_present_desktop_interface(void)
{
    if (!g_interface_alpha_desktop_ready || !g_interface_alpha_textures[0] ||
        !g_interface_alpha_width || !g_interface_alpha_height ||
        !p_glGetIntegerv || !p_glGetFloatvRaw) return 0;

    GLint previous_read = 0, previous_draw = 0, previous_program = 0;
    GLint previous_active_texture = GL_TEXTURE0, previous_texture = 0;
    GLint previous_matrix_mode = GL_MODELVIEW;
    GLint previous_blend[4] = {GL_ONE, GL_ZERO, GL_ONE, GL_ZERO};
    GLint previous_blend_equation[2] = {GL_FUNC_ADD, GL_FUNC_ADD};
    GLint previous_texture_env = GL_MODULATE;
    GLfloat previous_color[4] = {1, 1, 1, 1};
    GLboolean blend_enabled = p_glIsEnabledRaw(GL_BLEND);
    GLboolean depth_enabled = p_glIsEnabledRaw(GL_DEPTH_TEST);
    GLboolean scissor_enabled = p_glIsEnabledRaw(GL_SCISSOR_TEST);
    GLboolean cull_enabled = p_glIsEnabledRaw(GL_CULL_FACE);
    GLboolean alpha_test_enabled = p_glIsEnabledRaw(GL_ALPHA_TEST);
    p_glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &previous_read);
    p_glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &previous_draw);
    p_glGetIntegerv(GL_CURRENT_PROGRAM, &previous_program);
    p_glGetIntegerv(GL_ACTIVE_TEXTURE, &previous_active_texture);
    p_glActiveTextureRaw(GL_TEXTURE0);
    GLboolean texture_enabled = p_glIsEnabledRaw(GL_TEXTURE_2D);
    p_glGetIntegerv(GL_TEXTURE_BINDING_2D, &previous_texture);
    p_glGetTexEnvivRaw(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE,
                       &previous_texture_env);
    p_glGetIntegerv(GL_MATRIX_MODE, &previous_matrix_mode);
    p_glGetIntegerv(GL_BLEND_SRC_RGB, &previous_blend[0]);
    p_glGetIntegerv(GL_BLEND_DST_RGB, &previous_blend[1]);
    p_glGetIntegerv(GL_BLEND_SRC_ALPHA, &previous_blend[2]);
    p_glGetIntegerv(GL_BLEND_DST_ALPHA, &previous_blend[3]);
    p_glGetIntegerv(GL_BLEND_EQUATION_RGB, &previous_blend_equation[0]);
    p_glGetIntegerv(GL_BLEND_EQUATION_ALPHA, &previous_blend_equation[1]);
    p_glGetFloatvRaw(GL_CURRENT_COLOR, previous_color);

    p_glUseProgramRaw(0);
    p_glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    p_glBindTextureRaw(GL_TEXTURE_2D, g_interface_alpha_textures[0]);
    p_glTexEnviRaw(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    p_glEnableRaw(GL_TEXTURE_2D);
    p_glEnableRaw(GL_BLEND);
    p_glDisableRaw(GL_DEPTH_TEST);
    p_glDisableRaw(GL_SCISSOR_TEST);
    p_glDisableRaw(GL_CULL_FACE);
    p_glDisableRaw(GL_ALPHA_TEST);
    /* The interface target stores premultiplied RGB and source-over alpha. */
    p_glBlendFuncSeparateRaw(GL_ONE, GL_ONE_MINUS_SRC_ALPHA,
                             GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    p_glBlendEquationSeparateRaw(GL_FUNC_ADD, GL_FUNC_ADD);
    p_glColor4fRaw(1.0f, 1.0f, 1.0f, 1.0f);

    p_glMatrixModeRaw(GL_PROJECTION);
    p_glPushMatrixRaw();
    p_glLoadIdentityRaw();
    p_glOrthoRaw(0.0, 1.0, 0.0, 1.0, -1.0, 1.0);
    p_glMatrixModeRaw(GL_MODELVIEW);
    p_glPushMatrixRaw();
    p_glLoadIdentityRaw();
    p_glBeginRaw(GL_QUADS);
    p_glTexCoord2fRaw(0.0f, 0.0f); p_glVertex2fRaw(0.0f, 0.0f);
    p_glTexCoord2fRaw(1.0f, 0.0f); p_glVertex2fRaw(1.0f, 0.0f);
    p_glTexCoord2fRaw(1.0f, 1.0f); p_glVertex2fRaw(1.0f, 1.0f);
    p_glTexCoord2fRaw(0.0f, 1.0f); p_glVertex2fRaw(0.0f, 1.0f);
    p_glEndRaw();
    p_glPopMatrixRaw();
    p_glMatrixModeRaw(GL_PROJECTION);
    p_glPopMatrixRaw();

    p_glColor4fRaw(previous_color[0], previous_color[1],
                   previous_color[2], previous_color[3]);
    p_glBindTextureRaw(GL_TEXTURE_2D, (GLuint)previous_texture);
    p_glTexEnviRaw(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE,
                   previous_texture_env);
    if (!texture_enabled) p_glDisableRaw(GL_TEXTURE_2D);
    if (!blend_enabled) p_glDisableRaw(GL_BLEND);
    if (depth_enabled) p_glEnableRaw(GL_DEPTH_TEST);
    if (scissor_enabled) p_glEnableRaw(GL_SCISSOR_TEST);
    if (cull_enabled) p_glEnableRaw(GL_CULL_FACE);
    if (alpha_test_enabled) p_glEnableRaw(GL_ALPHA_TEST);
    p_glBlendFuncSeparateRaw((GLenum)previous_blend[0],
                             (GLenum)previous_blend[1],
                             (GLenum)previous_blend[2],
                             (GLenum)previous_blend[3]);
    p_glBlendEquationSeparateRaw((GLenum)previous_blend_equation[0],
                                 (GLenum)previous_blend_equation[1]);
    p_glActiveTextureRaw((GLenum)previous_active_texture);
    p_glUseProgramRaw((GLuint)previous_program);
    p_glMatrixModeRaw((GLenum)previous_matrix_mode);
    p_glBindFramebuffer(GL_READ_FRAMEBUFFER, (GLuint)previous_read);
    p_glBindFramebuffer(GL_DRAW_FRAMEBUFFER, (GLuint)previous_draw);
    return 1;
}

int openxr_bridge_capture_native_mirror(int preserve_desktop)
{
    if (!g_gl_interop_ready || !p_glBindTextureRaw || !p_glTexImage2DRaw ||
        !p_glTexParameteriRaw) return 0;
    GLint previous_read = 0, previous_draw = 0, viewport[4] = {0};
    GLint previous_texture = 0;
    p_glGetIntegerv(GL_TEXTURE_BINDING_2D, &previous_texture);
    p_glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &previous_read);
    p_glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &previous_draw);
    p_glGetIntegerv(GL_VIEWPORT, viewport);
    if (viewport[2] <= 0 || viewport[3] <= 0) return 0;
    if (!g_native_mirror_texture || g_native_mirror_width != viewport[2] ||
        g_native_mirror_height != viewport[3]) {
        if (!g_native_mirror_texture) p_glGenTextures(1, &g_native_mirror_texture);
        if (!g_native_mirror_framebuffer) p_glGenFramebuffers(1, &g_native_mirror_framebuffer);
        p_glBindTextureRaw(GL_TEXTURE_2D, g_native_mirror_texture);
        p_glTexParameteriRaw(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        p_glTexParameteriRaw(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        p_glTexImage2DRaw(GL_TEXTURE_2D, 0, GL_RGBA, viewport[2], viewport[3], 0,
                          GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        g_native_mirror_width = viewport[2];
        g_native_mirror_height = viewport[3];
    }
    p_glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    p_glBindFramebuffer(GL_DRAW_FRAMEBUFFER, g_native_mirror_framebuffer);
    p_glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                             g_native_mirror_texture, 0);
    GLenum status = p_glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);
    if (status == GL_FRAMEBUFFER_COMPLETE) {
        p_glBlitFramebuffer(viewport[0], viewport[1], viewport[0] + viewport[2],
                            viewport[1] + viewport[3], 0, 0, viewport[2], viewport[3],
                            GL_COLOR_BUFFER_BIT, GL_NEAREST);
        g_native_mirror_ready = 1;
    }
    if (status == GL_FRAMEBUFFER_COMPLETE && preserve_desktop) {
        if (!g_desktop_mirror_texture ||
            g_desktop_mirror_width != viewport[2] ||
            g_desktop_mirror_height != viewport[3]) {
            if (!g_desktop_mirror_texture)
                p_glGenTextures(1, &g_desktop_mirror_texture);
            if (!g_desktop_mirror_framebuffer)
                p_glGenFramebuffers(1, &g_desktop_mirror_framebuffer);
            p_glBindTextureRaw(GL_TEXTURE_2D, g_desktop_mirror_texture);
            p_glTexParameteriRaw(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            p_glTexParameteriRaw(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            p_glTexImage2DRaw(GL_TEXTURE_2D, 0, GL_RGBA, viewport[2], viewport[3],
                              0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
            g_desktop_mirror_width = viewport[2];
            g_desktop_mirror_height = viewport[3];
        }
        p_glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
        p_glBindFramebuffer(GL_DRAW_FRAMEBUFFER, g_desktop_mirror_framebuffer);
        p_glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                 GL_TEXTURE_2D, g_desktop_mirror_texture, 0);
        if (p_glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) ==
            GL_FRAMEBUFFER_COMPLETE) {
            p_glBlitFramebuffer(
                viewport[0], viewport[1], viewport[0] + viewport[2],
                viewport[1] + viewport[3], 0, 0, viewport[2], viewport[3],
                GL_COLOR_BUFFER_BIT, GL_NEAREST);
            g_desktop_mirror_ready = 1;
        }
    }
    p_glBindFramebuffer(GL_READ_FRAMEBUFFER, (GLuint)previous_read);
    p_glBindFramebuffer(GL_DRAW_FRAMEBUFFER, (GLuint)previous_draw);
    p_glBindTextureRaw(GL_TEXTURE_2D, (GLuint)previous_texture);
    return status == GL_FRAMEBUFFER_COMPLETE;
}

int openxr_bridge_set_geometry_active(int active)
{
    update_scene_mode();
    /* In-round interfaces no longer disable the projection. Their authored
       pixels are submitted in a separate transparent OpenXR layer. */
    int value = active && !g_flat_scene_mode ? 1 : 0;
    LONG previous = InterlockedExchange(&g_geometry_active, value);
    if (previous && !value) {
        /* A completed round/menu is one monoscopic panel, exactly like the
           startup screen. Keeping two identical captures in a projection
           layer applies different eye poses/FOVs to the same pixels and is
           perceived as crossed double vision. Clear every stereo-only bit at
           the handoff; capture_flat_frame will publish the new panel. */
        g_eye_used_depth_stereo[0] = 0;
        g_eye_used_depth_stereo[1] = 0;
        g_projection_anchor_valid = 0;
        g_menu_overlay_ready = 0;
        g_menu_overlay_exact_alpha = 0;
        g_interface_alpha_desktop_ready = 0;
        xr_log("Gameplay-to-menu handoff: cleared stereo projection and stale UI state");
    }
    if (g_geometry_activity_logged != value) {
        xr_log(value ? "Presentation source: live stereo gameplay"
                     : "Presentation source: startup flat panel");
        g_geometry_activity_logged = value;
    }
    return value;
}

int openxr_bridge_capture_flat_frame(void)
{
    if (!g_gl_interop_ready || !g_geometry_eye_ready[0] ||
        !g_geometry_eye_ready[1]) return 0;
    ULONGLONG now = GetTickCount64();
    g_menu_overlay_exact_alpha = 0;
    /* Escape/Tab state changes precede the game's completed interface by one
       or more frames. Retain stereo gameplay until the transition has settled
       instead of publishing a partial menu and then flickering between it and
       the final frame. */
    if (!g_flat_scene_mode && g_interface_heavy &&
        g_interface_heavy_since_tick &&
        now - g_interface_heavy_since_tick < 120) {
        g_menu_overlay_ready = 0;
        return 1;
    }
    if (now - g_flat_capture_tick < 33) return 1;
    g_flat_capture_tick = now;
    if (!TryAcquireSRWLockExclusive(&g_interop_capture_lock)) return 0;

    if (!g_flat_scene_mode &&
        InterlockedCompareExchange(&g_geometry_active, 0, 0) &&
        g_native_mirror_ready &&
        g_menu_interop_objects[0] && g_menu_interop_objects[1]) {
        /* Pause, Tab and level-up interfaces are appended after the true-eye
           renderer returns. Capture both the completed desktop frame and the
           same-frame renderer baseline. A D3D shader can then transfer only
           their difference onto each genuine stereo eye. When the world hook
           has stopped (for example, between server rounds), bypass this path
           and publish the complete backbuffer below; otherwise the headset
           retains the last gameplay pair while the desktop receives only an
           incomplete transition layer. */
        HANDLE menu_objects[2] = {
            g_menu_interop_objects[0], g_menu_interop_objects[1]
        };
        if (!p_wglDXLockObjectsNV(g_interop_device, 2, menu_objects)) {
            g_menu_overlay_ready = 0;
            ReleaseSRWLockExclusive(&g_interop_capture_lock);
            return 0;
        }
        GLint previous_read = 0, previous_draw = 0, viewport[4] = {0};
        p_glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &previous_read);
        p_glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &previous_draw);
        p_glGetIntegerv(GL_VIEWPORT, viewport);
        int target_width = g_eyes[0].width;
        int target_height = g_eyes[0].height;
        int fitted_height = viewport[2] > 0
            ? (viewport[3] * target_width) / viewport[2] : target_height;
        if (fitted_height > target_height) fitted_height = target_height;
        int x = 0;
        int y = (target_height - fitted_height) / 2;
        g_content_x = x;
        g_content_y = y;
        g_content_width = target_width;
        g_content_height = fitted_height;
        int complete = 1;
        for (int frame = 0; frame < 2; ++frame) {
            p_glBindFramebuffer(GL_READ_FRAMEBUFFER,
                                frame == 0 ? 0 : g_native_mirror_framebuffer);
            p_glBindFramebuffer(GL_DRAW_FRAMEBUFFER, g_interop_framebuffer);
            p_glFramebufferTexture2D(
                GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                g_menu_interop_gl_textures[frame], 0);
            if (p_glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) !=
                GL_FRAMEBUFFER_COMPLETE) {
                complete = 0;
                break;
            }
            int source_width = frame == 0 ? viewport[2] : g_native_mirror_width;
            int source_height = frame == 0 ? viewport[3] : g_native_mirror_height;
            p_glBlitFramebuffer(
                0, 0, source_width, source_height,
                x, y + fitted_height, x + target_width, y,
                GL_COLOR_BUFFER_BIT, GL_LINEAR);
        }
        p_glFinish();
        p_glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                 GL_TEXTURE_2D, 0, 0);
        p_glBindFramebuffer(GL_READ_FRAMEBUFFER, (GLuint)previous_read);
        p_glBindFramebuffer(GL_DRAW_FRAMEBUFFER, (GLuint)previous_draw);
        int unlocked = p_wglDXUnlockObjectsNV(
            g_interop_device, 2, menu_objects);
        g_menu_overlay_ready = complete && unlocked;
        ReleaseSRWLockExclusive(&g_interop_capture_lock);
        return g_menu_overlay_ready;
    }

    g_menu_overlay_ready = 0;

    HANDLE objects[EYE_COUNT] = {g_interop_objects[0], g_interop_objects[1]};
    if (!p_wglDXLockObjectsNV(g_interop_device, EYE_COUNT, objects)) {
        ReleaseSRWLockExclusive(&g_interop_capture_lock);
        return 0;
    }
    GLint previous_read = 0, previous_draw = 0, viewport[4] = {0};
    p_glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &previous_read);
    p_glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &previous_draw);
    p_glGetIntegerv(GL_VIEWPORT, viewport);
    int target_width = g_eyes[0].width;
    int target_height = g_eyes[0].height;
    int fitted_height = viewport[2] > 0
        ? (viewport[3] * target_width) / viewport[2] : target_height;
    if (fitted_height > target_height) fitted_height = target_height;
    int x = 0;
    int y = (target_height - fitted_height) / 2;
    g_content_x = x;
    g_content_y = y;
    g_content_width = target_width;
    g_content_height = fitted_height;
    p_glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    int complete = 1;
    for (int eye = 0; eye < EYE_COUNT; ++eye) {
        p_glBindFramebuffer(GL_DRAW_FRAMEBUFFER, g_interop_framebuffer);
        p_glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                 GL_TEXTURE_2D, g_interop_gl_textures[eye], 0);
        if (p_glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) !=
            GL_FRAMEBUFFER_COMPLETE) {
            complete = 0;
            break;
        }
        p_glBlitFramebuffer(viewport[0], viewport[1],
                            viewport[0] + viewport[2], viewport[1] + viewport[3],
                            x, y + fitted_height, x + target_width, y,
                            GL_COLOR_BUFFER_BIT, GL_LINEAR);
    }
    p_glFinish();
    p_glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                             GL_TEXTURE_2D, 0, 0);
    p_glBindFramebuffer(GL_READ_FRAMEBUFFER, (GLuint)previous_read);
    p_glBindFramebuffer(GL_DRAW_FRAMEBUFFER, (GLuint)previous_draw);
    int unlocked = p_wglDXUnlockObjectsNV(g_interop_device, EYE_COUNT, objects);

    if (complete && unlocked) {
        LONG current = InterlockedCompareExchange(&g_present_pair_index, -1, -1);
        LONG submitted = InterlockedCompareExchange(&g_submit_pair_index, -1, -1);
        int next = current < 0 ? 0 : ((int)current + 1) % PAIR_BUFFER_COUNT;
        if (next == submitted) next = (next + 1) % PAIR_BUFFER_COUNT;
        for (int eye = 0; eye < EYE_COUNT; ++eye)
            ID3D11DeviceContext_CopyResource(
                g_context, (ID3D11Resource *)g_present_pair_textures[next][eye],
                (ID3D11Resource *)g_interop_textures[eye]);
        g_present_pair_pose_valid[next] = 0;
        InterlockedExchange(&g_present_pair_index, next);
        /* Capture the exact completed pause/results backbuffer at its handoff
           point. The OpenXR copy thread can legitimately retain a published
           pair between refreshes, so waiting to service this diagnostic there
           made repeated flat-menu proof requests unreliable. */
        capture_cursor_composite_proof(g_interop_textures[0]);
    }
    ReleaseSRWLockExclusive(&g_interop_capture_lock);
    return complete && unlocked;
}

static int copy_game_frame_to_swapchain(uint32_t eye, ID3D11Texture2D *destination)
{
    if (!g_gl_interop_ready) return 0;
    if (g_geometry_stereo_requested && g_geometry_eye_ready[eye] &&
        InterlockedCompareExchange(&g_geometry_active, 0, 0)) {
        /* Eye calibration is already baked into the genuine off-axis render.
           Copy the complete image unchanged so its pixels, submitted FOV and
           render-time pose all describe one camera. */
        LONG pair_index = InterlockedCompareExchange(&g_submit_pair_index, -1, -1);
        ID3D11Texture2D *pair_source = pair_index >= 0
            ? g_present_pair_textures[pair_index][eye] : g_interop_textures[eye];
        ID3D11DeviceContext_CopyResource(
            g_context, (ID3D11Resource *)destination,
            (ID3D11Resource *)pair_source);
        g_eye_used_depth_stereo[eye] = 1;
        XrRect2Di rect = pair_index >= 0
            ? g_present_pair_rect[pair_index][eye] : g_geometry_eye_rect[eye];
        if (!g_interface_heavy)
            draw_vr_cursor(destination, g_eyes[eye].width, g_eyes[eye].height, &rect);
        capture_composite_proof(destination, g_geometry_proof_marker,
                                g_geometry_presented_proof_path[eye],
                                &g_geometry_presented_proof_written[eye],
                                eye == 0 ? "presented geometry left eye"
                                         : "presented geometry right eye");
        if (g_geometry_proof_written[0] && g_geometry_proof_written[1] &&
            g_geometry_presented_proof_written[0] &&
            g_geometry_presented_proof_written[1] &&
            g_geometry_final_interface_proof_written[0] &&
            g_geometry_final_interface_proof_written[1] &&
            g_geometry_hud_base_proof_written[0] &&
            g_geometry_hud_base_proof_written[1])
            DeleteFileA(g_geometry_proof_marker);
        return 1;
    }
    if (g_geometry_stereo_requested && g_geometry_eye_ready[eye] &&
        !InterlockedCompareExchange(&g_geometry_active, 0, 0)) {
        /* Full-screen interfaces are drawn after the hooked world renderer.
           capture_flat_frame publishes that completed backbuffer into the
           private D3D pair immediately before the XR tick. Copying the
           published pair here avoids trying to acquire WGL interop objects a
           second time during Escape, Tab, results and level-up overlays. */
        LONG pair_index = InterlockedCompareExchange(&g_submit_pair_index, -1, -1);
        if (pair_index < 0)
            pair_index = InterlockedCompareExchange(&g_present_pair_index, -1, -1);
        ID3D11Texture2D *source = pair_index >= 0
            ? g_present_pair_textures[pair_index][eye] : g_interop_textures[eye];
        ID3D11DeviceContext_CopyResource(
            g_context, (ID3D11Resource *)destination,
            (ID3D11Resource *)source);
        /* Geometry has stopped, so this is an actual menu/results transition,
           not an in-round overlay. Both interop eyes contain the same authored
           backbuffer and must be presented once as a monoscopic quad. */
        g_eye_used_depth_stereo[eye] = 0;
        draw_vr_cursor(destination, g_eyes[eye].width, g_eyes[eye].height, NULL);
        /* Do not consume a menu proof request during the short transition
           before its complete/baseline textures are ready. Flat startup mode
           has no pending overlay, so it remains useful to capture there. */
        if (g_flat_scene_mode)
            capture_cursor_composite_proof(destination);
        return 1;
    }
    /* Before the first genuine eye pair exists, retain the generic desktop
       blit as a startup fallback. */
    ID3D11RenderTargetView *clear_target = NULL;
    D3D11_RENDER_TARGET_VIEW_DESC clear_desc;
    memset(&clear_desc, 0, sizeof(clear_desc));
    clear_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    clear_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
    HRESULT hr = ID3D11Device_CreateRenderTargetView(g_device,
                                                     (ID3D11Resource *)g_interop_textures[eye],
                                                     &clear_desc, &clear_target);
    if (FAILED(hr)) return 0;
    const float black[4] = {0, 0, 0, 1};
    ID3D11DeviceContext_ClearRenderTargetView(g_context, clear_target, black);
    ID3D11RenderTargetView_Release(clear_target);
    ID3D11DeviceContext_Flush(g_context);

    HANDLE object = g_interop_objects[eye];
    if (!p_wglDXLockObjectsNV(g_interop_device, 1, &object)) return 0;
    GLint previous_read = 0;
    GLint previous_draw = 0;
    GLint viewport[4] = {0};
    p_glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &previous_read);
    p_glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &previous_draw);
    p_glGetIntegerv(GL_VIEWPORT, viewport);
    p_glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    p_glBindFramebuffer(GL_DRAW_FRAMEBUFFER, g_interop_framebuffer);
    p_glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                             g_interop_gl_textures[eye], 0);
    GLenum framebuffer_status = p_glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);
    if (framebuffer_status == GL_FRAMEBUFFER_COMPLETE) {
        int target_width = g_eyes[eye].width;
        int target_height = g_eyes[eye].height;
        /* Keep the complete 16:9 interface inside the central, lens-safe area.
           Projection layers lose their extreme edges during headset distortion. */
        int content_width = target_width;
        int fitted_height = viewport[2] > 0 ? (viewport[3] * content_width) / viewport[2] : target_height;
        if (fitted_height > target_height) fitted_height = target_height;
        int x = (target_width - content_width) / 2;
        int y = (target_height - fitted_height) / 2;
        g_content_x = x;
        g_content_y = y;
        g_content_width = content_width;
        g_content_height = fitted_height;
        /* Reverse destination Y so the OpenGL desktop image has D3D's top-left orientation. */
        p_glBlitFramebuffer(viewport[0], viewport[1], viewport[0] + viewport[2], viewport[1] + viewport[3],
                            x, y + fitted_height, x + content_width, y,
                            GL_COLOR_BUFFER_BIT, GL_LINEAR);
    }
    p_glFinish();
    p_glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                             GL_TEXTURE_2D, 0, 0);
    p_glBindFramebuffer(GL_READ_FRAMEBUFFER, (GLuint)previous_read);
    p_glBindFramebuffer(GL_DRAW_FRAMEBUFFER, (GLuint)previous_draw);
    if (!p_wglDXUnlockObjectsNV(g_interop_device, 1, &object)) return 0;
    if (framebuffer_status != GL_FRAMEBUFFER_COMPLETE) {
        xr_log("Interop framebuffer incomplete: 0x%X", framebuffer_status);
        return 0;
    }
    g_eye_used_depth_stereo[eye] = render_stereo_frame(eye, destination);
    if (!g_eye_used_depth_stereo[eye])
        ID3D11DeviceContext_CopyResource(g_context, (ID3D11Resource *)destination,
                                         (ID3D11Resource *)g_interop_textures[eye]);
    draw_vr_cursor(destination, g_eyes[eye].width, g_eyes[eye].height, NULL);
    capture_cursor_composite_proof(destination);
    if (g_stereo_enabled && g_world_capture_ready)
        capture_composite_proof(destination, g_stereo_proof_marker,
                                g_stereo_proof_path[eye], &g_stereo_proof_written[eye],
                                eye == 0 ? "stereo left eye" : "stereo right eye");
    ID3D11DeviceContext_Flush(g_context);
    return 1;
}

static int choose_swapchain_format(int64_t *chosen)
{
    uint32_t count = 0;
    XrResult result = p_xrEnumerateSwapchainFormats(g_session, 0, &count, NULL);
    if (XR_FAILED(result) || !count) { fail("xrEnumerateSwapchainFormats(count)", result); return 0; }
    int64_t formats[64];
    if (count > 64) count = 64;
    result = p_xrEnumerateSwapchainFormats(g_session, count, &count, formats);
    if (XR_FAILED(result)) { fail("xrEnumerateSwapchainFormats", result); return 0; }
    const int64_t preferred[] = {DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,
                                 DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_B8G8R8A8_UNORM};
    for (size_t p = 0; p < sizeof(preferred) / sizeof(preferred[0]); ++p) {
        for (uint32_t f = 0; f < count; ++f) {
            if (formats[f] == preferred[p]) { *chosen = preferred[p]; return 1; }
        }
    }
    *chosen = formats[0];
    return 1;
}

static int choose_depth_swapchain_format(int64_t *chosen)
{
    uint32_t count = 0;
    XrResult result = p_xrEnumerateSwapchainFormats(g_session, 0, &count, NULL);
    if (XR_FAILED(result) || !count) return 0;
    int64_t formats[64];
    if (count > 64) count = 64;
    result = p_xrEnumerateSwapchainFormats(g_session, count, &count, formats);
    if (XR_FAILED(result)) return 0;
    const int64_t preferred[] = {
        DXGI_FORMAT_D32_FLOAT,
        DXGI_FORMAT_D24_UNORM_S8_UINT,
        DXGI_FORMAT_D16_UNORM
    };
    for (size_t p = 0; p < sizeof(preferred) / sizeof(preferred[0]); ++p) {
        for (uint32_t f = 0; f < count; ++f) {
            if (formats[f] == preferred[p]) {
                *chosen = preferred[p];
                return 1;
            }
        }
    }
    return 0;
}

static int create_depth_swapchains(void)
{
    if (!g_composition_depth_extension_enabled) return 1;
    if (!choose_depth_swapchain_format(&g_depth_swapchain_format)) {
        xr_log("OpenXR composition depth disabled: no compatible depth format");
        return 0;
    }
    for (uint32_t eye = 0; eye < EYE_COUNT; ++eye) {
        DepthSwapchain *depth = &g_depth_eyes[eye];
        depth->width = g_eyes[eye].width;
        depth->height = g_eyes[eye].height;
        XrSwapchainCreateInfo info = {XR_TYPE_SWAPCHAIN_CREATE_INFO};
        info.usageFlags = XR_SWAPCHAIN_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        info.format = g_depth_swapchain_format;
        info.sampleCount = 1;
        info.width = (uint32_t)depth->width;
        info.height = (uint32_t)depth->height;
        info.faceCount = 1;
        info.arraySize = 1;
        info.mipCount = 1;
        XrResult result = p_xrCreateSwapchain(g_session, &info, &depth->handle);
        if (XR_FAILED(result)) {
            xr_log("Depth swapchain %u creation failed: %d", eye, (int)result);
            return 0;
        }
        result = p_xrEnumerateSwapchainImages(depth->handle, 0, &depth->image_count, NULL);
        if (XR_FAILED(result) || depth->image_count > MAX_SWAPCHAIN_IMAGES) return 0;
        for (uint32_t i = 0; i < depth->image_count; ++i) {
            depth->images[i].type = XR_TYPE_SWAPCHAIN_IMAGE_D3D11_KHR;
            depth->images[i].next = NULL;
            depth->images[i].texture = NULL;
        }
        result = p_xrEnumerateSwapchainImages(
            depth->handle, depth->image_count, &depth->image_count,
            (XrSwapchainImageBaseHeader *)depth->images);
        if (XR_FAILED(result)) return 0;
        for (uint32_t i = 0; i < depth->image_count; ++i) {
            D3D11_DEPTH_STENCIL_VIEW_DESC desc;
            memset(&desc, 0, sizeof(desc));
            desc.Format = (DXGI_FORMAT)g_depth_swapchain_format;
            desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
            desc.Texture2DArray.MipSlice = 0;
            desc.Texture2DArray.FirstArraySlice = 0;
            desc.Texture2DArray.ArraySize = 1;
            HRESULT hr = ID3D11Device_CreateDepthStencilView(
                g_device, (ID3D11Resource *)depth->images[i].texture,
                &desc, &depth->targets[i]);
            if (FAILED(hr)) {
                xr_log("Depth target %u/%u creation failed: 0x%08lX",
                       eye, i, (unsigned long)hr);
                return 0;
            }
        }
        xr_log("Eye %u depth swapchain: %dx%d, images=%u, DXGI format=%lld",
               eye, depth->width, depth->height, depth->image_count,
               (long long)g_depth_swapchain_format);
    }
    return 1;
}

static int create_interface_swapchain(void)
{
    EyeSwapchain *swapchain = &g_interface_layer;
    swapchain->width = 1536;
    swapchain->height = 864;
    XrSwapchainCreateInfo info = {XR_TYPE_SWAPCHAIN_CREATE_INFO};
    info.createFlags = 0;
    info.usageFlags = XR_SWAPCHAIN_USAGE_SAMPLED_BIT |
                      XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;
    info.format = g_swapchain_format;
    info.sampleCount = 1;
    info.width = (uint32_t)swapchain->width;
    info.height = (uint32_t)swapchain->height;
    info.faceCount = 1;
    info.arraySize = 1;
    info.mipCount = 1;
    XrResult result = p_xrCreateSwapchain(g_session, &info, &swapchain->handle);
    if (XR_FAILED(result)) {
        xr_log("Transparent interface swapchain creation failed: %d", (int)result);
        swapchain->handle = XR_NULL_HANDLE;
        return 0;
    }
    result = p_xrEnumerateSwapchainImages(
        swapchain->handle, 0, &swapchain->image_count, NULL);
    if (XR_FAILED(result) || !swapchain->image_count ||
        swapchain->image_count > MAX_SWAPCHAIN_IMAGES) {
        xr_log("Transparent interface swapchain image count failed: %d count=%u",
               (int)result, swapchain->image_count);
        swapchain->handle = XR_NULL_HANDLE;
        return 0;
    }
    for (uint32_t image = 0; image < swapchain->image_count; ++image) {
        swapchain->images[image].type = XR_TYPE_SWAPCHAIN_IMAGE_D3D11_KHR;
        swapchain->images[image].next = NULL;
        swapchain->images[image].texture = NULL;
    }
    result = p_xrEnumerateSwapchainImages(
        swapchain->handle, swapchain->image_count, &swapchain->image_count,
        (XrSwapchainImageBaseHeader *)swapchain->images);
    if (XR_FAILED(result)) {
        xr_log("Transparent interface swapchain enumeration failed: %d", (int)result);
        swapchain->handle = XR_NULL_HANDLE;
        return 0;
    }
    for (uint32_t image = 0; image < swapchain->image_count; ++image) {
        D3D11_RENDER_TARGET_VIEW_DESC target_desc;
        memset(&target_desc, 0, sizeof(target_desc));
        target_desc.Format = (DXGI_FORMAT)g_swapchain_format;
        target_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
        target_desc.Texture2DArray.ArraySize = 1;
        HRESULT hr = ID3D11Device_CreateRenderTargetView(
            g_device, (ID3D11Resource *)swapchain->images[image].texture,
            &target_desc, &swapchain->targets[image]);
        if (FAILED(hr)) {
            xr_log("Transparent interface target %u failed: 0x%08lX",
                   image, (unsigned long)hr);
            swapchain->handle = XR_NULL_HANDLE;
            return 0;
        }
    }
    xr_log("Transparent interface swapchain: %dx%d images=%u",
           swapchain->width, swapchain->height, swapchain->image_count);
    return 1;
}

static int create_swapchains(void)
{
    int64_t format;
    if (!choose_swapchain_format(&format)) return 0;
    g_swapchain_format = format;
    for (uint32_t eye = 0; eye < EYE_COUNT; ++eye) {
        EyeSwapchain *swapchain = &g_eyes[eye];
        int32_t recommended_width = (int32_t)g_view_config[eye].recommendedImageRectWidth;
        int32_t recommended_height = (int32_t)g_view_config[eye].recommendedImageRectHeight;
        swapchain->width = recommended_width;
        swapchain->height = recommended_height;
        /* Quest Link's 2272x2464 recommendation made this legacy two-pass
           renderer fall to 24-27 fps. Preserve aspect ratio while capping the
           pixel cost for this comfort candidate. */
        if (swapchain->width > 1536) {
            float scale = 1536.0f / (float)swapchain->width;
            swapchain->width = 1536;
            swapchain->height = ((int32_t)((float)swapchain->height * scale + 0.5f)) & ~1;
            xr_log("Eye %u comfort resolution: recommended %dx%d -> %dx%d", eye,
                   recommended_width, recommended_height,
                   swapchain->width, swapchain->height);
        }
        XrSwapchainCreateInfo info = {XR_TYPE_SWAPCHAIN_CREATE_INFO};
        info.usageFlags = XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;
        info.format = format;
        info.sampleCount = 1;
        info.width = (uint32_t)swapchain->width;
        info.height = (uint32_t)swapchain->height;
        info.faceCount = 1;
        info.arraySize = 1;
        info.mipCount = 1;
        XrResult result = p_xrCreateSwapchain(g_session, &info, &swapchain->handle);
        if (XR_FAILED(result)) { fail("xrCreateSwapchain", result); return 0; }
        result = p_xrEnumerateSwapchainImages(swapchain->handle, 0, &swapchain->image_count, NULL);
        if (XR_FAILED(result) || swapchain->image_count > MAX_SWAPCHAIN_IMAGES) {
            fail("xrEnumerateSwapchainImages(count)", result); return 0;
        }
        for (uint32_t i = 0; i < swapchain->image_count; ++i) {
            swapchain->images[i].type = XR_TYPE_SWAPCHAIN_IMAGE_D3D11_KHR;
            swapchain->images[i].next = NULL;
            swapchain->images[i].texture = NULL;
        }
        result = p_xrEnumerateSwapchainImages(swapchain->handle, swapchain->image_count,
                                              &swapchain->image_count,
                                              (XrSwapchainImageBaseHeader *)swapchain->images);
        if (XR_FAILED(result)) { fail("xrEnumerateSwapchainImages", result); return 0; }
        for (uint32_t i = 0; i < swapchain->image_count; ++i) {
            D3D11_RENDER_TARGET_VIEW_DESC target_desc;
            memset(&target_desc, 0, sizeof(target_desc));
            target_desc.Format = (DXGI_FORMAT)g_swapchain_format;
            target_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
            target_desc.Texture2DArray.MipSlice = 0;
            target_desc.Texture2DArray.FirstArraySlice = 0;
            target_desc.Texture2DArray.ArraySize = 1;
            HRESULT hr = ID3D11Device_CreateRenderTargetView(
                g_device, (ID3D11Resource *)swapchain->images[i].texture,
                &target_desc, &swapchain->targets[i]);
            if (FAILED(hr)) {
                xr_log("ERROR cached CreateRenderTargetView HRESULT=0x%08lX image=%u eye=%u",
                       (unsigned long)hr, i, eye);
                return 0;
            }
        }
        xr_log("Eye %u swapchain: %dx%d, images=%u, DXGI format=%lld", eye,
               swapchain->width, swapchain->height, swapchain->image_count, (long long)format);
    }
    if (!create_depth_swapchains()) {
        g_composition_depth_extension_enabled = 0;
        xr_log("OpenXR composition depth unavailable; retaining color-only projection");
    }
    if (!create_interface_swapchain())
        xr_log("Transparent interface layer unavailable; gameplay remains stereo without menu overlay");
    return 1;
}

static int initialize_bridge(const char *base_directory)
{
    snprintf(g_log_path, sizeof(g_log_path), "%s\\SkillshotVR-XR.log", base_directory);
    snprintf(g_cursor_proof_marker, sizeof(g_cursor_proof_marker),
             "%s\\SkillshotVR-cursor-proof.txt", base_directory);
    snprintf(g_cursor_proof_path, sizeof(g_cursor_proof_path),
             "%s\\SkillshotVR-cursor-proof.bmp", base_directory);
    snprintf(g_menu_complete_proof_path, sizeof(g_menu_complete_proof_path),
             "%s\\SkillshotVR-menu-complete-proof.bmp", base_directory);
    snprintf(g_menu_baseline_proof_path, sizeof(g_menu_baseline_proof_path),
             "%s\\SkillshotVR-menu-baseline-proof.bmp", base_directory);
    snprintf(g_stereo_proof_marker, sizeof(g_stereo_proof_marker),
             "%s\\SkillshotVR-stereo-proof.txt", base_directory);
    snprintf(g_stereo_proof_path[0], sizeof(g_stereo_proof_path[0]),
             "%s\\SkillshotVR-stereo-left.bmp", base_directory);
    snprintf(g_stereo_proof_path[1], sizeof(g_stereo_proof_path[1]),
             "%s\\SkillshotVR-stereo-right.bmp", base_directory);
    snprintf(g_geometry_proof_marker, sizeof(g_geometry_proof_marker),
             "%s\\SkillshotCityVR-geometry-proof.txt", base_directory);
    snprintf(g_geometry_proof_path[0], sizeof(g_geometry_proof_path[0]),
             "%s\\SkillshotCityVR-geometry-left.bmp", base_directory);
    snprintf(g_geometry_proof_path[1], sizeof(g_geometry_proof_path[1]),
             "%s\\SkillshotCityVR-geometry-right.bmp", base_directory);
    snprintf(g_geometry_presented_proof_path[0],
             sizeof(g_geometry_presented_proof_path[0]),
             "%s\\SkillshotCityVR-presented-left.bmp", base_directory);
    snprintf(g_geometry_presented_proof_path[1],
             sizeof(g_geometry_presented_proof_path[1]),
             "%s\\SkillshotCityVR-presented-right.bmp", base_directory);
    snprintf(g_geometry_final_interface_proof_path[0],
             sizeof(g_geometry_final_interface_proof_path[0]),
             "%s\\SkillshotCityVR-final-interface-left.bmp", base_directory);
    snprintf(g_geometry_final_interface_proof_path[1],
             sizeof(g_geometry_final_interface_proof_path[1]),
             "%s\\SkillshotCityVR-final-interface-right.bmp", base_directory);
    snprintf(g_geometry_hud_base_proof_path[0],
             sizeof(g_geometry_hud_base_proof_path[0]),
             "%s\\SkillshotCityVR-hud-base-left.bmp", base_directory);
    snprintf(g_geometry_hud_base_proof_path[1],
             sizeof(g_geometry_hud_base_proof_path[1]),
             "%s\\SkillshotCityVR-hud-base-right.bmp", base_directory);
    snprintf(g_interface_alpha_proof_path,
             sizeof(g_interface_alpha_proof_path),
             "%s\\SkillshotCityVR-interface-alpha.bmp", base_directory);
    snprintf(g_interface_alpha_mask_proof_path,
             sizeof(g_interface_alpha_mask_proof_path),
             "%s\\SkillshotCityVR-interface-alpha-mask.bmp", base_directory);
    snprintf(g_stereo_diagnostic_path[0], sizeof(g_stereo_diagnostic_path[0]),
             "%s\\SkillshotCityVR-stereo-side-by-side.bmp", base_directory);
    snprintf(g_stereo_diagnostic_path[1], sizeof(g_stereo_diagnostic_path[1]),
             "%s\\SkillshotCityVR-stereo-overlay.bmp", base_directory);
    snprintf(g_stereo_diagnostic_path[2], sizeof(g_stereo_diagnostic_path[2]),
             "%s\\SkillshotCityVR-stereo-anaglyph.bmp", base_directory);
    snprintf(g_stereo_diagnostic_path[3], sizeof(g_stereo_diagnostic_path[3]),
             "%s\\SkillshotCityVR-stereo-difference.bmp", base_directory);
    snprintf(g_stereo_diagnostic_path[4], sizeof(g_stereo_diagnostic_path[4]),
             "%s\\SkillshotCityVR-stereo-lens-approximation.bmp", base_directory);
    snprintf(g_stereo_diagnostic_report_path,
             sizeof(g_stereo_diagnostic_report_path),
             "%s\\SkillshotCityVR-stereo-diagnostics.txt", base_directory);
    snprintf(g_tabletop_recenter_marker, sizeof(g_tabletop_recenter_marker),
             "%s\\SkillshotCityVR-tabletop-recenter.txt", base_directory);
    snprintf(g_pose_trace_marker, sizeof(g_pose_trace_marker),
             "%s\\SkillshotCityVR-pose-trace.txt", base_directory);
    snprintf(g_pose_trace_path, sizeof(g_pose_trace_path),
             "%s\\SkillshotCityVR-pose-trace.csv", base_directory);
    char live_pose_marker[MAX_PATH];
    snprintf(live_pose_marker, sizeof(live_pose_marker),
             "%s\\SkillshotCityVR-live-pose-test.txt", base_directory);
    g_live_pose_diagnostic = GetFileAttributesA(live_pose_marker) != INVALID_FILE_ATTRIBUTES;
    char calibration_controls[MAX_PATH];
    snprintf(calibration_controls, sizeof(calibration_controls),
             "%s\\SkillshotVR-calibration-controls.txt", base_directory);
    g_calibration_keys_enabled = GetFileAttributesA(calibration_controls) != INVALID_FILE_ATTRIBUTES;
    snprintf(g_game_log_path, sizeof(g_game_log_path), "%s\\data\\log.txt", base_directory);
    char stereo_marker[MAX_PATH];
    snprintf(stereo_marker, sizeof(stereo_marker), "%s\\SkillshotVR-stereo-test.txt", base_directory);
    g_stereo_enabled = GetFileAttributesA(stereo_marker) != INVALID_FILE_ATTRIBUTES;
    char geometry_marker[MAX_PATH];
    snprintf(geometry_marker, sizeof(geometry_marker),
             "%s\\SkillshotCityVR-geometry-stereo.txt", base_directory);
    g_geometry_stereo_requested = GetFileAttributesA(geometry_marker) != INVALID_FILE_ATTRIBUTES;
    DeleteFileA(g_log_path);
    load_display_config(base_directory);
    xr_log("Starting embedded OpenXR bridge");
    xr_log("Geometry stereo requested: %s", g_geometry_stereo_requested ? "yes" : "no");
    xr_log("Flat display: width=%.2fm distance=%.2fm", g_display_width, g_display_distance);
    xr_log("Calibration: eye alignment=%.4f depth strength=%.4f (arrows adjust alignment)",
           g_eye_alignment, g_stereo_strength);
    if (g_live_pose_diagnostic)
        xr_log("DIAGNOSTIC: live projection pose active (stored-pose timewarp bypassed)");
    write_calibration_value("loaded");
    if (!load_openxr(base_directory)) return 0;

    const char *extensions[3] = {XR_KHR_D3D11_ENABLE_EXTENSION_NAME, NULL, NULL};
    uint32_t extension_count = 1;
    g_display_refresh_extension_enabled =
        instance_extension_available(XR_FB_DISPLAY_REFRESH_RATE_EXTENSION_NAME);
    if (g_display_refresh_extension_enabled)
        extensions[extension_count++] = XR_FB_DISPLAY_REFRESH_RATE_EXTENSION_NAME;
    int composition_depth_available =
        instance_extension_available(XR_KHR_COMPOSITION_LAYER_DEPTH_EXTENSION_NAME);
    /* The game's alternating eye/depth captures cannot provide temporally
       matched compositor depth under physical Link scheduling yet. Keep true
       stereo color, but disable depth reprojection to avoid world warping and
       remove the second WGL/D3D shared-resource failure path. */
    g_composition_depth_extension_enabled = 0;
    XrInstanceCreateInfo instance_info = {XR_TYPE_INSTANCE_CREATE_INFO};
    strncpy(instance_info.applicationInfo.applicationName, "Skillshot City VR", XR_MAX_APPLICATION_NAME_SIZE - 1);
    instance_info.applicationInfo.applicationVersion = 1;
    strncpy(instance_info.applicationInfo.engineName, "SkillshotVR Bridge", XR_MAX_ENGINE_NAME_SIZE - 1);
    instance_info.applicationInfo.engineVersion = 1;
    instance_info.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;
    instance_info.enabledExtensionCount = extension_count;
    instance_info.enabledExtensionNames = extensions;
    XrResult result = p_xrCreateInstance(&instance_info, &g_instance);
    if (XR_FAILED(result)) { fail("xrCreateInstance", result); return 0; }
    xr_log("OpenXR composition depth: disabled for color-only stability (extension exposed=%s)",
           composition_depth_available ? "yes" : "no");

    XrSystemGetInfo system_info = {XR_TYPE_SYSTEM_GET_INFO};
    system_info.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
    result = p_xrGetSystem(g_instance, &system_info, &g_system);
    if (XR_FAILED(result)) { fail("xrGetSystem", result); return 0; }

    XrSystemProperties properties = {XR_TYPE_SYSTEM_PROPERTIES};
    result = p_xrGetSystemProperties(g_instance, g_system, &properties);
    if (XR_FAILED(result)) { fail("xrGetSystemProperties", result); return 0; }
    xr_log("Runtime system: '%s', vendor=%u, max=%ux%u layers=%u", properties.systemName,
           properties.vendorId, properties.graphicsProperties.maxSwapchainImageWidth,
           properties.graphicsProperties.maxSwapchainImageHeight,
           properties.graphicsProperties.maxLayerCount);

    result = p_xrGetInstanceProcAddr(g_instance, "xrGetD3D11GraphicsRequirementsKHR",
                                     (PFN_xrVoidFunction *)&p_xrGetD3D11GraphicsRequirementsKHR);
    if (XR_FAILED(result) || !p_xrGetD3D11GraphicsRequirementsKHR) {
        fail("xrGetInstanceProcAddr(D3D11 requirements)", result); return 0;
    }
    XrGraphicsRequirementsD3D11KHR requirements = {XR_TYPE_GRAPHICS_REQUIREMENTS_D3D11_KHR};
    result = p_xrGetD3D11GraphicsRequirementsKHR(g_instance, g_system, &requirements);
    if (XR_FAILED(result)) { fail("xrGetD3D11GraphicsRequirementsKHR", result); return 0; }
    if (!create_d3d_device(&requirements)) return 0;

    XrGraphicsBindingD3D11KHR binding = {XR_TYPE_GRAPHICS_BINDING_D3D11_KHR};
    binding.device = g_device;
    XrSessionCreateInfo session_info = {XR_TYPE_SESSION_CREATE_INFO};
    session_info.next = &binding;
    session_info.systemId = g_system;
    result = p_xrCreateSession(g_instance, &session_info, &g_session);
    if (XR_FAILED(result)) { fail("xrCreateSession", result); return 0; }
    request_stable_display_refresh_rate();

    XrReferenceSpaceCreateInfo space_info = {XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
    space_info.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
    space_info.poseInReferenceSpace.orientation.w = 1.0f;
    result = p_xrCreateReferenceSpace(g_session, &space_info, &g_space);
    if (XR_FAILED(result)) { fail("xrCreateReferenceSpace", result); return 0; }
    space_info.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW;
    result = p_xrCreateReferenceSpace(g_session, &space_info, &g_view_space);
    if (XR_FAILED(result)) {
        g_view_space = XR_NULL_HANDLE;
        xr_log("Head-locked interface reference space unavailable: %d", (int)result);
    }

    uint32_t view_count = 0;
    result = p_xrEnumerateViewConfigurationViews(g_instance, g_system,
                                                  XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,
                                                  0, &view_count, NULL);
    if (XR_FAILED(result) || view_count != EYE_COUNT) { fail("stereo view configuration", result); return 0; }
    for (uint32_t eye = 0; eye < EYE_COUNT; ++eye) {
        g_view_config[eye].type = XR_TYPE_VIEW_CONFIGURATION_VIEW;
        g_views[eye].type = XR_TYPE_VIEW;
    }
    result = p_xrEnumerateViewConfigurationViews(g_instance, g_system,
                                                  XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,
                                                  EYE_COUNT, &view_count, g_view_config);
    if (XR_FAILED(result) || !create_swapchains()) return 0;

    initialize_gl_interop();
    if (g_stereo_enabled) initialize_stereo_compositor();
    if (g_composition_depth_extension_enabled && !initialize_depth_copy_compositor()) {
        g_composition_depth_extension_enabled = 0;
        xr_log("OpenXR composition depth disabled: copy compositor initialization failed");
    }
    initialize_vr_cursor();
    snprintf(g_status, sizeof(g_status), "session created for %.130s", properties.systemName);
    xr_log("OpenXR session created; waiting for READY event");
    return 1;
}

static void poll_events(void)
{
    XrEventDataBuffer event;
    memset(&event, 0, sizeof(event));
    event.type = XR_TYPE_EVENT_DATA_BUFFER;
    while (p_xrPollEvent(g_instance, &event) == XR_SUCCESS) {
        if (event.type == XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED) {
            const XrEventDataSessionStateChanged *changed = (const XrEventDataSessionStateChanged *)&event;
            g_session_state = changed->state;
            xr_log("Session state changed to %d", (int)g_session_state);
            if (g_session_state == XR_SESSION_STATE_READY && !g_running) {
                XrSessionBeginInfo begin = {XR_TYPE_SESSION_BEGIN_INFO};
                begin.primaryViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
                XrResult result = p_xrBeginSession(g_session, &begin);
                if (XR_SUCCEEDED(result)) {
                    g_running = 1;
                    g_session_stopped_tick = 0;
                    g_session_stall_logged = 0;
                    snprintf(g_status, sizeof(g_status), "running stereo (%lu frames)", g_submitted_frames);
                    xr_log("Stereo session began");
                } else fail("xrBeginSession", result);
            } else if (g_session_state == XR_SESSION_STATE_STOPPING && g_running) {
                p_xrEndSession(g_session);
                g_running = 0;
                g_session_stopped_tick = GetTickCount64();
                g_session_stall_logged = 0;
                xr_log("Stereo session stopped");
            }
        }
        memset(&event, 0, sizeof(event));
        event.type = XR_TYPE_EVENT_DATA_BUFFER;
    }
}

static int submit_diagnostic_frame(void)
{
    ULONGLONG timing_start = GetTickCount64();
    XrFrameWaitInfo wait_info = {XR_TYPE_FRAME_WAIT_INFO};
    XrFrameState frame_state = {XR_TYPE_FRAME_STATE};
    XrResult result = p_xrWaitFrame(g_session, &wait_info, &frame_state);
    if (XR_FAILED(result)) { fail("xrWaitFrame", result); return 0; }
    ULONGLONG timing_waited = GetTickCount64();
    XrFrameBeginInfo begin_info = {XR_TYPE_FRAME_BEGIN_INFO};
    result = p_xrBeginFrame(g_session, &begin_info);
    if (XR_FAILED(result)) { fail("xrBeginFrame", result); return 0; }

    XrViewLocateInfo locate = {XR_TYPE_VIEW_LOCATE_INFO};
    locate.viewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
    locate.displayTime = frame_state.predictedDisplayTime;
    locate.space = g_space;
    XrViewState view_state = {XR_TYPE_VIEW_STATE};
    uint32_t view_count = 0;
    AcquireSRWLockExclusive(&g_view_lock);
    result = p_xrLocateViews(g_session, &locate, &view_state, EYE_COUNT, &view_count, g_views);
    ReleaseSRWLockExclusive(&g_view_lock);
    ULONGLONG timing_located = GetTickCount64();
    if (g_live_pose_diagnostic && view_count == EYE_COUNT) {
        static ULONGLONG pose_trace_tick;
        ULONGLONG pose_now = GetTickCount64();
        if (pose_now - pose_trace_tick >= 500) {
            XrPosef pose = g_views[0].pose;
            pose.position.x = (g_views[0].pose.position.x +
                               g_views[1].pose.position.x) * 0.5f;
            pose.position.y = (g_views[0].pose.position.y +
                               g_views[1].pose.position.y) * 0.5f;
            pose.position.z = (g_views[0].pose.position.z +
                               g_views[1].pose.position.z) * 0.5f;
            xr_log("POSE raw center %.4f %.4f %.4f orientation %.5f %.5f %.5f %.5f",
                   pose.position.x, pose.position.y, pose.position.z,
                   pose.orientation.x, pose.orientation.y,
                   pose.orientation.z, pose.orientation.w);
            pose_trace_tick = pose_now;
        }
    }

    XrCompositionLayerProjectionView projection_views[EYE_COUNT];
    memset(projection_views, 0, sizeof(projection_views));
    XrCompositionLayerDepthInfoKHR depth_info[EYE_COUNT];
    memset(depth_info, 0, sizeof(depth_info));
    int submitted_depth[EYE_COUNT] = {0, 0};
    /* Completed stereo pairs are published into the private triple buffer by
       the game thread as soon as eye 0 finishes. Selection here is lock-free. */
    ULONGLONG timing_snapshotted = GetTickCount64();
    LONG coherent_pair_index = g_geometry_stereo_requested && g_present_pair_generation > 0
        ? InterlockedCompareExchange(&g_present_pair_index, -1, -1) : -1;
    int coherent_pair_completed_interface =
        coherent_pair_index >= 0 &&
        g_present_pair_completed_interface[coherent_pair_index];
    InterlockedExchange(&g_submit_pair_index, coherent_pair_index);
    if (!g_composition_depth_extension_enabled && coherent_pair_index >= 0 &&
        g_present_pair_pose_valid[coherent_pair_index]) {
        /* The retained pair carries its raw render-time pose. Submission below
           converts only the motion since that pose into the same reflected
           camera convention used by the OpenGL world. */
        static int color_only_pose_logged;
        if (!color_only_pose_logged) {
            xr_log("Color-only projection uses reflected incremental timewarp pose");
            color_only_pose_logged = 1;
        }
        static ULONGLONG pair_age_log_tick;
        ULONGLONG now = GetTickCount64();
        if (now - pair_age_log_tick >= 2000) {
            ULONGLONG pose_tick = g_present_pair_pose_tick[coherent_pair_index];
            ULONGLONG publish_tick = g_present_pair_publish_tick[coherent_pair_index];
            xr_log("Stereo pair timing: eye-span=%llums submit-age=%llums",
                   publish_tick >= pose_tick ? publish_tick - pose_tick : 0,
                   now >= publish_tick ? now - publish_tick : 0);
            pair_age_log_tick = now;
        }
    }
    if (XR_SUCCEEDED(result) && frame_state.shouldRender && view_count == EYE_COUNT) {
        for (uint32_t eye = 0; eye < EYE_COUNT; ++eye) {
            EyeSwapchain *swapchain = &g_eyes[eye];
            uint32_t image_index = 0;
            XrSwapchainImageAcquireInfo acquire = {XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};
            result = p_xrAcquireSwapchainImage(swapchain->handle, &acquire, &image_index);
            if (XR_FAILED(result)) { fail("xrAcquireSwapchainImage", result); return 0; }
            XrSwapchainImageWaitInfo image_wait = {XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};
            image_wait.timeout = XR_INFINITE_DURATION;
            result = p_xrWaitSwapchainImage(swapchain->handle, &image_wait);
            if (XR_FAILED(result)) { fail("xrWaitSwapchainImage", result); return 0; }

            ID3D11RenderTargetView *target = swapchain->targets[image_index];
            const float cleared_eye[4] = {0.004f, 0.007f, 0.014f, 1.0f};
            /* NV WGL interop and OpenXR both submit work through this one
               immediate D3D11 context. Prefer the fully serialized path; if
               WGL currently owns an interop image, the immutable published
               pair below permits a state-neutral copy without waiting. */
            int copied_eye = 0;
            if (TryAcquireSRWLockExclusive(&g_interop_capture_lock)) {
                ID3D11DeviceContext_ClearRenderTargetView(
                    g_context, target, cleared_eye);
                copied_eye = copy_game_frame_to_swapchain(
                    eye, swapchain->images[image_index].texture);
                if (!copied_eye)
                    ID3D11DeviceContext_ClearRenderTargetView(
                        g_context, target, cleared_eye);
                ReleaseSRWLockExclusive(&g_interop_capture_lock);
            } else if (coherent_pair_index >= 0 &&
                       g_present_pair_textures[coherent_pair_index][eye]) {
                /* The published triple-buffer entry cannot be selected as the
                   game thread's next write target while g_submit_pair_index
                   names it. CopyResource is a single state-neutral immediate
                   context call, and D3D11 multithread protection serializes
                   it without disturbing the resolver's shader state. Avoiding
                   the broader WGL ownership lock here prevents one busy game
                   capture from turning a smooth head update into a missed
                   72-Hz compositor interval. Cursor/proof drawing resumes on
                   the next ordinary locked frame. */
                ID3D11DeviceContext_CopyResource(
                    g_context,
                    (ID3D11Resource *)swapchain->images[image_index].texture,
                    (ID3D11Resource *)g_present_pair_textures
                        [coherent_pair_index][eye]);
                g_eye_used_depth_stereo[eye] = 1;
                copied_eye = 1;
                static LONG lock_avoidance_copies;
                LONG avoided = InterlockedIncrement(&lock_avoidance_copies);
                if (avoided == 1 || (avoided % 720) == 0)
                    xr_log("Headset cadence retained with published-pair fast copy "
                           "during render-lock contention (count=%ld)", avoided);
            } else {
                /* Startup has no immutable published pair yet, so initialize
                   the swapchain through the fully serialized path once. */
                AcquireSRWLockExclusive(&g_interop_capture_lock);
                ID3D11DeviceContext_ClearRenderTargetView(
                    g_context, target, cleared_eye);
                copied_eye = copy_game_frame_to_swapchain(
                    eye, swapchain->images[image_index].texture);
                if (!copied_eye)
                    ID3D11DeviceContext_ClearRenderTargetView(
                        g_context, target, cleared_eye);
                ReleaseSRWLockExclusive(&g_interop_capture_lock);
            }
            if (!copied_eye) {
                static ULONGLONG copy_failure_log_tick;
                ULONGLONG now = GetTickCount64();
                if (now - copy_failure_log_tick >= 2000) {
                    xr_log("Headset frame copy failed eye=%u; retaining neutral fallback", eye);
                    copy_failure_log_tick = now;
                }
            }
            /* Meta's physical Link compositor accepted the transparent quad
               swapchain but dropped that layer from the headset image.  The
               captured interface itself is exact and world-free, so blend it
               over each projection eye before release.  This keeps both eyes
               identical for UI, preserves the stereo world outside authored
               alpha, and removes the runtime-dependent extra layer. */
            if (copied_eye && g_interface_heavy && g_menu_overlay_ready &&
                g_menu_overlay_exact_alpha) {
                AcquireSRWLockExclusive(&g_interop_capture_lock);
                XrRect2Di interface_rect;
                if (render_menu_overlay_layer(
                        target, swapchain->images[image_index].texture,
                        swapchain->width, swapchain->height, (int)eye, 1,
                        &interface_rect)) {
                    draw_vr_cursor(swapchain->images[image_index].texture,
                                   swapchain->width, swapchain->height,
                                   &interface_rect);
                    capture_composite_proof(
                        swapchain->images[image_index].texture,
                        g_geometry_proof_marker,
                        g_geometry_final_interface_proof_path[eye],
                        &g_geometry_final_interface_proof_written[eye],
                        eye == 0 ? "final interface-composited left eye"
                                 : "final interface-composited right eye");
                    start_stereo_diagnostic_if_ready();
                }
                ReleaseSRWLockExclusive(&g_interop_capture_lock);
            }
            XrSwapchainImageReleaseInfo release = {XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
            result = p_xrReleaseSwapchainImage(swapchain->handle, &release);
            if (XR_FAILED(result)) { fail("xrReleaseSwapchainImage", result); return 0; }

            projection_views[eye].type = XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW;
            projection_views[eye].pose = g_views[eye].pose;
            if (!g_live_pose_diagnostic && coherent_pair_index >= 0 &&
                g_present_pair_pose_valid[coherent_pair_index]) {
                XrPosef render_pose = g_present_pair_pose[coherent_pair_index][eye];
                XrPosef live_pose = g_views[eye].pose;
                float render_position[3] = {
                    render_pose.position.x, render_pose.position.y,
                    render_pose.position.z
                };
                float render_orientation[4] = {
                    render_pose.orientation.x, render_pose.orientation.y,
                    render_pose.orientation.z, render_pose.orientation.w
                };
                float live_position[3] = {
                    live_pose.position.x, live_pose.position.y,
                    live_pose.position.z
                };
                float live_orientation[4] = {
                    live_pose.orientation.x, live_pose.orientation.y,
                    live_pose.orientation.z, live_pose.orientation.w
                };
                float submitted_position[3], submitted_orientation[4];
                float anchor_orientation[4] = {
                    g_projection_anchor_pose[0].orientation.x,
                    g_projection_anchor_pose[0].orientation.y,
                    g_projection_anchor_pose[0].orientation.z,
                    g_projection_anchor_pose[0].orientation.w
                };
                vr_reflected_timewarp_submission_pose_f(
                    anchor_orientation,
                    render_position, render_orientation,
                    live_position, live_orientation,
                    submitted_position, submitted_orientation);
                projection_views[eye].pose.position.x = submitted_position[0];
                projection_views[eye].pose.position.y = submitted_position[1];
                projection_views[eye].pose.position.z = submitted_position[2];
                projection_views[eye].pose.orientation.x = submitted_orientation[0];
                projection_views[eye].pose.orientation.y = submitted_orientation[1];
                projection_views[eye].pose.orientation.z = submitted_orientation[2];
                projection_views[eye].pose.orientation.w = submitted_orientation[3];
            }
            projection_views[eye].fov = coherent_pair_index >= 0 &&
                                        g_present_pair_pose_valid[coherent_pair_index]
                ? g_present_pair_fov[coherent_pair_index][eye] : g_views[eye].fov;
            projection_views[eye].subImage.swapchain = swapchain->handle;
            XrRect2Di projection_rect = coherent_pair_index >= 0
                ? g_present_pair_rect[coherent_pair_index][eye]
                : g_geometry_eye_rect[eye];
            if (projection_rect.extent.width <= 0 || projection_rect.extent.height <= 0) {
                projection_rect.offset.x = projection_rect.offset.y = 0;
                projection_rect.extent.width = swapchain->width;
                projection_rect.extent.height = swapchain->height;
            }
            projection_views[eye].subImage.imageRect = projection_rect;
            projection_views[eye].subImage.imageArrayIndex = 0;

            if (g_composition_depth_extension_enabled && coherent_pair_index >= 0 &&
                g_present_pair_depth_valid[coherent_pair_index]) {
                DepthSwapchain *depth = &g_depth_eyes[eye];
                uint32_t depth_index = 0;
                XrSwapchainImageAcquireInfo depth_acquire = {
                    XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO
                };
                XrResult depth_result = p_xrAcquireSwapchainImage(
                    depth->handle, &depth_acquire, &depth_index);
                int depth_acquired = XR_SUCCEEDED(depth_result);
                if (depth_acquired) {
                    XrSwapchainImageWaitInfo depth_wait = {
                        XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO
                    };
                    depth_wait.timeout = XR_INFINITE_DURATION;
                    depth_result = p_xrWaitSwapchainImage(depth->handle, &depth_wait);
                }
                if (XR_SUCCEEDED(depth_result) && depth_acquired) {
                    AcquireSRWLockExclusive(&g_interop_capture_lock);
                    submitted_depth[eye] = copy_pair_depth_to_swapchain(
                        eye, coherent_pair_index, depth->targets[depth_index]);
                    ReleaseSRWLockExclusive(&g_interop_capture_lock);
                }
                if (depth_acquired) {
                    XrSwapchainImageReleaseInfo depth_release = {
                        XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO
                    };
                    XrResult release_result = p_xrReleaseSwapchainImage(
                        depth->handle, &depth_release);
                    if (XR_FAILED(release_result)) submitted_depth[eye] = 0;
                }
                if (submitted_depth[eye]) {
                    depth_info[eye].type = XR_TYPE_COMPOSITION_LAYER_DEPTH_INFO_KHR;
                    depth_info[eye].subImage.swapchain = depth->handle;
                    depth_info[eye].subImage.imageRect = projection_rect;
                    depth_info[eye].subImage.imageArrayIndex = 0;
                    depth_info[eye].minDepth = 0.0f;
                    depth_info[eye].maxDepth = 1.0f;
                    /* The game camera uses 160 world units per physical metre.
                       These values describe the same GL near/far planes in the
                       OpenXR layer's metre-scaled space. */
                    depth_info[eye].nearZ = 32.0f / 160.0f;
                    depth_info[eye].farZ = 3797.10156f / 160.0f;
                }
            }
        }
    }
    if (submitted_depth[0] && submitted_depth[1]) {
        for (uint32_t eye = 0; eye < EYE_COUNT; ++eye)
            projection_views[eye].next = &depth_info[eye];
    }
    ULONGLONG timing_copied = GetTickCount64();
    InterlockedExchange(&g_submit_pair_index, -1);

    int use_projection_layer = frame_state.shouldRender && view_count == EYE_COUNT &&
                               g_eye_used_depth_stereo[0] && g_eye_used_depth_stereo[1];
    /* UI is now baked into the acquired projection images above.  Keep the
       reported secondary-layer state false so neither Meta nor the simulator
       can composite a duplicate. */
    int use_interface_layer = 0;
    if (use_projection_layer && coherent_pair_completed_interface) {
        static int completed_interface_pair_logged;
        if (!completed_interface_pair_logged) {
            xr_log("Presentation source: exact completed stereo interface pair");
            completed_interface_pair_logged = 1;
        }
    }
    if (use_projection_layer && submitted_depth[0] && submitted_depth[1] &&
        !g_depth_submission_logged) {
        xr_log("Depth-aware OpenXR projection active for both eyes (near=0.200m far=23.732m)");
        g_depth_submission_logged = 1;
    }
    update_stereo_calibration(use_projection_layer);
    int recenter_marker = GetFileAttributesA(g_tabletop_recenter_marker) !=
                          INVALID_FILE_ATTRIBUTES;
    int recenter_hotkey = (GetAsyncKeyState(VK_CONTROL) & 0x8000) &&
                          (GetAsyncKeyState(VK_MENU) & 0x8000) &&
                          (GetAsyncKeyState(VK_F9) & 1);
    int recenter_requested = recenter_marker || recenter_hotkey;
    if (use_projection_layer && (!g_projection_anchor_valid || recenter_requested)) {
        /* The original game camera corresponds to the physical head pose at
           this anchor. Subsequent full pose deltas move the visual-only game
           camera, while projection metadata uses the live OpenXR eye poses. */
        XrPosef shared_pose = g_views[0].pose;
        shared_pose.position.x = (g_views[0].pose.position.x + g_views[1].pose.position.x) * 0.5f;
        shared_pose.position.y = (g_views[0].pose.position.y + g_views[1].pose.position.y) * 0.5f;
        shared_pose.position.z = (g_views[0].pose.position.z + g_views[1].pose.position.z) * 0.5f;
        float horizontal = (-g_views[0].fov.angleLeft + g_views[0].fov.angleRight
                            -g_views[1].fov.angleLeft + g_views[1].fov.angleRight) * 0.25f * 1.15f;
        float vertical = (g_views[0].fov.angleUp - g_views[0].fov.angleDown
                          +g_views[1].fov.angleUp - g_views[1].fov.angleDown) * 0.25f * 1.15f;
        XrFovf shared_fov = {-horizontal, horizontal, vertical, -vertical};
        for (uint32_t eye = 0; eye < EYE_COUNT; ++eye) {
            g_projection_anchor_pose[eye] = shared_pose;
            g_projection_anchor_fov[eye] = shared_fov;
        }
        g_projection_anchor_valid = 1;
        if (recenter_marker) DeleteFileA(g_tabletop_recenter_marker);
        g_tabletop_motion_logged = 0;
        xr_log("Tabletop anchor %s at position %.3f %.3f %.3f orientation %.3f %.3f %.3f %.3f",
               recenter_requested ? "recentered" : "initialized",
               shared_pose.position.x, shared_pose.position.y, shared_pose.position.z,
               shared_pose.orientation.x, shared_pose.orientation.y,
               shared_pose.orientation.z, shared_pose.orientation.w);
        if (recenter_hotkey)
            xr_log("Tabletop recentered with Ctrl+Alt+F9 comfort shortcut");
        g_projection_layer_logged = 1;
    } else if (!use_projection_layer) {
        if (!g_quad_layer_logged && frame_state.shouldRender) {
            xr_log("Presentation mode: flat menu quad");
            g_quad_layer_logged = 1;
        }
    }

    XrCompositionLayerProjection projection_layer = {XR_TYPE_COMPOSITION_LAYER_PROJECTION};
    projection_layer.space = g_space;
    projection_layer.viewCount = EYE_COUNT;
    projection_layer.views = projection_views;
    /* Live eye poses are essential: the game render now follows the same
       tracked head transform instead of being mislabeled as a frozen view. */

    /* Menus remain a readable world-stable 16:9 panel. Gameplay switches to
       the two-eye projection layer above, filling the headset and preserving
       the independently reconstructed left/right views. */
    XrCompositionLayerQuad layer = {XR_TYPE_COMPOSITION_LAYER_QUAD};
    layer.space = g_space;
    layer.eyeVisibility = XR_EYE_VISIBILITY_BOTH;
    layer.subImage.swapchain = g_eyes[0].handle;
    layer.subImage.imageRect.offset.x = g_content_x;
    layer.subImage.imageRect.offset.y = g_content_y;
    layer.subImage.imageRect.extent.width = g_content_width;
    layer.subImage.imageRect.extent.height = g_content_height;
    layer.pose.orientation.w = 1.0f;
    layer.pose.position.z = -g_display_distance;
    layer.size.width = g_display_width;
    layer.size.height = g_content_width > 0
        ? layer.size.width * (float)g_content_height / (float)g_content_width : 1.7f;
    XrCompositionLayerQuad interface_layer = {XR_TYPE_COMPOSITION_LAYER_QUAD};
    interface_layer.layerFlags =
        XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT |
        XR_COMPOSITION_LAYER_UNPREMULTIPLIED_ALPHA_BIT;
    interface_layer.space = g_view_space;
    interface_layer.eyeVisibility = XR_EYE_VISIBILITY_BOTH;
    interface_layer.subImage.swapchain = g_interface_layer.handle;
    interface_layer.subImage.imageRect.offset.x = 0;
    interface_layer.subImage.imageRect.offset.y = 0;
    interface_layer.subImage.imageRect.extent.width = g_interface_layer.width;
    interface_layer.subImage.imageRect.extent.height = g_interface_layer.height;
    interface_layer.pose.orientation.w = 1.0f;
    interface_layer.pose.position.z = -2.0f;
    interface_layer.size.width = 2.20f;
    interface_layer.size.height = interface_layer.size.width * 9.0f / 16.0f;
    const XrCompositionLayerBaseHeader *layers[2];
    uint32_t layer_count = 0;
    if (use_projection_layer)
        layers[layer_count++] =
            (const XrCompositionLayerBaseHeader *)&projection_layer;
    else
        layers[layer_count++] = (const XrCompositionLayerBaseHeader *)&layer;
    if (use_projection_layer && use_interface_layer)
        layers[layer_count++] =
            (const XrCompositionLayerBaseHeader *)&interface_layer;
    update_pose_trace(frame_state.predictedDisplayTime, coherent_pair_index,
                      use_projection_layer, use_interface_layer,
                      projection_views);
    XrFrameEndInfo end = {XR_TYPE_FRAME_END_INFO};
    end.displayTime = frame_state.predictedDisplayTime;
    end.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
    end.layerCount = (frame_state.shouldRender && view_count == EYE_COUNT)
        ? layer_count : 0;
    end.layers = end.layerCount ? layers : NULL;
    result = p_xrEndFrame(g_session, &end);
    if (XR_FAILED(result)) { fail("xrEndFrame", result); return 0; }
    ULONGLONG timing_ended = GetTickCount64();
    if (timing_ended - timing_start >= 24 &&
        timing_ended - g_last_slow_frame_log >= 250) {
        xr_log("Slow OpenXR frame %llums: wait=%llums locate=%llums snapshot=%llums "
               "eye-copy=%llums end=%llums pairGen=%ld",
               timing_ended - timing_start, timing_waited - timing_start,
               timing_located - timing_waited, timing_snapshotted - timing_located,
               timing_copied - timing_snapshotted, timing_ended - timing_copied,
               InterlockedCompareExchange(&g_present_pair_generation, 0, 0));
        g_last_slow_frame_log = timing_ended;
    }
    ++g_submitted_frames;
    ULONGLONG cadence_now = GetTickCount64();
    if (!g_cadence_window_start) {
        g_cadence_window_start = cadence_now;
        g_cadence_submitted_start = g_submitted_frames;
        g_cadence_pair_start = InterlockedCompareExchange(&g_present_pair_generation, 0, 0);
        for (int eye = 0; eye < EYE_COUNT; ++eye)
            g_cadence_eye_start[eye] =
                InterlockedCompareExchange(&g_eye_capture_generation[eye], 0, 0);
    } else if (cadence_now - g_cadence_window_start >= 2000) {
        ULONGLONG elapsed = cadence_now - g_cadence_window_start;
        LONG pair_now = InterlockedCompareExchange(&g_present_pair_generation, 0, 0);
        LONG eye_now[EYE_COUNT];
        for (int eye = 0; eye < EYE_COUNT; ++eye)
            eye_now[eye] = InterlockedCompareExchange(&g_eye_capture_generation[eye], 0, 0);
        xr_log("Cadence %.1f XR Hz, %.1f coherent pairs/s, eyes %.1f/%.1f renders/s",
               (double)(g_submitted_frames - g_cadence_submitted_start) * 1000.0 /
                   (double)elapsed,
               (double)(pair_now - g_cadence_pair_start) * 1000.0 / (double)elapsed,
               (double)(eye_now[0] - g_cadence_eye_start[0]) * 1000.0 /
                   (double)elapsed,
               (double)(eye_now[1] - g_cadence_eye_start[1]) * 1000.0 /
                   (double)elapsed);
        g_cadence_window_start = cadence_now;
        g_cadence_submitted_start = g_submitted_frames;
        g_cadence_pair_start = pair_now;
        for (int eye = 0; eye < EYE_COUNT; ++eye) g_cadence_eye_start[eye] = eye_now[eye];
    }
    if (g_submitted_frames == 1 && frame_state.predictedDisplayPeriod > 0) {
        double refresh_hz = 1000000000.0 / (double)frame_state.predictedDisplayPeriod;
        xr_log("Runtime display cadence: %.2f Hz (period %lld ns)", refresh_hz,
               (long long)frame_state.predictedDisplayPeriod);
    }
    if (g_submitted_frames == 1 || g_submitted_frames % 300 == 0) {
        snprintf(g_status, sizeof(g_status), "running OpenXR (%lu frames)", g_submitted_frames);
        xr_log("Submitted OpenXR game frame %lu, shouldRender=%u", g_submitted_frames,
               (unsigned int)frame_state.shouldRender);
    }
    return 1;
}

static DWORD WINAPI openxr_frame_thread(void *unused)
{
    (void)unused;
    HANDLE thread = GetCurrentThread();
    /* MMCSS already protects the compositor cadence. ABOVE_NORMAL leaves
       Meta's USB/encoder and tracking workers enough scheduling headroom; the
       previous HIGHEST+MMCSS combination could monopolize a busy core while
       Link was recovering from a transport hiccup. */
    int priority_result = SetThreadPriority(thread, THREAD_PRIORITY_ABOVE_NORMAL);
    typedef HANDLE (WINAPI *PFNAVSETMMTHREADCHARACTERISTICSA)(LPCSTR, LPDWORD);
    typedef BOOL (WINAPI *PFNAVREVERTMMTHREADCHARACTERISTICS)(HANDLE);
    HMODULE avrt = LoadLibraryA("avrt.dll");
    PFNAVSETMMTHREADCHARACTERISTICSA set_mmcss = avrt
        ? (PFNAVSETMMTHREADCHARACTERISTICSA)GetProcAddress(
              avrt, "AvSetMmThreadCharacteristicsA") : NULL;
    PFNAVREVERTMMTHREADCHARACTERISTICS revert_mmcss = avrt
        ? (PFNAVREVERTMMTHREADCHARACTERISTICS)GetProcAddress(
              avrt, "AvRevertMmThreadCharacteristics") : NULL;
    DWORD task_index = 0;
    HANDLE mmcss = set_mmcss ? set_mmcss("Games", &task_index) : NULL;
    xr_log("Dedicated OpenXR frame thread active; priority=%s MMCSS=%s",
            priority_result ? "above-normal" : "unchanged",
            mmcss ? "Games" : "unavailable");
    while (!g_failed &&
           !InterlockedCompareExchange(&g_shutdown_requested, 0, 0)) {
        poll_events();
        if (g_running) {
            if (!submit_diagnostic_frame()) break;
        } else {
            if (g_session_stopped_tick && !g_session_stall_logged &&
                GetTickCount64() - g_session_stopped_tick >= 5000) {
                xr_log("OpenXR runtime has not returned READY five seconds after STOPPING; "
                       "game rendering remains active but Link transport/compositor recovery "
                       "is required");
                g_session_stall_logged = 1;
            }
            Sleep(2);
        }
    }
    if (mmcss && revert_mmcss) revert_mmcss(mmcss);
    if (avrt) FreeLibrary(avrt);
    return 0;
}

int openxr_bridge_tick(const char *base_directory, void *device_context)
{
    if (InterlockedCompareExchange(&g_shutdown_requested, 0, 0)) return 0;
    if (device_context) g_game_window = WindowFromDC((HDC)device_context);
    if (g_failed) return 0;
    if (!g_enabled) {
        char marker[MAX_PATH];
        snprintf(marker, sizeof(marker), "%s\\SkillshotVR-openxr.txt", base_directory);
        g_enabled = GetFileAttributesA(marker) != INVALID_FILE_ATTRIBUTES;
        if (!g_enabled) return 0;
    }
    if (!g_initialized) {
        g_initialized = 1;
        if (!initialize_bridge(base_directory)) return 0;
    }
    if (g_game_window && g_game_window != g_logged_game_window) {
        RECT client = {0};
        GetClientRect(g_game_window, &client);
        xr_log("Game window: hwnd=0x%llX client=%ldx%ld",
               (unsigned long long)(uintptr_t)g_game_window,
               client.right - client.left, client.bottom - client.top);
        g_logged_game_window = g_game_window;
    }
    /* Initialization and flat-menu capture require the game's current OpenGL
       context, so they stay synchronous. Once genuine eye textures exist,
       move the OpenXR frame loop off the game thread. The runtime can then
       maintain its own 72/90 Hz cadence while the older game produces new
       coherent stereo pairs at whatever rate its renderer can sustain. */
    if (g_geometry_eye_ready[0] && g_geometry_eye_ready[1] &&
        InterlockedCompareExchange(&g_frame_thread_started, 1, 0) == 0) {
        g_frame_thread = CreateThread(NULL, 0, openxr_frame_thread, NULL, 0, NULL);
        if (!g_frame_thread) {
            InterlockedExchange(&g_frame_thread_started, 0);
            xr_log("Dedicated OpenXR frame thread creation failed: %lu", GetLastError());
        } else {
            return 1;
        }
    }
    if (InterlockedCompareExchange(&g_frame_thread_started, 0, 0)) return 1;
    poll_events();
    if (g_running) return submit_diagnostic_frame();
    return 1;
}

#define RELEASE_D3D(object) do { \
    if ((object) != NULL) { \
        IUnknown_Release((IUnknown *)(object)); \
        (object) = NULL; \
    } \
} while (0)

static void release_interop_registration(HANDLE *object)
{
    if (*object && g_interop_device && p_wglDXUnregisterObjectNV)
        p_wglDXUnregisterObjectNV(g_interop_device, *object);
    *object = NULL;
}

int openxr_bridge_shutdown(void)
{
    if (InterlockedCompareExchange(&g_shutdown_complete, 0, 0)) return 1;
    if (InterlockedCompareExchange(&g_shutdown_started, 1, 0) != 0) {
        for (int spin = 0; spin < 100; ++spin) {
            if (InterlockedCompareExchange(&g_shutdown_complete, 0, 0)) return 1;
            Sleep(1);
        }
        return 0;
    }

    xr_log("OpenXR shutdown requested");
    InterlockedExchange(&g_shutdown_requested, 1);
    if (g_frame_thread) {
        DWORD worker_id = GetThreadId(g_frame_thread);
        if (worker_id != GetCurrentThreadId()) {
            DWORD wait = WaitForSingleObject(g_frame_thread, 2000);
            if (wait != WAIT_OBJECT_0) {
                xr_log("ERROR OpenXR frame worker did not stop within 2000 ms; "
                       "leaving runtime-owned resources intact to avoid a race");
                snprintf(g_status, sizeof(g_status), "shutdown timed out");
                InterlockedExchange(&g_shutdown_started, 0);
                return 0;
            }
        }
        CloseHandle(g_frame_thread);
        g_frame_thread = NULL;
        InterlockedExchange(&g_frame_thread_started, 0);
    }

    if (g_pose_trace_file) {
        fflush(g_pose_trace_file);
        fclose(g_pose_trace_file);
        g_pose_trace_file = NULL;
    }

    /* Ask a running runtime to enter STOPPING, then service its state events
       ourselves now that the frame worker is joined. This bounded wait keeps
       context destruction responsive even when Link has already failed. */
    if (g_session != XR_NULL_HANDLE && g_running && p_xrRequestExitSession) {
        XrResult request = p_xrRequestExitSession(g_session);
        xr_log("xrRequestExitSession during shutdown: %d", (int)request);
        ULONGLONG deadline = GetTickCount64() + 500;
        while (g_running && GetTickCount64() < deadline) {
            poll_events();
            if (g_running) Sleep(2);
        }
    }

    /* The WGL/D3D registrations must be released while the game's GL context
       is still current. The proxy invokes this function from wglDeleteContext
       before forwarding the real deletion. */
    AcquireSRWLockExclusive(&g_interop_capture_lock);
    for (int eye = 0; eye < EYE_COUNT; ++eye) {
        release_interop_registration(&g_interop_objects[eye]);
        release_interop_registration(&g_hud_base_interop_objects[eye]);
    }
    for (int frame = 0; frame < 2; ++frame)
        release_interop_registration(&g_menu_interop_objects[frame]);
    for (int resource = 0; resource < 2; ++resource)
        release_interop_registration(&g_world_interop_objects[resource]);
    if (g_interop_device && p_wglDXCloseDeviceNV)
        p_wglDXCloseDeviceNV(g_interop_device);
    g_interop_device = NULL;
    g_gl_interop_ready = 0;
    ReleaseSRWLockExclusive(&g_interop_capture_lock);

    for (int eye = 0; eye < EYE_COUNT; ++eye) {
        for (uint32_t image = 0; image < g_eyes[eye].image_count; ++image)
            RELEASE_D3D(g_eyes[eye].targets[image]);
        for (uint32_t image = 0; image < g_depth_eyes[eye].image_count; ++image)
            RELEASE_D3D(g_depth_eyes[eye].targets[image]);
    }
    for (uint32_t image = 0; image < g_interface_layer.image_count; ++image)
        RELEASE_D3D(g_interface_layer.targets[image]);

    if (p_xrDestroySwapchain) {
        for (int eye = 0; eye < EYE_COUNT; ++eye) {
            if (g_depth_eyes[eye].handle != XR_NULL_HANDLE) {
                p_xrDestroySwapchain(g_depth_eyes[eye].handle);
                g_depth_eyes[eye].handle = XR_NULL_HANDLE;
            }
            if (g_eyes[eye].handle != XR_NULL_HANDLE) {
                p_xrDestroySwapchain(g_eyes[eye].handle);
                g_eyes[eye].handle = XR_NULL_HANDLE;
            }
        }
        if (g_interface_layer.handle != XR_NULL_HANDLE) {
            p_xrDestroySwapchain(g_interface_layer.handle);
            g_interface_layer.handle = XR_NULL_HANDLE;
        }
    }
    if (g_view_space != XR_NULL_HANDLE && p_xrDestroySpace) {
        p_xrDestroySpace(g_view_space);
        g_view_space = XR_NULL_HANDLE;
    }
    if (g_space != XR_NULL_HANDLE && p_xrDestroySpace) {
        p_xrDestroySpace(g_space);
        g_space = XR_NULL_HANDLE;
    }
    if (g_session != XR_NULL_HANDLE && p_xrDestroySession) {
        XrResult destroyed = p_xrDestroySession(g_session);
        xr_log("xrDestroySession during shutdown: %d", (int)destroyed);
        g_session = XR_NULL_HANDLE;
    }
    g_running = 0;
    if (g_instance != XR_NULL_HANDLE && p_xrDestroyInstance) {
        XrResult destroyed = p_xrDestroyInstance(g_instance);
        xr_log("xrDestroyInstance during shutdown: %d", (int)destroyed);
        g_instance = XR_NULL_HANDLE;
    }

    for (int eye = 0; eye < EYE_COUNT; ++eye) {
        RELEASE_D3D(g_interop_views[eye]);
        RELEASE_D3D(g_interop_textures[eye]);
        RELEASE_D3D(g_hud_base_interop_views[eye]);
        RELEASE_D3D(g_hud_base_interop_textures[eye]);
        RELEASE_D3D(g_geometry_eye_depth_views[eye]);
        RELEASE_D3D(g_geometry_eye_depth_textures[eye]);
        for (int pair = 0; pair < PAIR_BUFFER_COUNT; ++pair) {
            RELEASE_D3D(g_present_pair_views[pair][eye]);
            RELEASE_D3D(g_present_pair_targets[pair][eye]);
            RELEASE_D3D(g_present_pair_textures[pair][eye]);
            RELEASE_D3D(g_present_pair_depth_views[pair][eye]);
            RELEASE_D3D(g_present_pair_depth_textures[pair][eye]);
        }
    }
    for (int frame = 0; frame < 2; ++frame) {
        RELEASE_D3D(g_menu_interop_views[frame]);
        RELEASE_D3D(g_menu_interop_textures[frame]);
    }
    RELEASE_D3D(g_world_color_view);
    RELEASE_D3D(g_world_depth_view);
    RELEASE_D3D(g_world_color_texture);
    RELEASE_D3D(g_world_depth_texture);
    RELEASE_D3D(g_world_depth_shader_texture);
    RELEASE_D3D(g_stereo_vertex_shader);
    RELEASE_D3D(g_stereo_pixel_shader);
    RELEASE_D3D(g_stereo_constants);
    RELEASE_D3D(g_stereo_sampler);
    RELEASE_D3D(g_stereo_rasterizer);
    RELEASE_D3D(g_pair_sharpen_vertex_shader);
    RELEASE_D3D(g_pair_sharpen_pixel_shader);
    RELEASE_D3D(g_menu_overlay_pixel_shader);
    RELEASE_D3D(g_menu_overlay_blend);
    RELEASE_D3D(g_pair_sharpen_constants);
    RELEASE_D3D(g_pair_sharpen_sampler);
    RELEASE_D3D(g_pair_sharpen_rasterizer);
    RELEASE_D3D(g_depth_copy_vertex_shader);
    RELEASE_D3D(g_depth_copy_pixel_shader);
    RELEASE_D3D(g_depth_copy_constants);
    RELEASE_D3D(g_depth_copy_state);
    RELEASE_D3D(g_cursor_view);
    RELEASE_D3D(g_cursor_texture);
    RELEASE_D3D(g_cursor_vertex_shader);
    RELEASE_D3D(g_cursor_pixel_shader);
    RELEASE_D3D(g_cursor_constants);
    RELEASE_D3D(g_cursor_sampler);
    RELEASE_D3D(g_cursor_blend);
    RELEASE_D3D(g_cursor_rasterizer);
    RELEASE_D3D(g_pair_copy_fence);
    RELEASE_D3D(g_d3d_multithread);
    RELEASE_D3D(g_context);
    RELEASE_D3D(g_device);

    if (g_loader) {
        FreeLibrary(g_loader);
        g_loader = NULL;
    }
    snprintf(g_status, sizeof(g_status), "stopped cleanly");
    xr_log("OpenXR shutdown complete");
    InterlockedExchange(&g_shutdown_complete, 1);
    return 1;
}

#undef RELEASE_D3D

const char *openxr_bridge_status(void)
{
    return g_status;
}

int openxr_bridge_should_expand_world(void)
{
    return (g_stereo_enabled || g_geometry_stereo_requested) && !g_flat_scene_mode;
}
