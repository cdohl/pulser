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

CmdCallback<3> cmdCallback;

CmdBuffer<32> myBuffer;
CmdParser     myParser;

char strSet[] = "SET";
char strReq[] = "REQ";
char strTrig[] = "TRIG";


#define CMD_SET_WIDTH  16
#define CMD_SET_DELAY  32
#define CMD_SET_ENABLE 64
#define CMD_SET_MUX    80
#define CMD_REQ_ENABLE 192
#define CMD_TRIGGER    240
#define DEBUG

/*
 * FPGA configuration bitstream built from fpga/readswitches.v
 * by icestorm toolchain (see fpga/makefile).
 */
//const byte bitstream[] = {
//#include "ice40bitstream.h"
//};
const int PIN_SPI2_SS = 20;
bool configured;  // true if FPGA has been configured successfully

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
}

void set_mux(unsigned long pulser, byte output){
  digitalWrite(PIN_SPI2_SS, 0);
  SPI2.transfer((byte)CMD_SET_MUX); SPI2.transfer(output); 
  spi_long(pulser);
  digitalWrite(PIN_SPI2_SS, 1);
}

void set_enable(unsigned long pulser){
  digitalWrite(PIN_SPI2_SS, 0);
  SPI2.transfer((byte)CMD_SET_ENABLE); SPI2.transfer((byte)0); 
  spi_long(pulser);
  digitalWrite(PIN_SPI2_SS, 1);
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
}

void set_trigger(){
  digitalWrite(PIN_SPI2_SS, 0);
  SPI2.transfer((byte)CMD_TRIGGER); SPI2.transfer((byte)0); 
  spi_long((unsigned long) 0);
  digitalWrite(PIN_SPI2_SS, 1);
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

void functReq(CmdParser *myParser){
  String param1, param2;
   // ENABLE
  if (myParser->equalCmdParam(1, "ENABLE")) {
    req_enable();
    #if defined(DEBUG)
      Serial.println("->Req pulse enable");
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
    // Command Unknwon
    else {
        Serial.println("ERROR: SET command not allowed!");
    }
}

void setup() {
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
  set_width(10000000, 0);
  set_width(20000000, 1);
  set_width(30000000, 2);
  set_width(40000000, 3);
  set_width(50000000, 4);
  set_width(60000000, 5);
  set_width(70000000, 6);
  set_width(80000000, 7);

  set_delay(00000000, 7);
  set_delay(10000000, 6);
  set_delay(20000000, 5);
  set_delay(30000000, 4);
  set_delay(40000000, 3);
  set_delay(50000000, 2);
  set_delay(60000000, 1);
  set_delay(70000000, 0);
  
/*  
  set_mux(1,0);  //bit & of pulser, output
  set_mux(2,1);
  set_mux(4,2);
  set_mux(4,3);
  set_mux(4,4);
  set_mux(4,5);
  set_mux(2,6);
  set_mux(1,7);
*/
  //Enable all channels
  set_enable(255);
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
  
  cmdCallback.loopCmdProcessing(&myParser, &myBuffer, &Serial);
 
  //SPI2.endTransaction(); //This is not called!
  delay(100);
  
}
