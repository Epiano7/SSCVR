#ifndef SKILLSHOT_OPENXR_BRIDGE_H
#define SKILLSHOT_OPENXR_BRIDGE_H

/* Returns nonzero once an OpenXR instance and D3D11 session exist. */
int openxr_bridge_tick(const char *base_directory, void *device_context);
/* Stop the frame worker and release the OpenXR session before the owning WGL
   context disappears. Safe to call repeatedly, including after a partial
   initialization failure. */
int openxr_bridge_shutdown(void);

/* Captures the game's resolved world color and depth before its 2D interface.
   eye is 0/1 for native geometry or -1 for the legacy flat-depth path. */
int openxr_bridge_capture_world(int source_x0, int source_y0, int source_x1, int source_y1,
                                int eye);
/* True-geometry stereo: capture the completed game render for one eye. */
int openxr_bridge_geometry_ready(void);
/* Preserve the completed perspective frame before Skillshot draws its fixed
   orthographic HUD. The bridge later composites that native-detail HUD into
   a lens-safe 16:9 region without increasing world raster resolution. */
int openxr_bridge_capture_hud_base(int eye, float safe_scale_x,
                                   float safe_scale_y);
int openxr_bridge_capture_eye(int eye);
/* Capture the same eye after the game has appended Tab/Escape/level-up UI.
   Completed interface pairs retain genuine stereo world pixels while keeping
   the game's exact authored interface, avoiding heuristic alpha extraction. */
int openxr_bridge_capture_completed_interface_eye(int eye);
int openxr_bridge_capture_native_mirror(int preserve_desktop);
/* Full-screen interfaces are appended after the hooked gameplay renderer.
   Redirect the complete pass to a transparent surface, preserving authored
   colour/coverage without ever copying or classifying world pixels. */
int openxr_bridge_begin_interface_alpha_capture(void);
int openxr_bridge_begin_interface_alpha_draw(int pass);
void openxr_bridge_end_interface_alpha_draw(void);
unsigned int openxr_bridge_interface_alpha_framebuffer(void);
int openxr_bridge_finish_interface_alpha_capture(int publish);
int openxr_bridge_set_geometry_active(int active);
int openxr_bridge_capture_flat_frame(void);
/* Replaces the desktop backbuffer with a retained eye so alternating stereo
   does not make the ordinary game window shake. */
int openxr_bridge_present_mirror_eye(int eye, float vertical_scale);
/* Alpha-composite the captured interface over the retained desktop mirror so
   the ordinary window remains a faithful, non-flickering spectator view. */
int openxr_bridge_present_desktop_interface(void);
int openxr_bridge_get_head_translation(float *x_metres, float *y_metres);
/* Full center-head view delta from the explicit tabletop anchor. */
int openxr_bridge_get_head_view_delta(float units_per_metre,
                                      float world_eye_separation,
                                      float matrix[16]);
int openxr_bridge_get_eye_size(int *width, int *height);
/* Current lens FOV reported by the active OpenXR runtime for one eye. */
int openxr_bridge_get_runtime_fov(int eye, float *angle_left,
                                  float *angle_right, float *angle_up,
                                  float *angle_down);
/* Exact projection angles used by the legacy game's reconstructed eye. */
void openxr_bridge_set_geometry_fov(int eye, float angle_left, float angle_right,
                                    float angle_up, float angle_down);
/* Signed fraction of the horizontal frustum width used to calibrate the
   rendered optical centre.  Applying this before rasterization keeps the
   color image and submitted FOV in the same camera model. */
float openxr_bridge_get_geometry_alignment(int eye);
void openxr_bridge_set_interface_load(int draw_calls, long long vertices);
/* True while Escape, Tab, level-up, results, or another full interface owns
   the frame. The bridge keeps the stereo world active and extracts the
   completed interface into a transparent head-locked OpenXR layer. */
int openxr_bridge_interface_heavy(void);
int openxr_bridge_should_expand_world(void);
/* Start a fresh Shift+1 diagnostic set in this process. Previous one-shot
   proof flags and files must not be reused for later Tab/Escape captures. */
void openxr_bridge_request_fresh_geometry_proof(void);

/* Short human-readable state for periodic proxy logging. */
const char *openxr_bridge_status(void);

#endif
