#ifndef SKILLSHOT_CITY_VR_GEOMETRY_MATH_H
#define SKILLSHOT_CITY_VR_GEOMETRY_MATH_H

#include <math.h>

#define VR_VERTICAL_HEAD_MOTION_SCALE 0.40f
#define VR_RENDER_PITCH_HEAD_MOTION_SCALE 1.00f
#define VR_RENDER_ROLL_HEAD_MOTION_SCALE 0.00f
#define VR_PITCH_HEAD_MOTION_SCALE 1.00f
#define VR_ROLL_HEAD_MOTION_SCALE -0.80f

/* Expand orthographic bounds around their centre so screen-space content is
   inset by scale_x/scale_y. Reversed Y bounds, as used by the game UI, remain
   reversed. */
static inline void vr_inset_ortho_d(double bounds[4], double scale_x,
                                     double scale_y)
{
    if (!bounds || !isfinite(scale_x) || !isfinite(scale_y) ||
        scale_x <= 0.0 || scale_y <= 0.0) return;
    double center_x = (bounds[0] + bounds[1]) * 0.5;
    double center_y = (bounds[2] + bounds[3]) * 0.5;
    bounds[0] = center_x + (bounds[0] - center_x) / scale_x;
    bounds[1] = center_x + (bounds[1] - center_x) / scale_x;
    bounds[2] = center_y + (bounds[2] - center_y) / scale_y;
    bounds[3] = center_y + (bounds[3] - center_y) / scale_y;
}

/* Keep a fixed HUD canvas at its authored aspect ratio on whatever eye
   resolution the active OpenXR runtime supplies. */
static inline double vr_hud_scale_y_for_eye(double scale_x, double eye_width,
                                             double eye_height,
                                             double canvas_aspect)
{
    if (!isfinite(scale_x) || !isfinite(eye_width) || !isfinite(eye_height) ||
        !isfinite(canvas_aspect) || scale_x <= 0.0 || eye_width <= 0.0 ||
        eye_height <= 0.0 || canvas_aspect <= 0.0)
        return 0.0;
    return scale_x * (eye_width / eye_height) / canvas_aspect;
}

/* Place a head-locked canvas around an eye's optical centre.  Both eyes use
   the same dimensions while their X origins follow the runtime's asymmetric
   FOV centres, matching the fixed gameplay HUD without introducing stereo
   disparity. */
static inline int vr_safe_canvas_rect_for_eye(int eye_width, int eye_height,
                                               double optical_center_x,
                                               double scale_x, double scale_y,
                                               int rectangle[4])
{
    if (!rectangle || eye_width <= 0 || eye_height <= 0 ||
        !isfinite(optical_center_x) || !isfinite(scale_x) ||
        !isfinite(scale_y) || scale_x <= 0.0 || scale_x > 1.0 ||
        scale_y <= 0.0 || scale_y > 1.0)
        return 0;
    int width = (int)floor((double)eye_width * scale_x + 0.5);
    int height = (int)floor((double)eye_height * scale_y + 0.5);
    int x = (int)floor(((double)eye_width * optical_center_x -
                        (double)width * 0.5) + 0.5);
    int y = (eye_height - height) / 2;
    if (width < 1) width = 1;
    if (height < 1) height = 1;
    if (width > eye_width) width = eye_width;
    if (height > eye_height) height = eye_height;
    if (x < 0) x = 0;
    if (x > eye_width - width) x = eye_width - width;
    if (y < 0) y = 0;
    if (y > eye_height - height) y = eye_height - height;
    rectangle[0] = x;
    rectangle[1] = y;
    rectangle[2] = width;
    rectangle[3] = height;
    return 1;
}

/* Match OpenXR's lens metadata to a calibrated horizontal texture shift so
   asynchronous timewarp does not bend straight geometry during head motion. */
static inline void vr_shift_horizontal_fov_for_texture_f(float *angle_left,
                                                          float *angle_right,
                                                          float signed_width)
{
    if (!angle_left || !angle_right || !isfinite(*angle_left) ||
        !isfinite(*angle_right) || !isfinite(signed_width)) return;
    float tangent_left = tanf(*angle_left);
    float tangent_right = tanf(*angle_right);
    float tangent_shift = signed_width * (tangent_right - tangent_left);
    *angle_left = atanf(tangent_left + tangent_shift);
    *angle_right = atanf(tangent_right + tangent_shift);
}

/* Convert the same normalized optical-centre calibration into a frustum-space
   shift. Applying this before rasterization avoids cropping the finished eye
   texture while preserving the calibrated horizontal rays exactly. */
static inline double vr_frustum_alignment_shift_d(double left, double right,
                                                   double signed_width)
{
    if (!isfinite(left) || !isfinite(right) || !isfinite(signed_width) ||
        right <= left)
        return 0.0;
    return signed_width * (right - left);
}

/* OpenXR reports conventional up-positive/down-negative FOV angles, while
   Skillshot's perspective pass uses vertically reversed glFrustum bounds.
   Build the game's exact near-plane bounds from the runtime eye FOV so the
   color image covers the lens instead of being timewarped as a narrow patch. */
static inline int vr_runtime_fov_to_reversed_frustum_d(
    float angle_left, float angle_right, float angle_up, float angle_down,
    double near_value, double bounds[4])
{
    if (!bounds || !isfinite(angle_left) || !isfinite(angle_right) ||
        !isfinite(angle_up) || !isfinite(angle_down) ||
        !isfinite(near_value) || near_value <= 0.0)
        return 0;
    bounds[0] = tan((double)angle_left) * near_value;
    bounds[1] = tan((double)angle_right) * near_value;
    bounds[2] = -tan((double)angle_down) * near_value;
    bounds[3] = -tan((double)angle_up) * near_value;
    return isfinite(bounds[0]) && isfinite(bounds[1]) &&
           isfinite(bounds[2]) && isfinite(bounds[3]) &&
           bounds[1] > bounds[0] && bounds[2] > bounds[3];
}

/* Fit a projection image into an eye texture without changing the ratio
   between its horizontal and vertical tangent-space FOV.  OpenXR reprojection
   assumes this ratio matches the submitted image rectangle; stretching a
   1.08:1 projection across a 0.92:1 eye texture bends geometry during
   timewarp even though the original desktop render remains straight. */
static inline int vr_fit_projection_rect_f(int target_width, int target_height,
                                            float angle_left, float angle_right,
                                            float angle_up, float angle_down,
                                            int rectangle[4])
{
    if (!rectangle || target_width <= 0 || target_height <= 0 ||
        !isfinite(angle_left) || !isfinite(angle_right) ||
        !isfinite(angle_up) || !isfinite(angle_down)) return 0;
    float tangent_width = tanf(angle_right) - tanf(angle_left);
    float tangent_height = tanf(angle_up) - tanf(angle_down);
    if (!isfinite(tangent_width) || !isfinite(tangent_height) ||
        tangent_width <= 0.0001f || tangent_height <= 0.0001f) return 0;
    float projection_aspect = tangent_width / tangent_height;
    int width = target_width;
    int height = (int)((float)width / projection_aspect + 0.5f);
    if (height > target_height) {
        height = target_height;
        width = (int)((float)height * projection_aspect + 0.5f);
    }
    if (width < 2 || height < 2) return 0;
    width &= ~1;
    height &= ~1;
    rectangle[0] = (target_width - width) / 2;
    rectangle[1] = (target_height - height) / 2;
    rectangle[2] = width;
    rectangle[3] = height;
    return 1;
}

/* Scale physical eye positions about their midpoint to the IPD represented by
   the game-space stereo cameras. */
static inline int vr_scale_stereo_eye_positions_f(float left[3], float right[3],
                                                   float target_ipd_metres)
{
    if (!left || !right || !isfinite(target_ipd_metres) ||
        target_ipd_metres <= 0.0f) return 0;
    float difference[3] = {
        right[0] - left[0], right[1] - left[1], right[2] - left[2]
    };
    float current_ipd = sqrtf(difference[0] * difference[0] +
                              difference[1] * difference[1] +
                              difference[2] * difference[2]);
    if (!isfinite(current_ipd) || current_ipd < 0.001f) return 0;
    float midpoint[3] = {
        (left[0] + right[0]) * 0.5f,
        (left[1] + right[1]) * 0.5f,
        (left[2] + right[2]) * 0.5f
    };
    float scale = target_ipd_metres / current_ipd;
    for (int axis = 0; axis < 3; ++axis) {
        left[axis] = midpoint[axis] + (left[axis] - midpoint[axis]) * scale;
        right[axis] = midpoint[axis] + (right[axis] - midpoint[axis]) * scale;
    }
    return 1;
}

/* OpenGL stores matrices column-major.  The third camera-space row is at
   indices 2, 6, 10, 14.  Scaling that entire row changes depth along the
   camera's forward axis even when the game's camera is pitched and rotated.

   The translation keeps z=-convergence fixed:
       z' = scale*z + convergence*(scale - 1)
   This gives foreground/background extra separation without changing the
   apparent scale of the gameplay focal plane. */
static inline void vr_scale_camera_depth_f(float matrix[16], float scale,
                                            float convergence)
{
    if (!matrix || !isfinite(scale) || !isfinite(convergence) ||
        scale <= 0.0f || convergence <= 0.0f || fabsf(scale - 1.0f) < 0.00001f)
        return;
    matrix[2] *= scale;
    matrix[6] *= scale;
    matrix[10] *= scale;
    matrix[14] = matrix[14] * scale + convergence * (scale - 1.0f);
}

static inline void vr_scale_camera_depth_d(double matrix[16], double scale,
                                            double convergence)
{
    if (!matrix || !isfinite(scale) || !isfinite(convergence) ||
        scale <= 0.0 || convergence <= 0.0 || fabs(scale - 1.0) < 0.00000001)
        return;
    matrix[2] *= scale;
    matrix[6] *= scale;
    matrix[10] *= scale;
    matrix[14] = matrix[14] * scale + convergence * (scale - 1.0);
}

/* Apply one OpenGL column-major affine matrix to a point. */
static inline void vr_transform_point_f(const float matrix[16], const float point[3],
                                         float output[3])
{
    output[0] = matrix[0] * point[0] + matrix[4] * point[1] +
                matrix[8] * point[2] + matrix[12];
    output[1] = matrix[1] * point[0] + matrix[5] * point[1] +
                matrix[9] * point[2] + matrix[13];
    output[2] = matrix[2] * point[0] + matrix[6] * point[1] +
                matrix[10] * point[2] + matrix[14];
}

static inline void vr_multiply_matrix_f(const float left[16], const float right[16],
                                         float output[16])
{
    float result[16];
    for (int column = 0; column < 4; ++column) {
        for (int row = 0; row < 4; ++row) {
            result[column * 4 + row] =
                left[0 * 4 + row] * right[column * 4 + 0] +
                left[1 * 4 + row] * right[column * 4 + 1] +
                left[2 * 4 + row] * right[column * 4 + 2] +
                left[3 * 4 + row] * right[column * 4 + 3];
        }
    }
    for (int i = 0; i < 16; ++i) output[i] = result[i];
}

/* Pitch the already top-down game camera into a tabletop below the viewer.
   Rotation occurs around the gameplay focal plane instead of the viewer, so
   the map centre stays at a stable distance. */
static inline void vr_build_tabletop_view_f(float pitch_degrees,
                                             float horizontal_offset,
                                             float vertical_offset,
                                             float pivot_distance,
                                             float matrix[16])
{
    const float radians = pitch_degrees * 0.01745329251994329577f;
    const float cosine = cosf(radians);
    const float sine = sinf(radians);
    matrix[0] = 1.0f; matrix[1] = 0.0f;   matrix[2] = 0.0f;  matrix[3] = 0.0f;
    matrix[4] = 0.0f; matrix[5] = cosine; matrix[6] = sine;  matrix[7] = 0.0f;
    matrix[8] = 0.0f; matrix[9] = -sine; matrix[10] = cosine; matrix[11] = 0.0f;
    matrix[12] = horizontal_offset;
    matrix[13] = vertical_offset - sine * pivot_distance;
    matrix[14] = (cosine - 1.0f) * pivot_distance;
    matrix[15] = 1.0f;
}

static inline void vr_quaternion_normalize_f(float quaternion[4])
{
    float length = sqrtf(quaternion[0] * quaternion[0] + quaternion[1] * quaternion[1] +
                         quaternion[2] * quaternion[2] + quaternion[3] * quaternion[3]);
    if (!isfinite(length) || length < 0.000001f) {
        quaternion[0] = quaternion[1] = quaternion[2] = 0.0f;
        quaternion[3] = 1.0f;
        return;
    }
    for (int i = 0; i < 4; ++i) quaternion[i] /= length;
}

static inline void vr_quaternion_multiply_f(const float left[4], const float right[4],
                                             float output[4])
{
    output[0] = left[3] * right[0] + left[0] * right[3] +
                left[1] * right[2] - left[2] * right[1];
    output[1] = left[3] * right[1] - left[0] * right[2] +
                left[1] * right[3] + left[2] * right[0];
    output[2] = left[3] * right[2] + left[0] * right[1] -
                left[1] * right[0] + left[2] * right[3];
    output[3] = left[3] * right[3] - left[0] * right[0] -
                left[1] * right[1] - left[2] * right[2];
}

static inline void vr_quaternion_matrix_f(const float quaternion[4], float matrix[16])
{
    float x = quaternion[0], y = quaternion[1], z = quaternion[2], w = quaternion[3];
    matrix[0] = 1.0f - 2.0f * (y * y + z * z);
    matrix[1] = 2.0f * (x * y + z * w);
    matrix[2] = 2.0f * (x * z - y * w);
    matrix[3] = 0.0f;
    matrix[4] = 2.0f * (x * y - z * w);
    matrix[5] = 1.0f - 2.0f * (x * x + z * z);
    matrix[6] = 2.0f * (y * z + x * w);
    matrix[7] = 0.0f;
    matrix[8] = 2.0f * (x * z + y * w);
    matrix[9] = 2.0f * (y * z - x * w);
    matrix[10] = 1.0f - 2.0f * (x * x + y * y);
    matrix[11] = 0.0f;
    matrix[12] = matrix[13] = matrix[14] = 0.0f;
    matrix[15] = 1.0f;
}

/* Preserve natural yaw while damping pitch and roll for a distant tabletop.
   A 1:1 pitch rotates the world around a far pivot and feels much larger than
   the same motion in a first-person camera. Scaling the shortest-path rotation
   vector remains continuous when several axes move together. */
static inline void vr_scale_quaternion_axes_f(const float input[4],
                                               float x_scale, float y_scale,
                                               float z_scale, float output[4])
{
    float normalized[4] = {input[0], input[1], input[2], input[3]};
    vr_quaternion_normalize_f(normalized);
    if (normalized[3] < 0.0f)
        for (int axis = 0; axis < 4; ++axis) normalized[axis] = -normalized[axis];
    float sine_half = sqrtf(normalized[0] * normalized[0] +
                            normalized[1] * normalized[1] +
                            normalized[2] * normalized[2]);
    if (sine_half < 0.000001f) {
        output[0] = output[1] = output[2] = 0.0f;
        output[3] = 1.0f;
        return;
    }
    float angle = 2.0f * atan2f(sine_half, normalized[3]);
    float vector[3] = {
        normalized[0] * angle * x_scale / sine_half,
        normalized[1] * angle * y_scale / sine_half,
        normalized[2] * angle * z_scale / sine_half
    };
    float scaled_angle = sqrtf(vector[0] * vector[0] + vector[1] * vector[1] +
                               vector[2] * vector[2]);
    if (scaled_angle < 0.000001f) {
        output[0] = output[1] = output[2] = 0.0f;
        output[3] = 1.0f;
        return;
    }
    float factor = sinf(scaled_angle * 0.5f) / scaled_angle;
    output[0] = vector[0] * factor;
    output[1] = vector[1] * factor;
    output[2] = vector[2] * factor;
    output[3] = cosf(scaled_angle * 0.5f);
    vr_quaternion_normalize_f(output);
}

/* The OpenGL framebuffer is copied into the D3D/OpenXR texture with Y flipped.
   Reflect the tracked pose through that same Y plane before building the game
   camera. A reflection changes polar vectors directly and axial rotation
   vectors by det(F)*F, hence quaternion (x,y,z) -> (-x,y,-z). Without this,
   horizontal tracking is correct while vertical translation, pitch and roll
   appear reversed in the headset. */
static inline void vr_reflect_pose_for_texture_y_flip_f(float position[3],
                                                         float orientation[4])
{
    position[1] = -position[1];
    orientation[0] = -orientation[0];
    orientation[2] = -orientation[2];
}

/* Build the OpenXR camera pose whose motion relative to the tabletop anchor
   matches the Y-reflected head delta used by the OpenGL game camera.  Reflect
   only the anchor-relative transform so the world-locked anchor itself does
   not jump when projection metadata changes from the raw tracked pose. */
static inline void vr_reflect_pose_delta_about_anchor_f(
    const float anchor_position[3], const float anchor_orientation[4],
    const float current_position[3], const float current_orientation[4],
    float output_position[3], float output_orientation[4])
{
    float anchor[4] = {anchor_orientation[0], anchor_orientation[1],
                       anchor_orientation[2], anchor_orientation[3]};
    float current[4] = {current_orientation[0], current_orientation[1],
                        current_orientation[2], current_orientation[3]};
    vr_quaternion_normalize_f(anchor);
    vr_quaternion_normalize_f(current);

    float inverse_anchor[4] = {-anchor[0], -anchor[1], -anchor[2], anchor[3]};
    float relative[4];
    vr_quaternion_multiply_f(inverse_anchor, current, relative);
    vr_quaternion_normalize_f(relative);
    relative[0] = -relative[0];
    relative[2] = -relative[2];
    vr_quaternion_multiply_f(anchor, relative, output_orientation);
    vr_quaternion_normalize_f(output_orientation);

    float anchor_matrix[16];
    vr_quaternion_matrix_f(anchor, anchor_matrix);
    float delta[3] = {
        current_position[0] - anchor_position[0],
        current_position[1] - anchor_position[1],
        current_position[2] - anchor_position[2]
    };
    float anchor_local[3] = {
        anchor_matrix[0] * delta[0] + anchor_matrix[1] * delta[1] +
            anchor_matrix[2] * delta[2],
        anchor_matrix[4] * delta[0] + anchor_matrix[5] * delta[1] +
            anchor_matrix[6] * delta[2],
        anchor_matrix[8] * delta[0] + anchor_matrix[9] * delta[1] +
            anchor_matrix[10] * delta[2]
    };
    anchor_local[1] = -anchor_local[1];
    output_position[0] = anchor_position[0] +
        anchor_matrix[0] * anchor_local[0] + anchor_matrix[4] * anchor_local[1] +
        anchor_matrix[8] * anchor_local[2];
    output_position[1] = anchor_position[1] +
        anchor_matrix[1] * anchor_local[0] + anchor_matrix[5] * anchor_local[1] +
        anchor_matrix[9] * anchor_local[2];
    output_position[2] = anchor_position[2] +
        anchor_matrix[2] * anchor_local[0] + anchor_matrix[6] * anchor_local[1] +
        anchor_matrix[10] * anchor_local[2];
}

/* Convert tracked orientation into the physical visual convention measured on
   Quest. Skillshot's reversed vertical frustum is compensated in the camera
   matrix, so reflecting pitch again here makes looking up look down. Roll
   is compositor-driven and physical Quest acceptance showed that applying
   the texture-axis reflection a second time reverses it. Its signed scale
   below cancels that duplicate reflection and slightly damps the distant
   tabletop response; yaw is unchanged. */
static inline void vr_visual_head_orientation_about_anchor_scaled_f(
    const float anchor_orientation[4], const float tracked_orientation[4],
    float pitch_scale, float roll_scale,
    float output_orientation[4])
{
    float anchor[4] = {anchor_orientation[0], anchor_orientation[1],
                       anchor_orientation[2], anchor_orientation[3]};
    float tracked[4] = {tracked_orientation[0], tracked_orientation[1],
                        tracked_orientation[2], tracked_orientation[3]};
    vr_quaternion_normalize_f(anchor);
    vr_quaternion_normalize_f(tracked);
    float inverse_anchor[4] = {-anchor[0], -anchor[1], -anchor[2], anchor[3]};
    float relative[4];
    vr_quaternion_multiply_f(inverse_anchor, tracked, relative);
    vr_quaternion_normalize_f(relative);
    relative[2] = -relative[2];
    float scaled[4];
    vr_scale_quaternion_axes_f(relative, pitch_scale, 1.0f, roll_scale,
                               scaled);
    vr_quaternion_multiply_f(anchor, scaled, output_orientation);
    vr_quaternion_normalize_f(output_orientation);
}

static inline void vr_render_head_orientation_about_anchor_f(
    const float anchor_orientation[4], const float tracked_orientation[4],
    float output_orientation[4])
{
    vr_visual_head_orientation_about_anchor_scaled_f(
        anchor_orientation, tracked_orientation,
        VR_RENDER_PITCH_HEAD_MOTION_SCALE, VR_RENDER_ROLL_HEAD_MOTION_SCALE,
        output_orientation);
}

static inline void vr_visual_head_orientation_about_anchor_f(
    const float anchor_orientation[4], const float tracked_orientation[4],
    float output_orientation[4])
{
    vr_visual_head_orientation_about_anchor_scaled_f(
        anchor_orientation, tracked_orientation,
        VR_PITCH_HEAD_MOTION_SCALE, VR_ROLL_HEAD_MOTION_SCALE,
        output_orientation);
}

/* Solve the projection pose from the two visual endpoints. Axis scaling and
   the texture-roll reflection are not distributive across mixed yaw/pitch/roll
   rotations: scaling only render-to-live motion can therefore jump whenever a
   retained stereo pair changes. The pitch convention is now physically fixed
   at the OpenGL bridge boundary, so endpoint synthesis can guarantee that the
   apparent submitted orientation exactly reaches the current visual target. */
static inline void vr_reflected_timewarp_submission_pose_f(
    const float anchor_orientation[4],
    const float render_position[3], const float render_orientation[4],
    const float live_position[3], const float live_orientation[4],
    float output_position[3], float output_orientation[4])
{
    float render[4] = {render_orientation[0], render_orientation[1],
                       render_orientation[2], render_orientation[3]};
    float live[4] = {live_orientation[0], live_orientation[1],
                     live_orientation[2], live_orientation[3]};
    vr_quaternion_normalize_f(render);
    vr_quaternion_normalize_f(live);
    float visual_render[4], visual_live[4];
    vr_render_head_orientation_about_anchor_f(
        anchor_orientation, render, visual_render);
    vr_visual_head_orientation_about_anchor_f(
        anchor_orientation, live, visual_live);
    float inverse_visual_render[4] = {
        -visual_render[0], -visual_render[1],
        -visual_render[2], visual_render[3]
    };
    float scaled_delta[4];
    vr_quaternion_multiply_f(
        inverse_visual_render, visual_live, scaled_delta);
    vr_quaternion_normalize_f(scaled_delta);
    float inverse_reflected[4] = {
        -scaled_delta[0], -scaled_delta[1], -scaled_delta[2], scaled_delta[3]
    };
    vr_quaternion_multiply_f(live, inverse_reflected, output_orientation);
    vr_quaternion_normalize_f(output_orientation);

    /* Translation must be expressed in the orientation actually baked into
       the image.  That used to equal the raw render pose on every axis.  Roll
       is now intentionally compositor-driven, so using the raw tracked roll
       here would couple lateral lean to a rotation absent from the texture
       and cause a snap whenever the retained pair changed. */
    float render_matrix[16];
    vr_quaternion_matrix_f(visual_render, render_matrix);
    float world_delta[3] = {
        live_position[0] - render_position[0],
        live_position[1] - render_position[1],
        live_position[2] - render_position[2]
    };
    float delta_position[3] = {
        render_matrix[0] * world_delta[0] + render_matrix[1] * world_delta[1] +
            render_matrix[2] * world_delta[2],
        render_matrix[4] * world_delta[0] + render_matrix[5] * world_delta[1] +
            render_matrix[6] * world_delta[2],
        render_matrix[8] * world_delta[0] + render_matrix[9] * world_delta[1] +
            render_matrix[10] * world_delta[2]
    };

    delta_position[1] *= -VR_VERTICAL_HEAD_MOTION_SCALE;
    float reflected_matrix[16];
    vr_quaternion_matrix_f(scaled_delta, reflected_matrix);
    float inverse_delta_position[3] = {
        -(reflected_matrix[0] * delta_position[0] +
          reflected_matrix[1] * delta_position[1] +
          reflected_matrix[2] * delta_position[2]),
        -(reflected_matrix[4] * delta_position[0] +
          reflected_matrix[5] * delta_position[1] +
          reflected_matrix[6] * delta_position[2]),
        -(reflected_matrix[8] * delta_position[0] +
          reflected_matrix[9] * delta_position[1] +
          reflected_matrix[10] * delta_position[2])
    };
    float live_matrix[16];
    vr_quaternion_matrix_f(live, live_matrix);
    output_position[0] = live_position[0] +
        live_matrix[0] * inverse_delta_position[0] +
        live_matrix[4] * inverse_delta_position[1] +
        live_matrix[8] * inverse_delta_position[2];
    output_position[1] = live_position[1] +
        live_matrix[1] * inverse_delta_position[0] +
        live_matrix[5] * inverse_delta_position[1] +
        live_matrix[9] * inverse_delta_position[2];
    output_position[2] = live_position[2] +
        live_matrix[2] * inverse_delta_position[0] +
        live_matrix[6] * inverse_delta_position[1] +
        live_matrix[10] * inverse_delta_position[2];
}

static inline int vr_projection_optical_center_x_f(float angle_left,
                                                    float angle_right,
                                                    float *center)
{
    if (!center || !isfinite(angle_left) || !isfinite(angle_right)) return 0;
    float left = tanf(angle_left), right = tanf(angle_right);
    float width = right - left;
    if (!isfinite(width) || width <= 0.0001f) return 0;
    *center = -left / width;
    return isfinite(*center) && *center > 0.0f && *center < 1.0f;
}

/* Build the camera transform caused by moving a tracked head from anchor to
   current pose. OpenXR and OpenGL camera space both use X right, Y up and -Z
   forward. At the anchor this is identity. Rotating the head produces the
   inverse rotation of the world, and translating the head produces the
   correspondingly rotated inverse translation. */
static inline void vr_build_head_view_delta_f(const float anchor_position[3],
                                               const float anchor_orientation[4],
                                               const float current_position[3],
                                               const float current_orientation[4],
                                               float units_per_metre,
                                               float matrix[16])
{
    float anchor[4] = {anchor_orientation[0], anchor_orientation[1],
                       anchor_orientation[2], anchor_orientation[3]};
    float inverse_current[4] = {-current_orientation[0], -current_orientation[1],
                                -current_orientation[2], current_orientation[3]};
    vr_quaternion_normalize_f(anchor);
    vr_quaternion_normalize_f(inverse_current);
    float relative[4];
    vr_quaternion_multiply_f(inverse_current, anchor, relative);
    vr_quaternion_normalize_f(relative);
    /* Physical Quest acceptance is authoritative here.  The ordinary inverse
       view pitch is the natural camera response after the final framebuffer
       copy.  Reflecting X at this point made head-up drive the camera down even
       though the submitted OpenXR pose itself had the expected sign. */
    float scaled_relative[4];
    vr_scale_quaternion_axes_f(relative,
                               VR_RENDER_PITCH_HEAD_MOTION_SCALE, 1.0f,
                               VR_RENDER_ROLL_HEAD_MOTION_SCALE,
                               scaled_relative);
    for (int axis = 0; axis < 4; ++axis) relative[axis] = scaled_relative[axis];
    vr_quaternion_matrix_f(relative, matrix);

    float displacement[3] = {
        (anchor_position[0] - current_position[0]) * units_per_metre,
        (anchor_position[1] - current_position[1]) * units_per_metre,
        (anchor_position[2] - current_position[2]) * units_per_metre
    };
    displacement[1] *= VR_VERTICAL_HEAD_MOTION_SCALE;
    matrix[12] = matrix[0] * displacement[0] + matrix[4] * displacement[1] +
                 matrix[8] * displacement[2];
    matrix[13] = matrix[1] * displacement[0] + matrix[5] * displacement[1] +
                 matrix[9] * displacement[2];
    matrix[14] = matrix[2] * displacement[0] + matrix[6] * displacement[1] +
                 matrix[10] * displacement[2];
}

/* Reproject a fixed-HUD target bearing through the visual head rotation. This
   preserves the game's target selection and distance logic while making its
   arrow position and orientation agree with the direction currently viewed in
   the headset. Translation is intentionally ignored: these remain indicators,
   not world geometry. */
static inline int vr_reproject_hud_indicator_f(
    float x, float y, float canvas_width, float canvas_height,
    float horizontal_half_tangent, float vertical_half_tangent,
    const float head_view[16], float margin,
    float *output_x, float *output_y, float *rotation_delta)
{
    if (!head_view || !output_x || !output_y || !rotation_delta ||
        !isfinite(x) || !isfinite(y) || canvas_width <= 1.0f ||
        canvas_height <= 1.0f || horizontal_half_tangent <= 0.0001f ||
        vertical_half_tangent <= 0.0001f) return 0;
    float center_x = canvas_width * 0.5f;
    float center_y = canvas_height * 0.5f;
    float half_width = center_x;
    float half_height = center_y;
    float ray[3] = {
        (x - center_x) * horizontal_half_tangent / half_width,
        -(y - center_y) * vertical_half_tangent / half_height,
        -1.0f
    };
    float transformed[3] = {
        head_view[0] * ray[0] + head_view[4] * ray[1] + head_view[8] * ray[2],
        head_view[1] * ray[0] + head_view[5] * ray[1] + head_view[9] * ray[2],
        head_view[2] * ray[0] + head_view[6] * ray[1] + head_view[10] * ray[2]
    };
    float forward = -transformed[2];
    if (!isfinite(forward) || forward < 0.05f) return 0;
    float projected_x = center_x +
        (transformed[0] / forward) * half_width / horizontal_half_tangent;
    float projected_y = center_y -
        (transformed[1] / forward) * half_height / vertical_half_tangent;
    if (!isfinite(projected_x) || !isfinite(projected_y)) return 0;
    if (margin < 0.0f) margin = 0.0f;
    float max_x = canvas_width - margin;
    float max_y = canvas_height - margin;
    if (projected_x < margin) projected_x = margin;
    if (projected_x > max_x) projected_x = max_x;
    if (projected_y < margin) projected_y = margin;
    if (projected_y > max_y) projected_y = max_y;
    float old_angle = atan2f(y - center_y, x - center_x);
    float new_angle = atan2f(projected_y - center_y,
                             projected_x - center_x);
    float delta = new_angle - old_angle;
    const float pi = 3.14159265358979323846f;
    while (delta > pi) delta -= 2.0f * pi;
    while (delta < -pi) delta += 2.0f * pi;
    *output_x = projected_x;
    *output_y = projected_y;
    *rotation_delta = delta;
    return 1;
}

#endif
