#include <stdio.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
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
  display.setTextSize(1);
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

void state_display(boolean busy){
  display.setTextSize(2);
  display.setCursor(0,0);
  if (busy)
    display.println("BUSY");
  else
    display.println("WAIT");
}

void delay_display(byte line, byte pulser, char* mn, unsigned long md){
  char pname[6];
  char pdelay[14];
  unsigned int ns,us,ms;
  
  
  if (line<4){
    if (pulser<10)
      display.drawChar(0, 16+line*13,48+pulser, WHITE, BLACK, 1);//0-9
    else
      display.drawChar(0, 16+line*13,55+pulser, WHITE, BLACK, 1);//A-Z
    sprintf(pname, "%5s", mn);
    display.setCursor(10, 16+line*13);
    display.println(pname);
    
    ns=md%100;
    md=(int)((md-ns)/100);
    us=md%1000;
    md=(int)((md-us)/1000);
    ms=md%1000;
    md=(int)((md-ms)/1000);
    sprintf(pdelay, "%3d.%03d%03d%02d0",md,ms,us,ns);
    display.setCursor(46, 16+line*13);
    display.println(pdelay);
 
  }
    
}

void info_display() {
  int i;
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  
  enable_display(stepi);
  for (i=0;i<4;i++)  
    delay_display(i,i,"ABCDE", (unsigned long) 1263212840);
  state_display(true);
  
/*  for (i=0;i<5;i++){
    display.setCursor(0, 16+i*13);
    display.println("X:ABCDE 10.123456789");
  }*/
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
