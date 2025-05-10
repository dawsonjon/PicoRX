import numpy as np
import matplotlib.pyplot as plt

fix_one = 32767

db_low = -5
db_high = 20
a_high = 6
a_low = 1

a = -(a_high - a_low) / (db_high - db_low)
b = a_high - a * db_low
lin_low, lin_high = 10 ** (db_low / 20), 10 ** (db_high / 20)

x = np.arange(lin_low, lin_high, 0.025)
db_x = 20 * np.log10(x)
alpha = a * db_x + b
alpha = np.clip(alpha, a_low, a_high)

lin_low_fix, lin_high_fix = round(lin_low * fix_one), round(lin_high * fix_one)
print(f"#define ALPHA_LOW ({round(fix_one * a_low)}) // {a_low}")
print(f"#define ALPHA_HIGH ({round(fix_one * a_high)}) // {a_high}")

print()
print(f"#define SNR_LIN_LOW ({lin_low_fix}) // {lin_low}")
print(f"#define SNR_LIN_HIGH ({lin_high_fix}) // {lin_high}")
print(f"#define SNR_LUT_SCALE ({(lin_high_fix - lin_low_fix) // len(x)})")

plt.plot(x, alpha, marker = "x")
plt.grid(True)
plt.show()

alpha = list(map(lambda x: round(x * fix_one), alpha))
alpha_lines = [alpha[i : i + 9] for i in range(0, len(alpha), 9)]
print()
print("static const uint32_t alpha_lut[] = {")
for line in alpha_lines:
    print("    " + ", ".join(map(str, line)) + ",")
print("};")
