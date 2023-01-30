#include <string.h>

const uint8_t PIN_AB = 20;
const uint8_t PIN_B  = 21;
const uint8_t PIN_MENU = 22;
const uint8_t PIN_BACK = 17;
const uint8_t PIN_DISPLAY_SDA = 18;
const uint8_t PIN_DISPLAY_SCL = 19;

bool check_button(unsigned button);
bool get_button(unsigned button);

// settings that get stored in eeprom
#define idx_frequency 0
#define idx_mode 1
#define idx_agc_speed 2
#define idx_step 3
#define idx_squelch 4
#define idx_volume 5
#define idx_max_frequency 6
#define idx_min_frequency 7
#define idx_cw_sidetone 8
#define idx_cw_speed 9
#define idx_pps_count 10

// settings that are transient
#define idx_band 11
#define idx_test_signal 12
#define idx_USB_audio 13
#define idx_tx 14
#define idx_mute 15

uint32_t settings[16];

#define WAIT_10MS sleep_us(10000);
#define WAIT_100MS sleep_us(100000);
#define WAIT_500MS sleep_us(500000);;


////////////////////////////////////////////////////////////////////////////////
// Encoder 
////////////////////////////////////////////////////////////////////////////////

int new_position;
int old_position;
const uint sm = 0;
PIO pio = pio1;
void setup_encoder()
{
    uint offset = pio_add_program(pio, &quadrature_encoder_program);
    quadrature_encoder_program_init(pio, sm, offset, PIN_AB, 1000);
    new_position = (quadrature_encoder_get_count(pio, sm) + 2)/4;
    old_position = new_position;
}

int get_encoder_change()
{
    new_position = (quadrature_encoder_get_count(pio, sm) + 2)/4;
    int delta = new_position - old_position;
    old_position = new_position;
    return delta;
}

int encoder_control(int *value, int min, int max)
{
	int position_change = get_encoder_change();
	*value += position_change;
	if(*value > max) *value = min;
	if(*value < min) *value = max;
	return position_change;
}

////////////////////////////////////////////////////////////////////////////////
// Buttons 
////////////////////////////////////////////////////////////////////////////////

void setup_buttons()
{
  gpio_init(PIN_MENU);
  gpio_set_dir(PIN_MENU, GPIO_IN);
  gpio_pull_up(PIN_MENU);
  gpio_init(PIN_BACK);
  gpio_set_dir(PIN_BACK, GPIO_IN);
  gpio_pull_up(PIN_BACK);
}

bool get_button(uint8_t button){
	if(!gpio_get(button)){
		while(!gpio_get(button)){}
		WAIT_10MS
		return 1;
	}
	WAIT_10MS
	return 0;
}

bool check_button(unsigned button){
	return !gpio_get(button);
}

////////////////////////////////////////////////////////////////////////////////
// Display
////////////////////////////////////////////////////////////////////////////////
ssd1306_t disp;
void setup_display() {
  i2c_init(i2c1, 400000);
  gpio_set_function(PIN_DISPLAY_SDA, GPIO_FUNC_I2C);
  gpio_set_function(PIN_DISPLAY_SCL, GPIO_FUNC_I2C);
  gpio_pull_up(PIN_DISPLAY_SDA);
  gpio_pull_up(PIN_DISPLAY_SCL);
  disp.external_vcc=false;
  ssd1306_init(&disp, 128, 64, 0x3C, i2c1);
}

uint8_t cursor_x = 0;
uint8_t cursor_y = 0;
void display_clear()
{
  cursor_x = 0;
  cursor_y = 0;
  ssd1306_clear(&disp);
}
void display_line1()
{
  cursor_y = 0;
  cursor_x = 0;
}
void display_line2()
{
  cursor_y = 1;
  cursor_x = 0;
}
void display_write(char x)
{
  ssd1306_draw_char(&disp, cursor_x*6, cursor_y*8, 1, x);
  cursor_x++;
}
void display_print(const char str[])
{
  ssd1306_draw_string(&disp, cursor_x*6, cursor_y*8, 1, str);
  cursor_x+=strlen(str);
}
void display_print_num(const char format[], int16_t num)
{
  char buff[16];
  snprintf(buff, 16, format, num);
  ssd1306_draw_string(&disp, cursor_x*6, cursor_y*8, 1, buff);
  cursor_x+=strlen(buff);
}
void display_show()
{
  ssd1306_show(&disp);
}

////////////////////////////////////////////////////////////////////////////////
// Generic Menu Options
////////////////////////////////////////////////////////////////////////////////

void print_option(const char options[], int option){
    char x;
    int i, idx=0;

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
uint32_t get_enum(const char title[], const char options[], uint32_t max, uint32_t *value)
{
  int select=*value;

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
int16_t get_num(const char title[], const char format[], int16_t min, int16_t max, int16_t multiple, uint32_t *value)
{
  int select=*value;

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

////////////////////////////////////////////////////////////////////////////////
// Frequency menu item (digit by digit)
////////////////////////////////////////////////////////////////////////////////
bool get_frequency(){

  int digit=0;
  int digits[8];
  int i, digit_val;
  int edit_mode = 0;
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
// This is the main UI loop. Should get called about 100 times/second
////////////////////////////////////////////////////////////////////////////////

const uint32_t step_sizes[10] = {10, 50, 100, 1000, 5000, 10000, 12500, 25000, 50000, 100000};
#define SMETER "s0#s1#s2##s4#s5#s6#s7#s8#s9#s9+10dB#s9+20dB#s9+30dB#"

bool do_ui()
{

    bool rx_settings_changed = false;

    //update frequency if encoder changes
    uint32_t encoder_change = get_encoder_change();
    if(encoder_change != 0)
    {
      rx_settings_changed = true;
      settings[idx_frequency] += encoder_change * step_sizes[settings[idx_step]];
    }

    //if button is pressed enter menu
    else if(check_button(PIN_MENU))
    {
      get_button(PIN_MENU);

      //top level menu
      uint32_t setting = 0;
      if(!get_enum("menu:", "Frequency#Volume#Mode#AGC Speed#Squelch#Frequency Step#CW Sidetone Frequency#USB Programming Mode#", 7, &setting)) return 1;

      switch(setting)
      {
        case 0 : 
          rx_settings_changed = get_frequency();
          break;

        case 1 : 
          rx_settings_changed = get_num("Volume", "%i", 0, 9, 1, &settings[idx_volume]);
          break;

        case 2 : 
          rx_settings_changed = get_enum("Mode", "AM#LSB#USB#FM#CW", 4, &settings[idx_mode]);
          break;

        case 3 :
          rx_settings_changed = get_enum("AGC Speed", "fast#normal#slow#very slow#", 3, &settings[idx_agc_speed]);
          break;

        case 4 :
          rx_settings_changed = get_enum("Squelch", SMETER, 12, &settings[idx_squelch]);
          break;

        case 5 : 
          rx_settings_changed = get_enum("Frequency Step", "10Hz#50Hz#100Hz#1kHz#5kHz#10kHz#12.5kHz#25kHz#50kHz#100kHz#", 9, &settings[idx_step]);
          settings[idx_frequency] -= settings[idx_frequency]%step_sizes[settings[idx_step]];
          break;

        case 6 : 
          rx_settings_changed = get_num("CW Sidetone Frequency", "%iHz", 1, 30, 100, &settings[idx_cw_sidetone]);
          break;

        case 7 : 
          uint32_t programming_mode = 0;
          get_enum("USB Programming Mode", "No#Yes#", 1, &programming_mode);
          if(programming_mode)
          {
            reset_usb_boot(0,0);
          }
          break;
      }
    }

    return rx_settings_changed;

}

