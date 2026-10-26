"""Tests for the banner tooling that need no ROM and no pycgfx (python3 tools/banner3d/test_banner3d.py)."""
import copy
import json
import math
import os
import sys
import unittest

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import cgfx_fingerprint  # noqa: E402
import decimate  # noqa: E402


def sphere(n):
    """A UV sphere as lugia_parts triangles: (material 0, [(joint, pos, uv, colour, normal)] * 3), one joint."""
    def vert(i, j):
        th, ph = math.pi * i / n, 2 * math.pi * j / (2 * n)
        p = [math.sin(th) * math.cos(ph), math.cos(th), math.sin(th) * math.sin(ph)]
        return (0, p, (j / (2 * n), i / n), (1, 1, 1), p)
    tris = []
    for i in range(n):
        for j in range(2 * n):
            a, b, c, d = vert(i, j), vert(i, j + 1), vert(i + 1, j + 1), vert(i + 1, j)
            if i:
                tris.append((0, [a, b, c]))
            if i < n - 1:
                tris.append((0, [a, c, d]))
    return tris


class Decimate(unittest.TestCase):
    def test_reaches_budget_and_keeps_vertices(self):
        tris = sphere(20)
        out, n = decimate.decimate(tris, len(tris) // 2)
        self.assertLessEqual(n, len(tris) // 2 + 2)
        pos = {tuple(round(x, 4) for x in v[1]) for _, t in tris for v in t}
        for _, t in out:
            for v in t:                                   # collapses only slide onto existing vertices
                self.assertIn(tuple(round(x, 4) for x in v[1]), pos)

    def test_small_models_untouched(self):
        tris = sphere(4)
        self.assertEqual(decimate.decimate(tris, 10_000), (tris, len(tris)))


class Fingerprint(unittest.TestCase):
    def setUp(self):
        with open(cgfx_fingerprint.REFERENCE, encoding="utf-8") as f:
            self.ref = json.load(f)

    def test_reference_matches_itself(self):
        problems, _ = cgfx_fingerprint.check(copy.deepcopy(self.ref), self.ref)
        self.assertEqual(problems, [])

    def test_extra_textures_fail(self):
        bad = copy.deepcopy(self.ref)
        bad["container"]["dict_counts"]["textures"] = 8
        bad["classes"]["ImageTexture"]["count"] = 8
        problems, _ = cgfx_fingerprint.check(bad, self.ref)
        self.assertTrue(any("textures" in p for p in problems))
        self.assertTrue(any("ImageTexture" in p for p in problems))

    def test_new_setting_value_fails(self):
        bad = copy.deepcopy(self.ref)
        bad["classes"]["Rasterization"]["slots"]["1"]["99"] = 1          # a cull mode Platinum never uses
        problems, _ = cgfx_fingerprint.check(bad, self.ref)
        self.assertTrue(any("Rasterization slot 1" in p for p in problems))

    def test_texture_size_may_differ_if_power_of_two(self):
        ok = copy.deepcopy(self.ref)
        ok["classes"]["ImageTexture"]["slots"]["6"]["64"] = 1
        self.assertEqual(cgfx_fingerprint.check(ok, self.ref)[0], [])
        bad = copy.deepcopy(self.ref)
        bad["classes"]["ImageTexture"]["slots"]["6"]["100"] = 1
        self.assertTrue(cgfx_fingerprint.check(bad, self.ref)[0])

    def test_unknown_class_fails(self):
        bad = copy.deepcopy(self.ref)
        bad["classes"]["VisibilityThing"] = {"count": 1, "bytes": 8, "slots": {}}
        self.assertTrue(cgfx_fingerprint.check(bad, self.ref)[0])


if __name__ == "__main__":
    unittest.main()
