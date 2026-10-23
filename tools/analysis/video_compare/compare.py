#!/usr/bin/env python3
"""Compares a recording of the hardware with the plugin playing the same part.

    compare.py hardware.wav part.part --start 19.0 --end 30.6 [--render build/babushka_render] [--out dir]

The part file (see babushka_render --play) holds the panel settings, the cables and the notes, with times from
the start of the excerpt. Both sides are measured the same way, on 10 ms frames:
  - bass: pitch of oscillator 1 (harmonic sum over 30-170 Hz),
  - sweep: the strongest peak above the 3.5th harmonic (with SYNC, the frequency of oscillator 2),
    expressed as r = log2(sweep / bass), octaves of oscillator 2 above oscillator 1 (independent of the key),
  - level, and the half-octave long-term spectrum normalised on the bass (90-180 Hz).
Writes metrics.txt, report.png (needs matplotlib) and loudness-matched WAVs of both sides. The exit code is 1 when
a metric is outside its limit (a regression check after changes to the sound). Needs numpy and scipy.
"""
import argparse, os, subprocess, sys
import numpy as np
from scipy import signal
from scipy.io import wavfile

FS = 48000
LIMITS = { 'sounding_diff': 15.0, 'level_rms': 10.0, 'sweep_rms': 0.8, 'sweep_found': 60.0, 'spectrum_rms': 4.0 }


def read_mono (path, start=None, end=None):
    fs, raw = wavfile.read (path)
    x = raw.astype (np.float64)
    if x.ndim > 1: x = x.mean (axis=1)
    if np.issubdtype (raw.dtype, np.integer): x /= float (np.iinfo (raw.dtype).max) + 1.0
    if fs != FS: x = signal.resample_poly (x, FS, fs)
    if start is not None: x = x[int (start * FS):int (end * FS)]
    return x


def tracks (x):
    win, hop = 4096, 480
    f = np.fft.rfftfreq (win, 1 / FS); hann = np.hanning (win); cands = np.arange (30, 170, 0.25)
    t, bass, sweep, level, spec = [], [], [], [], []
    for a in range (0, len (x) - win, hop):
        s = x[a:a + win]; m = np.abs (np.fft.rfft (s * hann)); lm = np.log (m + 1e-9)
        t.append ((a + win / 2) / FS); level.append (10 * np.log10 (np.mean (s * s) + 1e-12))
        f0 = cands[np.argmax (sum (np.interp (cands * h, f, lm) for h in range (1, 5)))]; bass.append (f0)
        band = (f > max (3.5 * f0, 250)) & (f < 20000); sm = np.convolve (m, np.ones (5) / 5, 'same')
        sweep.append (f[band][np.argmax (sm[band])]); spec.append (20 * np.log10 (m + 1e-9))
    t, bass, sweep, level = map (np.array, (t, bass, sweep, level))
    ok = (sweep > 600) & (sweep < 10500) & (level > -30)
    return { 't': t, 'level': level, 'r': np.where (ok, np.log2 (sweep / bass), np.nan), 'spec': np.array (spec), 'f': f }


def ltas (x):
    win = 4096; acc = 0; n = 0
    for a in range (0, len (x) - win, win // 2): acc = acc + np.abs (np.fft.rfft (x[a:a + win] * np.hanning (win))) ** 2; n += 1
    f = np.fft.rfftfreq (win, 1 / FS); edges = [63 * 2 ** (k / 2) for k in range (16)]
    bands = np.array ([10 * np.log10 (acc[(f >= edges[k]) & (f < edges[k + 1])].sum () / n + 1e-20) for k in range (15)])
    return bands - bands[1:4].mean (), np.array (edges[:-1])


def metrics (hw, pl, xh, xp):
    lh, lp = hw['level'], pl['level']
    lp = lp - np.median (lp[lp > -60]) + np.median (lh[lh > -60])
    n = min (len (lh), len (lp)); lh, lp = lh[:n], lp[:n]
    rh, rp = hw['r'][:n], pl['r'][:n]
    both = ~np.isnan (rh) & ~np.isnan (rp)
    Lh, edges = ltas (xh); Lp, _ = ltas (xp); band = (edges >= 200) & (edges <= 8000)
    m = lh > -45
    return { 'sounding_hw': 100 * np.mean (lh > -40), 'sounding_plugin': 100 * np.mean (lp > -40),
             'level_rms': float (np.sqrt (np.mean ((lp[m] - lh[m]) ** 2))),
             'sweep_rms': float (np.sqrt (np.mean ((rp[both] - rh[both]) ** 2))) if both.sum () > 10 else float ('nan'),
             'sweep_found': 100 * float (np.mean (~np.isnan (rp[~np.isnan (rh)]))),
             'spectrum_rms': float (np.sqrt (np.mean ((Lp[band] - Lh[band]) ** 2))) }


def main ():
    ap = argparse.ArgumentParser ()
    ap.add_argument ('hardware'); ap.add_argument ('part')
    ap.add_argument ('--start', type=float, required=True); ap.add_argument ('--end', type=float, required=True)
    ap.add_argument ('--render', default='build/babushka_render'); ap.add_argument ('--out', default='video_compare_out')
    a = ap.parse_args ()
    os.makedirs (a.out, exist_ok=True)
    plugin_wav = os.path.join (a.out, 'plugin.wav')
    subprocess.run ([a.render, '--play', a.part, plugin_wav], check=True)
    xh = read_mono (a.hardware, a.start, a.end); xp = read_mono (plugin_wav)
    n = min (len (xh), len (xp)); xh, xp = xh[:n], xp[:n]
    hw, pl = tracks (xh), tracks (xp)
    m = metrics (hw, pl, xh, xp)
    checks = [ ('sounding: plugin %.0f %% of the time, hardware %.0f %%' % (m['sounding_plugin'], m['sounding_hw']), abs (m['sounding_plugin'] - m['sounding_hw']) <= LIMITS['sounding_diff']),
               ('level against the hardware: rms %.1f dB' % m['level_rms'], m['level_rms'] <= LIMITS['level_rms']),
               ('sweep (osc 2 above osc 1): rms %.2f oct' % m['sweep_rms'], m['sweep_rms'] <= LIMITS['sweep_rms']),
               ('sweep found where the hardware has it: %.0f %%' % m['sweep_found'], m['sweep_found'] >= LIMITS['sweep_found']),
               ('spectrum 200 Hz - 8 kHz: rms %.1f dB' % m['spectrum_rms'], m['spectrum_rms'] <= LIMITS['spectrum_rms']) ]
    text = '\n'.join ('%-55s %s' % (c, 'OK' if ok else 'FAIL') for c, ok in checks)
    print (text); open (os.path.join (a.out, 'metrics.txt'), 'w').write (text + '\n')
    g = np.sqrt (np.mean (xh ** 2)) / max (np.sqrt (np.mean (xp ** 2)), 1e-12)
    wavfile.write (os.path.join (a.out, 'hardware.wav'), FS, xh.astype (np.float32))
    wavfile.write (os.path.join (a.out, 'plugin_matched.wav'), FS, (xp * g).astype (np.float32))
    try:
        import matplotlib; matplotlib.use ('Agg'); import matplotlib.pyplot as plt
        fig, ax = plt.subplots (4, 1, figsize=(14, 15))
        for k, (d, title) in enumerate (((hw, 'hardware'), (pl, 'plugin'))):
            s = d['spec'].T
            ax[k].pcolormesh (d['t'], d['f'][1:], s[1:], vmin=s.max () - 70, vmax=s.max (), shading='auto')
            ax[k].set_yscale ('log'); ax[k].set_ylim (40, 22000); ax[k].set_title (title); ax[k].set_ylabel ('Hz')
        for d, title in ((hw, 'hardware'), (pl, 'plugin')):
            ax[2].plot (d['t'], d['r'], '.', ms=3, label=title)
            ax[3].plot (d['t'], d['level'] - np.median (d['level'][d['level'] > -60]), label=title)
        ax[2].set_ylabel ('osc 2 above osc 1, octaves'); ax[2].set_ylim (0, 9); ax[2].legend ()
        ax[3].set_ylabel ('level, dB (median = 0)'); ax[3].set_ylim (-50, 10); ax[3].set_xlabel ('s'); ax[3].legend ()
        plt.tight_layout (); plt.savefig (os.path.join (a.out, 'report.png'), dpi=60)
    except ImportError:
        print ('(matplotlib not installed: no report.png)')
    return 0 if all (ok for _, ok in checks) else 1


if __name__ == '__main__':
    sys.exit (main ())
