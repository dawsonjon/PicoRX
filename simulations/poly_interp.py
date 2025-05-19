import numpy as np
import scipy.signal as sig
import matplotlib.pyplot as plt

M = 16
SR = 15000


def poly_linear_filter():
    h = [i / M for i in range(M)] + [1 - (i / M) for i in range(M)]
    return h


def poly_window_filter():
    N = 2
    h = np.array([np.sin(np.pi * i / (2 * M * N)) for i in range(2 * M * N)])
    h = (h * h) / N
    plt.stem(h)
    plt.show()
    return h


def poly_interp(s, h, prnt=False):
    slices = []
    N = len(h) // (2 * M)
    if prnt:
        print([round(32767 * x) for x in h])
    for i in range(M):
        f = sig.lfilter([h[i + j * M] for j in range(2 * N)], [1], s)
        slices.append(f)
    interp = np.array(slices).T.reshape(M * len(t))
    return interp


def linear_interp(s):
    last = 0
    integrator = 0
    out = []
    for ss in s:
        comb = ss - last
        last = ss
        for i in range(M):
            integrator += comb
            out.append(integrator / M)
    return out


end_time = 0.01
t = np.arange(0, end_time, 1 / SR)
s = np.sin(2 * np.pi * 1500 * t)

li = linear_interp(s)
pil = poly_interp(s, poly_linear_filter())
piw = poly_interp(s, poly_window_filter(), True)

t2 = np.arange(0, end_time, 1 / (M * SR))

plt.plot(t, s, marker="o")
plt.plot(t2, li, marker="x")
plt.plot(t2, pil, marker="x")
plt.plot(t2, piw, marker="x")
plt.grid(True)
plt.show()
