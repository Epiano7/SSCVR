#!/usr/bin/env python3
"""Analyze marker-gated Skillshot VR pose telemetry for pair-boundary jitter."""

import csv
import math
import statistics
import sys
from pathlib import Path


def quat(row, prefix):
    return tuple(float(row[f"{prefix}_q{axis}"]) for axis in "xyzw")


def normalize(q):
    length = math.sqrt(sum(value * value for value in q))
    return tuple(value / length for value in q) if length else (0.0, 0.0, 0.0, 1.0)


def multiply(a, b):
    ax, ay, az, aw = a
    bx, by, bz, bw = b
    return normalize((
        aw * bx + ax * bw + ay * bz - az * by,
        aw * by - ax * bz + ay * bw + az * bx,
        aw * bz + ax * by - ay * bx + az * bw,
        aw * bw - ax * bx - ay * by - az * bz,
    ))


def inverse(q):
    x, y, z, w = normalize(q)
    return (-x, -y, -z, w)


def distance_degrees(a, b):
    dot = abs(sum(x * y for x, y in zip(normalize(a), normalize(b))))
    return math.degrees(2.0 * math.acos(min(1.0, dot)))


def pitch_degrees(q):
    x, y, z, w = normalize(q)
    value = 2.0 * (w * x - y * z)
    return math.degrees(math.asin(max(-1.0, min(1.0, value))))


def rotation_vector_degrees(q):
    """Return the shortest quaternion rotation as XYZ axis-angle components."""
    q = normalize(q)
    if q[3] < 0.0:
        q = tuple(-value for value in q)
    sine_half = math.sqrt(sum(value * value for value in q[:3]))
    if sine_half < 1.0e-9:
        return (0.0, 0.0, 0.0)
    angle_degrees = math.degrees(2.0 * math.atan2(sine_half, q[3]))
    return tuple(value * angle_degrees / sine_half for value in q[:3])


def scale_axes(q, x_scale, y_scale, z_scale):
    q = normalize(q)
    if q[3] < 0.0:
        q = tuple(-value for value in q)
    sine_half = math.sqrt(sum(value * value for value in q[:3]))
    if sine_half < 1e-9:
        return (0.0, 0.0, 0.0, 1.0)
    angle = 2.0 * math.atan2(sine_half, q[3])
    vector = (
        q[0] * angle * x_scale / sine_half,
        q[1] * angle * y_scale / sine_half,
        q[2] * angle * z_scale / sine_half,
    )
    scaled_angle = math.sqrt(sum(value * value for value in vector))
    if scaled_angle < 1e-9:
        return (0.0, 0.0, 0.0, 1.0)
    factor = math.sin(scaled_angle * 0.5) / scaled_angle
    return normalize((
        vector[0] * factor, vector[1] * factor,
        vector[2] * factor, math.cos(scaled_angle * 0.5),
    ))


def percentile(values, fraction):
    if not values:
        return 0.0
    ordered = sorted(values)
    return ordered[min(len(ordered) - 1, round((len(ordered) - 1) * fraction))]


def describe(label, values):
    if not values:
        print(f"{label}: no samples")
        return
    print(
        f"{label}: mean={statistics.fmean(values):.4f} "
        f"p95={percentile(values, 0.95):.4f} max={max(values):.4f}"
    )


def main():
    if len(sys.argv) != 2:
        raise SystemExit("usage: analyze_pose_trace.py SkillshotCityVR-pose-trace.csv")
    path = Path(sys.argv[1])
    with path.open(newline="", encoding="utf-8") as handle:
        rows = [
            row for row in csv.DictReader(handle)
            if row["projection"] == "1" and int(row["pair"]) >= 0
        ]
    if len(rows) < 3:
        raise SystemExit("trace needs at least three rows")

    apparent = []
    invariant_errors = []
    live_steps = []
    apparent_steps = []
    live_pitch_steps = []
    apparent_pitch_steps = []
    apparent_to_live_errors = []
    signed_live_pitch_steps = []
    signed_apparent_pitch_steps = []
    signed_tracked_pitch_steps = []
    boundary_steps = []
    periods_ms = []
    axis_discontinuities = []
    previous_pair = rows[0]["pair"]

    for row in rows:
        runtime_delta = multiply(inverse(quat(row, "submit")), quat(row, "live"))
        expected_delta = multiply(
            inverse(quat(row, "visual_render")), quat(row, "visual_live")
        )
        pose = multiply(quat(row, "visual_render"), runtime_delta)
        apparent.append(pose)
        invariant_errors.append(distance_degrees(runtime_delta, expected_delta))
        apparent_to_live_errors.append(
            distance_degrees(pose, quat(row, "visual_live"))
        )

    for index in range(1, len(rows)):
        live_step = distance_degrees(
            quat(rows[index - 1], "visual_live"), quat(rows[index], "visual_live")
        )
        apparent_step = distance_degrees(apparent[index - 1], apparent[index])
        live_steps.append(live_step)
        apparent_steps.append(apparent_step)
        signed_live_pitch = (
            pitch_degrees(quat(rows[index], "visual_live")) -
            pitch_degrees(quat(rows[index - 1], "visual_live"))
        )
        signed_apparent_pitch = (
            pitch_degrees(apparent[index]) - pitch_degrees(apparent[index - 1])
        )
        signed_live_pitch_steps.append(signed_live_pitch)
        signed_apparent_pitch_steps.append(signed_apparent_pitch)
        signed_tracked_pitch_steps.append(
            pitch_degrees(quat(rows[index], "live")) -
            pitch_degrees(quat(rows[index - 1], "live"))
        )
        live_pitch_steps.append(abs(signed_live_pitch))
        apparent_pitch_steps.append(abs(signed_apparent_pitch))
        if rows[index]["pair"] != previous_pair:
            boundary_steps.append(apparent_step)
        previous_pair = rows[index]["pair"]
        periods_ms.append(
            (int(rows[index]["predicted_ns"]) - int(rows[index - 1]["predicted_ns"])) /
            1_000_000.0
        )
        live_axis = rotation_vector_degrees(multiply(
            inverse(quat(rows[index - 1], "visual_live")),
            quat(rows[index], "visual_live"),
        ))
        apparent_axis = rotation_vector_degrees(multiply(
            inverse(apparent[index - 1]), apparent[index],
        ))
        mismatch = math.sqrt(sum(
            (actual - expected) ** 2
            for actual, expected in zip(apparent_axis, live_axis)
        ))
        if mismatch > 0.5:
            axis_discontinuities.append((
                mismatch, rows[index]["frame"], rows[index]["pair"],
                rows[index - 1]["pair"], live_axis, apparent_axis,
            ))

    print(f"trace={path} rows={len(rows)} pair_boundaries={len(boundary_steps)}")
    describe("predicted frame period ms", periods_ms)
    describe("live visual angular step deg", live_steps)
    describe("apparent angular step deg", apparent_steps)
    describe("pair-boundary apparent step deg", boundary_steps)
    describe("live pitch step deg", live_pitch_steps)
    describe("apparent pitch step deg", apparent_pitch_steps)
    describe("apparent-to-visual-live error deg", apparent_to_live_errors)
    pitch_energy = sum(value * value for value in signed_live_pitch_steps)
    pitch_gain = (sum(live * apparent for live, apparent in zip(
        signed_live_pitch_steps, signed_apparent_pitch_steps
    )) / pitch_energy) if pitch_energy > 1e-9 else 0.0
    print(f"signed apparent/live pitch gain: {pitch_gain:.4f}")
    tracked_pitch_energy = sum(value * value for value in signed_tracked_pitch_steps)
    visual_from_tracked_pitch_gain = (sum(tracked * visual for tracked, visual in zip(
        signed_tracked_pitch_steps, signed_live_pitch_steps
    )) / tracked_pitch_energy) if tracked_pitch_energy > 1e-9 else 0.0
    print(f"signed visual/tracked pitch gain: {visual_from_tracked_pitch_gain:.4f}")
    describe("incremental reflected timewarp invariant error deg", invariant_errors)
    if axis_discontinuities:
        print("largest apparent/live axis discontinuities (deg):")
        for mismatch, frame, pair, previous, live_axis, apparent_axis in sorted(
                axis_discontinuities, reverse=True)[:10]:
            print(
                f"  frame={frame} pair={previous}->{pair} mismatch={mismatch:.3f} "
                f"liveXYZ={live_axis[0]:+.3f},{live_axis[1]:+.3f},{live_axis[2]:+.3f} "
                f"apparentXYZ={apparent_axis[0]:+.3f},{apparent_axis[1]:+.3f},{apparent_axis[2]:+.3f}"
            )
    if max(invariant_errors) > 0.05:
        raise SystemExit("FAIL: submitted pose does not reconstruct the intended visual pose")
    if boundary_steps and max(boundary_steps) > max(live_steps) * 1.5 + 0.05:
        raise SystemExit("FAIL: apparent motion spikes when the retained stereo pair changes")
    print("PASS: no mathematical pair-boundary discontinuity in recorded pose path")


if __name__ == "__main__":
    main()
