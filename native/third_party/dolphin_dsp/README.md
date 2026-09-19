# Reconstructed DSP coefficient data

`dsp_coef.bin` is vendored unchanged from Dolphin revision
`a2efdf1197be8132674b90fe9cf4761df39752ed`:

- [Original file](https://github.com/dolphin-emu/dolphin/blob/a2efdf1197be8132674b90fe9cf4761df39752ed/Data/Sys/GC/dsp_coef.bin)
- [Upstream reconstruction generator](https://github.com/dolphin-emu/dolphin/blob/a2efdf1197be8132674b90fe9cf4761df39752ed/docs/DSP/free_dsp_rom/generate_coefs.py)
- [Upstream licensing notice](https://github.com/dolphin-emu/dolphin/blob/a2efdf1197be8132674b90fe9cf4761df39752ed/COPYING)

SHA-256: `d7741279c2e8ec5c5fb318f8fbdd6de6bf583520d288e836a5383233a4238179`.
The upstream licensing notice and GPL-2.0-or-later text accompany this file.
These are reconstructed coefficients, not a verified dump from the user's
console. No original game assets are included in this directory.

From the repository root, `python3 native/tools/prepare_ax_coefficients.py`
converts the first 1,536 big-endian signed words into the checked-in C include.
`--check` verifies both the entire 4,096-byte dependency and generated include.
The three 512-word AX filter banks are embedded unchanged, including upstream
coefficient adjustments. The remaining 512 words are not used by AX resampling.
