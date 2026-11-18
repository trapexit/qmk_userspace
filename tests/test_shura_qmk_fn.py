# SPDX-License-Identifier: GPL-2.0-or-later
"""Compile and exercise actual portable QMK Fn arbitration, never a device."""
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
BOARD = Path(os.environ.get("QMK_HOME", ROOT.parent / "qmk_firmware")) / "keyboards/tex/shura"


class ShuraQMKFnTests(unittest.TestCase):
    def test_production_fn_ownership_click_and_scroll_contract(self):
        compiler = shutil.which("clang")
        self.assertIsNotNone(compiler, "Installed native Clang is required for this offline test")
        with tempfile.TemporaryDirectory(prefix="shura-qmk-fn-") as temporary:
            executable = Path(temporary) / "fn-contract"
            subprocess.run([
                compiler, "-std=c11", "-Wall", "-Wextra", "-Werror",
                "-Wconversion", "-Wshadow", "-pedantic",
                "-I", str(BOARD),
                str(BOARD / "shura_fn.c"),
                str(ROOT / "tests/shura_qmk_fn_test.c"),
                "-o", str(executable),
            ], check=True, capture_output=True, text=True, timeout=30)
            subprocess.run([str(executable)], check=True, capture_output=True,
                           text=True, timeout=10)


if __name__ == "__main__":
    unittest.main()
