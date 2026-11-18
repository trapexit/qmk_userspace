# SPDX-License-Identifier: GPL-2.0-or-later
"""Exercise the production BLE4 scheduler with a host compiler and a fake UART."""
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
QMK = Path(os.environ.get("QMK_HOME", ROOT.parent / "qmk_firmware"))


class ShuraQMKBLE4Tests(unittest.TestCase):
    def test_radio_scheduler(self):
        compiler = shutil.which("clang")
        self.assertIsNotNone(compiler, "Installed native Clang is required for this offline test")
        scratch = ROOT / "tmp"
        scratch.mkdir(exist_ok=True)
        with tempfile.TemporaryDirectory(prefix="shura-ble4-", dir=scratch) as temporary:
            executable = Path(temporary) / "ble4-test"
            subprocess.run([
                compiler, "-std=gnu11", "-Wall", "-Werror",
                '-DQMK_KEYBOARD_H="shura_ble4_test_board.h"', "-DSHURA_BLE4",
                "-I", str(ROOT / "tests"), "-I", str(QMK / "quantum"),
                "-I", str(QMK / "quantum/connection"), "-I", str(QMK / "drivers/bluetooth"),
                "-I", str(QMK / "tmk_core/protocol"),
                str(QMK / "keyboards/tex/shura/ble4.c"),
                str(ROOT / "tests/shura_qmk_ble4_test.c"), "-o", str(executable),
            ], check=True, timeout=30)
            for scenario in ("passkey", "connected-idle", "connected-handoff", "slot-wake", "mcu-wake",
                             "slot-window", "pair-window", "link-change"):
                with self.subTest(scenario=scenario):
                    subprocess.run([str(executable), scenario], cwd=temporary, check=True, timeout=10)


if __name__ == "__main__":
    unittest.main()
