#include "pico/multicore.h"

unsigned char dmxdata[512];

unsigned char led = 0;

unsigned char dir = 1;

void setup() {
  multicore_reset_core1();
  delay(66);
  multicore_launch_core1(core1_main);
  delay(66);
  pinMode(LED_BUILTIN, OUTPUT);
  for(int i = 0; i < 512; i += 1){
    dmxdata[i] = 0;
  }
  dmxdata[28] = 0;

  Serial.begin();

  delay(66);
}

void loop(){
  unsigned char buf = 0;
  unsigned int bufch = 0;
  unsigned char bufval = 0;
  for(;;){
    if(Serial.available()){
      led = (led + 1) % 2;
      buf = Serial.read();
      if      ((buf & 0b11000000) == 0b00000000){
        // チャンネル 上位5ビット
        bufch = (buf & 0b00011111) * 16;
      }else if((buf & 0b11000000) == 0b01000000){
        // チャンネル 下位4ビット + データ 上位2ビット
        bufch = bufch + (buf & 0b00111100) / 4;
        bufval = (buf & 0b00000011) * 64;
      }else if((buf & 0b11000000) == 0b10000000){
        // データ 下位6ビット
        bufval = bufval + (buf & 0b00111111);
        dmxdata[bufch] = bufval;
      }else if((buf & 0b11000000) == 0b11000000){
        // いろいろ
        if(buf == 0b11111111){
          // Clear
          for(int i = 0; i < 512; i += 1){
            dmxdata[i] = 0;
          }
        }
      } 
    }
  }
}

void core1_main() {
  constexpr unsigned char z = 0b00000000;
  Serial1.setTX(0);
  Serial1.begin(250000, SERIAL_8N2);
  for(unsigned int mcnt = 0;; mcnt += 1){
    delayMicroseconds(0);

    // ブレーク
    uart_set_baudrate(uart0, 19200);
    Serial1.write(z);
    Serial1.flush();
    
    // データ
    uart_set_baudrate(uart0, 250000);
    for(int i = 0; i < 512; i += 1){
      Serial1.write(dmxdata[i]);
    }
    Serial1.flush();
    delayMicroseconds(1850);
  }
}
