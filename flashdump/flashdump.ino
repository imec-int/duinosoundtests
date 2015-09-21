// **********************************************************************************
// This sketch is an example of using the SPIFlash library with a Moteino
// that has an onboard SPI Flash chip. This sketch listens to a few serial commands
// Hence type the following commands to interact with the SPI flash memory array:
// - 'd' dumps the first 256bytes of the flash chip to screen
// - 'e' erases the entire memory chip
// - 'i' print manufacturer/device ID
// - [0-9] writes a random byte to addresses [0-9] (either 0xAA or 0xBB)
// Get the SPIFlash library from here: https://github.com/LowPowerLab/SPIFlash
// **********************************************************************************
// Copyright Felix Rusu, LowPowerLab.com
// Library and code by Felix Rusu - felix@lowpowerlab.com
// **********************************************************************************
// License
// **********************************************************************************
// This program is free software; you can redistribute it 
// and/or modify it under the terms of the GNU General    
// Public License as published by the Free Software       
// Foundation; either version 3 of the License, or        
// (at your option) any later version.                    
//                                                        
// This program is distributed in the hope that it will   
// be useful, but WITHOUT ANY WARRANTY; without even the  
// implied warranty of MERCHANTABILITY or FITNESS FOR A   
// PARTICULAR PURPOSE. See the GNU General Public        
// License for more details.                              
//                                                        
// You should have received a copy of the GNU General    
// Public License along with this program.
// If not, see <http://www.gnu.org/licenses/>.
//                                                        
// Licence can be viewed at                               
// http://www.gnu.org/licenses/gpl-3.0.txt
//
// Please maintain this license information along with authorship
// and copyright notices in any redistribution of this code
// **********************************************************************************


#include <SPIFlash.h>    //get it here: https://github.com/LowPowerLab/SPIFlash
#include <SPI.h>

#define SERIAL_BAUD      115200
char input = 0;
//////////////////////////////////////////
// flash(SPI_CS, MANUFACTURER_ID)
// SPI_CS          - CS pin attached to SPI flash chip (8 in case of Moteino)
// MANUFACTURER_ID - OPTIONAL, 0x1F44 for adesto(ex atmel) 4mbit flash
//                             0xEF30 for windbond 4mbit flash
//////////////////////////////////////////
SPIFlash flash(2, 0x140);
FlashBuffer* fb;
SerialBuffer sb;
volatile uint32_t length = 0, teller = 0;
uint32_t lengte;
boolean readFlash = 0;
uint8_t brol[1024];
uint32_t stops=0;
int timest=0;
void setup(){
  Serial.begin(SERIAL_BAUD);
  Serial.print("Start...");
  fb = new FlashBuffer(2);

  if (flash.initialize())
    Serial.println("Init OK!");
  else
    Serial.println("Init FAIL!");
    fb->print();
Serial.println(length= fb->getItemLength(6));
// delay(1000);
// timest = millis();
// generateInterrupt();
    
 
}

void printHex(uint8_t byt) {
  char tmp[16];
  sprintf(tmp, "%.2X",byt); 
  Serial.print(tmp);
}

void generateInterrupt() {
  NRF_TIMER2->TASKS_STOP = 1;                                     // Stop timer
  NRF_TIMER2->MODE = TIMER_MODE_MODE_Timer;                        // sets the timer to TIME mode (instead of counter mode)
  NRF_TIMER2->BITMODE = TIMER_BITMODE_BITMODE_16Bit;               // with BLE only Timer 1 and Timer 2 and that too only in 16bit mode
  NRF_TIMER2->PRESCALER = 3;                                     // 16M /2 ^ 3timer     ->2M                                                             
  NRF_TIMER2->TASKS_CLEAR = 1;                                     // Clear timer
  NRF_TIMER2->CC[0] = 249;                                        //CC[0] register holds interval count value i.e your desired cycle for PWM. 8MHz / 250 -> 32kHz delen door 4 = 8kHz sample rate audio
//  NRF_TIMER2->CC[1] = 0;  //set this in interrupt to modify duty cycle
  NRF_TIMER2->INTENSET = (TIMER_INTENSET_COMPARE0_Enabled << TIMER_INTENSET_COMPARE0_Pos);                                     // Enable COMPARE0 Interrupt
  NRF_TIMER2->SHORTS = (TIMER_SHORTS_COMPARE0_CLEAR_Enabled << TIMER_SHORTS_COMPARE0_CLEAR_Pos);                             // Count then Complete mode enabled -> ik denk dat dit terug op 0 zet?? Idd!
  attachInterrupt(TIMER2_IRQn, TIMER2_Interrupt);                                                                            // also used in variant.cpp in the RFduino2.2 folder to configure the RTC1 
  NRF_TIMER2->TASKS_START = 1;                  
}

void TIMER2_Interrupt() {
  NRF_TIMER2->EVENTS_COMPARE[0] = 0;
//  Serial.print(length);
//  if(teller < length /*&& millis() > timest + 8000*/) {
//    Serial.print(sb.numberOfElements());
//    int value = sb.remove();
    
//    if(value != -1) {
      if(teller < 1024) {
        brol[teller] = fb->readItemAtIndex(3, teller);  
        teller++;
      }
//    } else if(teller>0) stops++;
//  }
}

void loop(){
  // Handle serial input (to allow basic DEBUGGING of FLASH chip)
  // ie: display first 256 bytes in FLASH, erase chip, write bytes at first 10 positions, etc
  if (Serial.available() > 0) {
    input = Serial.read();
    if (input == 'd') //d=dump flash area
    {
      Serial.println("Flash content:");
      int counter = 87038 ;

      while(counter<87300){
        printHex(flash.readByte(counter++));
      }
      
      Serial.println();
    }
    else if (input == 'e')
    {
      Serial.print("Erasing Flash chip ... ");
      flash.chipErase();
      while(flash.busy());
      Serial.println("DONE");
    }
    else if (input == 'i')
    {
      Serial.print("DeviceID: ");
      Serial.println(flash.readDeviceId(), HEX);
    }
    else if (input >= 48 && input <= 57) //0-9
    {
      Serial.print("\nWriteByte("); Serial.print(input); Serial.print(")");
      flash.writeByte(input-48, millis()%2 ? 0xaa : 0xbb);
    }
  }

  // Periodically blink the onboard LED while listening for serial commands
//  if ((int)(millis()/500) > lastPeriod)
//  {
//    lastPeriod++;
//    pinMode(LED, OUTPUT);
//    digitalWrite(LED, lastPeriod%2);
//  }
//  if(!readFlash) {
//    Serial.println("reading flash...");
//    readFlash = 1;
//    delay(1000);
//    generateInterrupt();
//    fb->fastReadItemFromFlash(3, lengte, sb);
//    Serial.println("read flash");
//    for(uint32_t i = 0; i < 1024; i++) {
//      printHex(brol[i]);
//    }
//    Serial.println();
//    Serial.print("stops: ");
//    Serial.print(stops);
//  }
if(teller==1024) {
  if(!readFlash) {
    readFlash = 1;
  for(uint16_t i = 0; i < 1024; i++) {
    printHex(brol[i]);
  }
  }
}
}
