#include "cw_keyer.h"
#include "pico/stdlib.h"

cw_keyer::cw_keyer(uint8_t paddle_type, uint8_t paris_wpm, uint32_t sample_rate_Hz, button &dit, button &dah):
dit(dit),
dah(dah)
{
  m_paddle_type = paddle_type;
  dit_length_samples = (sample_rate_Hz*60)/(50*paris_wpm);
}

bool __not_in_flash_func(cw_keyer::get_straight)()
{
  return dit.is_keyed() || dah.is_keyed(); 
}

int16_t __not_in_flash_func(cw_keyer::key_shape)(bool pressed)
{

  enum e_state{off, rising, on, falling};
  static e_state state;
  int16_t shape = 0;

  switch(state)
  {
    case off:
      if(pressed)
      {
        state = rising;
      }
      shape = 0;
      break;

    case rising:

      if(timer == 50)
      {
        shape = 32767;
        state = on;  
      }
      else
      {
        shape = (18307 + timer*(-18023 + timer*(11090 + (timer*-148))))>>8;
        timer++;
      }

      break;

    case on:
      shape = 32767;
      if(!pressed)
      {
        state = falling;
      }
      break;

    case falling: 

      if(timer == 0)
      {
        shape = 0;
        state = off;
      }
      else
      {
        shape = (18307 + timer*(-18023 + timer*(11090 + (timer*-148))))>>8;
        timer--;
      }

      break;
  }  

  return shape;
}

int16_t __not_in_flash_func(cw_keyer::get_sample)()
{
  if(m_paddle_type == IAMBIC_A || m_paddle_type == IAMBIC_B) return key_shape(get_iambic());
  return key_shape(get_straight());
}

bool __not_in_flash_func(cw_keyer::get_iambic)(){
   bool is_keyed = false;
   switch(keyer_state)
   {

     case IDLE:
       if(dit.is_keyed())
       {
         keyer_state = DIT;
         counter = dit_length_samples;
       } 
       else if(dah.is_keyed())
       {
         keyer_state = DAH;
         counter = dit_length_samples * 3;
       }
       break;

     case DIT:
       is_keyed = true;
       if(!counter--)
       {
         if(dit.is_keyed() && dah.is_keyed()) both_pressed = true;
         if(m_paddle_type == IAMBIC_A && dah.is_keyed() && dit.is_keyed())
         {
           counter = dit_length_samples;
           keyer_state = DIT_SPACE;
         }
         else if(m_paddle_type == IAMBIC_A && both_pressed)
         {
           counter = dit_length_samples;
           keyer_state = DIT_SPACE;
         }
         else 
         {
           counter = dit_length_samples;
           keyer_state = SPACE;
         }
         both_pressed = false;
       }
       break;

     case DAH:
       is_keyed = true;
       if(!counter--)
       {
         if(dit.is_keyed() && dah.is_keyed()) both_pressed = true;
         if(m_paddle_type == IAMBIC_A && dah.is_keyed() && dit.is_keyed())
         {
           counter = dit_length_samples;
           keyer_state = DAH_SPACE;
         } 
         else if(m_paddle_type == IAMBIC_B && both_pressed)
         {
           counter = dit_length_samples;
           keyer_state = DAH_SPACE;
         }
         else 
         {
           counter = dit_length_samples;
           keyer_state = SPACE;
         }
         both_pressed = false;
       }
       break;

     case SPACE:
       if(!counter--){
         if(dit.is_keyed())
         {
           keyer_state = DIT;
           counter = dit_length_samples;
         } 
         else if(dah.is_keyed())
         {
           keyer_state = DAH;
           counter = dit_length_samples * 3;
         }
         else
         {
           keyer_state = IDLE;
         }
       }
       break;

     case DIT_SPACE:
       if(!counter--){
         counter = dit_length_samples*3;
         keyer_state = DAH;
       }
       break;

     case DAH_SPACE:
       if(!counter--){
         counter = dit_length_samples;
         keyer_state = DIT;
       }
       break;

  }

  return is_keyed;

}
