#include <string.h>
#include <float.h>
#include <math.h>

#include "pico/multicore.h"
#include "ui.h"
#include "fft_filter.h"
#include <hardware/flash.h>
#include "pico/util/queue.h"

#define WATERFALL_WIDTH (128)
#define WATERFALL_MAX_VALUE (64)

////////////////////////////////////////////////////////////////////////////////
// Encoder 
////////////////////////////////////////////////////////////////////////////////
void ui::setup_encoder()
{
    gpio_set_function(PIN_AB, GPIO_FUNC_PIO1);
    gpio_set_function(PIN_AB+1, GPIO_FUNC_PIO1);
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
    if((settings[idx_hw_setup] >> flag_reverse_encoder) & 1)
    {
      return -delta;
    } else {
      return delta;
    }
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

void ui::display_clear(bool colour)
{
  cursor_x = 0;
  cursor_y = 0;
  ssd1306_clear(&disp, colour);
}

void ui::display_clear_str(uint32_t scale, bool colour)
{
  ssd1306_fill_rectangle(&disp, 0, cursor_y, 128, 9*scale, colour);
}

void ui::display_linen(uint8_t line)
{
  cursor_y = 9*(line-1);
  cursor_x = 0;
}

void ui::display_set_xy(int16_t x, int16_t y)
{
  cursor_x = x;
  cursor_y = y;
}

void ui::display_add_xy(int16_t x, int16_t y)
{
  cursor_x += x;
  cursor_y += y;
}

uint16_t ui::display_get_x() { return cursor_x; }
uint16_t ui::display_get_y() { return cursor_y; }

void ui::display_draw_separator(uint16_t y, uint32_t scale, bool colour){
  // always draw top line
  ssd1306_draw_line(&disp, 0, y, 127, y, colour);
  // for 2px, just draw another below
  if (scale == 2) {
    ssd1306_draw_line(&disp, 0, y+1, 127, y+1, colour);
  }
  // for 3px draw top and bottom, middle blank
  if (scale == 3) {
    ssd1306_draw_line(&disp, 0, y+2, 127, y+2, colour);
  }
}

void ui::display_print_char(char x, uint32_t scale, uint32_t style)
{
  if ( !(style&style_nowrap) && (cursor_x > 128 - 6*(signed)scale)) {
    cursor_x = 0;
    cursor_y += 9*scale;
  }
  int colour = 1;
  if (style & style_reverse) colour=0;
  if (style & style_xor) colour=2;
  if (scale & 0x01) { // odd numbers use 8x6 chars
    ssd1306_draw_char_with_font(&disp, cursor_x, cursor_y, scale, font_8x5, x, colour);
  } else { // even, use 16x12
    ssd1306_draw_char_with_font(&disp, cursor_x, cursor_y, scale/2, font_16x12, x, colour);
  }
  cursor_x += (6*scale);
}

/* return index of 1st match. -1 if not found */
int ui::strchr_idx(const char str[], uint8_t c) {
  for (unsigned int i=0; i<strlen(str);i++){
    if (str[i] == c) return i;
  }
  return -1;
}

void ui::display_print_str(const char str[], uint32_t scale, uint32_t style)
{
  int16_t box_x1 = INT16_MAX;
  int16_t box_y1 = INT16_MAX;
  int16_t box_x2 = INT16_MIN;
  int16_t box_y2 = INT16_MIN;

  bool colour = !(style&style_reverse);
  int next_ln;
  unsigned int length;

  // find the index of the next \n
  next_ln = strchr_idx( str, '\n');
  // if found, compute length of string, if not, length to end of str
  length = (next_ln<0) ? strlen(str) : (unsigned)next_ln;

  if (style & style_centered) {
    cursor_x = (128- 6*scale*length)/2;
  }
  if (style & style_right) {
    cursor_x = (128 - 6*scale*length);
  }

  for (size_t i=0; i<strlen(str); i++) {
    if (str[i] == '\a') {
      colour = !colour;
      continue;
    }
    if (str[i] == '\n') {
      next_ln = strchr_idx( &str[i+1], '\n');
      length = (next_ln<0) ? strlen(str)-(i+1) : (unsigned)next_ln-(i+1);

      if (style & style_centered) {
        cursor_x = (128- 6*scale*length)/2;
      } else if (style & style_right) {
        cursor_x = (128- 6*scale*length);
      } else {
        cursor_x = 0;
      }
      cursor_y += 9*scale;
      continue;
    }
    if ( !(style&style_nowrap) && (cursor_x > 128 - 6*(signed)scale)) {
      cursor_x = 0;
      cursor_y += 9*scale;
    }
    if (scale & 0x01) { // odd numbers use 8x6 chars
      ssd1306_draw_char_with_font(&disp, cursor_x, cursor_y, scale, font_8x5, str[i], colour);
    } else { // even, use 16x12
      ssd1306_draw_char_with_font(&disp, cursor_x, cursor_y, scale/2, font_16x12, str[i], colour);
    }
    if (style&style_bordered) {
      if (cursor_x < box_x1) box_x1=cursor_x;
      if (cursor_y < box_y1) box_y1=cursor_y;
      if ((signed)(cursor_x + 5*scale) > box_x2) box_x2 = (cursor_x + 5*scale);
      if ((signed)(cursor_y + 8*scale) > box_y2) box_y2 = (cursor_y + 8*scale);
    }
    cursor_x += 6*scale;
  }
  if (style&style_bordered) {
    // text, black, white, black
    ssd1306_draw_rectangle(&disp, box_x1-1, box_y1-1, box_x2-box_x1+1, box_y2-box_y1+1, 1-colour);
    ssd1306_draw_rectangle(&disp, box_x1-2, box_y1-2, box_x2-box_x1+3, box_y2-box_y1+3, colour);
    ssd1306_draw_rectangle(&disp, box_x1-3, box_y1-3, box_x2-box_x1+5, box_y2-box_y1+5, 1-colour);
  }
}

void ui::display_print_num(const char format[], int16_t num, uint32_t scale, uint32_t style)
{
  char buff[16];
  snprintf(buff, 16, format, num);
  display_print_str(buff, scale, style);
}

void ui::display_print_freq(char separator, uint32_t frequency, uint32_t scale, uint32_t style)
{
  char buff[16];
  const int32_t MHz = frequency / 1000000;
  frequency %= 1000000;
  const int32_t kHz = frequency / 1000;
  frequency %= 1000;
  const int32_t Hz = frequency;
  snprintf(buff, 16, "%2ld%c%03ld%c%03ld", MHz, separator, kHz, separator, Hz);
  display_print_str(buff, scale, style);
}

void ui::display_show()
{
  ssd1306_show(&disp);
}

////////////////////////////////////////////////////////////////////////////////
// Home page status display (original)
////////////////////////////////////////////////////////////////////////////////
void ui::renderpage_original(rx_status & status, rx & receiver)
{
  receiver.access(false);
  const float power_dBm = status.signal_strength_dBm;
  const float battery_voltage = 3.0f * 3.3f * (status.battery/65535.0f);
  const float temp_voltage = 3.3f * (status.temp/65535.0f);
  const float temp = 27.0f - (temp_voltage - 0.706f)/0.001721f;
  const float block_time = (float)adc_block_size/(float)adc_sample_rate;
  const float busy_time = ((float)status.busy_time*1e-6f);
  receiver.release();

  const uint8_t buffer_size = 21;
  char buff [buffer_size];
  display_clear();

  //frequency
  uint32_t remainder, MHz, kHz, Hz;
  MHz = (uint32_t)settings[idx_frequency]/1000000u;
  remainder = (uint32_t)settings[idx_frequency]%1000000u; 
  kHz = remainder/1000u;
  remainder = remainder%1000u; 
  Hz = remainder;
  display_set_xy(0,0);
  snprintf(buff, buffer_size, "%2lu.%03lu", MHz, kHz);
  display_print_str(buff,2);
  snprintf(buff, buffer_size, ".%03lu", Hz);
  display_print_str(buff,1);

  //mode
  display_print_str(modes[settings[idx_mode]],1, style_right);

  //step
  display_set_xy(0,8);
  display_print_str(steps[settings[idx_step]],1, style_right);

  //signal strength/cpu
  int8_t power_s = dBm_to_S(power_dBm);

  display_set_xy(0,24);
  display_print_str(smeter[power_s],1);
  display_print_num("% 4ddBm", (int)power_dBm, 1, style_right);

  snprintf(buff, buffer_size, "%2.1fV %2.0f%cC %3.0f%%", battery_voltage, temp, '\x7f', (100.0f*busy_time)/block_time);
  display_set_xy(0,16);
  display_print_str(buff, 1, style_right);

  draw_spectrum(32);
  display_show();
}

////////////////////////////////////////////////////////////////////////////////
// Home page status display with bigger spectrum view
////////////////////////////////////////////////////////////////////////////////
void ui::renderpage_bigspectrum(rx_status & status, rx & receiver)
{
  display_clear();
  draw_slim_status(0, status, receiver);
  draw_spectrum(8);
  display_show();
}

////////////////////////////////////////////////////////////////////////////////
// Home page status display with big waterfall
////////////////////////////////////////////////////////////////////////////////
void ui::renderpage_waterfall(bool view_changed, rx_status & status, rx & receiver)
{
  if (view_changed) display_clear();
  ssd1306_fill_rectangle(&disp, 0, 0, 128, 8, 0);
  draw_waterfall(8);
  draw_slim_status(0, status, receiver);
  display_show();
}

////////////////////////////////////////////////////////////////////////////////
// Home page status display with big simple text
////////////////////////////////////////////////////////////////////////////////
void ui::renderpage_bigtext(rx_status & status, rx & receiver)
{

  receiver.access(false);
  const float power_dBm = status.signal_strength_dBm;
  receiver.release();

  display_clear();
  display_set_xy(0,0);
  display_print_freq('.', settings[idx_frequency],2,style_centered);

  display_set_xy(0,24);
  //mode and step size
  display_print_str(modes[settings[idx_mode]],2);
  display_print_str(steps[settings[idx_step]], 2, style_right);

  //signal strength
  display_set_xy(0,48);
  int8_t power_s = dBm_to_S(power_dBm);

  display_print_str(smeter[power_s],2);

  display_show();
}

void ui::renderpage_fun(rx_status & status, rx & receiver)
{
  static int degrees = 0;
  static int xm, ym;

  if (degrees == 0) {
    xm = rand()%10+1;
    ym = rand()%10;
  }
  display_clear();
  ssd1306_bmp_show_image(&disp, crystal, sizeof(crystal));
  ssd1306_scroll_screen(&disp, 40*cos(xm*M_PI*degrees/180), 20*sin(ym*M_PI*degrees/180));
  display_show();
  if ((degrees+=3) >=360) degrees = 0;
}

// Draw a slim 8 pixel status line
void ui::draw_slim_status(uint16_t y, rx_status & status, rx & receiver)
{
  receiver.access(false);
  const float power_dBm = status.signal_strength_dBm;
  receiver.release();

  display_set_xy(0,y);
  display_print_freq(',', settings[idx_frequency],1);
  display_add_xy(4,0);

  //mode
  display_print_str(modes[settings[idx_mode]],1);

  //signal strength dBm
  display_print_num("% 4ddBm", (int)power_dBm, 1, style_right);
}

int ui::dBm_to_S(float power_dBm) {
  int power_s = floorf((power_dBm-S0)/6.0f);
  if(power_dBm >= S9) power_s = floorf((power_dBm-S9)/10.0f)+9;
  if(power_s < 0) power_s = 0;
  if(power_s > 12) power_s = 12;
  return (power_s);
}

void ui::draw_h_tick_marks(uint16_t startY)
{
  // tick marks at startY
  ssd1306_draw_line(&disp, 0, startY + 2, 127, startY + 2, 1);

  ssd1306_draw_line(&disp, 0, startY, 0, startY + 4, 1);
  ssd1306_draw_line(&disp, 64, startY, 64, startY + 4, 1);
  ssd1306_draw_line(&disp, 127, startY, 127, startY + 4, 1);

  ssd1306_draw_line(&disp, 32, startY + 1, 32, startY + 3, 1);
  ssd1306_draw_line(&disp, 96, startY + 1, 96, startY + 3, 1);
}

////////////////////////////////////////////////////////////////////////////////
// draw a classic analog meter movement.
////////////////////////////////////////////////////////////////////////////////
// height positive : a circle sector from the top down up to and including the full circle
// height positive : draw a linear movement meter like all the cheap CBs in the 80s
void ui::draw_analogmeter(    uint16_t startx, uint16_t starty, 
                              uint16_t width, int16_t height,
                              float  needle_pct, int numticks,
                              const char* legend, const char labels[][5]
                              ) {

  #define TICK_LEN 3

  // I hope you like high school trig and geometry...
  int segment_h = height;  // pixels high
  int segment_w2 = width/2;  // pixels wide
  // compute the radius
  float radius;
  float halfdeg_range;
  float deg_min, deg_max, deg_range;

  // pointless and crashed with DIV0
  if (height == 0) return;

  if (height > 0) { // positive height, angular meter
    if (height <= segment_w2) {
      radius = (pow(segment_w2, 2) / segment_h + segment_h) / 2;
      halfdeg_range = asinf(segment_w2/radius)*180.0 / M_PI;
      deg_min = (90-halfdeg_range);
      deg_max = (90+halfdeg_range);
      deg_range = (deg_max-deg_min);
    } else {  // (height > segment_w2)
      radius = segment_w2;
      halfdeg_range = acosf((segment_h-radius)/radius)*180.0 / M_PI;
      deg_min = (-90 +halfdeg_range);
      deg_max = (270 -halfdeg_range);
      deg_range = (deg_max-deg_min);
    }

    // draw arc
    for (int degrees=deg_min; degrees<=deg_max; degrees++) {
        ssd1306_draw_pixel(&disp, 
            (startx+width/2) + radius*cos(M_PI*degrees/180),
            (starty + radius) - radius*sin(M_PI*degrees/180),
            1);
        ssd1306_draw_pixel(&disp, 
            (startx+width/2) + (1+radius)*cos(M_PI*degrees/180),
            (starty + radius) - (1+radius)*sin(M_PI*degrees/180),
            1);
    }

    // tick marks
    if (numticks) {
      int i=0;
      for (float degrees=deg_max; degrees>=deg_min; degrees-=(float)(deg_range/(numticks-1))) {
        for (int8_t l = -TICK_LEN; l <= +TICK_LEN; l++){
          ssd1306_draw_pixel(&disp,
              (startx+width/2) + (radius+l)*cos(M_PI*degrees/180),
              (starty + radius) - (radius+l)*sin(M_PI*degrees/180),
              1);
        }
        // tick labels
        if ( (labels) &&  (strlen(labels[i])) ) {
          display_set_xy(
              (startx+width/2) + (radius+6)*cos(M_PI*degrees/180) - 2*strlen(labels[i]),
              (starty + radius) - (radius+6)*sin(M_PI*degrees/180) - 8
              );
          display_print_str(labels[i]);
        }
        i++;
      }
    }

    // draw legend
    if (strlen(legend)) {
      if (height == width) { // a circle
        display_set_xy(startx + width/2 - 12*strlen(legend)/2, starty+segment_h/2-8);
      } else {
        display_set_xy(startx + width/2 - 12*strlen(legend)/2, starty+segment_h-8);
      }
      display_print_str(legend, 2);
    }

    // draw the needle
    float degrees = needle_pct * deg_range/100.0;
    degrees = deg_max - degrees;
    if (degrees < deg_min) degrees = deg_min;
    if (degrees > deg_max) degrees = deg_max;
    // can skip invisible part of needle => radius-50
    // draw_line is crap at angled lines so plot pixels
    int startr=0;
    if (starty+radius > 64){
      startr = starty+radius-64; // 64 is display height
    }
    for (int r=startr; r<radius; r++) {
      ssd1306_draw_pixel(&disp, 
          (startx+width/2) + r*cos(M_PI*degrees/180),
          (starty + radius) - r*sin(M_PI*degrees/180),
          1);
    }
  } 
  else 
  {   // draw a CB style rectangular needle movement
    height *= -1;
    // draw straight arc
    ssd1306_draw_line(&disp, startx, starty+height/2-1, startx+width, starty+height/2-1, 1);
    ssd1306_draw_line(&disp, startx, starty+height/2, startx+width, starty+height/2, 1);

    // tick marks
    if (numticks) {
      for (int i=0; i < numticks; i++) {
        int x = startx + i*width/(numticks-1);
        ssd1306_draw_line(&disp, x, starty+(height/2)-TICK_LEN-1, x, starty+(height/2)+TICK_LEN, 1);
        // tick labels
        if ( (labels) && strlen(labels[i]) ) {
          display_set_xy( x - (3*strlen(labels[i])-1), starty+(height/2)-TICK_LEN-10);
          display_print_str(labels[i]);
        }
      }
    }

    // draw the needle
    int x = startx + width*needle_pct/100;
    ssd1306_draw_line(&disp, x, starty, x, starty+height, 1);

    // draw legend
    if (strlen(legend)) {
      display_set_xy(startx + width/2 - 6*strlen(legend)/2, starty+(height/2)+TICK_LEN+3);
      display_print_str(legend, 1);
    }  
  }
}

////////////////////////////////////////////////////////////////////////////////
// Home page status display - S meter
////////////////////////////////////////////////////////////////////////////////
void ui::renderpage_smeter(bool view_changed, rx_status & status, rx & receiver)
{

  #define NUM_DBM 3
  static int dBm_ptr = 0;
  static float dBm_avg[NUM_DBM] = {-FLT_MAX, -FLT_MAX, -FLT_MAX};

  receiver.access(false);
  const float power_dBm = status.signal_strength_dBm;
  receiver.release();

  dBm_avg[dBm_ptr++] = power_dBm;
  if (dBm_ptr >= NUM_DBM) dBm_ptr = 0;

  float avg_power_dBm = 0.0;
  for (uint8_t i=0; i<NUM_DBM; i++) {
    avg_power_dBm += dBm_avg[i];
  } 
  avg_power_dBm /= NUM_DBM;

  display_clear();

  draw_slim_status(0, status, receiver);
  // -127dBm is needle to left
  // 100 percent needle swing
  // 84 dB of swing range
  uint16_t percent = (avg_power_dBm+127) * 100/84;

  const char labels[13][5] = {
      "",    "1",    "",    "3",
      "",    "5",    "",    "7",
      "",    "9",    "",    "+12",
      ""
  };

  // angular meter movement
  draw_analogmeter( 9, 33, 110, 15, percent, 13, "S", labels );

  ssd1306_draw_rectangle(&disp, 0,9,127,54,1);
  display_show();
}

////////////////////////////////////////////////////////////////////////////////
// Paints the spectrum from startY to bottom of screen
////////////////////////////////////////////////////////////////////////////////
void ui::draw_spectrum(uint16_t startY)
{
  //Display spectrum capture
  draw_h_tick_marks(startY);

  //plot
  const uint8_t max_height = (64-startY-3);
  const uint8_t scale = 256/max_height;
  for(uint16_t x=0; x<128; x++)
  {
    int16_t y = spectrum[x*2]/scale;
    ssd1306_draw_line(&disp, x, 63-y, x, 63, 1);
  }


  for (int16_t y = 0; y < max_height; y += ((uint16_t)4*dB10/scale))
  {
    for (uint8_t x = 0; x < 128; x += 4)
    {
      ssd1306_draw_line(&disp, x, 63 - y, x + 1, 63 - y, 2);
    }
  }
}

void ui::draw_waterfall(uint16_t starty)
{
  static int8_t tmp_line[WATERFALL_WIDTH];
  static int8_t curr_line[WATERFALL_WIDTH];

  // Move waterfall down to  make room for the new line
  ssd1306_scroll_screen(&disp, 0, 1);

  int16_t err = 0;

  for(uint16_t x=0; x<WATERFALL_WIDTH; x++)
  {
      int16_t y = spectrum[2*x]>>2;//scale from 8 to 6 bits
      curr_line[x] = y + tmp_line[x];
      tmp_line[x] = 0;
  }

  for(uint16_t x=0; x<WATERFALL_WIDTH; x++)
  {
      // Simple Floyd-Steinberg dithering
      if(curr_line[x] > 32)
      {
        ssd1306_draw_pixel(&disp, x, starty + 3, 1);
        err = curr_line[x] - 64;
      } else {
        ssd1306_draw_pixel(&disp, x, starty + 3, 0);
        err = curr_line[x] - 0;
      }

      if(x < (WATERFALL_WIDTH - 1))
      {
        curr_line[x + 1] += 7 * err / 16;
        tmp_line[x + 1] += err / 16;
      }
      tmp_line[x] += 5 * err / 16;
      if(x > 0)
      {
        tmp_line[x - 1] += 3 * err / 16;
      }
  }

  draw_h_tick_marks(starty);

}

////////////////////////////////////////////////////////////////////////////////
// Generic Menu Options
////////////////////////////////////////////////////////////////////////////////

void ui::print_enum_option(const char options[], uint8_t option){
#define MAX_OPTS 32
  char *splits[MAX_OPTS];
  int num_splits;
  char *new_options;

  new_options = (char*)malloc(strlen(options)+1);
  strcpy (new_options, options);

  splits[0] = strtok(new_options, "#");
  for ( num_splits = 1; num_splits < MAX_OPTS; num_splits++) {
          splits[num_splits] = strtok(NULL, "#");
          if (!splits[num_splits]) break;
  }

  if ( (num_splits==2) && (strlen(splits[0])+strlen(splits[1]))*12 < 128) {
    display_print_str(splits[0],2, (option==0) ? style_reverse : 0);
    display_print_str(" ");
    display_print_str(splits[1],2, style_right|((option==1) ? style_reverse : 0));
  } else {
    // default to 1st option if invalid
    if (option > num_splits) option = 0;
    display_print_str(splits[option], 2, style_centered);
  }
}

bool ui::bit_entry(const char title[], const char options[], uint8_t bit_position, uint32_t *value, bool &ok)
{
    uint32_t bit = (*value >> bit_position) & 1;
    bool done = enumerate_entry(title, options, &bit, ok);
    if(bit)
    {
     *value |= (1 << bit_position);
    } else {
     *value &= ~(1 << bit_position);
    }
    return done;
}

//choose from an enumerate list of settings
bool ui::menu_entry(const char title[], const char options[], uint32_t *value, bool &ok)
{
  enum e_state{idle, active};
  static e_state state = idle;
  static int32_t select = 0;
  bool draw_display = false;

  if(state == idle)
  {
    draw_display = true;
    select = *value;
    state = active;
  }
  else if(state == active)
  {
    uint32_t max = 0;
    for (size_t i=0; i<strlen(options); i++) {
      if (options[i] == '#') max++;
    }
    // workaround for accidental last # omissions
    if (options[strlen(options)-1] != '#') max++;
    if (max > 0) max--;

    draw_display = encoder_control(&select, 0, max)!=0;

    //select menu item
    if(menu_button.is_pressed() || encoder_button.is_pressed()){
      *value = select;
      ok = true;
      state = idle;
      return true;
    }

    //cancel
    if(back_button.is_pressed()){
      ok = false;
      state = idle;
      return true;
    }
  }

  if(draw_display)
  {
      display_clear();
      display_print_str(title, 2, style_centered);
      display_draw_separator(18,3);
      display_linen(4);
      print_enum_option(options, select);
      display_show();
  }

  return false;
}

//choose from an enumerate list of settings
bool ui::enumerate_entry(const char title[], const char options[], uint32_t *value, bool &ok)
{
  enum e_state{idle, active};
  static e_state state = idle;
  static int32_t select = 0;
  bool draw_display = false;

  if(state == idle)
  {
    draw_display = true;
    select = *value;
    state = active;
  }
  else if(state == active)
  {
    uint32_t max = 0;
    for (size_t i=0; i<strlen(options); i++) {
      if (options[i] == '#') max++;
    }
    // workaround for accidental last # oissions
    if (options[strlen(options)-1] != '#') max++;
    if (max > 0) max--;

    draw_display = encoder_control(&select, 0, max)!=0;

    //select menu item
    if(menu_button.is_pressed() || encoder_button.is_pressed()){
      *value = select;
      ok = true;
      state = idle;
      return true;
    }

    //cancel
    if(back_button.is_pressed()){
      ok = false;
      state = idle;
      return true;
    }
  }

  if(draw_display)
  {
      display_clear();
      display_print_str(title, 2, style_centered);
      display_draw_separator(40,1);
      display_linen(6);
      print_enum_option(options, select);
      display_show();
  }

  return false;
}

//select a number in a range
bool ui::number_entry(const char title[], const char format[], int16_t min, int16_t max, int16_t multiple, uint32_t *value, bool &ok)
{
  enum e_state{idle, active};
  static e_state state = idle;
  static int32_t select = 0;
  bool draw_display = false;

  if(state == idle)
  {
    draw_display = true;
    select=*value/multiple;
    state = active;
  }
  else if(state == active)
  {
    draw_display = encoder_control(&select, min, max)!=0;

    //select menu item
    if(menu_button.is_pressed() || encoder_button.is_pressed()){
      *value = select*multiple;
      ok = true;
      state = idle;
      return true;
    }

    //cancel
    if(back_button.is_pressed()){
      ok = false;
      state = idle;
      return true;
    }
  }

  if(draw_display)
  {
      display_clear();
      display_print_str(title, 2, style_centered);
      display_draw_separator(40,1);
      display_linen(6);
      display_print_num(format, select*multiple, 2, style_centered);
      display_show();
  }

  return false;
}

//Apply settings
void ui::apply_settings(bool suspend)
{
  receiver.access(true);
  settings_to_apply.tuned_frequency_Hz = settings[idx_frequency];
  settings_to_apply.agc_speed = settings[idx_agc_speed];
  settings_to_apply.enable_auto_notch = settings[idx_rx_features] >> flag_enable_auto_notch & 1;
  settings_to_apply.mode = settings[idx_mode];
  settings_to_apply.volume = settings[idx_volume];
  settings_to_apply.squelch = settings[idx_squelch];
  settings_to_apply.step_Hz = step_sizes[settings[idx_step]];
  settings_to_apply.cw_sidetone_Hz = settings[idx_cw_sidetone];
  settings_to_apply.gain_cal = settings[idx_gain_cal];
  settings_to_apply.suspend = suspend;
  settings_to_apply.swap_iq = (settings[idx_hw_setup] >> flag_swap_iq) & 1;
  settings_to_apply.bandwidth = settings[idx_bandwidth];
  settings_to_apply.deemphasis = (settings[idx_rx_features] & mask_deemphasis) >> flag_deemphasis;
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
      if(autosave_memory[empty_channel - 1][i] != settings[i])
      {
        difference_found = true;
        break;
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

  uint16_t last_channel_written = 255;
  if(empty_channel > 0) last_channel_written = empty_channel - 1;
  if(!empty_channel_found) last_channel_written = 255;
  for(uint8_t i=0; i<16; i++){
    settings[i] = autosave_memory[last_channel_written][i];
  }

  apply_settings(false);
  uint8_t display_timeout_setting = (settings[idx_hw_setup] & mask_display_timeout) >> flag_display_timeout;
  display_timeout_max = timeout_lookup[display_timeout_setting];
  display_time = time_us_32();
  ssd1306_flip(&disp, (settings[idx_hw_setup] >> flag_flip_oled) & 1);
  ssd1306_type(&disp, (settings[idx_hw_setup] >> flag_oled_type) & 1);
  ssd1306_contrast(&disp, 17 * ((settings[idx_hw_setup] & mask_display_contrast) >> flag_display_contrast));
  waterfall_inst.configure_display((settings[idx_hw_setup] & mask_tft_settings) >> flag_tft_settings);

}

//Upload memories via USB interface
bool ui::upload_memory()
{
      display_clear();
      display_print_str("Ready for\nmemories",2, style_centered);
      display_show();

      uint8_t progress_ctr=0;

      //work out which flash sector the channel sits in.
      const uint32_t num_channels_per_sector = FLASH_SECTOR_SIZE/(sizeof(int)*chan_size);

      //copy sector to RAM
      bool done = false;
      const uint32_t num_sectors = num_chans/num_channels_per_sector;
      for(uint8_t sector = 0; sector < num_sectors; sector++)
      {

        const uint32_t first_channel_in_sector = num_channels_per_sector * sector;
        static uint32_t sector_copy[num_channels_per_sector][chan_size];
        for(uint16_t channel=0; channel<num_channels_per_sector; channel++)
        {
          for(uint16_t location=0; location<chan_size; location++)
          {
            if(!done)
            {
              printf("sector %u channel %u location %u>\n", sector, channel, location);
              char line [256];
              uint32_t data;
              fgets(line, 256, stdin);
              if(line[0] == 'q' || line[0] == 'Q')
              {
                sector_copy[channel][location] = 0xffffffffu;
                done = true;
              }
              if (sscanf(line, " %lx", &data))
              {
                sector_copy[channel][location] = data;
              }
            }
            else
            {
              sector_copy[channel][location] = 0xffffffffu;
            }
          }
        }
        
        // show some progress
        ssd1306_invert( &disp, 0x1 & (++progress_ctr));

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

      }

      return false;
}

//save current settings to memory
bool ui::memory_store(bool &ok)
{

  //encoder loops through memories
  const int32_t min = 0;
  const int32_t max = num_chans-1;
  static int32_t select = 0;
  static char name[17];

  enum e_frequency_state{select_channel, enter_name, delete_channel, save_channel};
  static e_frequency_state state = select_channel;

  if(state == select_channel)
  {
      encoder_control(&select, min, max);
      get_memory_name(name, select, true);

      display_clear();
      display_print_str("Store");
      display_print_num(" %03i ", select, 1, style_centered);
      display_print_str("\n", 1);
      if (12*strlen(name) > 128) {
        display_add_xy(0,4);
        display_print_str(name,1,style_nowrap|style_centered);
      } else {
        display_print_str(name,2,style_nowrap|style_centered);
      }
      display_show();

      if(encoder_button.is_pressed()||menu_button.is_pressed()){
        get_memory_name(name, select, false);
        state = enter_name;
      }

      //cancel
      if(back_button.is_pressed()){
        ok=false;
        state = select_channel;
        return true;
      }

  } 
  else if(state == enter_name)
  {
      //modify the selected channel name
      bool del = false;
      if(string_entry(name, ok, del))
      {
        if(ok)
        {
          if(del) state = delete_channel;
          else state = save_channel;
        }
        else
        {
          state = select_channel;
          return true;
        }
      }
  }
  else if(state == delete_channel)
  {
      name[12] = 0xffu; name[13] = 0xffu;
      name[14] = 0xffu; name[15] = 0xffu;
      state = save_channel;
  }
  else if(state == save_channel)
  {
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
    
      // pack string into uint32 array
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

      //ssd1306_invert( &disp, 0);

      ok = true;
      state = select_channel;
      return true;

  }
  return false;
}

//load a channel from memory
bool ui::memory_recall(bool &ok)
{

  //encoder loops through memories
  const int32_t min = 0;
  const int32_t max = num_chans-1;
  static int32_t select = 0;
  static uint32_t stored_settings[settings_to_store];
  bool load_and_update_display = false;

  enum e_frequency_state{idle, active};
  static e_frequency_state state = idle;

  if(state == idle)
  {
    //remember where we were incase we need to cancel
    for(uint8_t i=0; i<settings_to_store; i++){
      stored_settings[i] = settings[i];
    }

    //skip blank channels
    load_and_update_display = true;
    for(uint16_t i = 0; i<num_chans; i++)
    {
      if(radio_memory[select][9] != 0xffffffff) break;
      select++;
      if(select > max) select = min;
    }

    state = active;
  }
  else if(state == active)
  {

    int32_t encoder_position = encoder_control(&select, min, max);
    load_and_update_display = encoder_position != 0;

    //skip blank channels
    for(uint16_t i = 0; i<num_chans; i++)
    {
      if(radio_memory[select][9] != 0xffffffff) break;
      select += encoder_position>0?1:-1;
      if(select < min) select = max;
      if(select > max) select = min;
    }

    //ok
    if(encoder_button.is_pressed()||menu_button.is_pressed()){
      ok=true;
      state = idle;
      return true;
    }

    //cancel
    if(back_button.is_pressed()){
      //put things back how they were to start with
      for(uint8_t i=0; i<settings_to_store; i++){
        settings[i] = stored_settings[i];
      }
      apply_settings(false);
      ok=false;
      state = idle;
      return true;
    }
  }

  if(load_and_update_display)
  {
    //draw screen
    char name[17];
    get_memory_name(name, select, true);

    //(temporarily) apply lodaed settings to RX
    for(uint8_t i=0; i<settings_to_store; i++){
      settings[i] = radio_memory[select][i];
    }
    apply_settings(false);

    //print selected menu item
    display_clear();
    display_print_str("Recall");
    display_print_num(" %03i ", select, 1, style_centered);
    const char* mode_ptr = modes[radio_memory[select][idx_mode]];
    display_set_xy(128-6*strlen(mode_ptr)-8, display_get_y());
    display_print_str(mode_ptr,1);
    display_print_str("\n", 1);
    if (12*strlen(name) > 128) {
      display_add_xy(0,4);
      display_print_str(name,1,style_nowrap|style_centered);
    } else {
      display_print_str(name,2,style_nowrap|style_centered);
    }

    //draw frequency
    display_set_xy(0,27);
    display_print_freq('.', radio_memory[select][idx_frequency], 2);
    display_print_str("\n",2);
    display_print_str("from: ", 1);
    display_print_freq(',', radio_memory[select][idx_min_frequency], 1);
    display_print_str(" Hz\n",1);
    display_print_str("  To: ", 1);
    display_print_freq(',', radio_memory[select][idx_max_frequency], 1);
    display_print_str(" Hz\n",1);
    display_show();
  }

  //draw power meter
  receiver.access(false);
  int8_t power_s = dBm_to_S(status.signal_strength_dBm);
  receiver.release();
  static int8_t last_power_s = 255;
  if(power_s != last_power_s)
  {
    int bar_len = power_s * 62 / 12;
    ssd1306_fill_rectangle(&disp, 124, 0, 3, 63, 0);
    ssd1306_fill_rectangle(&disp, 124, 63 - bar_len, 3, bar_len + 1, 1);
    display_show();
  }


  return false; 
}

// Scan across the stored memories
bool ui::memory_scan(bool &ok)
{

  //encoder loops through memories
  const int32_t min = 0;
  const int32_t max = num_chans-1;
  static int32_t select = 0;
  static uint32_t stored_settings[settings_to_store];
  bool load_and_update_display = false;

  enum e_frequency_state{idle, active};
  static e_frequency_state state = idle;

  static int32_t scan_speed = 0;

  if(state == idle)
  {

    //remember where we were incase we need to cancel
    for(uint8_t i=0; i<settings_to_store; i++){
      stored_settings[i] = settings[i];
    }

    //skip blank channels
    load_and_update_display = true;
    for(uint16_t i = 0; i<num_chans; i++)
    {
      if(radio_memory[select][9] != 0xffffffff) break;
      select++;
      if(select > max) select = min;
    }

    scan_speed = 0;
    state = active;
  }
  else if(state == active)
  {

    int32_t pos_change = encoder_control(&select, min, max);

    static int8_t direction=0;
    if ( pos_change > 0 ){
      direction = 1;
      if(++scan_speed>4) scan_speed=4;
      load_and_update_display = true;
    }
    if ( pos_change < 0 ){
      direction = -1;
      if(--scan_speed<-4) scan_speed=-4;
      load_and_update_display = true;
    }

    static uint32_t last_time = 0u;
    uint32_t now_time = to_ms_since_boot(get_absolute_time());
    if (scan_speed && (now_time - last_time) > (uint32_t)1000/abs(scan_speed)) {
      last_time = now_time;
      select += direction;
      if (++select > max) select = min;
      if (--select < min) select = max;
      load_and_update_display = true;
    }

    //skip blank channels
    for(uint16_t i = 0; i<num_chans; i++)
    {
      if(radio_memory[select][9] != 0xffffffff) break;
      if(select < min) select = max;
      if(select > max) select = min;
    }

    //ok
    if(encoder_button.is_pressed()||menu_button.is_pressed()){
      ok=true;
      state = idle;
      return true;
    }

    //cancel
    if(back_button.is_pressed()){
      //put things back how they were to start with
      for(uint8_t i=0; i<settings_to_store; i++){
        settings[i] = stored_settings[i];
      }
      apply_settings(false);
      ok=false;
      state = idle;
      return true;
    }
  }

  if(load_and_update_display)
  {
    char name[17];
    get_memory_name(name, select, true);

    //(temporarily) apply lodaed settings to RX
    for(uint8_t i=0; i<settings_to_store; i++){
      settings[i] = radio_memory[select][i];
    }
    apply_settings(false);

    //draw screen
    display_clear();
    display_print_str("Scanner");
    display_print_num(" %03i ", select, 1, style_centered);

    const char* mode_ptr = modes[radio_memory[select][idx_mode]];
    display_set_xy(128-6*strlen(mode_ptr)-8, display_get_y());
    display_print_str(mode_ptr,1);

    display_print_str("\n", 1);
    if (12*strlen(name) > 128) {
      display_add_xy(0,4);
      display_print_str(name,1,style_nowrap);
    } else {
      display_print_str(name,2,style_nowrap);
    }

    //draw frequency
    display_set_xy(0,27);
    display_print_freq('.', radio_memory[select][idx_frequency], 2);
    display_print_str("\n",2);

    display_print_str("Speed",2);
    display_print_speed(91, display_get_y(), 2, scan_speed);
  }

  //draw power meter
  receiver.access(false);
  int8_t power_s = dBm_to_S(status.signal_strength_dBm);
  receiver.release();
  static int8_t last_power_s = 255;
  if(power_s != last_power_s)
  {
    int bar_len = power_s * 62 / 12;
    ssd1306_fill_rectangle(&disp, 124, 0, 3, 63, 0);
    ssd1306_fill_rectangle(&disp, 124, 63 - bar_len, 3, bar_len + 1, 1);
    display_show();
  }


  return false; 

}

// print pause, play, reverse play with extra > or < based on speed
// x is the midpoint of the graphic/central character
void ui::display_print_speed(int16_t x, int16_t y, uint32_t scale, int speed)
{
  display_set_xy(x-3*scale,y);
  if (speed >= 1 ) {
    display_print_char(CHAR_PLAY, scale);
    if (speed >= 2 ) {
      for ( int i=1; i<speed; i++) {
        display_add_xy(-3*scale,0);
        display_print_char('>', scale, style_xor);
      }
    }
  }
  if (speed == 0 ) {
    display_print_char(CHAR_PAUSE, scale);
  }
  if (speed <= -1 ) {
    if (scale==1) display_add_xy(1,0);  // workaround font differences
    display_print_char(CHAR_REVPLAY, scale);
    if (scale==1) display_add_xy(-1,0);  // workaround font differences
    if (speed <= -2 ) {
      for ( int i = -1; i>speed; i--) {
        display_add_xy(-9*scale,0);
        display_print_char('<', scale, style_xor);
      }
    }
  }
}

// Scan across the frequency band
bool ui::frequency_scan(bool &ok)
{

  bool load_and_update_display = false;
  enum e_frequency_state{idle, active};
  static e_frequency_state state = idle;
  static int32_t scan_speed = 0;
  const int32_t min = 0;
  const int32_t max = num_chans-1;
  static int32_t select = 0;

  if(state == idle)
  {
    load_and_update_display = true;
    scan_speed = 0;
    state = active;
  }
  else if(state == active)
  {

    int32_t pos_change = encoder_control(&select, min, max);
    load_and_update_display = pos_change != 0;

    static int8_t direction = 1;
    if ( pos_change > 0 ){
       if(++scan_speed>4) scan_speed=4;
       direction = 1;
    }
    if ( pos_change < 0 ){
       if(--scan_speed<-4) scan_speed=-4;
       direction = -1;
    }

    uint32_t now_time = to_ms_since_boot(get_absolute_time());
    static uint32_t last_time = 0u;
    if (scan_speed && (now_time - last_time) > (uint32_t)1000/abs(scan_speed)) {
      last_time = now_time;

      //update frequency 
      settings[idx_frequency] += direction * step_sizes[settings[idx_step]];

      if (settings[idx_frequency] > settings[idx_max_frequency])
          settings[idx_frequency] = settings[idx_min_frequency];

      if ((int)settings[idx_frequency] < (int)settings[idx_min_frequency])
          settings[idx_frequency] = settings[idx_max_frequency];

      apply_settings(false);
      load_and_update_display = true;
    }

    //ok
    if(encoder_button.is_pressed()||menu_button.is_pressed()){
      ok=true;
      state = idle;
      return true;
    }

    //cancel
    if(back_button.is_pressed()){
      ok=false;
      state = idle;
      return true;
    }
  }

  if(load_and_update_display)
  {
      display_clear();
      display_print_str("Scanner");

      const char *p = steps[settings[idx_step]];
      uint16_t x_center = (display_get_x()+120-24)/2;
      display_set_xy(x_center - 6*strlen(p)/2, 0);
      display_print_str(p ,1 );

      // print mode
      const char* mode_ptr = modes[settings[idx_mode]];
      display_set_xy(120-6*strlen(mode_ptr), display_get_y());
      display_print_str(mode_ptr);
      display_print_str("\n");

      //frequency
      display_print_freq('.', settings[idx_frequency],2);
      display_print_str("\n",2);

      display_print_str("From:  ", 1);
      display_print_freq(',', settings[idx_min_frequency], 1);
      display_print_str(" Hz\n",1);

      display_print_str("  To:  ", 1);
      display_print_freq(',', settings[idx_max_frequency], 1);
      display_print_str(" Hz\n",1);

      //draw scanning speed
      display_set_xy(0,48);
      display_print_str("Speed",2);
      display_print_speed(91, display_get_y(), 2, scan_speed);
  }

  //draw power meter
  receiver.access(false);
  int8_t power_s = dBm_to_S(status.signal_strength_dBm);
  receiver.release();
  static int8_t last_power_s = 255;
  if(power_s != last_power_s)
  {
    int bar_len = power_s * 62 / 12;
    ssd1306_fill_rectangle(&disp, 124, 0, 3, 63, 0);
    ssd1306_fill_rectangle(&disp, 124, 63 - bar_len, 3, bar_len + 1, 1);
    display_show();
  }

  return false;
}


int ui::get_memory_name(char* name, int select, bool strip_spaces)
{
      if(radio_memory[select][9] != 0xffffffff)
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
      } else {
        strcpy(name, "BLANK           ");
      }

      // strip trailing spaces
      if (strip_spaces) {
        for (int i=15; i>=0; i--) {
          if (name[i] != ' ') break;
          name[i] = 0;
        }
      }
      return (strlen(name));
}

////////////////////////////////////////////////////////////////////////////////
// String Entry (digit by digit)
////////////////////////////////////////////////////////////////////////////////
int ui::string_entry(char string[], bool &ok, bool &del){

  static int32_t position=0;
  del = false;

  enum e_state{idle, select_position, select_char};
  static e_state state = idle;
  int32_t encoder_position = 0;
  bool draw_display = false;
  const char letters[] = " abcdefghijklmnopqrstuvwxyz 0129356789 ABCDEFGHIJKLMNOPQRSTUVWXYZ 0129456789"; 
  static int32_t val;

  if(state == idle)
  {
    draw_display = true;
    state = select_position;
  }
  else if(state == select_position)
  {
      //change between chars
      encoder_position = encoder_control(&position, 0, 19);
      if(encoder_position) draw_display = true;

      if(menu_button.is_pressed() || encoder_button.is_pressed())
      {
        if(position==16)//yes
        {
          ok=true;
          state=idle;
          return true;
        }
        else if(position==17)//clear
        {
           memset(string, ' ', strlen(string));
           position = 0;
        }
        else if(position==18)//delete
        {
          del=true;
          ok=true;
          state=idle;
          return true;
        }
        else if(position==19)//exit
        {
          ok=false;
          state=idle;
          return true;
        }
        else
        {
          val = strchr(letters, string[position])-letters;
          state = select_char;
        }
      }

      if(back_button.is_pressed())
      {
        ok=false;
        state=idle;
        return true;
      }
  }
  else if(state == select_char)
  {
      //change value of char
      encoder_position = encoder_control(&val, 0, 75);
      if(encoder_position) draw_display = true;
      string[position]=letters[val];
      if(menu_button.is_pressed() || encoder_button.is_pressed())
      {
        state = select_position;
      }
  }

  if(draw_display)
  {
      display_set_xy(0,9);
      display_clear_str(2,false);

      // compute starting point to scroll display
      const uint8_t screen_width = 10;
      const uint8_t buffer_width = 16;
      uint8_t start = 0;
      if (position < screen_width) start = 0;
      else if (position < buffer_width) start = position-(screen_width-1);
      else start = (encoder_position > 0) ? (buffer_width-screen_width) : 0;

      //write preset name to lcd
      for(int i=start; i<16; i++) {
        if (state==select_position && (i==position)) {
          display_print_char(string[i], 2, style_nowrap|style_reverse);
        } else {
          display_print_char(string[i], 2, style_nowrap);
        }
      }

      // print scroll bar
      const uint8_t yp = 25;
      const uint8_t len = screen_width*128/buffer_width;
      ssd1306_draw_line(&disp, 0, yp+1, 127, yp+1, false);
      ssd1306_draw_line(&disp, start*8, yp+1, len+start*8, yp+1, true);
      ssd1306_draw_line(&disp, 0, yp, 0, yp+2, true);
      ssd1306_draw_line(&disp, 127, yp, 127, yp+2, true);

      display_draw_separator(40,1);
      display_linen(6);
      display_clear_str(2,false);
      if (position>=16) {
        display_set_xy(0, display_get_y());
        display_print_str(">",2,style_reverse);
        display_print_str("<",2,style_reverse|style_right);
        print_enum_option("OK#CLEAR#DELETE#EXIT#", position-16);
      }
      display_set_xy(0, display_get_y());

      display_show();
  }
  return false;

}

////////////////////////////////////////////////////////////////////////////////
// Frequency menu item (digit by digit)
////////////////////////////////////////////////////////////////////////////////
bool ui::frequency_entry(const char title[], uint32_t which_setting, bool &ok){

  static int32_t digit=0;
  static int32_t digits[8];
  enum e_frequency_state{idle, digit_select, digit_change};
  static e_frequency_state state = idle;

  if(state == idle)
  {
    //convert to BCD representation
    uint32_t frequency = settings[which_setting];
    uint32_t digit_val = 10000000;
    for(uint8_t i=0; i<8; i++){
        digits[i] = frequency / digit_val;
        frequency %= digit_val;
        digit_val /= 10;
    }
    state = digit_select;
  }

  else if(state == digit_select)
  {
    //change between digits
    encoder_control(&digit, 0, 9);

    if(menu_button.is_pressed() || encoder_button.is_pressed())
    {
      if(digit==8) //Yes, Ok
      {

        //convert back to a binary representation
        uint32_t digit_val = 10000000;
        settings[which_setting] = 0;
        for(uint8_t i=0; i<8; i++)
        {
          settings[which_setting] += (digits[i] * digit_val);
          digit_val /= 10;
        }

        //force max frequency to be larger than min frequency
        if (settings[idx_max_frequency] < settings[idx_min_frequency]){
          settings[idx_max_frequency] = settings[idx_min_frequency];
        }

        switch (which_setting) {
          case idx_frequency:
            //when manually changing to a frequency outside the current band, remove any band limits
            if((settings[idx_frequency] > settings[idx_max_frequency]) || (settings[idx_frequency] < settings[idx_max_frequency]))
            {
              settings[idx_min_frequency] = 0;
              settings[idx_max_frequency] = 30000000;
            }
          case idx_max_frequency:
            //when changing band limits, force frequency within
            if(settings[idx_frequency] > settings[idx_max_frequency])
            {
              settings[idx_frequency] = settings[idx_max_frequency];
            }
            break;
          case idx_min_frequency:
            //when changing band limits, force frequency within
            if(settings[idx_frequency] < settings[idx_min_frequency])
            {
              settings[idx_frequency] = settings[idx_min_frequency];
            }
            break;
        }

        state = idle;
        ok = true;
        return true;
      }

      //No
      else if(digit==9)
      {
        state = idle;
        ok = false;
        return true;
      }

      else
      {
        state = digit_change;
      }
    }

    if(back_button.is_pressed())
    {
      state = idle;
      ok = false;
      return true;
    }

  }
  else if(state == digit_change)
  {
    //change the value of a digit 
    encoder_control(&digits[digit], 0, 9);

    if(menu_button.is_pressed() || encoder_button.is_pressed())
    {
      state = digit_select;
    }
  }
  
  //Draw Display
  display_clear();
  display_print_str(title,1);
  display_set_xy(4,9);
  for(uint8_t i=0; i<8; i++)
  {
    if (state==digit_select && (i==digit)) {
      display_print_char(digits[i] + '0', 2, style_reverse);
    } else {
      display_print_char(digits[i] + '0', 2 );
    }
    if(i==1||i==4) display_print_char('.', 2 );
  }
  display_draw_separator(40,1);
  display_linen(6);
  print_enum_option(" OK #EXIT#", digit-8);
  display_show();

  return false;

}

bool ui::configuration_menu(bool &ok)
{
    enum e_ui_state{select_menu_item, menu_item_active};
    static e_ui_state ui_state = select_menu_item;
    
    static uint32_t menu_selection = 0;

    //chose menu item
    if(ui_state == select_menu_item)
    {
      if(menu_entry("HW Config", "Display\nTimeout#Regulator\nMode#Reverse\nEncoder#Swap IQ#Gain Cal#Flip OLED#OLED Type#Display\nContrast#TFT\nSettings#USB\nUpload#", &menu_selection, ok))
      {
        if(ok) 
        {
          //OK button pressed, more work to do
          ui_state = menu_item_active;
          return false;
        }
        else
        {
          //cancel button pressed, done with menu
          menu_selection = 0;
          ui_state = select_menu_item;
          return true;
        }
      }
    }

    //menu item active
    else if(ui_state == menu_item_active)
    {
      bool done = false;
      static uint32_t setting_word;
      switch(menu_selection)
      {
        case 0: 
          setting_word = (settings[idx_hw_setup] & mask_display_timeout) >> flag_display_timeout;
          done = enumerate_entry("Display\nTimeout", "Never#5 Sec#10 Sec#15 Sec#30 Sec#1 Min#2 Min#4 Min#", &setting_word, ok);
          settings[idx_hw_setup] &=  ~mask_display_timeout;
          settings[idx_hw_setup] |=  setting_word << flag_display_timeout;
          display_time = time_us_32();
          display_timeout_max = timeout_lookup[setting_word];
          break;

        case 1 : 
          done = enumerate_entry("PSU Mode", "FM#PWM#", &regmode, ok);
          gpio_set_dir(23, GPIO_OUT);
          gpio_put(23, regmode);
          break;

        case 2 : 
          done =  bit_entry("Reverse\nEncoder", "Off#On#", flag_reverse_encoder, &settings[idx_hw_setup], ok);
          break;

        case 3 : 
          done =  bit_entry("Swap IQ", "Off#On#", flag_swap_iq, &settings[idx_hw_setup], ok);
          break;

        case 4 : 
          done =  number_entry("Gain Cal", "%idB", 1, 100, 1, &settings[idx_gain_cal], ok);
          break;

        case 5 : 
          done = bit_entry("Flip OLED", "Off#On#", flag_flip_oled, &settings[idx_hw_setup], ok);
          ssd1306_flip(&disp, (settings[idx_hw_setup] >> flag_flip_oled) & 1);
          break;

        case 6: 
          done = bit_entry("OLED Type", "SSD1306#SH1106#", flag_oled_type, &settings[idx_hw_setup], ok);
          ssd1306_type(&disp, (settings[idx_hw_setup] >> flag_oled_type) & 1);
          break;

        case 7:
          setting_word = (settings[idx_hw_setup] & mask_display_contrast) >> flag_display_contrast;
          done =  number_entry("Display\nContrast", "%i", 0, 15, 1, &setting_word, ok);
          ssd1306_contrast(&disp, 17 * setting_word);
          settings[idx_hw_setup] &= ~mask_display_contrast;
          settings[idx_hw_setup] |= setting_word << flag_display_contrast;
          break;

        case 8:
          setting_word = (settings[idx_hw_setup] & mask_tft_settings) >> flag_tft_settings;
          done =  enumerate_entry("TFT\nSettings", "Off#Rotation 1#Rotation 2#Rotation 3#Rotation 4#", &setting_word, ok);
          settings[idx_hw_setup] &= ~mask_tft_settings;
          settings[idx_hw_setup] |= setting_word << flag_tft_settings;
          waterfall_inst.configure_display(setting_word);
          break;

        case 9: 
          setting_word = 0;
          enumerate_entry("USB Upload", "Back#Memory#Firmware#", &setting_word, ok);
          if(setting_word==1) {
            upload_memory();
          } else if (setting_word==2) {
            display_clear();
            display_print_str("Ready for\nfirmware",2, style_centered);
            display_show();
            reset_usb_boot(0,0);
          }
          break;
      }
      if(done)
      {
        menu_selection = 0;
        ui_state = select_menu_item;
        return true;
      }
    }
    return false;

}

bool ui::main_menu(bool & ok)
{

    enum e_ui_state {select_menu_item, menu_item_active};
    static e_ui_state ui_state = select_menu_item;
    static uint32_t menu_selection = 0;

    //chose menu item
    if(ui_state == select_menu_item)
    {
      if(menu_entry("Menu", "Frequency#Recall#Store#Volume#Mode#AGC Speed#Bandwidth#Squelch#Auto Notch#De-\nEmphasis#Band Start#Band Stop#Frequency\nStep#CW Tone\nFrequency#Scanner#HW Config#", &menu_selection, ok))
      {
        if(ok) 
        {
          //ok button pressed, more work to do
          ui_state = menu_item_active;
          return false;
        }
        else
        {
          //cancel button pressed, done with menu
          menu_selection = 0;
          ui_state = select_menu_item;
          return true;
        }
      }
    }

    //menu item active
    else if(ui_state == menu_item_active)
    {
       bool done = false;
       switch(menu_selection)
        {
          case 0 :  
            done = frequency_entry("frequency", idx_frequency, ok);
            break;
          case 1 : 
            done = memory_recall(ok);
            break;
          case 2 : 
            done = memory_store(ok); 
            break;
          case 3 :  
            done = number_entry("Volume", "%i", 0, 9, 1, &settings[idx_volume], ok);
            break;
          case 4 :  
            done = enumerate_entry("Mode", "AM#AM-Sync#LSB#USB#FM#CW#", &settings[idx_mode], ok);
            break;
          case 5 :
            done = enumerate_entry("AGC Speed", "Fast#Normal#Slow#Very slow#", &settings[idx_agc_speed], ok);
            break;
          case 6 :  
            done = enumerate_entry("Bandwidth", "V Narrow#Narrow#Normal#Wide#Very Wide#", &settings[idx_bandwidth], ok);
            break;
          case 7 :  
            done = enumerate_entry("Squelch", "S0#S1#S2#S3#S4#S5#S6#S7#S8#S9#S9+10dB#S9+20dB#S9+30dB#", &settings[idx_squelch], ok);
            break;
          case 8 :  
            done = bit_entry("Auto Notch", "Off#On#", flag_enable_auto_notch, &settings[idx_rx_features], ok);
            break;
          case 9 :
            {
              uint32_t v = (settings[idx_rx_features] & mask_deemphasis) >> flag_deemphasis;
              done = enumerate_entry("De-\nemphasis", "Off#50us#75us#", &v, ok);
              settings[idx_rx_features] &= ~(mask_deemphasis);
              settings[idx_rx_features] |= (v << flag_deemphasis);
            }
            break;
          case 10 :  
            done = frequency_entry("Band Start", idx_min_frequency, ok);
            break;
          case 11 : 
            done = frequency_entry("Band Stop", idx_max_frequency, ok);
            break;
          case 12 : 
            done = enumerate_entry("Frequency\nStep", "10Hz#50Hz#100Hz#1kHz#5kHz#10kHz#12.5kHz#25kHz#50kHz#100kHz#", &settings[idx_step], ok);
            settings[idx_frequency] -= settings[idx_frequency]%step_sizes[settings[idx_step]];
            break;
          case 13 : 
            done = number_entry("CW Tone\nFrequency", "%iHz", 1, 30, 100, &settings[idx_cw_sidetone], ok);
            break;
          case 14 : 
            done = scanner_menu(ok);
            break;
          case 15 : 
            done = configuration_menu(ok);
            break;
        }
        if(done)
        {
          menu_selection = 0;
          ui_state = select_menu_item;
          return true;
        }
    }

    return false;
}


bool ui::scanner_menu(bool &ok)
{
    enum e_ui_state {select_menu_item, menu_item_active};
    static e_ui_state ui_state = select_menu_item;
    static uint32_t menu_selection = 0;

    //chose menu item
    if(ui_state == select_menu_item)
    {
      if(menu_entry("Scan", "Memories#Frequency\nRange#", &menu_selection, ok))
      {
        if(ok) 
        {
          //ok button pressed, more work to do
          ui_state = menu_item_active;
          return false;
        }
        else
        {
          //cancel button pressed, done with menu
          menu_selection = 0;
          ui_state = select_menu_item;
          return true;
        }
      }
    }

    //menu item active
    else if(ui_state == menu_item_active)
    {
       bool done = false;
       switch(menu_selection)
        {
          case 0 :  
            done = memory_scan(ok);
            break;
          case 1 : 
            done = frequency_scan(ok);
            break;
        }
        if(done)
        {
          menu_selection = 0;
          ui_state = select_menu_item;
          return true;
        }
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////
// This is the startup animation
////////////////////////////////////////////////////////////////////////////////
bool ui::do_splash()
{
  static int step=0;
  if (step++ >= 20) {  // we're done
    step = 0;
    return true;
  }

  display_clear();
  ssd1306_bmp_show_image(&disp, crystal, sizeof(crystal));

  int i=-1;
#if 0
// zoom in
       if (step <= 5) i=0;        // image for 3 tenths
  else if (step <= 7) i=step-5;
  else if (step <= 12) i=3;
  else if (step <= 18) i=step-7;

#else
// zoom out
       if (step <= 6) i=10-step;
  else if (step <= 11) i=3;
  else if (step <= 13) i=14-step;
  else if (step <= 18) i=0;

#endif

  if (i==0) {
    // do nothing, leave the bitmap
  } else if (i>0) {
    display_set_xy(0,(64-i*8)/2); // disp height - text height /2
    display_print_str("PicoRX",i,style_centered|style_nowrap|style_bordered);
  } else if (i==-1) {
    display_clear();
  }
  display_show();
  return false;

}

////////////////////////////////////////////////////////////////////////////////
// This is the main UI loop. Should get called about 10 times/second
////////////////////////////////////////////////////////////////////////////////
void ui::do_ui()
{

    bool update_settings = false;
    bool autosave_settings = false;
    enum e_ui_state {splash, idle, menu, recall, sleep};
    static e_ui_state ui_state = splash;
    static bool frequency_autosave_pending = false;
    static uint32_t frequency_autosave_time = 0;
    static uint8_t display_option = 0;
    const uint8_t num_display_options = 6;
    bool view_changed = false;
    
    
    //gui is idle, just update the display
    if(ui_state == splash)
    {
      if(do_splash())
        ui_state = idle;
      if(menu_button.is_pressed()||encoder_button.is_pressed()||back_button.is_pressed())
        ui_state = idle;
    }

    //gui is idle, just update the display
    else if(ui_state == idle)
    {

      //launch menu or recall
      if(menu_button.is_pressed())
      {
        ui_state = menu;
        display_time = time_us_32();
      }
      else if(encoder_button.is_pressed())
      {
        ui_state = recall;
        display_time = time_us_32();
      }
      else if(back_button.is_pressed())
      {
        display_option++;
        view_changed = true;
        if(display_option==num_display_options) display_option=0u;
      }

      //adjust frequency when encoder is turned
      uint32_t encoder_change = get_encoder_change();
      if(encoder_change != 0)
      {
        display_time = time_us_32();

        //very fast tuning
        if(menu_button.is_held() && back_button.is_held())
          settings[idx_frequency] += encoder_change * step_sizes[settings[idx_step]] * 100;
        //fast tuning
        else if(menu_button.is_held())
          settings[idx_frequency] += encoder_change * step_sizes[settings[idx_step]] * 10;
        //slow tuning
        else if(back_button.is_held())
          settings[idx_frequency] += encoder_change * (step_sizes[settings[idx_step]] / 10);
        //normal tuning
        else
          settings[idx_frequency] += encoder_change * step_sizes[settings[idx_step]];

        //wrap frequency at band limits
        if (settings[idx_frequency] > settings[idx_max_frequency])
            settings[idx_frequency] = settings[idx_min_frequency];
        if ((int)settings[idx_frequency] < (int)settings[idx_min_frequency])
            settings[idx_frequency] = settings[idx_max_frequency];

        //update settings now, but don't autosave until later
        update_settings = true;
        frequency_autosave_pending = true;
        frequency_autosave_time = time_us_32();

      }
      
      switch(display_option)
      {
        case 0: renderpage_original(status, receiver);break;
        case 1: renderpage_bigspectrum(status, receiver);break;
        case 2: renderpage_waterfall(view_changed, status, receiver);break;
        case 3: renderpage_bigtext(status, receiver);break;
        case 4: renderpage_smeter(view_changed, status, receiver); break;
        case 5: renderpage_fun(status, receiver);break;
      }
    }

    //menu is active, if menu completes update settings
    else if(ui_state == menu)
    {
      bool ok = false;
      if(main_menu(ok))
      {
        ui_state = idle;
        update_settings = ok;
        autosave_settings = ok;
        display_time = time_us_32();
      }
    }

    //push button enters recall menu directly
    else if(ui_state == recall)
    {
      bool ok = false;
      if(ui::memory_recall(ok))
      {
        ui_state = idle;
        update_settings = ok;
        autosave_settings = ok;
        display_time = time_us_32();
      }
    }

    //if display times out enter sleep mode
    else if(ui_state == sleep)
    {
      if(menu_button.is_pressed() || encoder_button.is_pressed() || back_button.is_pressed() || get_encoder_change())
      {
        display_time = time_us_32();
        ssd1306_poweron(&disp);
        ui_state = idle;
      }
    }

    //automatically switch off display after a period of inactivity
    if(ui_state == idle && display_timeout_max && (time_us_32() - display_time) > display_timeout_max)
    {
      ui_state = sleep;
      ssd1306_poweroff(&disp);
    }

    //autosave frequency only after is has been stable for 1 second
    if(frequency_autosave_pending)
    {
      if((time_us_32() - frequency_autosave_time) > 1000000u)
      {
        autosave_settings = true;
        frequency_autosave_pending = false;
      }
    }

    //autosave current settings to flash
    if(autosave_settings)
      autosave();
 
    //apply settings to receiver
    if(update_settings)
    {
      receiver.access(true);
      settings_to_apply.tuned_frequency_Hz = settings[idx_frequency];
      settings_to_apply.agc_speed = settings[idx_agc_speed];
      settings_to_apply.enable_auto_notch = (settings[idx_rx_features] & mask_enable_auto_notch) >> flag_enable_auto_notch;
      settings_to_apply.mode = settings[idx_mode];
      settings_to_apply.volume = settings[idx_volume];
      settings_to_apply.squelch = settings[idx_squelch];
      settings_to_apply.step_Hz = step_sizes[settings[idx_step]];
      settings_to_apply.cw_sidetone_Hz = settings[idx_cw_sidetone];
      settings_to_apply.bandwidth = settings[idx_bandwidth];
      settings_to_apply.gain_cal = settings[idx_gain_cal];
      settings_to_apply.deemphasis = (settings[idx_rx_features] & mask_deemphasis) >> flag_deemphasis;
      receiver.release();
    }

}

ui::ui(rx_settings & settings_to_apply, rx_status & status, rx &receiver, uint8_t *spectrum, uint8_t &dB10, waterfall &waterfall_inst) : 
  menu_button(PIN_MENU), 
  back_button(PIN_BACK), 
  encoder_button(PIN_ENCODER_PUSH),
  settings_to_apply(settings_to_apply),
  status(status), 
  receiver(receiver), 
  spectrum(spectrum),
  dB10(dB10),
  waterfall_inst(waterfall_inst)
{
  setup_display();
  setup_encoder();
}

