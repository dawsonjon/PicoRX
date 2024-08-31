sample_rate = 500000/16
cutoff_freq = 500

RC = 1.0 / (2.0 * 3.141592653589793 * cutoff_freq);
dt = 1.0 / sample_rate;
alpha = RC / (RC + dt);

print(RC, dt, alpha*(2**15))
