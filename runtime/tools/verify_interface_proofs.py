"""Check synthetic OpenXR integration pixels, using only Python's standard library.

Usage: python verify_interface_proofs.py <integration-output-directory>
These are submitted-texture checks, not a physical-headset acceptance test.
"""

import hashlib
import json
from pathlib import Path
import re
import struct
import sys


class Bitmap:
    def __init__(self, path):
        self.data = path.read_bytes()
        self.offset = struct.unpack_from("<I", self.data, 10)[0]
        self.width, self.height = struct.unpack_from("<ii", self.data, 18)
        bits = struct.unpack_from("<H", self.data, 28)[0]
        compression = struct.unpack_from("<I", self.data, 30)[0]
        if self.data[:2] != b"BM" or bits != 32 or compression != 0:
            raise ValueError(f"Expected uncompressed 32-bit BMP: {path.name}")
        if self.width <= 0 or not self.height:
            raise ValueError("Invalid bitmap dimensions")
        if len(self.data) < self.offset + self.width * abs(self.height) * 4:
            raise ValueError("Incomplete bitmap")

    def rgb(self, x, y):
        if not (0 <= x < self.width and 0 <= y < abs(self.height)):
            raise ValueError("Pixel outside bitmap")
        row = self.height - 1 - y if self.height > 0 else y
        at = self.offset + (row * self.width + x) * 4
        b, g, r = self.data[at:at + 3]
        return r, g, b


def verify(root):
    failures, measurements = [], {}

    def require(condition, message):
        if not condition:
            failures.append(message)

    result = (root / "integration-result.txt").read_text()
    require(re.search(r"\bpassed=1\b", result), "Integration harness failed")
    hashes = []
    for name, button in (("proof-first", (235, 41, 31)),
                         ("proof-second", (31, 214, 61))):
        directory = root / name
        alpha = Bitmap(directory / "SkillshotCityVR-interface-alpha-mask.bmp")
        report_path = directory / "SkillshotCityVR-stereo-diagnostics.txt"
        if report_path.exists():
            report = report_path.read_text()
            match = re.search(r"interface_rect=(\d+),(\d+) (\d+)x(\d+)", report)
            if not match:
                raise ValueError("Missing interface rectangle")
            x, y, width, height = map(int, match.groups())
            present = [re.search(
                rf"interface_present_{eye}=(\d+),(\d+) (\d+)x(\d+)", report)
                for eye in ("left", "right")]
            require(all(present), f"{name}: missing per-eye presentation rectangles")
            present_rects = [tuple(map(int, item.groups()))
                             for item in present if item]
        else:
            # The five-image Shift+1 visualization is written asynchronously
            # and is not part of the submitted-pixel contract. The bridge
            # independently logs the exact source/destination rectangles when
            # it first composites the UI, so use that authoritative record if
            # the optional visualization report races a proof archive.
            xr_log = (root / "SkillshotVR-XR.log").read_text()
            fallback = re.search(
                r"from source rect (\d+),(\d+) (\d+)x(\d+) "
                r"into HUD-safe rect (\d+),(\d+) (\d+)x(\d+)", xr_log)
            if not fallback:
                raise ValueError("Missing interface presentation rectangles")
            values = tuple(map(int, fallback.groups()))
            x, y, width, height = values[:4]
            present_rect = values[4:]
            present_rects = [present_rect, present_rect]
        require(all(abs(pw / ph - 960 / 540) < 0.005
                    for _, _, pw, ph in present_rects),
                f"{name}: presented UI does not retain 16:9 HUD aspect")
        require(present_rects[0][2:] == present_rects[1][2:],
                f"{name}: presented UI dimensions differ between eyes")
        require(abs(width / height - 960 / 540) < 0.005,
                f"{name}: UI aspect ratio changed")
        captured = Bitmap(directory / "SkillshotCityVR-interface-alpha.bmp")
        eyes = [Bitmap(directory / f"SkillshotCityVR-final-interface-{eye}.bmp")
                for eye in ("left", "right")]
        require(all((eye.width, eye.height) == (alpha.width, alpha.height)
                    for eye in eyes), f"{name}: eye dimensions disagree")

        def point(sx, sy):
            return x + round(sx * width / 960), y + round(sy * height / 540)

        checks = {}
        for label, source, expected_alpha, expected_rgb in (
            ("opaque_button", (480, 270), 255, button),
            ("edge_marker", (44, 44), 255, (217, 166, 26)),
            ("translucent_panel", (300, 180), 140, (11, 48, 121)),
            ("untouched_world", (100, 100), 0, (0, 0, 0)),
        ):
            pixel = point(*source)
            actual_alpha = alpha.rgb(*pixel)[0]
            actual_rgb = captured.rgb(*pixel)
            require(abs(actual_alpha - expected_alpha) <= 2,
                    f"{name}/{label}: alpha {actual_alpha}, expected {expected_alpha}")
            require(max(abs(a - b) for a, b in zip(actual_rgb, expected_rgb)) <= 3,
                    f"{name}/{label}: captured RGB {actual_rgb}, expected {expected_rgb}")
            final = []
            for eye, (px, py, pw, ph) in zip(eyes, present_rects):
                final.append(eye.rgb(px + round(source[0] * pw / 960),
                                     py + round(source[1] * ph / 540)))
            if expected_alpha == 255:
                for eye_name, rgb in zip(("left", "right"), final):
                    require(max(abs(a - b) for a, b in zip(rgb, expected_rgb)) <= 4,
                            f"{name}/{label}/{eye_name}: submitted RGB {rgb}, "
                            f"expected {expected_rgb}")
                require(max(abs(a - b) for a, b in zip(*final)) <= 2,
                        f"{name}/{label}: opaque UI differs between eyes")
            checks[label] = {"alpha": actual_alpha, "captured_rgb": actual_rgb,
                             "submitted_rgb": final}

        # Scan letterbox rows and clear canvas areas, catching garbage alpha
        # from WRITE_DISCARD storage or an incompletely cleared capture.
        border_max = max(alpha.rgb(px, py)[0]
                         for py in (0, abs(alpha.height) - 1)
                         for px in range(0, alpha.width, 8))
        require(border_max == 0, f"{name}: nontransparent letterbox")
        # An opaque rectangle's edges must occupy the same pixels in each eye.
        def button_bounds(eye, rectangle):
            px, py, pw, ph = rectangle
            sample_y = py + round(270 * ph / 540)
            matches = [sample_x for sample_x in range(px, px + pw)
                       if max(abs(a - b) for a, b in zip(eye.rgb(sample_x, sample_y), button)) <= 4]
            return (min(matches), max(matches)) if matches else None
        bounds = [button_bounds(eye, rectangle)
                  for eye, rectangle in zip(eyes, present_rects)]
        centered_bounds = [None if bound is None else
                           (bound[0] - rectangle[0], bound[1] - rectangle[0])
                           for bound, rectangle in zip(bounds, present_rects)]
        require(bounds[0] is not None and centered_bounds[0] == centered_bounds[1],
                f"{name}: head-locked button edges do not align: {bounds}")
        expected_width = 160 * present_rects[0][2] / 960
        require(bounds[0] is not None and
                abs(bounds[0][1] - bounds[0][0] + 1 - expected_width) <= 4,
                f"{name}: button size differs from authored UI")
        measurements[name] = {"interface_rect": [x, y, width, height],
                              "present_rects": present_rects,
                              "pixels": checks, "button_bounds": bounds,
                              "letterbox_alpha_max": border_max}
        hashes.append(hashlib.sha256(eyes[0].data).hexdigest())
    require(hashes[0] != hashes[1], "Repeated capture reused the preceding image")

    # Ordinary announcements are composited from a shared HUD source, unlike
    # the exact-alpha full-screen menu path above.  Their translucent pixels
    # must be reconstructed over each eye's own world.  The harness paints a
    # strongly different world patch under each eye so copying the source-eye
    # gameplay into the bar cannot accidentally pass this check.
    center = root / "proof-center"
    presented = [Bitmap(center / f"SkillshotCityVR-presented-{eye}.bmp")
                 for eye in ("left", "right")]
    bases = [Bitmap(center / f"SkillshotCityVR-hud-base-{eye}.bmp")
             for eye in ("left", "right")]
    fixture_colors = ((20, 148, 209), (184, 20, 56))

    def fixture_bounds(bitmap, color):
        matches = []
        # The fixture is hundreds of pixels wide; stride scanning is both
        # deterministic and much cheaper than reading every 1536x1608 pixel.
        for y in range(0, abs(bitmap.height), 3):
            for x in range(0, bitmap.width, 3):
                if max(abs(a - b) for a, b in zip(bitmap.rgb(x, y), color)) <= 3:
                    matches.append((x, y))
        return ((min(x for x, _ in matches), min(y for _, y in matches),
                 max(x for x, _ in matches), max(y for _, y in matches))
                if matches else None)

    bounds = [fixture_bounds(image, color)
              for image, color in zip(bases, fixture_colors)]
    require(all(bounds), "proof-center: eye-specific scenery fixture is missing")
    if all(bounds):
        # The dark translucent bar is inset 25/460 of the scenery width. Sample
        # well inside it but away from the central opaque text block.
        common = (max(item[0] for item in bounds), max(item[1] for item in bounds),
                  min(item[2] for item in bounds), min(item[3] for item in bounds))
        x = common[0] + round((common[2] - common[0]) * 0.20)
        y = round((common[1] + common[3]) * 0.5)
        tx = round((common[0] + common[2]) * 0.5)
        base_panel = [image.rgb(x, y) for image in bases]
        final_panel = [image.rgb(x, y) for image in presented]
        final_text = [image.rgb(tx, y) for image in presented]
        require(max(abs(a - b) for a, b in zip(*base_panel)) >= 80,
                "proof-center: eye-specific scenery fixture is not distinct")
        require(max(abs(a - b) for a, b in zip(*final_panel)) >= 18,
                "proof-center: translucent bar copied one eye's gameplay into both eyes")
        require(max(abs(a - b) for a, b in zip(*final_text)) <= 8,
                "proof-center: opaque announcement text is not eye-aligned")
        bx0, by0, bx1, by1 = bounds[1]
        leak_pixels = 0
        wrong_eye_fringe = 0
        for py in range(by0 + round((by1 - by0) * 0.18),
                        by0 + round((by1 - by0) * 0.82), 2):
            for px in range(bx0 + round((bx1 - bx0) * 0.07),
                            bx0 + round((bx1 - bx0) * 0.93), 2):
                actual = presented[1].rgb(px, py)
                if max(abs(a - b) for a, b in zip(
                        actual, fixture_colors[0])) <= 3:
                    leak_pixels += 1
                if actual[2] > actual[0] + 15:
                    wrong_eye_fringe += 1
        require(leak_pixels <= 8,
                f"proof-center: {leak_pixels} source-eye scenery samples leaked into right-eye bar")
        require(wrong_eye_fringe <= 8,
                f"proof-center: {wrong_eye_fringe} wrong-eye cyan fringe samples remain")
        measurements["proof-center"] = {
            "fixture_bounds": bounds,
            "panel_point": [x, y],
            "text_point": [tx, y],
            "base_panel_rgb": base_panel,
            "submitted_panel_rgb": final_panel,
            "submitted_text_rgb": final_text,
            "source_eye_leak_samples": leak_pixels,
            "wrong_eye_fringe_samples": wrong_eye_fringe,
        }
    return {"passed": not failures, "failures": failures,
            "measurements": measurements, "capture_sha256": hashes,
            "scope": "synthetic submitted textures; physical Quest check still required"}


if __name__ == "__main__":
    try:
        verification = verify(Path(sys.argv[1]))
    except (OSError, ValueError, IndexError, struct.error) as error:
        verification = {"passed": False, "failures": [str(error)]}
    print(json.dumps(verification, indent=2))
    sys.exit(0 if verification["passed"] else 1)
