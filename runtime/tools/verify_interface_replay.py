"""Validate locally captured game UI replay; never packages the source captures."""

import json
import math
from pathlib import Path
import re
import sys

from verify_interface_proofs import Bitmap


def rectangle(directory, name='interface_rect'):
    report = (directory / 'SkillshotCityVR-stereo-diagnostics.txt').read_text()
    match = re.search(rf'{name}=(\d+),(\d+) (\d+)x(\d+)', report)
    if not match:
        raise ValueError('Missing interface rectangle')
    return tuple(map(int, match.groups()))


def verify(run, fixture):
    source = Bitmap(fixture / 'SkillshotCityVR-interface-alpha.bmp')
    source_alpha = Bitmap(fixture / 'SkillshotCityVR-interface-alpha-mask.bmp')
    sx, sy, sw, sh = rectangle(fixture)
    failures, results = [], {}
    for case in ('proof-first', 'proof-second'):
        directory = run / case
        color = Bitmap(directory / 'SkillshotCityVR-interface-alpha.bmp')
        alpha = Bitmap(directory / 'SkillshotCityVR-interface-alpha-mask.bmp')
        eyes = [Bitmap(directory / f'SkillshotCityVR-final-interface-{eye}.bmp')
                for eye in ('left', 'right')]
        dx, dy, dw, dh = rectangle(directory)
        present_rects = [rectangle(directory, f'interface_present_{eye}')
                         for eye in ('left', 'right')]
        sampled = opaque = translucent = 0
        rgb_error = alpha_error = eye_error = stereo_error = 0
        # Reference the two real linear resampling stages: archived UI to the
        # 960x540 authored canvas, then that canvas to the eye's fitted rect.
        # This includes text edges rather than assuming they are flat colors.
        def interpolate(fetch, x, y):
            ix, iy = math.floor(x), math.floor(y)
            fx, fy = x - ix, y - iy
            samples = [fetch(ix, iy), fetch(ix + 1, iy),
                       fetch(ix, iy + 1), fetch(ix + 1, iy + 1)]
            return tuple(round((samples[0][c] * (1-fx) + samples[1][c] * fx) * (1-fy)
                               + (samples[2][c] * (1-fx) + samples[3][c] * fx) * fy)
                         for c in range(3))

        def expected(bitmap, tx, ty):
            def canvas(cx, cy):
                cx, cy = max(0, min(959, cx)), max(0, min(539, cy))
                def fetch(px, py):
                    return bitmap.rgb(sx + max(0, min(sw-1, px)),
                                      sy + max(0, min(sh-1, py)))
                return interpolate(fetch, (cx + 0.5) * sw / 960 - 0.5,
                                   (cy + 0.5) * sh / 540 - 0.5)
            return interpolate(canvas, (tx + 0.5) * 960 / dw - 0.5,
                               (ty + 0.5) * 540 / dh - 0.5)

        def submitted_expected(bitmap, target_x, target_y, present_width,
                               present_height):
            # The final menu pass samples the already captured fitted canvas.
            # Model that last bilinear stage from its actual proof instead of
            # comparing a destination pixel to a differently rounded source
            # coordinate (which exaggerates errors around real text edges).
            def fitted(px, py):
                px = max(0, min(dw - 1, px))
                py = max(0, min(dh - 1, py))
                return bitmap.rgb(dx + px, dy + py)
            return interpolate(
                fitted,
                (target_x + 0.5) * dw / present_width - 0.5,
                (target_y + 0.5) * dh / present_height - 0.5)

        for uy in range(8, 532, 8):
            for ux in range(8, 952, 8):
                tx, ty = round(ux * dw / 960), round(uy * dh / 540)
                rgb = expected(source, tx, ty)
                a = expected(source_alpha, tx, ty)[0]
                target = dx + tx, dy + ty
                actual = color.rgb(*target)
                actual_alpha = alpha.rgb(*target)[0]
                rgb_error = max(rgb_error, max(abs(v - w) for v, w in zip(rgb, actual)))
                alpha_error = max(alpha_error, abs(a - actual_alpha))
                sampled += 1
                if 0 < a < 255:
                    translucent += 1
                if a == 255:
                    opaque += 1
                    final = []
                    final_expected = []
                    for eye, (px, py, pw, ph) in zip(eyes, present_rects):
                        tx_final = round(ux * pw / 960)
                        ty_final = round(uy * ph / 540)
                        final.append(eye.rgb(px + tx_final, py + ty_final))
                        final_expected.append(submitted_expected(
                            color, tx_final, ty_final, pw, ph))
                    eye_error = max(
                        eye_error,
                        *(abs(v - w) for sample, reference in
                          zip(final, final_expected)
                          for v, w in zip(sample, reference)))
                    stereo_error = max(stereo_error, *(abs(v - w) for v, w in zip(*final)))
        border_alpha = max(alpha.rgb(px, py)[0] for px in range(0, alpha.width, 8)
                           for py in (0, abs(alpha.height)-1))
        if sampled < 100 or opaque < 20 or translucent < 20:
            failures.append(f'{case}: insufficient opaque/translucent reference samples')
        if border_alpha != 0:
            failures.append(f'{case}: letterbox is not transparent')
        if max(rgb_error, alpha_error, eye_error) > 4 or stereo_error > 2:
            failures.append(f'{case}: replay color/alpha/alignment mismatch')
        results[case] = dict(samples=sampled, opaque_samples=opaque,
                             translucent_samples=translucent,
                             letterbox_alpha_max=border_alpha,
                             max_capture_rgb_error=rgb_error,
                             max_alpha_error=alpha_error,
                             max_submitted_opaque_rgb_error=eye_error,
                             max_opaque_stereo_difference=stereo_error)
    return dict(passed=not failures, failures=failures, measurements=results,
                scope='replay of archived game UI through current compositor; '
                      'does not exercise live game UI selection or physical Quest optics')


if __name__ == '__main__':
    try:
        result = verify(Path(sys.argv[1]), Path(sys.argv[2]))
    except (OSError, ValueError, IndexError) as error:
        result = dict(passed=False, failures=[str(error)])
    print(json.dumps(result, indent=2))
    sys.exit(0 if result['passed'] else 1)
