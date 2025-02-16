#include "pico/multicore.h"

unsigned char dmxdata[512];

#define LED_PWR 8
#define LED_LNK 9
#define LED_ERR LED_BUILTIN

unsigned char dir = 1;


void setup() {
  pinMode(LED_PWR, OUTPUT);
  pinMode(LED_LNK, OUTPUT);
  pinMode(LED_ERR, OUTPUT);
  gpio_put(LED_PWR, 0);
  gpio_put(LED_LNK, 0);
  gpio_put(LED_ERR, 0);

  delay(100);
  multicore_reset_core1();
  delay(100);
  multicore_launch_core1(core1_main);
  delay(100);
  for(int i = 0; i < 512; i += 1){
    dmxdata[i] = 0;
  }
  dmxdata[28] = 0;

  Serial.begin();

  delay(1500);

  gpio_put(LED_PWR, 0);
  gpio_put(LED_LNK, 1);
  gpio_put(LED_ERR, 1);
}

void loop(){
  unsigned char buf = 0;
  unsigned int bufch = 0;
  unsigned char bufval = 0;
  unsigned char led_state_lnk = 0;
  for(;;){
    // begin for

    // receive
    led_state_lnk = 1;
    for(unsigned char i = 0; i < 256; i += 1){
      if(Serial.available()){
        if(led_state_lnk == 1){
          led_state_lnk = 0;
          gpio_put(LED_LNK, led_state_lnk);
        }
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
            for(int i = 0; i < 512; i += 1){ dmxdata[i] = 0; }
          }
        }
      }else{
        break;
      }
    }
    // leds
    gpio_put(LED_LNK, led_state_lnk);
    
    // end for
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
