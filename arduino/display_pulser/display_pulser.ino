#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Fonts/FreeMono9pt7b.h>
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

int p_enable = 0;
int N=8;
int stepi=0;

void enable_display(int en) {
  boolean isBitSet;
  byte i;
  for (i=0;i<N;i++){
    if (i<4)
      display.setCursor((9+i*3)*6,0);
    else if (i<8)
      display.setCursor((9+(i-4)*3)*6,8);
    if (i<8){
      if (en & (1 << i))
        display.println("[o]");
      else
        display.println("[ ]");
    }
  }
}

void info_display() {
  int i;
  display.clearDisplay();
  //display.setFont(&FreeMono9pt7b);
  display.setTextSize(1);
  display.setTextColor(WHITE);
  //display.println("         [0][0][0][0]         [0][0][0][0]");
  enable_display(stepi);  
  for (i=0;i<5;i++){
    display.setCursor(0, 16+i*13);
    display.println("X:ABCDE 10.123456789");
  }
  display.setTextSize(2);
  display.setCursor(0,0);
  display.println("BUSY");
  
  // Display static text
  display.display();
}


void setup() {
  int i;
  Serial.begin(115200);

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  delay(2000);
}


void loop() {
   
  info_display();
  delay(2000);
  stepi +=1;
}
