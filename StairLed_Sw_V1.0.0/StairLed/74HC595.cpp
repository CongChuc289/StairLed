#include"74HC595.h"

buffer_eeprom buffer_config;

signed char number_up = buffer_config.number_step;
signed char number_down = 0;
signed char i = 0;
signed char j = buffer_config.number_step - 1;
uint8_t flag = 0;

signed char LED_SCAN = 0;
signed char state = MODE_STANDAR;

extern uint8_t Dir1;
extern uint8_t Dir2;

void ShiftByte(uint32_t byte_t){
  if(buffer_config.number_step <= 8) LED_SCAN = 8;
  else if(buffer_config.number_step > 8 && buffer_config.number_step <= 16) LED_SCAN = 16;
  else if(buffer_config.number_step > 16 && buffer_config.number_step <= 24) LED_SCAN = 24;
  else if(buffer_config.number_step > 24 && buffer_config.number_step <= 32) LED_SCAN = 32;

  for(unsigned char i = 0; i < LED_SCAN; i++){
    if(buffer_config.number_step <= 8){
      if(byte_t & 0x80){
        DATA_ON;
      }
      else{
        DATA_OFF;
      }
    }
    else if(buffer_config.number_step > 8 && buffer_config.number_step <= 16){
      if(byte_t & 0x8000){
        DATA_ON;
      }
      else{
        DATA_OFF;
      }
    }
    else if(buffer_config.number_step > 16 && buffer_config.number_step <= 24){
      if(byte_t & 0x800000){
        DATA_ON;
      }
      else{
        DATA_OFF;
      }
    }
    else if(buffer_config.number_step > 24 && buffer_config.number_step <= 32){
      if(byte_t & 0x80000000){
        DATA_ON;
      }
      else{
        DATA_OFF;
      }
    }
    CLK_ON;
    byte_t <<= 1;
    CLK_OFF;
  }
  LAT_ON;
  LAT_OFF;
}
void Scan(uint32_t time_set, uint32_t time_off){
  if(Dir1 == 1 && Dir2 == 0){
    state = MODE_UP;
    Serial.println("MODE UP");
  }
  else if(Dir1 == 0 && Dir2 == 1){
    state = MODE_DOWN;
    Serial.println("MODE DOWN");
  }
  else if(Dir1 == 1 && Dir2 == 1){
    state = MODE_UP_DOWN;
    Serial.println("MODE UP DOWN");
  }
  else{
    state = MODE_STANDAR;
    Serial.println("MODE STAND");
  }
  switch(state){
    case MODE_UP:
    {
      SetPin(i, ST_SET);
      delay(time_set);
      //Serial.println("i =" + String(i));dd
      i++;
      if(i >= 25){
        i = 0;
        flag = 1;
      }
      if(flag == 1){
        Dir1 = Dir2 = 0;
        delay(time_off);
        for(signed char k = 0; k < buffer_config.number_step; k++){
          if(Dir1 == 1 && Dir2 == 0){
            number_up = k;
            state = MODE_UP;
            break;
          }
          else if(Dir1 == 0 && Dir2 == 1){
            number_down = 0;
            state = MODE_DOWN;
            break;
          }
          else{
            number_up = buffer_config.number_step;   // reset value i
            SetPin(k, ST_RESET);
            delay(time_set);
          }
        }
        flag = 0;
      }
    }
      break;
    case MODE_DOWN:
    {
      SetPin(j, ST_SET);
      delay(time_set);
      //Serial.println("j =" + String(j));
      j--;
      if(j < 0){
        j = buffer_config.number_step - 1;
        flag = 1;
      }
      if(flag == 1){
        Dir1 = Dir2 = 0;
        delay(time_off);
        for(signed char k = buffer_config.number_step - 1; k >= 0; k--){
          if(Dir1 == 0 && Dir2 == 1){
            number_down = k;
            state = MODE_DOWN;
            break;
          }
          else if(Dir1 == 1 && Dir2 == 0){
            number_up = buffer_config.number_step;
            state = MODE_UP;
            break;
          }
          else{
            number_down = 0;  // reset value j
            SetPin(k, ST_RESET);
            delay(time_set);
          }
        }
        flag = 0;
      }
    }
      break;
    case MODE_UP_DOWN:
    {
//      signed char j = 25 - 1; // buffer_config.number_step
//      signed char i = 0;
      SetPin(i, ST_SET);
      SetPin(j, ST_SET);
      delay(time_set);
      i++;
      j--;
      if(i >= j){
        SetPin(i, ST_SET);
        flag = 1;
        i = 0;
        j = buffer_config.number_step - 1;
      }
      if(flag == 1){
        Dir1 = Dir2 = 0;
        delay(time_off);
        for(signed char k = 0; k < buffer_config.number_step; k++){
          if(Dir1 == 1 && Dir2 == 0){
            state = MODE_UP;
            break;
          }
          else if(Dir1 == 0 && Dir2 == 1){
            state = MODE_DOWN;
            break;
          }
          else if(Dir1 == 1 && Dir2 == 1){
            i = 0;
            j = buffer_config.number_step - 1;
            state = MODE_UP_DOWN;
            break;
          }
          else{
            SetPin(k, ST_RESET);
            SetPin(buffer_config.number_step - k, ST_RESET);
            delay(time_set);
          }
        }
        flag = 0;
      }
    }
      break;
    case MODE_STANDAR:
    {
      ShiftByte(0x00);
    }
      break;
    default:
      break;
  }
}
void SetPin(unsigned char Pin, STATE state){
  static uint32_t byte_t = 0;
  if(state){
    byte_t |= (1 << Pin);
  }
  else{
    byte_t &= ~(1 << Pin);
  }
  ShiftByte(byte_t);
}
void TogglePin(unsigned char Pin){
  static uint32_t byte = 0;
  byte ^= (1 << Pin);
  ShiftByte(byte);
}
