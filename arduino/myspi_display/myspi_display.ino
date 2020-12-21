/*
 * Simple example to show interaction with an FPGA circuit:
 * first configure the FPGA with a bitstream compiled into
 * this program, then use SPI to communicate with the
 * configured circuit:  reading the onboard switch settings
 * and printing them to the USB Serial port whenever they change.
 * Output can be read using the Arduino Serial Monitor tool.
 */

#include <MyStorm.h>
#include <SPI.h>
#include <CmdBuffer.hpp>
#include <CmdCallback.hpp>
#include <CmdParser.hpp>
#include <stdio.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>


CmdCallback<3> cmdCallback;
CmdBuffer<32> myBuffer;
CmdParser     myParser;



char strSet[] = "SET";
char strReq[] = "REQ";
char strTrig[] = "TRIG";

#define CMD_SET_WIDTH   16
#define CMD_SET_DELAY   32
#define CMD_SET_ENABLE  64
#define CMD_SET_MUX     80
#define CMD_REQ_WIDTH  148
#define CMD_REQ_DELAY  160
#define CMD_REQ_VER    176
#define CMD_REQ_ENABLE 192
#define CMD_REQ_MUX    208
#define CMD_REQ_BUSY   224
#define CMD_TRIGGER    240
#define SCREEN_WIDTH   128 // OLED display width, in pixels
#define SCREEN_HEIGHT   64 // OLED display height, in pixels
#define N_DISP           8 // Number of channels to take care on the display 
#define DISPLAY

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
//#define DEBUG

/*
 * FPGA configuration bitstream built from fpga/readswitches.v
 * by icestorm toolchain (see fpga/makefile).
 */
//const byte bitstream[] = {
//#include "ice40bitstream.h"
//};
const int PIN_SPI2_SS = 20;
bool configured;  // true if FPGA has been configured successfully


struct p_set
{
    boolean en;
    unsigned long del;//delay
    unsigned long wid;//width
    char pname[6];//label for the pulser
};

struct p_set p_sets[N_DISP];

void state_display(boolean busy){
  display.setTextSize(2);
  display.setCursor(0,0);
  display.fillRect(0, 0, 48, 15, BLACK);
  if (busy)
    display.println("BUSY");
  else
    display.println("WAIT");
}

void delay_display(){
  byte i, pulser;
  unsigned long md;
  unsigned int ns,us,ms;
  char pdelay[14];
  
  display.setTextSize(1);
  display.fillRect(0, 16, 128, 48, BLACK);
  pulser=0;
  for (i=0;i<N_DISP;i++){
    if ((p_sets[i].en) && (pulser<4)){
      display.drawChar(0, 16+pulser*13,48+i, WHITE, BLACK, 1);//0-9
      md=p_sets[i].del;
      ns=md%100;
      md=(long int)((md-ns)/100);
      us=md%1000;
      md=(long int)((md-us)/1000);
      ms=md%1000;
      md=(long int)((md-ms)/1000);
      sprintf(pdelay, "%3d.%03d%03d%02d0",md,ms,us,ns);
      display.setCursor(46, 16+pulser*13);
      display.println(pdelay);
      Serial.println(p_sets[i].del);
      ++pulser;
    }
  }
}


void enable_display( unsigned long en){
  byte i;// less than 255 channels :)

  display.setTextSize(1);
  display.fillRect(48, 0, 80, 15, BLACK);
  for (i=0;i<N_DISP;i++){
    if (i<4)
      display.setCursor((9+i*3)*6,0);
    else if (i<8)
      display.setCursor((9+(i-4)*3)*6,8);
    if (i<8){
      if (en & (1 << i)){
        display.print("[o]");
        p_sets[i].en = true;
      }
      else{
        display.print("[ ]");
        p_sets[i].en = false;
      }
    }
  }
  delay_display();
}



void spi_long(unsigned long data){
  byte pbuffer[4];
  pbuffer[0] = data;
  pbuffer[1] = data >> 8;
  pbuffer[2] = data >> 16;
  pbuffer[3] = data >> 24;
  SPI2.transfer(pbuffer[3]);
  SPI2.transfer(pbuffer[2]);
  SPI2.transfer(pbuffer[1]);
  SPI2.transfer(pbuffer[0]);
  #if defined(DEBUG)
    Serial.print(pbuffer[3]);Serial.print(":");  Serial.print(pbuffer[2]);Serial.print(":");
    Serial.print(pbuffer[1]);Serial.print(":");  Serial.print(pbuffer[0]);Serial.print(":");
  #endif
}

void trigger(){
  digitalWrite(PIN_SPI2_SS, 0);
  SPI2.transfer((byte)CMD_TRIGGER); SPI2.transfer((byte)0); 
  spi_long(0);
  digitalWrite(PIN_SPI2_SS, 1);
  #if defined(DISPLAY)
    state_display(true);display.display();
  #endif
}

void set_mux(unsigned long pulser, byte output){
  digitalWrite(PIN_SPI2_SS, 0);
  SPI2.transfer((byte)CMD_SET_MUX); SPI2.transfer(output); 
  spi_long(pulser);
  digitalWrite(PIN_SPI2_SS, 1);
}

void set_enable(unsigned long npulser){
  digitalWrite(PIN_SPI2_SS, 0);
  SPI2.transfer((byte)CMD_SET_ENABLE); SPI2.transfer((byte)0); 
  spi_long(npulser);
  digitalWrite(PIN_SPI2_SS, 1);
  #if defined(DISPLAY)
    enable_display(npulser);display.display();
  #endif  
}

void set_width(unsigned long lwidth, byte pulser){
  digitalWrite(PIN_SPI2_SS, 0);
  SPI2.transfer((byte)CMD_SET_WIDTH); SPI2.transfer(pulser); 
  spi_long(lwidth);
  digitalWrite(PIN_SPI2_SS, 1);
}

void set_delay(unsigned long ldelay, byte pulser){
  digitalWrite(PIN_SPI2_SS, 0);
  SPI2.transfer((byte)CMD_SET_DELAY); SPI2.transfer(pulser); 
  spi_long(ldelay);
  digitalWrite(PIN_SPI2_SS, 1);
  p_sets[pulser].del=ldelay;
  #if defined(DISPLAY)
    delay_display();display.display();
  #endif  
}


void req_enable(){
  byte pbuffer[4], i;
  for (i=0;i<2;i=i+1){
    digitalWrite(PIN_SPI2_SS, 0);
    pbuffer[0] = SPI2.transfer((byte)CMD_REQ_ENABLE); // Request Busy
    pbuffer[0] = SPI2.transfer((byte)0);  
    pbuffer[0] = SPI2.transfer((byte)0);
    pbuffer[1] = SPI2.transfer((byte)0);
    pbuffer[2] = SPI2.transfer((byte)0);
    pbuffer[3] = SPI2.transfer((byte)0);
    digitalWrite(PIN_SPI2_SS, 1);
  }
  Serial.print(pbuffer[0]);Serial.print(":");  Serial.print(pbuffer[1]);Serial.print(":");
  Serial.print(pbuffer[2]);Serial.print(":");  Serial.print(pbuffer[3]);Serial.println(".");
}
void req_mux( byte pulser){
  byte pbuffer[4], i;
  for (i=0;i<2;i=i+1){
    digitalWrite(PIN_SPI2_SS, 0);
    pbuffer[0] = SPI2.transfer((byte)CMD_REQ_MUX); // Request mux
    pbuffer[0] = SPI2.transfer((byte)pulser);  
    pbuffer[0] = SPI2.transfer((byte)0);
    pbuffer[1] = SPI2.transfer((byte)0);
    pbuffer[2] = SPI2.transfer((byte)0);
    pbuffer[3] = SPI2.transfer((byte)0);
    digitalWrite(PIN_SPI2_SS, 1);
  }
  Serial.print(pbuffer[0]);Serial.print(":");  Serial.print(pbuffer[1]);Serial.print(":");
  Serial.print(pbuffer[2]);Serial.print(":");  Serial.print(pbuffer[3]);Serial.println(".");
}

boolean req_busy(){
    byte pbuffer[4], i;
    for (i=0;i<2;i=i+1){
      digitalWrite(PIN_SPI2_SS, 0);
      pbuffer[0] = SPI2.transfer((byte)CMD_REQ_BUSY); // Request Busy
      pbuffer[0] = SPI2.transfer((byte)0);  
      pbuffer[0] = SPI2.transfer((byte)0);
      pbuffer[1] = SPI2.transfer((byte)0);
      pbuffer[2] = SPI2.transfer((byte)0);
      pbuffer[3] = SPI2.transfer((byte)0);
      digitalWrite(PIN_SPI2_SS, 1);
  }
  #if defined(DEBUG)
     Serial.println("->Req Busy");
     Serial.println(pbuffer[0]);     
  #endif
  return(pbuffer[0]==1);
}

void req_ver(){
  byte pbuffer[4], i;
  for (i=0;i<2;i=i+1){
    digitalWrite(PIN_SPI2_SS, 0);
    pbuffer[0] = SPI2.transfer((byte)CMD_REQ_VER); // Request Busy
    pbuffer[0] = SPI2.transfer((byte)0);  
    pbuffer[0] = SPI2.transfer((byte)0);
    pbuffer[1] = SPI2.transfer((byte)0);
    pbuffer[2] = SPI2.transfer((byte)0);
    pbuffer[3] = SPI2.transfer((byte)0);
    digitalWrite(PIN_SPI2_SS, 1);
  }
  //N: N_out: Version: Version_low
  Serial.print(pbuffer[0]);Serial.print(":");  Serial.print(pbuffer[1]);Serial.print(":");
  Serial.print(pbuffer[2]);Serial.print(":");  Serial.print(pbuffer[3]);Serial.println(".");
}

void functReq(CmdParser *myParser){
  String param1, param2;
   // ENABLE
  if (myParser->equalCmdParam(1, "ENABLE")) {
    req_enable();
    #if defined(DEBUG)
      Serial.println("->Req pulse enable");
    #endif
  }
  else if (myParser->equalCmdParam(1, "MUX")) {
    param1=myParser->getCmdParam(2);
    req_mux((byte) param1.toInt());
    #if defined(DEBUG)
      Serial.println("->Req pulse enable");
    #endif
  }
  else if (myParser->equalCmdParam(1, "VER")) {
    req_ver();
    #if defined(DEBUG)
      Serial.println("->Req version");
    #endif
  }
  else {
        Serial.println("ERROR:Req command not allowed!");
  } 
}

void functTrig(CmdParser *myParser){
   // Trigger
  trigger();
  #if defined(DEBUG)
    Serial.println("Device triggered!");
  #endif
}

void functSet(CmdParser *myParser){
String param1, param2;  
    // WIDTH
    if (myParser->equalCmdParam(1, "WIDTH")) {
        param1=myParser->getCmdParam(2);
        param2=myParser->getCmdParam(3);
        set_width(param1.toInt(), (byte) param2.toInt());
        #if defined(DEBUG)
          Serial.print(  (byte) param2.toInt());      
          Serial.println("->Set pulse width");
        #endif
    }
    else if (myParser->equalCmdParam(1, "DELAY")) {
        param1=myParser->getCmdParam(2);
        param2=myParser->getCmdParam(3);
        set_delay(param1.toInt(), (byte) param2.toInt());
        #if defined(DEBUG)
          Serial.print(  (byte) param2.toInt());      
          Serial.println("->Set pulse delay");
        #endif
    }
    else if (myParser->equalCmdParam(1, "ENABLE")) {
        param1=myParser->getCmdParam(2);
        param2=myParser->getCmdParam(3);
        set_enable(param1.toInt());
        #if #defined(DEBUG)
          Serial.println("->Set pulse enable");
        #endif
    }
    else if (myParser->equalCmdParam(1, "MUX")) {
        param1=myParser->getCmdParam(2);
        param2=myParser->getCmdParam(3);
        set_mux(param1.toInt(), (byte) param2.toInt() );
        #if #defined(DEBUG)
          Serial.println("->Set mux");
        #endif
    }// Command Unknwon
    else {
        Serial.println("ERROR: SET command not allowed!");
    }
}


unsigned long myTime;

void setup() {
  byte i;
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);

  cmdCallback.addCmd(strSet, &functSet);
  cmdCallback.addCmd(strReq, &functReq);
  cmdCallback.addCmd(strTrig, &functTrig);
  
  //myBuffer.setEcho(true);

  SPI2.begin();
  pinMode(PIN_SPI2_SS, OUTPUT);
  digitalWrite(PIN_SPI2_SS, 1);
  // configure the FPGA
  configured =myStorm.FPGAConfigure((const byte*)0x0801F000, 135100);
  //configured = myStorm.FPGAConfigure(bitstream, sizeof bitstream);
  
  SPI2.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE1));
  #if defined(DISPLAY)
    if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
      Serial.println(F("SSD1306 allocation failed"));
      for(;;);
    }
    delay(2000);
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
  #endif
  
  set_enable(0);
  for (i=0;i<N_DISP;i=i+1){
    set_width(1000, i);
    set_delay(0, i);
    p_sets[i].en=false;
    p_sets[i].wid=1000;
    p_sets[i].del=0;
  }
  myTime=millis();
}


void loop() {
  static int toggle = 0;
  static int prevswitches = -1;
  
  if (!configured) {
    // if FPGConfig failed, just flash LED5 at 1Hz
    digitalWrite(LED_BUILTIN, toggle);
    toggle = ~toggle;
    delay(500);
    return;
  }
  
  cmdCallback.updateCmdProcessing(&myParser, &myBuffer, &Serial);
 
  //SPI2.endTransaction(); //This is not called!
  //Check every now an then if busy
  if ((millis()-myTime)>100){
    state_display(req_busy());display.display();
    myTime=millis();
  }

  //delay(100);
}
