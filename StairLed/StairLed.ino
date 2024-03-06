#define LAT 2
#define CLK 3
#define DATA 4

unsigned char arr[] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};

void setup() {
  pinMode(LAT, OUTPUT);
  pinMode(CLK, OUTPUT);
  pinMode(DATA, OUTPUT);
}
void loop() {
}
void ShiftByte(unsigned char byte){
  for(unsigned char i = 0; i < 8; i++){
    if(byte & 0x80){
      digitalWrite(DATA, LOW);
    }
    else{
      digitalWrite(DATA, HIGH);
    }
    digitalWrite(CLK, HIGH);
    byte <<= 1;
    digitalWrite(CLK, LOW);
  }
}
void Mode1(void){
  ShiftByte(0xFF);
  delay(500);
  ShiftByte(0x00);
  delay(500);
}
void Mode2(void){
  static unsigned char i = 0;
  ShiftByte(arr[i]);
  delay(500);
  i++;
  if(i >= 8) i = 0;
}
