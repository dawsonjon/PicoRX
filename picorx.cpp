#include "pico/stdlib.h"

// Our assembled program:
#include "rx.h"

int main() {
    stdio_init_all();
    
    rx receiver;
    receiver.set_frequency_Hz(1215e3);
    receiver.set_frequency_Hz(198e3);
    receiver.run();
}
