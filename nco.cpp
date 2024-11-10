#include "nco.h"

#include "hardware/clocks.h"
#include "hardware/pio.h"
#include "pico/stdlib.h"
#include <cmath>

float nco_set_frequency(PIO pio, uint sm, float tuned_frequency, uint32_t &system_clock_frequency_out) {

    //We can get closer to the derired frequency if we allow small adjustments
    //to the system clock. system clocks in the range 125 - 133 MHz are fast
    //enough to run the software. There are nearly 50 different frequencies in this
    //range, by chosing the frequency that gives the best match, we can get within about 4Khz.
    struct PLLSettings{
      uint32_t frequency;
      uint8_t refdiv;
      uint16_t fbdiv;
      uint8_t postdiv1;
      uint8_t postdiv2;
    };

    //A list of all the achievable frequencies in range
    PLLSettings possible_frequencies[] = {
    {125000000, 1, 125, 6, 2},
    {125142857, 1, 73, 7, 1},
    {125333333, 1, 94, 3, 3},
    {126000000, 1, 126, 6, 2},
    {126666666, 1, 95, 3, 3},
    {126857142, 1, 74, 7, 1},
    {127000000, 1, 127, 6, 2},
    {127200000, 1, 106, 5, 2},
    {127500000, 1, 85, 4, 2},
    {128000000, 1, 128, 6, 2},
    {128400000, 1, 107, 5, 2},
    {128571428, 1, 75, 7, 1},
    {129000000, 1, 129, 6, 2},
    {129333333, 1, 97, 3, 3},
    {129600000, 1, 108, 5, 2},
    {130000000, 1, 130, 6, 2},
    {130285714, 1, 76, 7, 1},
    {130500000, 1, 87, 4, 2},
    {130666666, 1, 98, 3, 3},
    {130800000, 1, 109, 5, 2},
    {131000000, 1, 131, 6, 2},
    {132000000, 1, 132, 6, 2},
    {133000000, 1, 133, 6, 2}

    #if PICO_PLATFORM==rp2350-arm-s
    ,{133200000, 1, 111, 5, 2},
    {133333333, 1, 100, 3, 3},
    {133500000, 1, 89, 4, 2},
    {133714285, 1, 78, 7, 1},
    {134000000, 1, 67, 6, 1},
    {134400000, 1, 112, 5, 2},
    {134666666, 1, 101, 3, 3},
    {135000000, 1, 90, 4, 2},
    {135428571, 1, 79, 7, 1},
    {135600000, 1, 113, 5, 2},
    {136000000, 1, 102, 3, 3},
    {136500000, 1, 91, 4, 2},
    {136800000, 1, 114, 5, 2},
    {137142857, 1, 80, 7, 1},
    {137333333, 1, 103, 3, 3},
    {138000000, 1, 115, 5, 2},
    {138666666, 1, 104, 3, 3},
    {138857142, 1, 81, 7, 1},
    {139200000, 1, 116, 5, 2},
    {139500000, 1, 93, 4, 2},
    {140000000, 1, 105, 3, 3},
    {140400000, 1, 117, 5, 2},
    {140571428, 1, 82, 7, 1},
    {141000000, 1, 94, 4, 2},
    {141333333, 1, 106, 3, 3},
    {141600000, 1, 118, 5, 2},
    {142000000, 1, 71, 6, 1},
    {142285714, 1, 83, 7, 1},
    {142500000, 1, 95, 4, 2},
    {142666666, 1, 107, 3, 3},
    {142800000, 1, 119, 5, 2},
    {144000000, 1, 120, 5, 2},
    {145200000, 1, 121, 5, 2},
    {145333333, 1, 109, 3, 3},
    {145500000, 1, 97, 4, 2},
    {145714285, 1, 85, 7, 1},
    {146000000, 1, 73, 6, 1},
    {146400000, 1, 122, 5, 2},
    {146666666, 1, 110, 3, 3},
    {147000000, 1, 98, 4, 2},
    {147428571, 1, 86, 7, 1},
    {147600000, 1, 123, 5, 2},
    {148000000, 1, 111, 3, 3},
    {148500000, 1, 99, 4, 2},
    {148800000, 1, 124, 5, 2},
    {149142857, 1, 87, 7, 1},
    {149333333, 1, 112, 3, 3},
    {150000000, 1, 125, 5, 2}
    #endif

    #if PICO_PLATFORM==rp2350-riscv
    ,{133200000, 1, 111, 5, 2},
    {133333333, 1, 100, 3, 3},
    {133500000, 1, 89, 4, 2},
    {133714285, 1, 78, 7, 1},
    {134000000, 1, 67, 6, 1},
    {134400000, 1, 112, 5, 2},
    {134666666, 1, 101, 3, 3},
    {135000000, 1, 90, 4, 2},
    {135428571, 1, 79, 7, 1},
    {135600000, 1, 113, 5, 2},
    {136000000, 1, 102, 3, 3},
    {136500000, 1, 91, 4, 2},
    {136800000, 1, 114, 5, 2},
    {137142857, 1, 80, 7, 1},
    {137333333, 1, 103, 3, 3},
    {138000000, 1, 115, 5, 2},
    {138666666, 1, 104, 3, 3},
    {138857142, 1, 81, 7, 1},
    {139200000, 1, 116, 5, 2},
    {139500000, 1, 93, 4, 2},
    {140000000, 1, 105, 3, 3},
    {140400000, 1, 117, 5, 2},
    {140571428, 1, 82, 7, 1},
    {141000000, 1, 94, 4, 2},
    {141333333, 1, 106, 3, 3},
    {141600000, 1, 118, 5, 2},
    {142000000, 1, 71, 6, 1},
    {142285714, 1, 83, 7, 1},
    {142500000, 1, 95, 4, 2},
    {142666666, 1, 107, 3, 3},
    {142800000, 1, 119, 5, 2},
    {144000000, 1, 120, 5, 2},
    {145200000, 1, 121, 5, 2},
    {145333333, 1, 109, 3, 3},
    {145500000, 1, 97, 4, 2},
    {145714285, 1, 85, 7, 1},
    {146000000, 1, 73, 6, 1},
    {146400000, 1, 122, 5, 2},
    {146666666, 1, 110, 3, 3},
    {147000000, 1, 98, 4, 2},
    {147428571, 1, 86, 7, 1},
    {147600000, 1, 123, 5, 2},
    {148000000, 1, 111, 3, 3},
    {148500000, 1, 99, 4, 2},
    {148800000, 1, 124, 5, 2},
    {149142857, 1, 87, 7, 1},
    {149333333, 1, 112, 3, 3},
    {150000000, 1, 125, 5, 2}
    #endif

    };

    double adjusted_frequency_up = tuned_frequency + 4500.0;
    double adjusted_frequency_down = tuned_frequency - 4500.0;
    PLLSettings best_settings = {0};
    double best_frequency = 1.0;
    double best_divider = 0.0;
    double best_error = 1000000.0;

    for(uint8_t idx = 0; idx < sizeof(possible_frequencies)/sizeof(PLLSettings); idx++)
    {

      uint32_t system_clock_frequency = possible_frequencies[idx].frequency;

      double ideal_divider = system_clock_frequency/(4.0*adjusted_frequency_up);
      double nearest_divider = round(256.0*ideal_divider)/256.0;
      double actual_frequency = system_clock_frequency/nearest_divider;
      double error = abs(actual_frequency - 4.0*adjusted_frequency_up);
      if(error < best_error)
      {
        best_frequency = actual_frequency;
        best_settings = possible_frequencies[idx];
        best_divider = nearest_divider;
        best_error = error;
        system_clock_frequency_out = system_clock_frequency;
      }

      ideal_divider = system_clock_frequency/(4.0*adjusted_frequency_down);
      nearest_divider = round(256.0*ideal_divider)/256.0;
      actual_frequency = system_clock_frequency/nearest_divider;
      error = abs(actual_frequency - 4.0*adjusted_frequency_down);
      if(error < best_error)
      {
        best_frequency = actual_frequency;
        best_settings = possible_frequencies[idx];
        best_divider = nearest_divider;
        best_error = error;
        system_clock_frequency_out = system_clock_frequency;
      }
    }

    assert(best_error < 1000000);
    //adjust system clock
    uint32_t vco_freq = (12000000 / best_settings.refdiv) * best_settings.fbdiv;
    set_sys_clock_pll(vco_freq, best_settings.postdiv1, best_settings.postdiv2);

    //set pio divider
    pio_sm_set_clkdiv(pio, sm, best_divider);

    //return actual frequency
    return best_frequency/4.0;
}
