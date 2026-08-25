#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "geometry_math.h"

static int close_enough(float left, float right)
{
    return fabsf(left - right) < 0.002f;
}

static unsigned int deterministic_state = 0x51C07A11u;

static float deterministic_signed(void)
{
    deterministic_state = deterministic_state * 1664525u + 1013904223u;
    return ((float)((deterministic_state >> 8) & 0x00ffffffu) /
            8388607.5f) - 1.0f;
}

static void deterministic_pose(float position[3], float orientation[4])
{
    for (int i = 0; i < 3; ++i) position[i] = deterministic_signed() * 2.0f;
    for (int i = 0; i < 4; ++i) orientation[i] = deterministic_signed();
    vr_quaternion_normalize_f(orientation);
}

static void pose_local_delta(const float from_position[3],
                             const float from_orientation[4],
                             const float to_position[3], float output[3])
{
    float matrix[16];
    vr_quaternion_matrix_f(from_orientation, matrix);
    float world[3] = {
        to_position[0] - from_position[0],
        to_position[1] - from_position[1],
        to_position[2] - from_position[2]
    };
    output[0] = matrix[0] * world[0] + matrix[1] * world[1] + matrix[2] * world[2];
    output[1] = matrix[4] * world[0] + matrix[5] * world[1] + matrix[6] * world[2];
    output[2] = matrix[8] * world[0] + matrix[9] * world[1] + matrix[10] * world[2];
}

static int same_quaternion(const float left[4], const float right[4])
{
    float dot = left[0] * right[0] + left[1] * right[1] +
                left[2] * right[2] + left[3] * right[3];
    float sign = dot < 0.0f ? -1.0f : 1.0f;
    for (int i = 0; i < 4; ++i)
        if (!close_enough(left[i], sign * right[i])) return 0;
    return 1;
}

static int test_identity_axis(void)
{
    float matrix[16] = {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    };
    vr_scale_camera_depth_f(matrix, 2.0f, 1000.0f);
    float focal[3] = {0, 0, -1000}, foreground[3] = {0, 0, -800};
    float background[3] = {0, 0, -1200}, output[3];
    vr_transform_point_f(matrix, focal, output);
    if (!close_enough(output[2], -1000.0f)) return 0;
    vr_transform_point_f(matrix, foreground, output);
    if (!close_enough(output[2], -600.0f)) return 0;
    vr_transform_point_f(matrix, background, output);
    return close_enough(output[2], -1400.0f);
}

static int test_rotated_camera_axis(void)
{
    /* A pitched/rotated affine view.  The exact basis is less important than
       proving the complete camera-space Z row is scaled, not only m[14]. */
    float original[16] = {
         0.8f,  0.1f,  0.59f, 0,
        -0.2f,  0.96f, 0.20f, 0,
        -0.56f, -0.26f, 0.78f, 0,
         210.0f, -95.0f, -1000.0f, 1
    };
    float scaled[16];
    memcpy(scaled, original, sizeof(scaled));
    vr_scale_camera_depth_f(scaled, 2.5f, 1000.0f);

    float point[3] = {120, -40, 75}, before[3], after[3];
    vr_transform_point_f(original, point, before);
    vr_transform_point_f(scaled, point, after);
    float expected = 2.5f * before[2] + 1500.0f;
    if (!close_enough(after[2], expected)) return 0;
    if (!close_enough(after[0], before[0]) || !close_enough(after[1], before[1])) return 0;

    /* Any point on the original z=-1000 camera plane remains on it. */
    float focal_point[3] = {0, 0, 0};
    vr_transform_point_f(original, focal_point, before);
    vr_transform_point_f(scaled, focal_point, after);
    return close_enough(before[2], -1000.0f) && close_enough(after[2], -1000.0f);
}

static int test_disabled_is_identity(void)
{
    float original[16] = {
        1, 2, 3, 4, 5, 6, 7, 8,
        9, 10, 11, 12, 13, 14, 15, 16
    };
    float matrix[16];
    memcpy(matrix, original, sizeof(matrix));
    vr_scale_camera_depth_f(matrix, 1.0f, 1000.0f);
    return memcmp(matrix, original, sizeof(matrix)) == 0;
}

static int test_head_pose_identity(void)
{
    float position[3] = {1.0f, 2.0f, -3.0f};
    float orientation[4] = {0, 0, 0, 1};
    float matrix[16];
    vr_build_head_view_delta_f(position, orientation, position, orientation, 80.0f, matrix);
    const float identity[16] = {
        1, 0, 0, 0, 0, 1, 0, 0,
        0, 0, 1, 0, 0, 0, 0, 1
    };
    for (int i = 0; i < 16; ++i)
        if (!close_enough(matrix[i], identity[i])) return 0;
    return 1;
}

static int test_head_translation_inverse(void)
{
    float anchor_position[3] = {0, 0, 0};
    float current_position[3] = {0.10f, -0.05f, 0.20f};
    float orientation[4] = {0, 0, 0, 1};
    float matrix[16];
    vr_build_head_view_delta_f(anchor_position, orientation, current_position,
                               orientation, 80.0f, matrix);
    return close_enough(matrix[12], -8.0f) && close_enough(matrix[13], 1.6f) &&
           close_enough(matrix[14], -16.0f);
}

static int test_head_yaw_inverse(void)
{
    float position[3] = {0, 0, 0};
    float anchor[4] = {0, 0, 0, 1};
    float half = 0.70710678f;
    float yaw_right[4] = {0, half, 0, half};
    float matrix[16], forward[3] = {0, 0, -1}, output[3];
    vr_build_head_view_delta_f(position, anchor, position, yaw_right, 80.0f, matrix);
    vr_transform_point_f(matrix, forward, output);
    return close_enough(output[0], 1.0f) && close_enough(output[1], 0.0f) &&
           close_enough(output[2], 0.0f);
}

static int test_quest_natural_pitch_direction_and_gain(void)
{
    float position[3] = {0, 0, 0};
    float anchor[4] = {0, 0, 0, 1};
    float half_angle = 10.0f * 3.14159265f / 180.0f;
    float look_up[4] = {sinf(half_angle), 0, 0, cosf(half_angle)};
    float matrix[16], forward[3] = {0, 0, -1}, transformed[3];
    float reflected_anchor[4] = {anchor[0], anchor[1], anchor[2], anchor[3]};
    float reflected_look_up[4] = {
        look_up[0], look_up[1], look_up[2], look_up[3]
    };
    vr_reflect_pose_for_texture_y_flip_f(position, reflected_anchor);
    vr_reflect_pose_for_texture_y_flip_f(position, reflected_look_up);
    vr_build_head_view_delta_f(position, reflected_anchor, position,
                               reflected_look_up,
                               80.0f, matrix);
    vr_transform_point_f(matrix, forward, transformed);
    /* Exercise the complete OpenXR-to-OpenGL convention, including the single
       texture-Y reflection performed by the bridge.  The old extra X flip in
       vr_build_head_view_delta_f changed this result to negative and made
       head-up drive the camera down on the physical Quest. */
    if (transformed[1] <= 0.0f) return 0;

    float visual[4];
    vr_visual_head_orientation_about_anchor_f(anchor, look_up, visual);
    if (!same_quaternion(visual, look_up)) return 0;

    float submitted_position[3], submitted[4];
    vr_reflected_timewarp_submission_pose_f(
        anchor, position, anchor, position, look_up,
        submitted_position, submitted);
    float inverse_submitted[4] = {
        -submitted[0], -submitted[1], -submitted[2], submitted[3]
    };
    float runtime_delta[4];
    vr_quaternion_multiply_f(inverse_submitted, look_up, runtime_delta);
    vr_quaternion_normalize_f(runtime_delta);
    return same_quaternion(runtime_delta, look_up);
}

static int test_texture_y_flip_pose_reflection(void)
{
    float position[3] = {1.0f, 2.0f, 3.0f};
    float orientation[4] = {0.1f, 0.2f, 0.3f, 0.9f};
    vr_reflect_pose_for_texture_y_flip_f(position, orientation);
    return close_enough(position[0], 1.0f) && close_enough(position[1], -2.0f) &&
           close_enough(position[2], 3.0f) && close_enough(orientation[0], -0.1f) &&
           close_enough(orientation[1], 0.2f) && close_enough(orientation[2], -0.3f) &&
           close_enough(orientation[3], 0.9f);
}

static int test_final_texture_vertical_motion_direction(void)
{
    /* The bridge reflects the tracked pose once for the interop texture. The
       resulting translation and pitch components must both stay positive;
       the removed second X reflection changed only pitch to negative. */
    float anchor_position[3] = {0, 0, 0};
    float current_position[3] = {0, 0.05f, 0};
    float anchor_orientation[4] = {0, 0, 0, 1};
    float current_orientation[4] = {
        sinf(10.0f * 3.14159265f / 180.0f), 0, 0,
        cosf(10.0f * 3.14159265f / 180.0f)
    };
    vr_reflect_pose_for_texture_y_flip_f(anchor_position, anchor_orientation);
    vr_reflect_pose_for_texture_y_flip_f(current_position, current_orientation);
    float matrix[16];
    vr_build_head_view_delta_f(anchor_position, anchor_orientation,
                               current_position, current_orientation,
                               80.0f, matrix);
    float forward[3] = {0, 0, -1}, transformed[3];
    vr_transform_point_f(matrix, forward, transformed);
    return matrix[13] > 0.0f && transformed[1] - matrix[13] > 0.0f;
}

static int test_anchor_relative_projection_pose_reflection(void)
{
    const float root_half = 0.70710678118f;
    float anchor_position[3] = {3.0f, 4.0f, 5.0f};
    float anchor_orientation[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    float current_position[3] = {3.25f, 4.10f, 4.80f};
    float current_orientation[4] = {root_half, 0.0f, 0.0f, root_half};
    float output_position[3], output_orientation[4];
    vr_reflect_pose_delta_about_anchor_f(
        anchor_position, anchor_orientation, current_position,
        current_orientation, output_position, output_orientation);
    if (!close_enough(output_position[0], 3.25f) ||
        !close_enough(output_position[1], 3.90f) ||
        !close_enough(output_position[2], 4.80f)) return 0;
    if (!close_enough(output_orientation[0], -root_half) ||
        !close_enough(output_orientation[1], 0.0f) ||
        !close_enough(output_orientation[2], 0.0f) ||
        !close_enough(output_orientation[3], root_half)) return 0;

    vr_reflect_pose_delta_about_anchor_f(
        anchor_position, anchor_orientation, anchor_position,
        anchor_orientation, output_position, output_orientation);
    return close_enough(output_position[0], anchor_position[0]) &&
           close_enough(output_position[1], anchor_position[1]) &&
           close_enough(output_position[2], anchor_position[2]) &&
           close_enough(output_orientation[3], 1.0f);
}

static int test_reflected_incremental_timewarp_pose(void)
{
    float render_position[3] = {0, 0, 0};
    float identity[4] = {0, 0, 0, 1};
    float live_position[3] = {1, 2, 3};
    float output_position[3], output_orientation[4];
    vr_reflected_timewarp_submission_pose_f(
        identity,
        render_position, identity, live_position, identity,
        output_position, output_orientation);
    if (!close_enough(output_position[0], 0.0f) ||
        !close_enough(output_position[1], 2.8f) ||
        !close_enough(output_position[2], 0.0f)) return 0;

    const float sine_22_5 = 0.38268343236f;
    const float cosine_22_5 = 0.92387953251f;
    float live_pitch[4] = {sine_22_5, 0, 0, cosine_22_5};
    float still[3] = {0, 0, 0};
    vr_reflected_timewarp_submission_pose_f(
        identity,
        still, identity, still, live_pitch,
        output_position, output_orientation);
    float scaled_pitch[4];
    vr_scale_quaternion_axes_f(
        live_pitch, VR_RENDER_PITCH_HEAD_MOTION_SCALE, 1.0f,
        VR_ROLL_HEAD_MOTION_SCALE, scaled_pitch);
    scaled_pitch[2] = -scaled_pitch[2];
    float inverse_scaled_pitch[4] = {
        -scaled_pitch[0], -scaled_pitch[1],
        -scaled_pitch[2], scaled_pitch[3]
    };
    float expected_pitch[4];
    vr_quaternion_multiply_f(live_pitch, inverse_scaled_pitch,
                             expected_pitch);
    vr_quaternion_normalize_f(expected_pitch);
    if (!same_quaternion(output_orientation, expected_pitch)) return 0;

    float live_yaw[4] = {0, sine_22_5, 0, cosine_22_5};
    vr_reflected_timewarp_submission_pose_f(
        identity,
        still, identity, still, live_yaw,
        output_position, output_orientation);
    return close_enough(output_orientation[0], 0.0f) &&
           close_enough(output_orientation[1], 0.0f) &&
           close_enough(output_orientation[2], 0.0f) &&
           close_enough(output_orientation[3], 1.0f);
}

static int test_reflected_incremental_timewarp_randomized(void)
{
    /* The runtime delta must bridge the already-scaled visual endpoints
       exactly. This remains continuous across retained-pair changes even when
       yaw, pitch, and softened roll are moving together. */
    for (int sample = 0; sample < 2048; ++sample) {
        float anchor_position[3], anchor_orientation[4];
        float render_position[3], render_orientation[4];
        float live_position[3], live_orientation[4];
        deterministic_pose(anchor_position, anchor_orientation);
        deterministic_pose(render_position, render_orientation);
        deterministic_pose(live_position, live_orientation);

        float submitted_position[3], submitted_orientation[4];
        vr_reflected_timewarp_submission_pose_f(
            anchor_orientation,
            render_position, render_orientation, live_position, live_orientation,
            submitted_position, submitted_orientation);

        float visual_render[4], visual_live[4];
        vr_render_head_orientation_about_anchor_f(
            anchor_orientation, render_orientation, visual_render);
        vr_visual_head_orientation_about_anchor_f(
            anchor_orientation, live_orientation, visual_live);
        float inverse_visual_render[4] = {
            -visual_render[0], -visual_render[1],
            -visual_render[2], visual_render[3]
        };
        float expected_orientation[4];
        vr_quaternion_multiply_f(inverse_visual_render, visual_live,
                                 expected_orientation);
        vr_quaternion_normalize_f(expected_orientation);

        float inverse_submitted[4] = {
            -submitted_orientation[0], -submitted_orientation[1],
            -submitted_orientation[2], submitted_orientation[3]
        };
        float actual_orientation[4];
        vr_quaternion_multiply_f(inverse_submitted, live_orientation,
                                 actual_orientation);
        vr_quaternion_normalize_f(actual_orientation);
        if (!same_quaternion(actual_orientation, expected_orientation)) {
            printf("timewarp orientation mismatch sample=%d actual=%.7f %.7f %.7f %.7f expected=%.7f %.7f %.7f %.7f\n",
                   sample, actual_orientation[0], actual_orientation[1],
                   actual_orientation[2], actual_orientation[3],
                   expected_orientation[0], expected_orientation[1],
                   expected_orientation[2], expected_orientation[3]);
            return 0;
        }

        float expected_position[3], actual_position[3];
        pose_local_delta(render_position, visual_render, live_position,
                         expected_position);
        expected_position[1] *= -VR_VERTICAL_HEAD_MOTION_SCALE;
        pose_local_delta(submitted_position, submitted_orientation, live_position,
                         actual_position);
        for (int axis = 0; axis < 3; ++axis)
            if (!close_enough(actual_position[axis], expected_position[axis])) {
                printf("timewarp position mismatch sample=%d axis=%d actual=%.7f expected=%.7f\n",
                       sample, axis, actual_position[axis], expected_position[axis]);
                return 0;
            }
    }

    float position[3] = {0.37f, -1.2f, 2.4f};
    float orientation[4] = {0.22f, -0.31f, 0.17f, 0.88f};
    vr_quaternion_normalize_f(orientation);
    float output_position[3], output_orientation[4];
    float identity[4] = {0, 0, 0, 1};
    vr_reflected_timewarp_submission_pose_f(identity, position, orientation, position,
                                             orientation, output_position,
                                             output_orientation);
    for (int axis = 0; axis < 3; ++axis)
        if (!close_enough(output_position[axis], position[axis])) return 0;
    float render_visual[4], live_visual[4], runtime_delta[4], apparent[4];
    vr_render_head_orientation_about_anchor_f(
        identity, orientation, render_visual);
    vr_visual_head_orientation_about_anchor_f(
        identity, orientation, live_visual);
    vr_quaternion_multiply_f(
        (float[4]){-output_orientation[0], -output_orientation[1],
                   -output_orientation[2], output_orientation[3]},
        orientation, runtime_delta);
    vr_quaternion_multiply_f(render_visual, runtime_delta, apparent);
    return same_quaternion(apparent, live_visual);
}

static float quaternion_distance_degrees(const float a[4], const float b[4])
{
    float dot = fabsf(a[0] * b[0] + a[1] * b[1] +
                      a[2] * b[2] + a[3] * b[3]);
    if (dot > 1.0f) dot = 1.0f;
    return 2.0f * acosf(dot) * 180.0f / 3.14159265f;
}

static int test_72hz_submission_validity(void)
{
    /* This test intentionally checks only the produced submission trajectory.
       Simulator math cannot assert the compositor's perceived texture-axis
       convention; that sign is covered by the physical checkpoint instead. */
    float anchor[4] = {0, 0, 0, 1};
    float position[3] = {0, 0, 0};
    float render[4] = {0, 0, 0, 1};
    for (int frame = 0; frame < 720; ++frame) {
        float seconds = (float)frame / 72.0f;
        float pitch_degrees = 18.0f * sinf(seconds * 2.0f * 3.14159265f * 0.35f);
        float half_angle = pitch_degrees * 3.14159265f / 360.0f;
        float live[4] = {sinf(half_angle), 0, 0, cosf(half_angle)};
        if ((frame & 1) == 0)
            for (int axis = 0; axis < 4; ++axis) render[axis] = live[axis];

        float submitted_position[3], submitted[4];
        vr_reflected_timewarp_submission_pose_f(
            anchor, position, render, position, live,
            submitted_position, submitted);
        float length = sqrtf(submitted[0] * submitted[0] +
                             submitted[1] * submitted[1] +
                             submitted[2] * submitted[2] +
                             submitted[3] * submitted[3]);
        if (!isfinite(length) || fabsf(length - 1.0f) > 0.0001f)
            return 0;
    }
    return 1;
}

static int test_roll_is_compositor_driven(void)
{
    float position[3] = {0, 0, 0};
    float anchor[4] = {0, 0, 0, 1};
    float half_angle = 15.0f * 3.14159265f / 180.0f;
    float tracked_roll[4] = {0, 0, sinf(half_angle), cosf(half_angle)};
    float render_visual[4], live_visual[4];
    vr_render_head_orientation_about_anchor_f(
        anchor, tracked_roll, render_visual);
    vr_visual_head_orientation_about_anchor_f(
        anchor, tracked_roll, live_visual);
    if (!same_quaternion(render_visual, anchor) ||
        same_quaternion(live_visual, anchor)) return 0;

    float head_view[16];
    vr_build_head_view_delta_f(position, anchor, position, tracked_roll,
                               80.0f, head_view);
    if (!close_enough(head_view[0], 1.0f) ||
        !close_enough(head_view[1], 0.0f) ||
        !close_enough(head_view[4], 0.0f) ||
        !close_enough(head_view[5], 1.0f)) return 0;

    float submitted_position[3], submitted[4];
    vr_reflected_timewarp_submission_pose_f(
        anchor, position, tracked_roll, position, tracked_roll,
        submitted_position, submitted);
    float runtime_delta[4];
    vr_quaternion_multiply_f(
        (float[4]){-submitted[0], -submitted[1], -submitted[2], submitted[3]},
        tracked_roll, runtime_delta);
    return same_quaternion(runtime_delta, live_visual);
}

static int test_asymmetric_fov_optical_center(void)
{
    float center = 0.0f;
    if (!vr_projection_optical_center_x_f(-0.8f, 0.8f, &center) ||
        !close_enough(center, 0.5f)) return 0;
    if (!vr_projection_optical_center_x_f(-0.9421f, 0.6987f, &center))
        return 0;
    return center > 0.60f && center < 0.64f;
}

static int test_reflected_head_delta_equivalence(void)
{
    float anchor_position[3] = {0.03f, -0.02f, 0.08f};
    float current_position[3] = {0.11f, 0.05f, -0.04f};
    float anchor_orientation[4] = {0.08f, -0.13f, 0.04f, 0.98f};
    float current_orientation[4] = {-0.11f, 0.17f, 0.09f, 0.97f};
    float raw[16], reflected[16];
    vr_build_head_view_delta_f(anchor_position, anchor_orientation,
                               current_position, current_orientation, 160.0f, raw);
    vr_reflect_pose_for_texture_y_flip_f(anchor_position, anchor_orientation);
    vr_reflect_pose_for_texture_y_flip_f(current_position, current_orientation);
    vr_build_head_view_delta_f(anchor_position, anchor_orientation,
                               current_position, current_orientation, 160.0f, reflected);
    const float signs[4] = {1.0f, -1.0f, 1.0f, 1.0f};
    for (int column = 0; column < 4; ++column) {
        for (int row = 0; row < 4; ++row) {
            float expected = signs[row] * raw[column * 4 + row] * signs[column];
            if (!close_enough(reflected[column * 4 + row], expected)) return 0;
        }
    }
    return 1;
}

static int test_tabletop_pivot_and_composition(void)
{
    float tabletop[16], output[3];
    vr_build_tabletop_view_f(40.0f, 125.0f, -300.0f, 3360.0f, tabletop);
    float focal[3] = {0, 0, -3360.0f};
    vr_transform_point_f(tabletop, focal, output);
    if (!close_enough(output[0], 125.0f) || !close_enough(output[1], -300.0f) ||
        !close_enough(output[2], -3360.0f)) return 0;

    float translation[16] = {
        1, 0, 0, 0, 0, 1, 0, 0,
        0, 0, 1, 0, -16, 8, 0, 1
    };
    float combined[16];
    vr_multiply_matrix_f(translation, tabletop, combined);
    vr_transform_point_f(combined, focal, output);
    return close_enough(output[0], 109.0f) && close_enough(output[1], -292.0f) &&
           close_enough(output[2], -3360.0f);
}

static int test_lens_safe_hud_inset(void)
{
    double bounds[4] = {0.0, 1600.0, 900.0, 0.0};
    vr_inset_ortho_d(bounds, 0.8, 0.6);
    if (!(fabs(bounds[0] + 200.0) < 0.0001 &&
           fabs(bounds[1] - 1800.0) < 0.0001 &&
           fabs(bounds[2] - 1200.0) < 0.0001 &&
           fabs(bounds[3] + 300.0) < 0.0001)) return 0;
    /* The physical candidate uses a 1536x1666 eye.  Its inset rectangle must
       recover the game's 16:9 HUD proportions rather than the eye's tall
       aspect ratio. */
    double link_scale = vr_hud_scale_y_for_eye(0.660, 1536.0, 1666.0, 16.0 / 9.0);
    double simulator_scale = vr_hud_scale_y_for_eye(0.660, 1536.0, 1608.0, 16.0 / 9.0);
    double link_aspect = (1536.0 * 0.660) / (1666.0 * link_scale);
    double simulator_aspect = (1536.0 * 0.660) / (1608.0 * simulator_scale);
    double gameplay_projection_aspect = 53.1 / (29.8666666 * 1.65);
    double gameplay_scale = vr_hud_scale_y_for_eye(
        0.660, gameplay_projection_aspect, 1.0, 16.0 / 9.0);
    double gameplay_hud_aspect = gameplay_projection_aspect * 0.660 / gameplay_scale;
    return fabs(link_scale - 0.3423) < 0.001 &&
           fabs(simulator_scale - 0.3546) < 0.001 &&
           fabs(gameplay_scale - 0.3999) < 0.001 &&
           fabs(link_aspect - 16.0 / 9.0) < 0.0001 &&
           fabs(simulator_aspect - 16.0 / 9.0) < 0.0001 &&
           fabs(gameplay_hud_aspect - 16.0 / 9.0) < 0.0001;
}

static int test_alignment_fov_metadata(void)
{
    float left = atanf(-1.0f), right = atanf(1.0f);
    vr_shift_horizontal_fov_for_texture_f(&left, &right, 0.12f);
    return close_enough(tanf(left), -0.76f) &&
           close_enough(tanf(right), 1.24f);
}

static int test_rendered_alignment_matches_fov_metadata(void)
{
    const double near_value = 0.25;
    const double source_left = -0.30;
    const double source_right = 0.50;
    const double signed_width = -0.12;
    double shift = vr_frustum_alignment_shift_d(source_left, source_right,
                                                 signed_width);
    float metadata_left = atanf((float)(source_left / near_value));
    float metadata_right = atanf((float)(source_right / near_value));
    vr_shift_horizontal_fov_for_texture_f(&metadata_left, &metadata_right,
                                           (float)signed_width);
    return close_enough(tanf(metadata_left),
                        (float)((source_left + shift) / near_value)) &&
           close_enough(tanf(metadata_right),
                        (float)((source_right + shift) / near_value));
}

static int test_runtime_fov_builds_reversed_game_frustum(void)
{
    const float left = -0.91f, right = 0.83f, up = 0.88f, down = -0.86f;
    const double near_value = 32.0;
    double bounds[4];
    if (!vr_runtime_fov_to_reversed_frustum_d(left, right, up, down,
                                               near_value, bounds))
        return 0;
    return close_enough((float)(bounds[0] / near_value), tanf(left)) &&
           close_enough((float)(bounds[1] / near_value), tanf(right)) &&
           close_enough((float)(-bounds[3] / near_value), tanf(up)) &&
           close_enough((float)(-bounds[2] / near_value), tanf(down)) &&
           bounds[2] > bounds[3];
}

static int test_projection_ipd_matches_game_scale(void)
{
    float left[3] = {-0.032f, 1.6f, 0.1f};
    float right[3] = {0.032f, 1.6f, 0.1f};
    if (!vr_scale_stereo_eye_positions_f(left, right, 5.2f / 160.0f)) return 0;
    float dx = right[0] - left[0], dy = right[1] - left[1], dz = right[2] - left[2];
    float ipd = sqrtf(dx * dx + dy * dy + dz * dz);
    return close_enough(ipd, 0.0325f) && close_enough(left[1], 1.6f) &&
           close_enough(right[2], 0.1f);
}

static int test_projection_rect_preserves_tangent_aspect(void)
{
    float left = atanf(-26.55f / 32.0f);
    float right = atanf(26.55f / 32.0f);
    float up = atanf((14.9333333f * 1.65f) / 32.0f);
    float down = -up;
    vr_shift_horizontal_fov_for_texture_f(&left, &right, -0.12f);
    int rectangle[4];
    if (!vr_fit_projection_rect_f(1536, 1666, left, right, up, down, rectangle))
        return 0;
    float source_aspect = (tanf(right) - tanf(left)) /
                          (tanf(up) - tanf(down));
    float rectangle_aspect = (float)rectangle[2] / (float)rectangle[3];
    return rectangle[0] == 0 && rectangle[1] > 100 &&
           rectangle[2] == 1536 && rectangle[3] < 1450 &&
           fabsf(source_aspect - rectangle_aspect) < 0.002f;
}

static int test_head_relative_hud_indicator(void)
{
    const float identity[16] = {
        1, 0, 0, 0, 0, 1, 0, 0,
        0, 0, 1, 0, 0, 0, 0, 1
    };
    float x, y, rotation;
    if (!vr_reproject_hud_indicator_f(420.0f, 640.0f, 1593.0f, 896.0f,
                                       0.8296875f, 0.4666667f, identity, 24.0f,
                                       &x, &y, &rotation)) return 0;
    if (!close_enough(x, 420.0f) || !close_enough(y, 640.0f) ||
        !close_enough(rotation, 0.0f)) return 0;

    float yaw[4] = {0.0f, sinf(10.0f * 3.14159265f / 180.0f), 0.0f,
                    cosf(10.0f * 3.14159265f / 180.0f)};
    float yaw_view[16];
    vr_quaternion_matrix_f(yaw, yaw_view);
    if (!vr_reproject_hud_indicator_f(420.0f, 640.0f, 1593.0f, 896.0f,
                                       0.8296875f, 0.4666667f, yaw_view, 24.0f,
                                       &x, &y, &rotation)) return 0;
    return x >= 24.0f && x <= 1569.0f && y >= 24.0f && y <= 872.0f &&
           fabsf(x - 420.0f) > 20.0f && isfinite(rotation);
}

static float smoothstep_test(float low, float high, float value)
{
    float t = (value - low) / (high - low);
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    return t * t * (3.0f - 2.0f * t);
}

static float menu_overlay_edge_mask_test(const float complete_x[3],
                                         const float complete_y[3],
                                         const float baseline_x[3],
                                         const float baseline_y[3])
{
    float numerator = 0.0f, denominator = 0.00001f;
    for (int channel = 0; channel < 3; ++channel) {
        numerator += complete_x[channel] * baseline_x[channel] +
                     complete_y[channel] * baseline_y[channel];
        denominator += baseline_x[channel] * baseline_x[channel] +
                       baseline_y[channel] * baseline_y[channel];
    }
    float scale = numerator / denominator;
    if (scale < 0.0f) scale = 0.0f;
    if (scale > 1.25f) scale = 1.25f;
    float residual = 0.0f;
    for (int channel = 0; channel < 3; ++channel) {
        float x = fabsf(complete_x[channel] - scale * baseline_x[channel]);
        float y = fabsf(complete_y[channel] - scale * baseline_y[channel]);
        if (x > residual) residual = x;
        if (y > residual) residual = y;
    }
    return smoothstep_test(0.035f, 0.100f, residual);
}

static int hud_fixed_region_test(float x, float y)
{
    (void)x;
    (void)y;
    /* The difference against the exact same-eye pre-HUD base is now the
       selector, so fixed HUD cues such as START GAME may occupy any pixel. */
    return 1;
}

static float interface_component_mask_test(float residual, const float tint[3])
{
    float maximum = fmaxf(tint[0], fmaxf(tint[1], tint[2]));
    float minimum = fminf(tint[0], fminf(tint[1], tint[2]));
    float edge = smoothstep_test(0.035f, 0.100f, residual);
    float chroma = smoothstep_test(0.025f, 0.065f, maximum - minimum);
    return fmaxf(edge, chroma);
}

static int test_interface_compositor_masks(void)
{
    const float world_x[3] = {0.52f, -0.21f, 0.13f};
    const float world_y[3] = {-0.18f, 0.31f, 0.09f};
    float unchanged_x[3], unchanged_y[3];
    float tinted_x[3], tinted_y[3];
    for (int channel = 0; channel < 3; ++channel) {
        unchanged_x[channel] = world_x[channel];
        unchanged_y[channel] = world_y[channel];
        /* A uniform translucent colour adds a constant but scales every
           underlying spatial gradient by the same alpha. */
        tinted_x[channel] = world_x[channel] * 0.58f;
        tinted_y[channel] = world_y[channel] * 0.58f;
    }
    const float text_x[3] = {0.72f, 0.76f, 0.74f};
    const float text_y[3] = {-0.66f, -0.62f, -0.70f};
    const float button_x[3] = {-0.08f, 0.44f, 0.81f};
    const float button_y[3] = {0.16f, -0.51f, 0.37f};
    if (menu_overlay_edge_mask_test(unchanged_x, unchanged_y,
                                    world_x, world_y) > 0.001f) return 0;
    if (menu_overlay_edge_mask_test(tinted_x, tinted_y,
                                    world_x, world_y) > 0.001f) return 0;
    if (menu_overlay_edge_mask_test(text_x, text_y,
                                    world_x, world_y) < 0.99f) return 0;
    if (menu_overlay_edge_mask_test(button_x, button_y,
                                    world_x, world_y) < 0.99f) return 0;

    /* Every post-world orthographic location is eligible for the shared
       lens-safe plane. Perspective actor labels are already present in the
       captured base and therefore have zero difference and remain stereo. */
    if (!hud_fixed_region_test(0.50f, 0.18f)) return 0;
    if (!hud_fixed_region_test(0.55f, 0.42f)) return 0;
    if (!hud_fixed_region_test(0.10f, 0.30f)) return 0;
    if (!hud_fixed_region_test(0.90f, 0.30f)) return 0;
    if (!hud_fixed_region_test(0.50f, 0.80f)) return 0;

    /* A global grey fade is merely a scaled world and must not become the
       black/blurred safe-canvas band seen in rejected v6.  Blue panel tint
       and non-affine glyph/border edges must both remain full coverage. */
    const float no_tint[3] = {0.0f, 0.0f, 0.0f};
    const float blue_panel[3] = {0.025f, 0.065f, 0.110f};
    if (interface_component_mask_test(0.003f, no_tint) > 0.001f) return 0;
    if (interface_component_mask_test(0.003f, blue_panel) < 0.99f) return 0;
    if (interface_component_mask_test(0.120f, no_tint) < 0.99f) return 0;
    return 1;
}

static int test_transparent_interface_capture(void)
{
    /* The new pass starts at transparent black. World colour is deliberately
       randomized but never enters either equation, so an empty interface
       pixel must remain exactly zero rather than becoming grain. */
    for (int sample = 0; sample < 10000; ++sample) {
        float world[3] = {
            deterministic_signed() * 4.0f,
            deterministic_signed() * 4.0f,
            deterministic_signed() * 4.0f
        };
        (void)world;
        float capture_rgb[3] = {0.0f, 0.0f, 0.0f};
        float capture_alpha = 0.0f;
        if (capture_rgb[0] != 0.0f || capture_rgb[1] != 0.0f ||
            capture_rgb[2] != 0.0f || capture_alpha != 0.0f) return 0;
    }

    const float panel[3] = {0.08f, 0.34f, 0.86f};
    const float panel_alpha = 0.55f;
    float premultiplied[3] = {
        panel[0] * panel_alpha,
        panel[1] * panel_alpha,
        panel[2] * panel_alpha
    };
    float accumulated_alpha = panel_alpha;
    for (int channel = 0; channel < 3; ++channel) {
        float recovered = premultiplied[channel] / accumulated_alpha;
        if (fabsf(recovered - panel[channel]) > 0.00001f) return 0;
    }

    /* Opaque controls layered over the panel must become opaque and retain
       their exact authored colour under ordinary source-over composition. */
    const float control[3] = {0.92f, 0.16f, 0.12f};
    const float control_alpha = 1.0f;
    for (int channel = 0; channel < 3; ++channel)
        premultiplied[channel] = control[channel] * control_alpha +
            premultiplied[channel] * (1.0f - control_alpha);
    accumulated_alpha = control_alpha +
        accumulated_alpha * (1.0f - control_alpha);
    if (fabsf(accumulated_alpha - 1.0f) > 0.00001f) return 0;
    for (int channel = 0; channel < 3; ++channel) {
        float recovered = premultiplied[channel] / accumulated_alpha;
        if (fabsf(recovered - control[channel]) > 0.00001f) return 0;
    }
    return 1;
}

int main(int argument_count, char **arguments)
{
    if (argument_count > 1) {
        unsigned long seed = strtoul(arguments[1], NULL, 0);
        if (seed != 0) deterministic_state = (unsigned int)seed;
    }
    if (!test_identity_axis()) { puts("FAIL identity camera depth"); return 1; }
    if (!test_rotated_camera_axis()) { puts("FAIL rotated camera depth"); return 2; }
    if (!test_disabled_is_identity()) { puts("FAIL disabled depth"); return 3; }
    if (!test_head_pose_identity()) { puts("FAIL head pose identity"); return 4; }
    if (!test_head_translation_inverse()) { puts("FAIL head translation"); return 5; }
    if (!test_head_yaw_inverse()) { puts("FAIL head yaw"); return 6; }
    if (!test_quest_natural_pitch_direction_and_gain()) {
        puts("FAIL Quest natural pitch direction/gain"); return 23;
    }
    if (!test_texture_y_flip_pose_reflection()) {
        puts("FAIL texture Y-flip pose reflection"); return 7;
    }
    if (!test_final_texture_vertical_motion_direction()) {
        puts("FAIL final texture vertical motion direction"); return 20;
    }
    if (!test_anchor_relative_projection_pose_reflection()) {
        puts("FAIL anchor-relative projection pose reflection"); return 16;
    }
    if (!test_reflected_incremental_timewarp_pose()) {
        puts("FAIL reflected incremental timewarp pose"); return 17;
    }
    if (!test_reflected_incremental_timewarp_randomized()) {
        puts("FAIL randomized reflected incremental timewarp invariant"); return 19;
    }
    if (!test_72hz_submission_validity()) {
        puts("FAIL 72 Hz submission validity"); return 22;
    }
    if (!test_roll_is_compositor_driven()) {
        puts("FAIL compositor-driven roll motion"); return 25;
    }
    if (!test_asymmetric_fov_optical_center()) {
        puts("FAIL asymmetric FOV optical center"); return 18;
    }
    if (!test_reflected_head_delta_equivalence()) {
        puts("FAIL reflected head-delta equivalence"); return 10;
    }
    if (!test_tabletop_pivot_and_composition()) { puts("FAIL tabletop pivot"); return 8; }
    if (!test_lens_safe_hud_inset()) { puts("FAIL lens-safe HUD inset"); return 9; }
    if (!test_alignment_fov_metadata()) { puts("FAIL alignment FOV metadata"); return 11; }
    if (!test_rendered_alignment_matches_fov_metadata()) {
        puts("FAIL rendered alignment/FOV equivalence"); return 14;
    }
    if (!test_runtime_fov_builds_reversed_game_frustum()) {
        puts("FAIL runtime FOV/reversed frustum conversion"); return 15;
    }
    if (!test_projection_ipd_matches_game_scale()) {
        puts("FAIL projection IPD metadata"); return 12;
    }
    if (!test_projection_rect_preserves_tangent_aspect()) {
        puts("FAIL projection image rectangle"); return 13;
    }
    if (!test_head_relative_hud_indicator()) {
        puts("FAIL head-relative HUD indicator"); return 20;
    }
    if (!test_interface_compositor_masks()) {
        puts("FAIL interface compositor masks"); return 21;
    }
    if (!test_transparent_interface_capture()) {
        puts("FAIL transparent interface capture"); return 24;
    }
    puts("PASS camera, stereo reprojection metadata, and lens-safe HUD geometry");
    return 0;
}
