# TEX Shura QMK userspace

`shinobi` is a VIA-enabled keymap for the **TEX Shura** with the older Shinobi BLE4 add-on. It does not target the Shinobi keyboard. Caps Lock becomes Left Ctrl. Hold the middle button for H/J/K/L arrows, A/E Home/End, and comma/period/slash for Bluetooth slots 1, 2 and 3. The other build target, `default`, uses the firmware tree's stock layout and BLE5 driver.

To configure `shinobi` in VIA before its definition is distributed, enable **Show Design tab** in VIA's settings, then load the [Shura v3 definition](https://raw.githubusercontent.com/trapexit/keyboards/tex-shura/v3/tex/shura/shura.json) from the Design tab. The diagram omits JIS-specific switches and the physical Bluetooth Pair switch; hold the middle button and use comma, period or slash to select a host on this keymap. For the default layout with VIA instead, see the separate [VIA keymap branch](https://github.com/trapexit/qmk_userspace_via/tree/tex-shura/keyboards/tex/shura/keymaps/via).

## Build from a clean checkout

Use two sibling checkouts. The keyboard and both Bluetooth drivers come from the public [tex-shura firmware branch](https://github.com/trapexit/qmk_firmware/tree/tex-shura/keyboards/tex/shura). Do not copy keymaps into that tree.

```sh
git clone --depth 1 --branch tex-shura https://github.com/trapexit/qmk_firmware.git
git clone https://github.com/trapexit/qmk_userspace.git
cd qmk_userspace
export QMK_HOME="$(realpath ../qmk_firmware)"
export QMK_USERSPACE="$PWD"
git -C "$QMK_HOME" submodule update --init --depth 1 lib/chibios lib/chibios-contrib lib/printf
```

On Ubuntu 24.04, install the host compiler, ARM toolchain and QMK CLI:

```sh
sudo apt-get update
sudo apt-get install --no-install-recommends build-essential clang gcc-arm-none-eabi binutils-arm-none-eabi libnewlib-arm-none-eabi libhidapi-hidraw0 python3-venv
python3 -m venv .venv
. .venv/bin/activate
python -m pip install 'qmk==1.2.0' -r "$QMK_HOME/requirements.txt"
```

Lint the board and both keymaps, run the native Fn and BLE4 regression tests, then compile both targets in `qmk.json`:

```sh
qmk lint -kb tex/shura -km default
qmk lint -kb tex/shura -km shinobi
python -m unittest discover -s tests -p 'test_shura_qmk_*.py' -v
qmk userspace-compile -j 2 --print-failures
```

The matching pairs are `$QMK_HOME/.build/tex_shura_default.{elf,bin}` and `$QMK_HOME/.build/tex_shura_shinobi.{elf,bin}`. The [build workflow](https://github.com/trapexit/qmk_userspace/actions/workflows/build_binaries.yaml) also compares each BIN with a fresh binary conversion of its ELF before packaging the pair as a downloadable artifact. Keep files from the same keymap and run together. These host tests and builds do not flash or exercise hardware.

Based on the [QMK external userspace template](https://github.com/qmk/qmk_userspace/tree/940d6f5678f042cd5291c228a7142fb28b55cb34).
