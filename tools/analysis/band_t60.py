#!/usr/bin/env python3
"""T60 per octave band of a reverb tail (recording of the hardware tank or a render of the model).

    band_t60.py file.wav [file2.wav ...]

Same estimator for both sides of the comparison: zero-phase 4th-order Butterworth band-pass,
squared and smoothed (2nd-order low-pass, 15 Hz), dB-linear fit from 0.3 s after the energy
peak (searched in the first 1.2 s) down to 30 dB below it, at most 4 s long. T60 = -60 / slope.
Needs numpy and scipy. tools/render.cpp runs a causal re-implementation of the same estimate
against the numbers measured on the recordings.
"""
import sys
import numpy as np
from scipy import signal
from scipy.io import wavfile

BANDS = [(60, 120), (120, 240), (240, 480), (480, 960), (960, 1920), (1920, 3840), (3840, 7680)]


def to_mono_float(x):
    if x.dtype == np.int16:
        x = x / 32768.0
    elif x.dtype == np.int32:
        x = x / 2147483648.0
    x = x.astype(np.float64)
    return x.mean(axis=1) if x.ndim > 1 else x


def band_t60(x, sr):
    out = []
    t = np.arange(len(x)) / sr
    smooth = signal.butter(2, 15, 'lp', fs=sr, output='sos')
    for lo, hi in BANDS:
        y = signal.sosfiltfilt(signal.butter(4, [lo, hi], 'bp', fs=sr, output='sos'), x)
        e = signal.sosfiltfilt(smooth, y ** 2).clip(1e-14)
        db = 10 * np.log10(e)
        pk = np.argmax(db[: int(1.2 * sr)])
        m = (t > t[pk] + 0.3) & (db > db[pk] - 30) & (t < t[pk] + 4)
        out.append(-60 / np.polyfit(t[m], db[m], 1)[0] if m.sum() > 100 else float('nan'))
    return out


if __name__ == '__main__':
    print('bands (Hz):', ' '.join(f'{lo}-{hi}' for lo, hi in BANDS))
    for path in sys.argv[1:]:
        sr, x = wavfile.read(path)
        print(f'{path}: T60 (s) ' + ' '.join(f'{v:5.1f}' for v in band_t60(to_mono_float(x), sr)))
