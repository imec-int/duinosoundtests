#include <SPI.h>
#include <SPIFlash.h>

SerialBuffer sBuffer;
boolean lenTransferred[3] ={0};
boolean transferOngoing = 0, idTransferred = 0;
volatile boolean bufferNotEmpty = 0;
volatile boolean paused = 0;
volatile boolean writeStarted = 0;
byte id;
uint32_t length = 0;
volatile uint32_t counter = 0; //32 bit processor so atomic operations
FlashBuffer* flashBuffer;




void setup() {
  transferOngoing = false;
  Serial.begin(57600);
//  FlashBuffer fb(2);
//  flashBuffer = &fb; //or with new; but I guess this stuff stays alive the whole time ->not working with callback; address changes if you change field!?
//  fb.print();
  flashBuffer = new FlashBuffer(2);
  flashBuffer->setResumeCallback(resume); //resume will be executed when there is space available in the buffer
}

//void pause() {
//  
//}
void resume() {
  if(paused) {
    Serial.write((byte) (counter >> 16));
    Serial.write((byte) (counter >> 8) & 0xFF);
    Serial.write((byte) (counter & 0xFF));
    paused = 0;
  }
//  Serial.write((byte)0);
//  Serial.write((byte)1);
//  Serial.write((byte)2); 
}

void loop() {
  // put your main code here, to run repeatedly:
//  Serial.write(6);
//Serial.write("lala");

  if(bufferNotEmpty && !writeStarted) { //initialisation variable
    writeStarted = 1;
//    fb.print();
    flashBuffer->writeItemToFlash(id, length, sBuffer);
    sBuffer.reset();
    bufferNotEmpty = 0;
//    byte value = sBuffer.remove();
//    if(value != -1) {
//      Serial.write(value);
//      if(paused) {
//        sBuffer.sendResumeOverSerial();
//        paused = 0;
//      }
//    }
  }
}



void serialEvent(void){
  int ch;
  ch = Serial.read();
  if(!transferOngoing) {
    if(!idTransferred) {
      id = (byte) ch;
//      Serial.write(id);
      idTransferred = 1;
    } else { //only execute this when id already initialized
      for(byte i = 0; i < 3; i++) {
        if(!lenTransferred[i]) {
          length |= ch << 16 - i * 8;
          lenTransferred[i] = 1;
          if(i == 2) transferOngoing = 1;
          break;
        }
      }
    }
  } else {
//    Serial.write("lala");
//    Serial.write((byte)ch);
//    if(sBuffer.add((byte) ch) == -1) sBuffer.sendPauseOverSerial();
      if(!paused) {
        if(sBuffer.add((byte) ch) == -1) {
          paused = 1;
//        send 3 zero's to stop the writing on the other side (to resume it's also 3 bytes)
          Serial.write((byte) 0x00);
          Serial.write((byte) 0x00);
          Serial.write((byte) 0x00);
          
        } else {
          counter++;
        }
        if(counter==256) bufferNotEmpty = 1;
      }
  }
//    Serial.write(id);
//    Serial.write(length >> 16);
//    Serial.write((length >> 8) /*& 0xFF*/); //mask is not necessary; lowest byte is picked automatically
//    Serial.write(length /*& 0xFF*/);  
}
