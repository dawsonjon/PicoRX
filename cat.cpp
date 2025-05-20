#include "cat.h"
#include "settings.h"
#include <cstdint>
#include <cstring>
#include <algorithm>

#include "pico/stdlib.h"

void process_cat_control(rx_settings & settings_to_apply, rx_status & status, rx &receiver, s_settings &settings)
{
    const uint16_t buffer_length = 256;
    static char buf[buffer_length];
    static uint16_t read_idx = 0;
    static uint16_t write_idx = 0;
    static uint16_t items_in_buffer = 0;

    //write any new data into circular buffer
    {
      const uint16_t bytes_to_end = buffer_length-write_idx;
      int32_t retval = stdio_get_until(buf+write_idx, bytes_to_end, make_timeout_time_us(100));
      if(retval != PICO_ERROR_TIMEOUT)
      {
        items_in_buffer += retval;
        write_idx += retval;
        if(write_idx >= buffer_length) write_idx -= buffer_length;
      }
    }


    //copy command from circular buffer to command buffer
    static char cmd[buffer_length];
    const uint16_t bytes_to_read = items_in_buffer;
    const uint16_t bytes_to_end = std::min(bytes_to_read, (uint16_t)(buffer_length-read_idx));
    const uint16_t bytes_remaining = bytes_to_read - bytes_to_end;
    memcpy(cmd, buf+read_idx, bytes_to_end);
    memcpy(cmd+bytes_to_end, buf, bytes_remaining);

    //read command from circular_buffer
    char *command_end = (char*)memchr(cmd, ';', items_in_buffer);
    if(command_end == NULL) {
      //buffer full discard
      if(items_in_buffer == buffer_length){
        items_in_buffer = 0;
        read_idx = 0;
        write_idx = 0;
      }
      return;
    }

    //remove the command from circular buffer
    uint32_t command_length = command_end - cmd + 1;
    read_idx += command_length;
    if(read_idx > buffer_length) read_idx -= buffer_length;
    items_in_buffer -= command_length;

    bool settings_changed = false;
    const char mode_translation[] = "551243";
    static char vfor = '0';
    static char vfot = '0';

    if (strncmp(cmd, "FA", 2) == 0) {

        // Handle mode set/get commands
        if (cmd[2] == ';') {
            printf("FA%011lu;", settings.channel.frequency);
        } else {
            uint32_t frequency_Hz;
            sscanf(cmd+2, "%lu", &frequency_Hz);
            if(frequency_Hz <= 30000000)
            {
              settings.channel.frequency=frequency_Hz;
              settings_changed = true;
            }
            else
            {
              printf("?;");
            }
        }
    
    } else if (strncmp(cmd, "FB", 2) == 0) {

        // Handle mode set/get commands
        if (cmd[2] == ';') {
            printf("FA%011lu;", settings.channel.frequency);
        } else {
            uint32_t frequency_Hz;
            sscanf(cmd+2, "%lu", &frequency_Hz);
            if(frequency_Hz <= 30000000)
            {
              settings.channel.frequency=frequency_Hz;
              settings_changed = true;
            }
            else
            {
              printf("?;");
            }
        }

    } else if (strncmp(cmd, "SM", 2) == 0) {

        // Handle mode set/get commands
        if (cmd[3] == ';') {
            receiver.access(false);
            float power_dBm = status.signal_strength_dBm;
            receiver.release();
            float power_scaled = 020*((power_dBm - (-127))/114);
            power_scaled = std::min((float)0x20, power_scaled);
            power_scaled = std::max((float)0, power_scaled);
            printf("SM%05X;", (uint16_t)power_scaled);
        } else {
            printf("?;");
        }

    } else if (strncmp(cmd, "MD", 2) == 0) {

        // Handle mode set/get commands
        if (cmd[2] == ';') {
            char mode_status = mode_translation[settings.channel.mode];
            printf("MD%c;", mode_status);
        } else if (cmd[2] == '1') {
            settings_changed = true;
            settings.channel.mode = MODE_LSB;
        } else if (cmd[2] == '2') {
            settings_changed = true;
            settings.channel.mode = MODE_USB;
        } else if (cmd[2] == '3') {
            settings_changed = true;
            settings.channel.mode = MODE_CW;
        } else if (cmd[2] == '4') {
            settings_changed = true;
            settings.channel.mode = MODE_FM;
        } else if (cmd[2] == '5') {
            settings_changed = true;
            settings.channel.mode = MODE_AM;
        }

    } else if (strncmp(cmd, "IF", 2) == 0) {
        if (cmd[2] == ';') {
            printf("IF%011lu     +0000000000%c%c00000 ;", settings.channel.frequency, mode_translation[settings.channel.mode], vfor);
        }

    //fake TX for now
    } else if (strncmp(cmd, "ID", 2) == 0) {
        if (cmd[2] == ';') {
            printf("ID020;");
        }
    } else if (strncmp(cmd, "FR", 2) == 0) {
        if (cmd[2] == ';') {
            printf("FR%c;", vfor);
        }
        else if(cmd[3] == ';'){
            vfor = cmd[2];
        }
    } else if (strncmp(cmd, "FT", 2) == 0) {
        if (cmd[2] == ';') {
            printf("FT%c;", vfot);
        }
        else if(cmd[3] == ';'){
            vfot = cmd[2];
        }
    } else if (strncmp(cmd, "AI", 2) == 0) {
        if (cmd[2] == ';') {
            printf("AI0;");
        }
    } else if (strncmp(cmd, "AG", 2) == 0) {
        if (cmd[2] == ';') {
            printf("AG0;");
        }
    } else if (strncmp(cmd, "XT", 2) == 0) {
        if (cmd[2] == ';') {
            printf("XT1;");
        }
    } else if (strncmp(cmd, "RT", 2) == 0) {
        if (cmd[2] == ';') {
            printf("RT1;");
        }
    } else if (strncmp(cmd, "RC", 2) == 0) {
        if (cmd[2] == ';') {
            printf("RC;");
        }
    } else if (strncmp(cmd, "FL", 2) == 0) {
        if (cmd[2] == ';') {
            printf("FL0;");
        }
    } else if (strncmp(cmd, "PS", 2) == 0) {
        if (cmd[2] == ';') {
            printf("PS1;");
        }
    } else if (strncmp(cmd, "VX", 2) == 0) {
        if (cmd[2] == ';') {
            printf("VX0;");
        }
    } else if (strncmp(cmd, "RS", 2) == 0) {
        if (cmd[2] == ';') {
            printf("RS0;");
        }
    } else if (strncmp(cmd, "FL", 2) == 0) {
        if (cmd[2] == ';') {
            printf("FL0;");
        }
    } else if (strncmp(cmd, "AC", 2) == 0) {
        if (cmd[2] == ';') {
            printf("AC010;");
        }
    } else if (strncmp(cmd, "SL", 2) == 0) {
        if (cmd[2] == ';') {
            printf("SL00;");
        }
    } else if (strncmp(cmd, "SH", 2) == 0) {
        if (cmd[2] == ';') {
            printf("SH00;");
        }
    } else if (strncmp(cmd, "IS", 2) == 0) {
        if (cmd[2] == ';') {
            printf("IS+0000;");
        }
    } else if (strncmp(cmd, "PR", 2) == 0) {
        if (cmd[2] == ';') {
            printf("PR0;");
        }
    } else if (strncmp(cmd, "NB", 2) == 0) {
        if (cmd[2] == ';') {
            printf("NB0;");
        }
    } else if (strncmp(cmd, "LK", 2) == 0) {
        if (cmd[2] == ';') {
            printf("LK00;");
        }
    } else if (strncmp(cmd, "MG", 2) == 0) {
        if (cmd[2] == ';') {
            printf("MG000;");
        }
    } else if (strncmp(cmd, "PL", 2) == 0) {
        if (cmd[2] == ';') {
            printf("PL000000;");
        }
    } else if (strncmp(cmd, "VD", 2) == 0) {
        if (cmd[2] == ';') {
            printf("VD0000;");
        }
    } else if (strncmp(cmd, "VG", 2) == 0) {
        if (cmd[2] == ';') {
            printf("VG000;");
        }
    } else if (strncmp(cmd, "BC", 2) == 0) {
        if (cmd[2] == ';') {
            printf("BC0;");
        }
    } else if (strncmp(cmd, "ML", 2) == 0) {
        if (cmd[2] == ';') {
            printf("ML000;");
        }
    } else if (strncmp(cmd, "NR", 2) == 0) {
        if (cmd[2] == ';') {
            printf("NR0;");
        }
    } else if (strncmp(cmd, "SD", 2) == 0) {
        if (cmd[2] == ';') {
            printf("SD0000;");
        }
    } else if (strncmp(cmd, "KS", 2) == 0) {
        if (cmd[2] == ';') {
            printf("KS010;");
        }
    } else if (strncmp(cmd, "EX", 2) == 0) {
        if (cmd[2] == ';') {
            printf("EX000000000;");
        }
    } else if (strncmp(cmd, "RL", 2) == 0) {
        if (cmd[2] == ';') {
            printf("RL00;");
        }
    } else if (strncmp(cmd, "SQ", 2) == 0) {
        if (cmd[2] == ';') {
            printf("SQ0000;");
        }
    } else if (strncmp(cmd, "RG", 2) == 0) {
        if (cmd[2] == ';') {
            printf("RG000;");
        }
    } else if (strncmp(cmd, "RM", 2) == 0) {
        if (cmd[2] == ';') {
            printf("RM10000;");
        }
    } else if (strncmp(cmd, "PA", 2) == 0) {
        if (cmd[2] == ';') {
            printf("PA00;");
        }
    } else if (strncmp(cmd, "RA", 2) == 0) {
        if (cmd[2] == ';') {
            printf("RA0000;");
        }
    } else if (strncmp(cmd, "GT", 2) == 0) {
        if (cmd[2] == ';') {
            printf("GT000;");
        }
    } else if (strncmp(cmd, "PC", 2) == 0) {
        if (cmd[2] == ';') {
            printf("PC005;");
        }
    } else if (strncmp(cmd, "FW", 2) == 0) {
        if (cmd[2] == ';') {
            printf("FW0000;");
        }
    } else if (strncmp(cmd, "TX", 2) == 0) {
        static uint8_t tx_status = 0;

        // Set or Get TX mode command (TX)
        if (cmd[2] == ';') {
            // Get current transmit status
            printf("TX%d;", tx_status);
        } else if (cmd[2] == '1') {
            // Switch to TX mode
            tx_status = 1;
        } else if (cmd[2] == '0') {
            // Switch to RX mode
            tx_status = 0;
        } else {
            // Invalid TX command format
            printf("?;");
        }

    } else if (strncmp(cmd, "ZUP", 3) == 0) {
        if (cmd[134] == ';') {

          //Extract channel number
          char channel_string[4];
          memcpy(channel_string, &cmd[3], 3); cmd[3] = 0;
          uint32_t channel_number = strtoul(channel_string, NULL, 16);

          //Read word data
          uint32_t words[16];
          for(uint8_t word_idx=0; word_idx<16; ++word_idx)
          {
            char word_string[9];
            memcpy(word_string, &cmd[(word_idx * 8) + 6], 8); cmd[8] = 0;
            words[word_idx] = strtoul(word_string, NULL, 16);
          }

          s_memory_channel memory_channel;
          memcpy(&memory_channel, words, sizeof(memory_channel)); 
          memory_store_channel(memory_channel, channel_number, settings, receiver, settings_to_apply);
          printf("ZUP%03lx;", channel_number);

        } else {
            printf("?;");
        }
    } else if (strncmp(cmd, "ZDN", 3) == 0) {
        if (cmd[6] == ';') {
          char channel_string[4];
          memcpy(channel_string, &cmd[3], 3); cmd[3] = 0;
          uint32_t channel_number = strtoul(channel_string, NULL, 16);

          printf("ZDN%03lx", channel_number);
          s_memory_channel memory_channel = get_channel(channel_number);
          uint32_t words[16];
          memcpy(words, &memory_channel, sizeof(memory_channel)); 
          for(uint8_t word_idx=0; word_idx<16; ++word_idx)
          {
            printf("%08lx", words[word_idx]);
          }
          printf(";");

        } else {
            printf("?;");
        }
    } else {
        // Unknown command
        printf("?;");
    }

    //apply settings to receiver
    apply_settings_to_rx(receiver, settings_to_apply, settings, false, settings_changed);

}
