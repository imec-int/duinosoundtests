// Copyright (c) 2013-2015 by Felix Rusu, LowPowerLab.com
// SPI Flash memory library for arduino/moteino.
// This works with 256byte/page SPI flash memory
// For instance a 4MBit (512Kbyte) flash chip will have 2048 pages: 256*2048 = 524288 bytes (512Kbytes)
// Minimal modifications should allow chips that have different page size but modifications
// DEPENDS ON: Arduino SPI library
// > Updated Jan. 5, 2015, TomWS1, modified writeBytes to allow blocks > 256 bytes and handle page misalignment.
// > Updated Feb. 26, 2015 TomWS1, added support for SPI Transactions (Arduino 1.5.8 and above)
// > Selective merge by Felix after testing in IDE 1.0.6, 1.6.4
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

#include <SPIFlash.h>

volatile byte SerialBuffer::ring_data[RING_SIZE]={0};

int SerialBuffer::add(byte b) {
  uint16_t next_head = (ring_head + 1) & (RING_SIZE - 1); //or % 512, but less compute-intensive.
  if(next_head != ring_tail) {
    // there is room
    ring_data[ring_head] = b;
    ring_head = next_head;
    return 0;
  } else {
    return -1;
  }
}

int SerialBuffer::remove(void) {
  if(ring_head != ring_tail) {
    int b = ring_data[ring_tail];
    ring_tail = (ring_tail + 1 ) & (RING_SIZE - 1);
    return b;
  } else {
    return -1;
  }
}

void SerialBuffer::reset(void) {
  ring_tail = ring_head;
}

uint16_t SerialBuffer::numberOfElements(void) {
  if(ring_head >= ring_tail) {
    return ring_head - ring_tail;
  } else {
    return RING_SIZE - ring_tail + ring_head;
  }
}


FlashBuffer::FlashBuffer(uint8_t pin) : flash(pin, 0x140) {
  // read bytes at block beginnings and check header
  flash.initialize();
  currentItemAddress = 0;
  isContinuationOfItem = false;
  uint8_t blockHeaders[16];
  for(uint8_t i = 0; i < 16; i++) {
    blockHeaders[i] = flash.readByte((uint32_t)i << 16 ); // shift to block addresses
  }

  for (latestBlockId = 0; latestBlockId < 15; latestBlockId++) {
    if (((blockHeaders[latestBlockId] + 1) & 31) != blockHeaders[latestBlockId + 1]) { // see https://www.approxion.com/?p=199, no problem with empty blocks ff, since latest block comes before empty block
      // block counter wraps around at 32 > 16; difference with 255 (FF) is clear
      break;
    }
  }
  blockCounter = blockHeaders[latestBlockId] > 31 ? 0 : blockHeaders[latestBlockId];
  checkForLatestItemAddress(((uint32_t)latestBlockId << 16) + 1); //add 1 to skip block header
  // normally we're on the last iteration of the index table if we look at the last item
  uint8_t indexTableWithHeader[249]; //if we're on a block boundary we already skipped the header, so 35 * 7 + itemHeader (=4)
  flash.readBytes(latestItemAddress, indexTableWithHeader, 4);
  if(indexTableWithHeader[0] == 0x7F) { //largest number with most significant bit (=no partial item) 0 -> this is fixed id for index table by agreement
    memcpy(indexTable, indexTableWithHeader + 4, 245);
  }
}


uint16_t FlashBuffer::checkForLatestItemAddress(uint32_t address) {
  uint8_t itemHeader[4]; //1 byte id; 3 bytes length. Indicate partial items in most significant bit -> still 127 items possible
  flash.readBytes(address, itemHeader, 4);
  if(itemHeader[0] == 0xFF) { //still empty
    // next case only occurs when mem is empty; otherwise latestBlockId would still be the previous blockHeaders
    if(currentItemAddress == 0 && !isContinuationOfItem) {//
      latestItemAddress = 0;
      nextPageId = 0;
      return latestItemAddress;
    } else if(currentItemAddress == 0 && isContinuationOfItem){
       return checkForLatestItemAddress(((uint32_t)(latestBlockId - 1 & 15) << 16) + 1);
    } else {
      latestItemAddress = currentItemAddress;
      return latestItemAddress;
    }
  } else{
    isContinuationOfItem = itemHeader[0] >> 7; // long items can span multiple blocks; in that case backtrack on previous block
    if(!isContinuationOfItem) {
      currentItemAddress = address;
    }
    uint32_t length = itemHeader[1] << 16 | itemHeader[2] << 8 | itemHeader[3];
    uint32_t nextAddress = address + 4 + length;
    uint32_t realNextAddress = nextAddress;
    uint8_t i = 0;
    while(realNextAddress > (uint32_t)latestBlockId + 1 + i << 16) { //check for block crosses
      realNextAddress = nextAddress + 5 * (i + 1); // block header plus repeat file header
    }
    nextPageId = (realNextAddress >> 8) + 1 * ((realNextAddress & 255) != 0); //divide by 256 and if remainder != 0 add 1
    realNextAddress = (uint32_t)nextPageId << 8;
    if((realNextAddress & 65535) == 0) realNextAddress++; //add byte for block header if it's on a block
    return checkForLatestItemAddress(realNextAddress);
  }
}

void FlashBuffer::print(void) {

  Serial.print("latestBlockId: ");
  Serial.println(latestBlockId);
  Serial.print("blockCounter: ");
  Serial.println(blockCounter);
  Serial.print("latestItemAddress: ");
  Serial.println(latestItemAddress);
  Serial.print("currentItemAddress: ");
  Serial.println(currentItemAddress);
  Serial.print("nextPageId: ");
  Serial.println(nextPageId);
}

void FlashBuffer::setResumeCallback(void (*aFunc) ()) {
  resumeCallback = aFunc;
}

void FlashBuffer::setPauseCallback(void (*aFunc) ()) {
  pauseCallback = aFunc;
}

void FlashBuffer::writeItemToFlash(uint8_t id, uint32_t length, SerialBuffer &serialBuffer) {
    // we start at beginning of page
  uint32_t startAddress = (uint32_t)nextPageId << 8;
  uint32_t addressInIndex = startAddress;
  uint16_t n;
  boolean onNewBlock = false;
  if((startAddress & 65535) == 0) {
    onNewBlock = true;
    if(flash.readByte(startAddress) != 0xFF) {//not an empty block -> erase it
      flash.blockErase64K(startAddress);
      // erase elements from array if necessary
      for(uint8_t i = 0; i < 35; i++) {
        uint32_t itemAddress = indexTable[i * 7 + 1] << 16 | indexTable[i * 7 + 2] << 8 | indexTable[i * 7 + 3];
        if(itemAddress != 0xFFFFFF && startAddress <=  itemAddress && itemAddress < startAddress + 65536) {
          for(uint8_t j = 0; j < 7; j++) {
            indexTable[i * 7 + j] = 0xFF;
          }
        }
      }
    }
  }
  uint16_t maxBytes = 256;  // aligned with page
  boolean firstPage = onNewBlock? false : true; //on new block have to rewrite headers anyway
  while (length > 0)
  {
    // n: number of bytes that will be written to the page
    n = (length + 5 * onNewBlock + 4 * firstPage <= maxBytes) ? length : maxBytes - 5 * onNewBlock - 4 * firstPage; //how much is written of the file itself without headers.
    flash.command(SPIFLASH_BYTEPAGEPROGRAM, true);  // Byte/Page Program
    SPI.transfer(startAddress >> 16);
    SPI.transfer(startAddress >> 8);
    SPI.transfer(startAddress);
    if(onNewBlock) {
      blockCounter = blockCounter + 1 & 31;
      latestBlockId = latestBlockId + 1 & 15;
      SPI.transfer(blockCounter);
    }
    if(firstPage || onNewBlock) {
      SPI.transfer(id);
      SPI.transfer(length >> 16);
      SPI.transfer(length >> 8);
      SPI.transfer(length);
    }

    for (uint16_t i = 0; i < n; i++)  {
      int readValue;
      while((readValue = serialBuffer.remove()) == -1) {
        delay(200);
      }
      SPI.transfer((uint8_t) readValue);
    }
    flash.unselect();

    startAddress+= n + 5 * onNewBlock + 4 * firstPage;  // adjust the addresses and remaining bytes by what we've just transferred.
    length -= n;
    if((startAddress & 65535) == 0) {
      onNewBlock = true;
      id = id | 0x80; // set most significant bit to 1 for partials
      if(flash.readByte(startAddress) != 0xFF) {//not an empty block -> erase it
        flash.blockErase64K(startAddress);
        // erase elements from array if necessary
        for(uint8_t i = 0; i < 35; i++) {
          uint32_t itemAddress = indexTable[i * 7 + 1] << 16 | indexTable[i * 7 + 2] << 8 | indexTable[i * 7 + 3];
          if(itemAddress != 0xFFFFFF && startAddress <=  itemAddress && itemAddress < startAddress + 65536) {
            for(uint8_t j = 0; j < 7; j++) {
              indexTable[i * 7 + j] = 0xFF;
            }
          }
        }
      }
    }
    else {
      onNewBlock = false;
    }
    firstPage = false;
    if(serialBuffer.numberOfElements() < RING_SIZE / 2) { // make sure there is some space in the buffer before allowing refill
      resumeCallback();
    } //else {
      //pauseCallback(); //not necessary to do this within else, but pause will only happen if buffer is almost full
    //}
  }
  // write index table on next page -> our limit for the number of items, since this makes everything easier :-p
  // page has 256 bytes, 4 are taken by its own header. Maybe one by a block header (if on new block)
  // 7 bytes per indexed item (id(1)|adres(3)|length(3)) -> room for (256-5) / 7 items = 35
  boolean slotAvailable = false;
  for(uint8_t i = 0; i < 35; i++) {
    // check for empty slot or overwrite existing entry if already present
    if(indexTable[i * 7] == 0xFF || indexTable[i * 7] == id) {
      indexTable[i * 7] = id;
      for(uint8_t j = 0; j < 3; j++) {
        indexTable[i * 7 + 1 + j] = addressInIndex >> 16 - j * 8;
        indexTable[i * 7 + 4 + j] = length >> 16 - j * 8;
      }
      slotAvailable = true;
      break;
    }
  }
  if(!slotAvailable) {
    memcpy(indexTable, indexTable + 7, 238); // shift array one element to the left by copying all but one items
    indexTable[238] = id;
    for(uint8_t j = 0; j < 3; j++) {
      indexTable[238 + 1 + j] = addressInIndex >> 16 - j * 8;
      indexTable[238 + 4 + j] = length >> 16 - j * 8;
    }
  }
  // write indexTable to next page!
  // check again for block boundary; change counters if necessary, nextPageId...
  if((startAddress & 255) != 0) {
    // start on next page
    startAddress = (startAddress / 256 + 1) * 256;
  }
  flash.command(SPIFLASH_BYTEPAGEPROGRAM, true);  // Byte/Page Program
  SPI.transfer(startAddress >> 16);
  SPI.transfer(startAddress >> 8);
  SPI.transfer(startAddress);
  if((startAddress & 65535) == 0) {
    // only need one page; so block erase only necessary if now on start block
    if(flash.readByte(startAddress) != 0xFF) {
      flash.blockErase64K(startAddress);
      // erase elements from array if necessary
      for(uint8_t i = 0; i < 35; i++) {
        uint32_t itemAddress = indexTable[i * 7 + 1] << 16 | indexTable[i * 7 + 2] << 8 | indexTable[i * 7 + 3];
        if(startAddress <=  itemAddress && itemAddress < startAddress + 65536) {
          for(uint8_t j = 0; j < 7; j++) {
            indexTable[i * 7 + j] = 0xFF;
          }
        }
      }
    }
    blockCounter = blockCounter + 1 & 31;
    latestBlockId = latestBlockId + 1 & 15;
    SPI.transfer(blockCounter);
  }
  SPI.transfer(0x7F);
  // next 3 are the length of the indexTable
  SPI.transfer(0);
  SPI.transfer(0);
  SPI.transfer(245);
  for(uint8_t i = 0; i < 245; i++) {
    SPI.transfer(indexTable[i]);
  }
  flash.unselect();
  nextPageId = startAddress / 256 + 1;
}

int FlashBuffer::readItemFromFlash(uint8_t id, uint32_t &length, SerialBuffer &serialBuffer) {
  int result = -1;
  uint32_t address = 0;
  length = 0;
  for(uint8_t i = 34; i >= 0; i--) { //loop backwards; more recent items were added to the back
    if(indexTable[i * 7] == id) {
      for(uint8_t j = 0; j < 3; j++) {
        address |= indexTable[i * 7 + 1 + j] << 16 - j * 8;
        length |= indexTable[i * 7 + 4 + j] << 16 - j * 8;
      }
      break;
    }
  }
  if(address == 0 && length == 0) return -1; // address itself can be zero; first sector
  if((address & 65535) == 0) address++; //skip blockheader
  uint8_t idInFlash = flash.readByte(address);
  if(idInFlash != id) return result; //check whether we're at the correct item if not return -1.
  // no speed advantage in using fastread so read byte per byte since rfduino is slow
  // we already got length from indexTable so skip it in flash
  else result = id;
  address += 4;
  uint32_t bytesRead = 0;
  while(bytesRead < length) {
    if((address & 65535) == 0) address += 5; //skip blockheader and rewrite of item header
    uint8_t data = flash.readByte(address);
    while(serialBuffer.add(data) == -1) {
      delay(200);
    }
    address++;
    bytesRead++;
  }
  return result;
}

uint8_t SPIFlash::UNIQUEID[8];

/// IMPORTANT: NAND FLASH memory requires erase before write, because
///            it can only transition from 1s to 0s and only the erase command can reset all 0s to 1s
/// See http://en.wikipedia.org/wiki/Flash_memory
/// The smallest range that can be erased is a sector (4K, 32K, 64K); there is also a chip erase command

/// Constructor. JedecID is optional but recommended, since this will ensure that the device is present and has a valid response
/// get this from the datasheet of your flash chip
/// Example for Atmel-Adesto 4Mbit AT25DF041A: 0x1F44 (page 27: http://www.adestotech.com/sites/default/files/datasheets/doc3668.pdf)
/// Example for Winbond 4Mbit W25X40CL: 0xEF30 (page 14: http://www.winbond.com/NR/rdonlyres/6E25084C-0BFE-4B25-903D-AE10221A0929/0/W25X40CL.pdf)
SPIFlash::SPIFlash(uint8_t slaveSelectPin, uint16_t jedecID) {
  _slaveSelectPin = slaveSelectPin;
  _jedecID = jedecID;
}

/// Select the flash chip
void SPIFlash::select() {
  //save current SPI settings
#ifndef SPI_HAS_TRANSACTION
  // noInterrupts();
#endif
  // _SPCR = SPCR;
  // _SPSR = SPSR;

#ifdef SPI_HAS_TRANSACTION
  SPI.beginTransaction(_settings);
#else
  // set FLASH SPI settings
  SPI.setDataMode(SPI_MODE0);
  SPI.setBitOrder(MSBFIRST);
  // SPI.setClockDivider(SPI_CLOCK_DIV4); //decided to slow down from DIV2 after SPI stalling in some instances, especially visible on mega1284p when RFM69 and FLASH chip both present
  // setClockDivider has empty implementation on RFduino but default is 4Mhz (equals DIV4)
  SPI.begin();
#endif
  digitalWrite(_slaveSelectPin, LOW);
}

/// UNselect the flash chip
void SPIFlash::unselect() {
  digitalWrite(_slaveSelectPin, HIGH);
  //restore SPI settings to what they were before talking to the FLASH chip
#ifdef SPI_HAS_TRANSACTION
  SPI.endTransaction();
#else
  // interrupts();
#endif
  // SPCR = _SPCR;
  // SPSR = _SPSR;
}

/// setup SPI, read device ID etc...
boolean SPIFlash::initialize()
{
  // _SPCR = SPCR;
  // _SPSR = SPSR;
  pinMode(_slaveSelectPin, OUTPUT);
#ifdef SPI_HAS_TRANSACTION
  _settings = SPISettings(4000000, MSBFIRST, SPI_MODE0);
#endif

  unselect();
  wakeup();

  if (_jedecID == 0 || readDeviceId() == _jedecID) {
    command(SPIFLASH_STATUSWRITE, true); // Write Status Register
    SPI.transfer(0);                     // Global Unprotect
    unselect();
    return true;
  }
  return false;
}

/// Get the manufacturer and device ID bytes (as a short word)
uint16_t SPIFlash::readDeviceId()
{
#if defined(__AVR_ATmega32U4__) // Arduino Leonardo, MoteinoLeo
  command(SPIFLASH_IDREAD); // Read JEDEC ID
#else
  select();
  SPI.transfer(SPIFLASH_IDREAD);
#endif
  uint16_t jedecid = SPI.transfer(0) << 8;
  jedecid |= SPI.transfer(0);
  unselect();
  return jedecid;
}

/// Get the 64 bit unique identifier, stores it in UNIQUEID[8]. Only needs to be called once, ie after initialize
/// Returns the byte pointer to the UNIQUEID byte array
/// Read UNIQUEID like this:
/// flash.readUniqueId(); for (uint8_t i=0;i<8;i++) { Serial.print(flash.UNIQUEID[i], HEX); Serial.print(' '); }
/// or like this:
/// flash.readUniqueId(); uint8_t* MAC = flash.readUniqueId(); for (uint8_t i=0;i<8;i++) { Serial.print(MAC[i], HEX); Serial.print(' '); }
uint8_t* SPIFlash::readUniqueId()
{
  command(SPIFLASH_MACREAD);
  SPI.transfer(0);
  SPI.transfer(0);
  SPI.transfer(0);
  SPI.transfer(0);
  for (uint8_t i=0;i<8;i++)
    UNIQUEID[i] = SPI.transfer(0);
  unselect();
  return UNIQUEID;
}

/// read 1 byte from flash memory
uint8_t SPIFlash::readByte(uint32_t addr) {
  command(SPIFLASH_ARRAYREADLOWFREQ);
  SPI.transfer(addr >> 16);
  SPI.transfer(addr >> 8);
  SPI.transfer(addr);
  uint8_t result = SPI.transfer(0);
  unselect();
  return result;
}

/// read unlimited # of bytes
void SPIFlash::readBytes(uint32_t addr, void* buf, uint16_t len) {
  command(SPIFLASH_ARRAYREAD);
  SPI.transfer(addr >> 16);
  SPI.transfer(addr >> 8);
  SPI.transfer(addr);
  SPI.transfer(0); //"dont care"
  for (uint16_t i = 0; i < len; ++i)
    ((uint8_t*) buf)[i] = SPI.transfer(0);
  unselect();
}

/// Send a command to the flash chip, pass TRUE for isWrite when its a write command
void SPIFlash::command(uint8_t cmd, boolean isWrite){
// #if defined(__AVR_ATmega32U4__) // Arduino Leonardo, MoteinoLeo
//   DDRB |= B00000001;            // Make sure the SS pin (PB0 - used by RFM12B on MoteinoLeo R1) is set as output HIGH!
//   PORTB |= B00000001;
// #endif
  if (isWrite)
  {
    command(SPIFLASH_WRITEENABLE); // Write Enable
    unselect();
  }
  //wait for any write/erase to complete
  //  a time limit cannot really be added here without it being a very large safe limit
  //  that is because some chips can take several seconds to carry out a chip erase or other similar multi block or entire-chip operations
  //  a recommended alternative to such situations where chip can be or not be present is to add a 10k or similar weak pulldown on the
  //  open drain MISO input which can read noise/static and hence return a non 0 status byte, causing the while() to hang when a flash chip is not present
  while(busy());
  select();
  SPI.transfer(cmd);
}

/// check if the chip is busy erasing/writing
boolean SPIFlash::busy()
{
  /*
  select();
  SPI.transfer(SPIFLASH_STATUSREAD);
  uint8_t status = SPI.transfer(0);
  unselect();
  return status & 1;
  */
  return readStatus() & 1;
}

/// return the STATUS register
uint8_t SPIFlash::readStatus()
{
  select();
  SPI.transfer(SPIFLASH_STATUSREAD);
  uint8_t status = SPI.transfer(0);
  unselect();
  return status;
}


/// Write 1 byte to flash memory
/// WARNING: you can only write to previously erased memory locations (see datasheet)
///          use the block erase commands to first clear memory (write 0xFFs)
void SPIFlash::writeByte(uint32_t addr, uint8_t byt) {
  command(SPIFLASH_BYTEPAGEPROGRAM, true);  // Byte/Page Program
  SPI.transfer(addr >> 16);
  SPI.transfer(addr >> 8);
  SPI.transfer(addr);
  SPI.transfer(byt);
  unselect();
}

/// write multiple bytes to flash memory (up to 64K)
/// WARNING: you can only write to previously erased memory locations (see datasheet)
///          use the block erase commands to first clear memory (write 0xFFs)
/// This version handles both page alignment and data blocks larger than 256 bytes.
///
void SPIFlash::writeBytes(uint32_t addr, const void* buf, uint16_t len) {
  uint16_t n;
  uint16_t maxBytes = 256-(addr%256);  // force the first set of bytes to stay within the first page
  uint16_t offset = 0;
  while (len>0)
  {
    n = (len<=maxBytes) ? len : maxBytes;
    command(SPIFLASH_BYTEPAGEPROGRAM, true);  // Byte/Page Program
    SPI.transfer(addr >> 16);
    SPI.transfer(addr >> 8);
    SPI.transfer(addr);

    for (uint16_t i = 0; i < n; i++)
      SPI.transfer(((uint8_t*) buf)[offset + i]);
    unselect();

    addr+=n;  // adjust the addresses and remaining bytes by what we've just transferred.
    offset +=n;
    len -= n;
    maxBytes = 256;   // now we can do up to 256 bytes per loop
  }
}

/// erase entire flash memory array
/// may take several seconds depending on size, but is non blocking
/// so you may wait for this to complete using busy() or continue doing
/// other things and later check if the chip is done with busy()
/// note that any command will first wait for chip to become available using busy()
/// so no need to do that twice
void SPIFlash::chipErase() {
  command(SPIFLASH_CHIPERASE, true);
  unselect();
}

/// erase a 4Kbyte block
void SPIFlash::blockErase4K(uint32_t addr) {
  command(SPIFLASH_BLOCKERASE_4K, true); // Block Erase
  SPI.transfer(addr >> 16);
  SPI.transfer(addr >> 8);
  SPI.transfer(addr);
  unselect();
}

/// erase a 32Kbyte block
void SPIFlash::blockErase32K(uint32_t addr) {
  command(SPIFLASH_BLOCKERASE_32K, true); // Block Erase
  SPI.transfer(addr >> 16);
  SPI.transfer(addr >> 8);
  SPI.transfer(addr);
  unselect();
}

/// erase a 64Kbyte block
void SPIFlash::blockErase64K(uint32_t addr) {
  command(SPIFLASH_BLOCKERASE_64K, true); // Block Erase
  SPI.transfer(addr >> 16);
  SPI.transfer(addr >> 8);
  SPI.transfer(addr);
  unselect();
}

void SPIFlash::sleep() {
  command(SPIFLASH_SLEEP);
  unselect();
}

void SPIFlash::wakeup() {
  command(SPIFLASH_WAKE);
  unselect();
}

/// cleanup
void SPIFlash::end() {
  SPI.end();
}

// void Test::setCallback(void (*aFunc)()) {
//   callback = aFunc;
// }

// void Test::fireCallback() {
//   // Serial.print('vlo');
//   callback();
// }