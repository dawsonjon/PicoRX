import math

FS = 250000 // 32


def to_fixed(arr):
    return list(map(lambda x: round(x * 32768), arr))

def standard_deemph(tau=50e-6, fs=FS):
    w_c = 1.0 / tau

    w_ca = 2.0 * fs * math.tan(w_c / (2.0 * fs))

    k = -w_ca / (2.0 * fs)
    z1 = -1.0
    p1 = (1.0 + k) / (1.0 - k)
    b0 = -k / (1.0 - k)

    btaps = [b0 * 1.0, b0 * -z1]
    ataps = [1.0, -p1]

    return to_fixed(btaps), to_fixed(ataps)

b, a = standard_deemph()
print(f"De-emphasis 50us: bs={b}, as={a}")
b, a = standard_deemph(75e-6)
print(f"De-emphasis 75us: bs={b}, as={a}")
