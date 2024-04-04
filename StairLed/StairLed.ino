#define LAT D3
#define CLK D4
#define DATA D2
#define Sensor1 D5
#define Sensor2 D1

typedef enum{
  ST_RESET,
  ST_SET
} STATE;
enum{
  MODE_STANDAR,
  MODE_UP,
  MODE_DOWN,
  MODE_UP_DOWN,
};

signed char number_up = 8;
signed char number_down = 0;
signed char i = 0;
signed char j = 7;
unsigned char state = MODE_STANDAR;
unsigned char Dir1 = 0, Dir2 = 0;
uint32_t first_flag = 0, second_flag = 0;

ICACHE_RAM_ATTR void SENSOR_UP(void) {
    Dir1 = 1;
    first_flag = millis();
}

ICACHE_RAM_ATTR void SENSOR_DOWN(void) {
    Dir2 = 1;
    second_flag = millis();
}

void setup() {
  pinMode(LAT, OUTPUT);
  pinMode(CLK, OUTPUT);
  pinMode(DATA, OUTPUT);
  pinMode(Sensor1, INPUT_PULLUP);
  pinMode(Sensor2, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(Sensor1), SENSOR_UP, FALLING);
  attachInterrupt(digitalPinToInterrupt(Sensor2), SENSOR_DOWN, FALLING);
}
void loop() {
  Scan(500);
}
void ShiftByte(unsigned char byte){
  for(unsigned char i = 0; i < 8; i++){
    if(byte & 0x80){
      digitalWrite(DATA, HIGH);
    }
    else{
      digitalWrite(DATA, LOW);
    }
    digitalWrite(CLK, HIGH);
    byte <<= 1;
    digitalWrite(CLK, LOW);
  }
  digitalWrite(LAT, HIGH);
  digitalWrite(LAT, LOW);
}
void SetPin(unsigned char Pin, STATE state){
  static unsigned char byte = 0;
  if(state){
    byte |= (1 << Pin);
  }
  else{
    byte &= ~(1 << Pin);
  }
  ShiftByte(byte);
}
void TogglePin(unsigned char Pin){
  static unsigned char byte = 0;
  byte ^= (1 << Pin);
  ShiftByte(byte);
}
//void Scan(void){
//  for(unsigned char i  = 0; i < 8; i++){
//    SetPin(i, ST_RESET);
//    delay(100);
//  }
//}
void Scan(int time){
  if(Dir1 == 1 && Dir2 == 0){
    state = MODE_UP;
  }
  else if(Dir1 == 0 && Dir2 == 1){
    state = MODE_DOWN;
  }
  else if(Dir1 == 1 && Dir2 == 1){
    state = MODE_UP_DOWN;
  }
  else{
    state = MODE_STANDAR;
  }
  switch(state){
    case MODE_UP:
    {
      unsigned char flag = 0;
      SetPin(i, ST_SET);
      delay(time);
      i++;
      if(i >= number_up){
        flag = 1;
        i = 0;
      }
      if(flag == 1){
        Dir1 = Dir2 = 0;
        for(signed char k = 0; k < 8; k++){
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
            number_up = 8;   // reset value i
            SetPin(k, ST_RESET);
            delay(time);
          }
        }
        flag = 0;
      }
    }
      break;
    case MODE_DOWN:
    {
      unsigned char flag = 0;
      SetPin(j, ST_SET);
      delay(time);
      j--;
      if(j < number_down){
        flag = 1;
        j = 7;
      }
      if(flag == 1){
        Dir1 = Dir2 = 0;
        for(signed char k = 7; k >= 0; k--){
          if(Dir1 == 0 && Dir2 == 1){
            number_down = k;
            state = MODE_DOWN;
            break;
          }
          else if(Dir1 == 1 && Dir2 == 0){
            number_up = 8;
            state = MODE_UP;
            break;
          }
          else{
            number_down = 0;  // reset value j
            SetPin(k, ST_RESET);
            delay(time);
          }
        }
        flag = 0;
      }
    }
      break;
    case MODE_UP_DOWN:
    {
      unsigned char flag = 0;
      SetPin(i, ST_SET);
      SetPin(j, ST_SET);
      delay(time);
      i++;
      j--;
      if(i >= j){
        SetPin(i, ST_SET);
        flag = 1;
        i = 0;
        j = 7;
      }
      if(flag == 1){
        Dir1 = Dir2 = 0;
        if(first_flag > second_flag){
          for(signed char k = 0; k < 8; k++){
            if(Dir1 == 1 && Dir2 == 0){
              number_up = k;  // current value up
              state = MODE_UP;
              break;
            }
            else if(Dir1 == 0 && Dir2 == 1){
              number_down = k; // current value down
              state = MODE_DOWN;
              break;
            }
            else if(Dir1 == 1 && Dir2 == 1){
              state = MODE_UP_DOWN;
              break;
            }
            else{
              SetPin(k, ST_RESET);
              delay(time);
            }
          }
          first_flag = 1;
          second_flag = first_flag - 1;
        }
        else if(first_flag < second_flag){
          for(signed char k = 7; k >= 0; k--){
            if(Dir1 == 1 && Dir2 == 0){
              number_up = k;  // current value up
              state = MODE_UP;
              break;
            }
            else if(Dir1 == 0 && Dir2 == 1){
              number_down = k; // current value down
              state = MODE_DOWN;
              break;
            }
            else if(Dir1 == 1 && Dir2 == 1){
              state = MODE_UP_DOWN;
              break;
            }
            else{
              SetPin(k, ST_RESET);
              delay(time);
            }
          }
          second_flag = 1;
          first_flag = second_flag - 1;
        }
        flag = 0;
      }
    }
      break;
    case MODE_STANDAR:
    {

    }
      break;
    default:
      break;
  }
}
