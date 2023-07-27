#include <string.h>
#include "pico/multicore.h"
#include "ui.h"
#include <hardware/flash.h>


////////////////////////////////////////////////////////////////////////////////
// Encoder 
////////////////////////////////////////////////////////////////////////////////

void ui::setup_encoder()
{
    uint offset = pio_add_program(pio, &quadrature_encoder_program);
    quadrature_encoder_program_init(pio, sm, offset, PIN_AB, 1000);
    new_position = (quadrature_encoder_get_count(pio, sm) + 2)/4;
    old_position = new_position;
}

int32_t ui::get_encoder_change()
{
    new_position = -((quadrature_encoder_get_count(pio, sm) + 2)/4);
    int32_t delta = new_position - old_position;
    old_position = new_position;
    return delta;
}

int32_t ui::encoder_control(int32_t *value, int32_t min, int32_t max)
{
	int32_t position_change = get_encoder_change();
	*value += position_change;
	if(*value > max) *value = min;
	if(*value < min) *value = max;
	return position_change;
}

////////////////////////////////////////////////////////////////////////////////
// Buttons 
////////////////////////////////////////////////////////////////////////////////

void ui::setup_buttons()
{
  gpio_init(PIN_MENU);
  gpio_set_dir(PIN_MENU, GPIO_IN);
  gpio_pull_up(PIN_MENU);
  gpio_init(PIN_BACK);
  gpio_set_dir(PIN_BACK, GPIO_IN);
  gpio_pull_up(PIN_BACK);
}

bool ui::get_button(uint8_t button){
	if(!gpio_get(button)){
		while(!gpio_get(button)){}
		WAIT_10MS
		return 1;
	}
	WAIT_10MS
	return 0;
}

bool ui::check_button(unsigned button){
	return !gpio_get(button);
}

////////////////////////////////////////////////////////////////////////////////
// Display
////////////////////////////////////////////////////////////////////////////////
void ui::setup_display() {
  i2c_init(i2c1, 400000);
  gpio_set_function(PIN_DISPLAY_SDA, GPIO_FUNC_I2C);
  gpio_set_function(PIN_DISPLAY_SCL, GPIO_FUNC_I2C);
  gpio_pull_up(PIN_DISPLAY_SDA);
  gpio_pull_up(PIN_DISPLAY_SCL);
  disp.external_vcc=false;
  ssd1306_init(&disp, 128, 64, 0x3C, i2c1);
}

void ui::display_clear()
{
  cursor_x = 0;
  cursor_y = 0;
  ssd1306_clear(&disp);
}

void ui::display_line1()
{
  cursor_y = 0;
  cursor_x = 0;
}

void ui::display_line2()
{
  cursor_y = 1;
  cursor_x = 0;
}

void ui::display_write(char x)
{
  ssd1306_draw_char(&disp, cursor_x*6, cursor_y*8, 1, x);
  cursor_x++;
}

void ui::display_print(const char str[])
{
  ssd1306_draw_string(&disp, cursor_x*6, cursor_y*8, 1, str);
  cursor_x+=strlen(str);
}

void ui::display_print_num(const char format[], int16_t num)
{
  char buff[16];
  snprintf(buff, 16, format, num);
  ssd1306_draw_string(&disp, cursor_x*6, cursor_y*8, 1, buff);
  cursor_x+=strlen(buff);
}

void ui::display_show()
{
  ssd1306_show(&disp);
}

////////////////////////////////////////////////////////////////////////////////
// Generic Menu Options
////////////////////////////////////////////////////////////////////////////////
void ui::update_display(rx_status & status, rx & receiver)
{
  char buff [21];
  ssd1306_clear(&disp);

  //frequency
  uint32_t remainder, MHz, kHz, Hz;
  MHz = (uint32_t)settings[idx_frequency]/1000000u;
  remainder = (uint32_t)settings[idx_frequency]%1000000u; 
  kHz = remainder/1000u;
  remainder = remainder%1000u; 
  Hz = remainder;
  snprintf(buff, 21, "%2u.%03u", MHz, kHz);
  ssd1306_draw_string(&disp, 0, 0, 2, buff);
  snprintf(buff, 21, ".%03u", Hz);
  ssd1306_draw_string(&disp, 72, 0, 1, buff);

  //signal strength
  const float power_dBm = status.signal_strength_dBm;

  //Battery
  const float battery_voltage = 3.0f * 3.3f * (status.battery/65535.0f);
  
  //CPU Temp
  const float temp_voltage = 3.3f * (status.temp/65535.0f);
  const float temp = 27.0f - (temp_voltage - 0.706f)/0.001721f;

  //CPU 
  const float block_time = (float)adc_block_size/(float)adc_sample_rate;
  const float busy_time = ((float)status.busy_time*1e-6f);

  //mode
  static const char modes[][4]  = {" AM", "LSB", "USB", " FM", " CW"};
  ssd1306_draw_string(&disp, 102, 0, 1, modes[settings[idx_mode]]);

  //step
  static const char steps[][8]  = {
    "   10Hz", "   50Hz", "  100Hz", "   1kHz",
    "   5kHz", "  10kHz", "12.5kHz", "  25kHz", 
    "  50kHz", " 100kHz"};
  ssd1306_draw_string(&disp, 78, 8, 1, steps[settings[idx_step]]);

  //signal strength/cpu
  static const char smeter[][12]  = {
    "S0         ", "S1|        ", "S2-|       ", "S3--|      ", 
    "S4---|     ", "S5----|    ", "S6-----|   ", "S7------|  ", 
    "S8-------| ", "S9--------|", "S9+10dB---|", "S9+20dB---|", 
    "S9+30dB---|"};
  uint8_t power_s = floorf((power_dBm-S0)/6.0f);
  if(power_dBm >= S9) power_s = floorf((power_dBm-S9)/10.0f)+9;
  if(power_s < 0) power_s = 0;
  snprintf(buff, 21, "%s  % 4.0fdBm", smeter[power_s], power_dBm);
  ssd1306_draw_string(&disp, 0, 24, 1, buff);
  snprintf(buff, 21, "       %2.1fV %2.0f%cC %2.0f%%", battery_voltage, temp, '\x7f', (100.0f*busy_time)/block_time);
  ssd1306_draw_string(&disp, 0, 16, 1, buff);

  //Display spectrum capture
  static float spectrum[128];
  int16_t offset;
  receiver.get_spectrum(spectrum, offset);
  ssd1306_draw_string(&disp, 64, 32, 1, "v");

  //auto scale
  float min=100;
  float max=0;
  for(uint16_t x=0; x<128; x++)
  {
      float level = log10f(spectrum[x]);
      if(level > max) max = ceilf(level);
      if(level < min) min = floorf(level);
  }
  float scale = 32.0f/(max-min);

  //plot
  for(uint16_t x=0; x<128; x++)
  {
      int16_t y = scale*(log10f(spectrum[x])-min);
      if(y < 0) y=0;
      if(y > 31) y=31;
      ssd1306_draw_line(&disp, x, 63-y, x, 63);
  }

  ssd1306_show(&disp);

}

////////////////////////////////////////////////////////////////////////////////
// Generic Menu Options
////////////////////////////////////////////////////////////////////////////////

void ui::print_option(const char options[], uint8_t option){
    char x;
    uint8_t i, idx=0;

    //find nth substring
    for(i=0; i<option; i++){ 
      while(options[idx++]!='#'){}
    }

    //print substring
    while(1){
      x = options[idx];
      if(x==0 || x=='#') return;
      display_write(x);
      idx++;
    }
}

//choose from an enumerate list of settings
uint32_t ui::enumerate_entry(const char title[], const char options[], uint32_t max, uint32_t *value)
{
  int32_t select=*value;
  bool draw_once = true;
  while(1){
    if(encoder_control(&select, 0, max)!=0 || draw_once)
    {
      //print selected menu item
      draw_once = false;
      display_clear();
      display_print(title);
      display_line2();
      print_option(options, select);
      display_show();
    }

    //select menu item
    if(get_button(PIN_MENU)){
      *value = select;
      return 1;
    }

    //cancel
    if(get_button(PIN_BACK)){
      return 0;
    }

    WAIT_100MS
  }
}

//select a number in a range
int16_t ui::number_entry(const char title[], const char format[], int16_t min, int16_t max, int16_t multiple, uint32_t *value)
{
  int32_t select=*value;

  bool draw_once = true;
  while(1){
    if(encoder_control(&select, min, max)!=0 || draw_once)
    {
      //print selected menu item
      draw_once = false;
      display_clear();
      display_print(title);
      display_line2();
      display_print_num(format, select*multiple);
      display_show();
    }

    //select menu item
    if(get_button(PIN_MENU)){
      *value = select*multiple;
      return 1;
    }

    //cancel
    if(get_button(PIN_BACK)){
      return 0;
    }

    WAIT_100MS
  }
}

//Apply settings
void ui::apply_settings(bool suspend)
{
  receiver.access(true);
  settings_to_apply.tuned_frequency_Hz = settings[idx_frequency];
  settings_to_apply.agc_speed = settings[idx_agc_speed];
  settings_to_apply.mode = settings[idx_mode];
  settings_to_apply.volume = settings[idx_volume];
  settings_to_apply.squelch = settings[idx_squelch];
  settings_to_apply.step_Hz = step_sizes[settings[idx_step]];
  settings_to_apply.cw_sidetone_Hz = settings[idx_cw_sidetone];
  settings_to_apply.suspend = suspend;
  receiver.release();
}

//remember settings across power cycles
void ui::autosave()
{
  //The flash endurance may not be more than 100000 erase cycles.
  //Cycle through 512 locations, only erasing the flash when they have all been used.
  //This should give an endurance of 51,200,000 cycles.

  //find the next unused channel, an unused channel will be 0xffffffff
  uint16_t empty_channel = 0;
  bool empty_channel_found = false;
  for(uint16_t i=0; i<512; i++)
  {
    if(autosave_memory[i][0] == 0xffffffff)
    {
      empty_channel = i;
      empty_channel_found = true;
      break;
    } 
  }

  //check whether data differs
  if(empty_channel > 0)
  {
    bool difference_found = false;
    for(uint8_t i=0; i<16; i++){
      if(i != idx_frequency) //ignore frequency changes, they happen too often
      {
        if(autosave_memory[empty_channel - 1][i] != settings[i])
        {
          difference_found = true;
          break;
        }
      }
    }
    //data hasn't changed, no need to save
    if(!difference_found)
    {
      return;
    }
  }

  //if there are no free channels, erase all the pages
  if(!empty_channel_found)
  {
    const uint32_t address = (uint32_t)&(autosave_memory[0]);
    const uint32_t flash_address = address - XIP_BASE; 
    //!!! PICO is **very** fussy about flash erasing, there must be no code running in flash.  !!!
    //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    apply_settings(true);                                //suspend rx to disable all DMA transfers
    WAIT_100MS                                           //wait for suspension to take effect
    multicore_lockout_start_blocking();                  //halt the second core
    const uint32_t ints = save_and_disable_interrupts(); //disable all interrupts

    //safe to erase flash here
    //--------------------------------------------------------------------------------------------
    flash_range_erase(flash_address, sizeof(int)*16*512);
    //--------------------------------------------------------------------------------------------

    restore_interrupts (ints);                           //restore interrupts
    multicore_lockout_end_blocking();                    //restart the second core
    apply_settings(false);                               //resume rx operation
    //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    //!!! Normal operation resumed
  }

  //work out which flash sector the channel sits in.
  const uint32_t num_channels_per_sector = FLASH_SECTOR_SIZE/(sizeof(int)*chan_size);
  const uint32_t first_channel_in_sector = num_channels_per_sector * (empty_channel/num_channels_per_sector);
  const uint32_t channel_offset_in_sector = empty_channel%num_channels_per_sector;

  //copy sector to RAM
  static uint32_t sector_copy[num_channels_per_sector][chan_size];
  for(uint16_t channel=0; channel<num_channels_per_sector; channel++)
  {
    for(uint16_t location=0; location<chan_size; location++)
    {
      if(channel+first_channel_in_sector < num_chans)
      {
        sector_copy[channel][location] = autosave_memory[channel+first_channel_in_sector][location];
      }
    }
  }
    
  //modify the selected channel
  for(uint8_t i=0; i<16; i++)
  {
    sector_copy[channel_offset_in_sector][i] = settings[i];
  }

  //write sector to flash
  const uint32_t address = (uint32_t)&(autosave_memory[first_channel_in_sector]);
  const uint32_t flash_address = address - XIP_BASE; 

  //!!! PICO is **very** fussy about flash erasing, there must be no code running in flash.  !!!
  //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  apply_settings(true);                                //suspend rx to disable all DMA transfers
  WAIT_100MS                                           //wait for suspension to take effect
  multicore_lockout_start_blocking();                  //halt the second core
  const uint32_t ints = save_and_disable_interrupts(); //disable all interrupts

  //safe to erase flash here
  //--------------------------------------------------------------------------------------------
  flash_range_program(flash_address, (const uint8_t*)&sector_copy, FLASH_SECTOR_SIZE);
  //--------------------------------------------------------------------------------------------

  restore_interrupts (ints);                           //restore interrupts
  multicore_lockout_end_blocking();                    //restart the second core
  apply_settings(false);                               //resume rx operation
  //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  //!!! Normal operation resumed

}

//remember settings across power cycles
void ui::autorestore()
{

  //find the next unused channel, an unused channel will be 0xffffffff
  uint16_t empty_channel = 0;
  bool empty_channel_found = false;
  for(uint16_t i=0; i<512; i++)
  {
    if(autosave_memory[i][0] == 0xffffffff)
    {
      empty_channel = i;
      empty_channel_found = true;
      break;
    } 
  }

  uint16_t last_channel_written;
  if(empty_channel > 0) last_channel_written = empty_channel - 1;
  if(!empty_channel_found) last_channel_written = 255;
  for(uint8_t i=0; i<16; i++){
    settings[i] = autosave_memory[last_channel_written][i];
  }

  apply_settings(false);

}

//save current settings to memory
bool ui::store()
{

  //encoder loops through memories
  uint32_t min = 0;
  uint32_t max = num_chans-1;
  int32_t select=min;
  char name[17];

  bool draw_once = true;
  while(1){
    if(encoder_control(&select, min, max)!=0 || draw_once)
    {

      if(radio_memory[select][9] != 0xffffffffu)
      {
        //load name from memory
        name[0] = radio_memory[select][6] >> 24;
        name[1] = radio_memory[select][6] >> 16;
        name[2] = radio_memory[select][6] >> 8;
        name[3] = radio_memory[select][6];
        name[4] = radio_memory[select][7] >> 24;
        name[5] = radio_memory[select][7] >> 16;
        name[6] = radio_memory[select][7] >> 8;
        name[7] = radio_memory[select][7];
        name[8] = radio_memory[select][8] >> 24;
        name[9] = radio_memory[select][8] >> 16;
        name[10] = radio_memory[select][8] >> 8;
        name[11] = radio_memory[select][8];
        name[12] = radio_memory[select][9] >> 24;
        name[13] = radio_memory[select][9] >> 16;
        name[14] = radio_memory[select][9] >> 8;
        name[15] = radio_memory[select][9];
        name[16] = 0;

        //print selected menu item
        draw_once = false;
        display_clear();
        display_print("Store");
        display_line2();
        display_print_num("%03i ", select);
        display_print(name);
        display_show();
      }
      else
      {
        //print selected menu item
        draw_once = false;
        display_clear();
        display_print("Store");
        display_line2();
        display_print_num("%03i ", select);
        display_print("BLANK");
        display_show();
      }
    }

    //select menu item
    if(get_button(PIN_MENU)){

      //work out which flash sector the channel sits in.
      const uint32_t num_channels_per_sector = FLASH_SECTOR_SIZE/(sizeof(int)*chan_size);
      const uint32_t first_channel_in_sector = num_channels_per_sector * (select/num_channels_per_sector);
      const uint32_t channel_offset_in_sector = select%num_channels_per_sector;

      //copy sector to RAM
      static uint32_t sector_copy[num_channels_per_sector][chan_size];
      for(uint16_t channel=0; channel<num_channels_per_sector; channel++)
      {
        for(uint16_t location=0; location<chan_size; location++)
        {
          if(channel+first_channel_in_sector < num_chans)
          {
            sector_copy[channel][location] = radio_memory[channel+first_channel_in_sector][location];
          }
        }
      }
      
      //modify the selected channel
      strcpy(name, "SAVED CHANNEL   ");
      if(!string_entry(name)) return false;
      printf("Saving\n");
      for(uint8_t lw=0; lw<4; lw++)
      {
        sector_copy[channel_offset_in_sector][lw+6] = (name[lw*4+0] << 24 | name[lw*4+1] << 16 | name[lw*4+2] << 8 | name[lw*4+3]);
      }
      for(uint8_t i=0; i<settings_to_store; i++){
        sector_copy[channel_offset_in_sector][i] = settings[i];
      }

      //write sector to flash
      const uint32_t address = (uint32_t)&(radio_memory[first_channel_in_sector]);
      const uint32_t flash_address = address - XIP_BASE; 

      //!!! PICO is **very** fussy about flash erasing, there must be no code running in flash.  !!!
      //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
      apply_settings(true);                                //suspend rx to disable all DMA transfers
		  WAIT_100MS                                           //wait for suspension to take effect
      multicore_lockout_start_blocking();                  //halt the second core
      const uint32_t ints = save_and_disable_interrupts(); //disable all interrupts

      //safe to erase flash here
      //--------------------------------------------------------------------------------------------
      flash_range_erase(flash_address, FLASH_SECTOR_SIZE);
      flash_range_program(flash_address, (const uint8_t*)&sector_copy, FLASH_SECTOR_SIZE);
      //--------------------------------------------------------------------------------------------

      restore_interrupts (ints);                           //restore interrupts
      multicore_lockout_end_blocking();                    //restart the second core
      apply_settings(false);                               //resume rx operation
      //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
      //!!! Normal operation resumed

      return false;
    }

    //cancel
    if(get_button(PIN_BACK)){
      return false;
    }

    WAIT_100MS
  }
}

//load a channel from memory
bool ui::recall()
{

  //encoder loops through memories
  uint32_t min = 0;
  uint32_t max = num_chans-1;
  int32_t select=min;

  //remember where we were incase we need to cancel
  uint32_t stored_settings[settings_to_store];
  for(uint8_t i=0; i<settings_to_store; i++){
    stored_settings[i] = settings[i];
  }

  bool draw_once = true;
  while(1){
    if(encoder_control(&select, min, max)!=0 || draw_once)
    {

      if(radio_memory[select][9] != 0xffffffff)
      {
        //load name from memory
        char name[17];
        name[0] = radio_memory[select][6] >> 24;
        name[1] = radio_memory[select][6] >> 16;
        name[2] = radio_memory[select][6] >> 8;
        name[3] = radio_memory[select][6];
        name[4] = radio_memory[select][7] >> 24;
        name[5] = radio_memory[select][7] >> 16;
        name[6] = radio_memory[select][7] >> 8;
        name[7] = radio_memory[select][7];
        name[8] = radio_memory[select][8] >> 24;
        name[9] = radio_memory[select][8] >> 16;
        name[10] = radio_memory[select][8] >> 8;
        name[11] = radio_memory[select][8];
        name[12] = radio_memory[select][9] >> 24;
        name[13] = radio_memory[select][9] >> 16;
        name[14] = radio_memory[select][9] >> 8;
        name[15] = radio_memory[select][9];
        name[16] = 0;

        //(temporarily) apply lodaed settings to RX
        for(uint8_t i=0; i<settings_to_store; i++){
          settings[i] = radio_memory[select][i];
        }
        apply_settings(false);
        
        //print selected menu item
        draw_once = false;
        display_clear();
        display_print("Recall");
        display_line2();
        display_print_num("%03i ", select);
        display_print(name);
        display_show();
      }
      else
      {
        //print selected menu item
        draw_once = false;
        display_clear();
        display_print("Recall");
        display_line2();
        display_print_num("%03i ", select);
        display_print("BLANK");
        display_show();
      }
    }

    //select menu item
    if(get_button(PIN_MENU)){
      return 1;
    }

    //cancel
    if(get_button(PIN_BACK)){
      //put things back how they were to start with
      for(uint8_t i=0; i<settings_to_store; i++){
        settings[i] = stored_settings[i];
      }
      apply_settings(false);
      return 0;
    }

    WAIT_100MS
  }
}

////////////////////////////////////////////////////////////////////////////////
// String Entry (digit by digit)
////////////////////////////////////////////////////////////////////////////////
bool ui::string_entry(char string[]){

  int32_t position=0;
  int32_t i, digit_val;
  int32_t edit_mode = 0;
  unsigned frequency;

  bool draw_once = true;
  while(1){

    bool encoder_changed;
    if(edit_mode){
      //change the value of a digit 
      int32_t val = string[position];
      if     ('a' <= string[position] && string[position] <= 'z') val = string[position]-'a'+1;
      else if('A' <= string[position] && string[position] <= 'Z') val = string[position]-'A'+1;
      else if('0' <= string[position] && string[position] <= '9') val = string[position]-'0'+27;
      else if(string[position] == ' ') val = 0;
      encoder_changed = encoder_control(&val, 0, 36);
      if     (1 <= val  && val <= 26) string[position] = 'A' + val - 1;
      else if(27 <= val && val <= 36) string[position] = '0' + val - 27;
      else string[position] = ' ';
    } 
    else 
    {
      //change between chars
      encoder_changed = encoder_control(&position, 0, 17);
    }

    //if encoder changes, or screen hasn't been updated
    if(encoder_changed || draw_once)
    {
      draw_once = false;
      display_clear();

      //write frequency to lcd
      display_line1();
      display_print(string);
      display_print("YN");

      //print cursor
      display_line2();
      for(i=0; i<18; i++)
      {
        if(i==position)
        {
          if(edit_mode)
          {
            display_write('X');
          } 
          else 
          {
            display_write('^');
          }
        } 
        else 
        {
          display_write(' ');
        }
      }
      display_show();
    }

    //select menu item
    if(get_button(PIN_MENU))
    {
      draw_once = true;
	    edit_mode = !edit_mode;
	    if(position==16) //Yes
      {
		    return true;
	    }
	    if(position==17) return false; //No
	  }

    //cancel
    if(get_button(PIN_BACK))
    {
      return false;
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
// Frequency menu item (digit by digit)
////////////////////////////////////////////////////////////////////////////////
bool ui::frequency_entry(){

  int32_t digit=0;
  int32_t digits[8];
  int32_t i, digit_val;
  int32_t edit_mode = 0;
  unsigned frequency;

  //convert to binary representation
  frequency = settings[idx_frequency];
  digit_val = 10000000;
  for(i=0; i<8; i++){
      digits[i] = frequency / digit_val;
      frequency %= digit_val;
      digit_val /= 10;
  }

  bool draw_once = true;
  while(1){

    bool encoder_changed;
    if(edit_mode){
      //change the value of a digit 
      encoder_changed = encoder_control(&digits[digit], 0, 9);
    } 
    else 
    {
      //change between digits
      encoder_changed = encoder_control(&digit, 0, 9);
    }

    //if encoder changes, or screen hasn't been updated
    if(encoder_changed || draw_once)
    {
      draw_once = false;
      display_clear();

      //write frequency to lcd
      display_line1();
      for(i=0; i<8; i++)
      {
        display_write(digits[i] + '0');
        if(i==1||i==4) display_write('.');
      }
      display_print(" Y N");

      //print cursor
      display_line2();
      for(i=0; i<16; i++)
      {
        if(i==digit)
        {
          if(edit_mode)
          {
            display_write('X');
          } 
          else 
          {
            display_write('^');
          }
        } 
        else 
        {
          display_write(' ');
        }
        if(i==1||i==4||i==7|i==8) display_write(' ');
      }
      display_show();
    }

    //select menu item
    if(get_button(PIN_MENU))
    {
      draw_once = true;
	    edit_mode = !edit_mode;
	    if(digit==8) //Yes
      {
	      digit_val = 10000000;

        //convert back to a binary representation
        settings[idx_frequency] = 0;
        for(i=0; i<8; i++)
        {
	        settings[idx_frequency] += (digits[i] * digit_val);
		      digit_val /= 10;
		    }
		    return true;
	    }
	    if(digit==9) return 0; //No
	  }

    //cancel
    if(get_button(PIN_BACK))
    {
      return false;
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
// This is the main UI loop. Should get called about 10 times/second
////////////////////////////////////////////////////////////////////////////////
bool ui::do_ui(bool rx_settings_changed)
{

    //update frequency if encoder changes
    uint32_t encoder_change = get_encoder_change();
    switch(button_state)
    {
      case idle:
        if(check_button(PIN_MENU))
        {
          button_state = down;
          timeout = 100;
        }
        break;
      case down:
        if(encoder_change != 0 || (timeout-- == 0))
        {
          button_state = fast_mode;
        }
        else if(!check_button(PIN_MENU))
        {
          button_state = menu;
        }
        break;
      case fast_mode:
        if(!check_button(PIN_MENU))
        {
          button_state = idle;
        }
        break;
      case menu:
        button_state = idle;
        break;
    }

    if(encoder_change != 0)
    {
      rx_settings_changed = true;
      if(button_state == fast_mode && check_button(PIN_BACK))
      {
        //very fast if both buttons pressed
        settings[idx_frequency] += encoder_change * step_sizes[settings[idx_step]] * 100;
      }
      else if(button_state == fast_mode)
      {
        //fast if menu button held
        settings[idx_frequency] += encoder_change * step_sizes[settings[idx_step]] * 10;
      }
      else if(check_button(PIN_BACK))
      {
        //slow if cancel button held
        settings[idx_frequency] += encoder_change * (step_sizes[settings[idx_step]] / 10);
      }
      else settings[idx_frequency] += encoder_change * step_sizes[settings[idx_step]];

      if (settings[idx_frequency] > settings[idx_max_frequency])
          settings[idx_frequency] = settings[idx_min_frequency];

      if ((int)settings[idx_frequency] < (int)settings[idx_min_frequency])
          settings[idx_frequency] = settings[idx_max_frequency];
      
    }

    //if button is pressed enter menu
    else if(button_state == menu)
    {

      //top level menu
      uint32_t setting = 0;
      if(!enumerate_entry("menu:", "Frequency#Recall#Store#Volume#Mode#AGC Speed#Squelch#Frequency Step#CW Sidetone Frequency#Regulator Mode#USB Programming Mode#", 10, &setting)) return 1;

      switch(setting)
      {
        case 0 : 
          rx_settings_changed = frequency_entry();
          break;

        case 1:
          rx_settings_changed = recall();
          break;

        case 2:
          store();
          break;

        case 3 : 
          rx_settings_changed = number_entry("Volume", "%i", 0, 9, 1, &settings[idx_volume]);
          break;

        case 4 : 
          rx_settings_changed = enumerate_entry("Mode", "AM#LSB#USB#FM#CW", 4, &settings[idx_mode]);
          break;

        case 5 :
          rx_settings_changed = enumerate_entry("AGC Speed", "fast#normal#slow#very slow#", 3, &settings[idx_agc_speed]);
          break;

        case 6 :
          rx_settings_changed = enumerate_entry("Squelch", "S0#S1#S2#S3#S4#S5#S6#S7#S8#S9#S9+10dB#S9+20dB#S9+30dB", 13, &settings[idx_squelch]);
          break;

        case 7 : 
          rx_settings_changed = enumerate_entry("Frequency Step", "10Hz#50Hz#100Hz#1kHz#5kHz#10kHz#12.5kHz#25kHz#50kHz#100kHz#", 9, &settings[idx_step]);
          settings[idx_frequency] -= settings[idx_frequency]%step_sizes[settings[idx_step]];
          break;

        case 8 : 
          rx_settings_changed = number_entry("CW Sidetone Frequency", "%iHz", 1, 30, 100, &settings[idx_cw_sidetone]);
          break;

        case 9 : 
          uint32_t regmode;
          enumerate_entry("PSU Mode", "FM#PWM#", 2, &regmode);
          gpio_set_dir(23, GPIO_OUT);
          gpio_put(23, regmode);
          break;

        case 10 : 
          uint32_t programming_mode = 0;
          enumerate_entry("USB Programming Mode", "No#Yes#", 1, &programming_mode);
          if(programming_mode)
          {
            reset_usb_boot(0,0);
          }
          break;
      }
    }

    if(rx_settings_changed)
    {
      autosave();
    }

    receiver.access(rx_settings_changed);
    if(rx_settings_changed)
    {
      settings_to_apply.tuned_frequency_Hz = settings[idx_frequency];
      settings_to_apply.agc_speed = settings[idx_agc_speed];
      settings_to_apply.mode = settings[idx_mode];
      settings_to_apply.volume = settings[idx_volume];
      settings_to_apply.squelch = settings[idx_squelch];
      settings_to_apply.step_Hz = step_sizes[settings[idx_step]];
      settings_to_apply.cw_sidetone_Hz = settings[idx_cw_sidetone];
    }
    receiver.release();
    update_display(status, receiver);

    return rx_settings_changed;

}

ui::ui(rx_settings & settings_to_apply, rx_status & status, rx &receiver) : settings_to_apply(settings_to_apply), status(status), receiver(receiver)
{
  setup_display();
  setup_encoder();
  setup_buttons();

  button_state = idle;
}
