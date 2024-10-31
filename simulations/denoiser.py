import numpy as np
from matplotlib import pyplot as plt

if __name__ == "__main__":
    tau = 30e-3
    TFRAME = 64 / 15000
    kFrame = np.exp(-TFRAME / tau)
    kFrame = round(32767 * kFrame)
    print(f"#define KFRAME ({kFrame})")
    print(f"#define ONE_MINUS_KFRAME ({32767 - kFrame})")

    print()
    x = np.arange(1, 3000, 1)
    y = 32767 * (((20 * np.log10(x)) * 0.02))
    print(f"static const q31_t db_lut[{len(y)}] = {{")
    y_str = list(map(lambda a: str(round(a)), y))
    lines = [y_str[i : i + 16] for i in range(0, len(y_str), 16)]
    for l in lines:
        print("    " + ", ".join(l) + ",")
    print("};")
    plt.plot(x, np.round(y) / 32767)
    plt.grid(True)
    plt.tight_layout()
    plt.show()
