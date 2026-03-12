import numpy as np
from matplotlib import pyplot as plt

def envelope_follower(data):
    envelope = []
    y = 0
    for x in data[10:]:
        attack = 1.0
        decay = 0.004
        env = abs(x)
        if env > y:
            y += attack * (env-y)
        else:
            y += decay * (env-y)
        envelope.append(y)

    for i in range(10):
        envelope.append(y)

    return envelope

def envelope_compressor(data, envelope, limit):
    compressed = []
    for x, e in zip(data, envelope):
        if e > limit:
            compressed.append(x*limit/e)
        else:
            compressed.append(x)
    return np.array(compressed)


with open("test.pcm") as inf:
    data = np.fromfile(inf, dtype='int16')

#remove DC
data = data - np.mean(data)

peak = np.max(np.abs(data))
rms = np.sqrt(np.mean(data**2))
print(peak/rms, 20*np.log10(peak/rms))
rms_orig = rms
orig_peak = peak

limit = 2048
envelope = envelope_follower(data)
compressed = envelope_compressor(data, envelope, limit)
peak = np.max(np.abs(compressed))
rms = np.sqrt(np.mean(compressed**2))
print(peak/rms, 20*np.log10(peak/rms))
gain = orig_peak/peak
compressed *= gain

rms = np.sqrt(np.mean(compressed**2))
print(20*np.log10(rms/rms_orig))

plt.plot(data)
plt.plot(envelope)
plt.plot(compressed)
plt.show()

with open("test_out.pcm", 'wb') as outf:
    out = compressed.astype("int16").tobytes()
    outf.write(out)
