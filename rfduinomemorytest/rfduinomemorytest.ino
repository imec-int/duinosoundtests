/*
 Copyright (c) 2013 OpenSourceRF.com.  All right reserved.

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 See the GNU Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

/*
This sketch demonstrates how to read and write a structure
to flash memory on the RFduino.

This example is located under RFduinoNonBLE, but the same
code can be used with Bluetooth Low Energy 4 functionality.
*/

// select a flash page that isn't in use (see Memory.h for more info)
#define  MY_FLASH_PAGE  251

// double level of indirection required to get gcc
// to apply the stringizing operator correctly
#define  str(x)   xstr(x)
#define  xstr(x)  #x

//struct data_t
//{
//  char a;
//  // the compiler will insert 3 bytes of padding here so b is aligned on a dword boundary
//  int b;
//  float c;
//};

//struct data_t value = { '1', 2, 3.0 };

char mystruct[20] = "allomicro";
byte values[10];
//
//void dump_data( struct data_t *p )
//{
//  Serial.print("  a = ");
//  Serial.println(p->a);
//  Serial.print("  b = ");
//  Serial.println(p->b);
//  Serial.print("  c = ");
//  Serial.println(p->c);  
//}
int count = 0;
void setup() {
  delay(5000);
  Serial.begin(9600);
  Serial.println(sizeof(mystruct));
  Serial.println("All output will appear on the serial monitor.");

  // a flash page is 1K in length, so page 251 starts at address 251 * 1024 = 257024 = 3EC00 hex
  char *blabla = (char*)ADDRESS_OF_PAGE(MY_FLASH_PAGE);
  int rc;

  Serial.println("The data stored in flash page " str(MY_FLASH_PAGE) " contains: ");
  Serial.println(blabla);
  
//  Serial.print("Attempting to erase flash page " str(MY_FLASH_PAGE) ": ");
//  rc = flashPageErase(MY_FLASH_PAGE);
//  if (rc == 0)
//    Serial.println("Success");
//  else if (rc == 1)
//    Serial.println("Error - the flash page is reserved");
//  else if (rc == 2)
//    Serial.println("Error - the flash page is used by the sketch");

//  Serial.println("The data stored in flash page " str(MY_FLASH_PAGE) " contains: ");
//  Serial.println(blabla);
  
//  Serial.print("Attempting to write data to flash page " str(MY_FLASH_PAGE) ": ");
//  rc = flashWriteBlock(ADDRESS_OF_PAGE(MY_FLASH_PAGE), mystruct, sizeof(mystruct));
//  if (rc == 0)
//    Serial.println("Success");
//  else if (rc == 1)
//    Serial.println("Error - the flash page is reserved");
//  else if (rc == 2)
//    Serial.println("Error - the flash page is used by the sketch");
//
//  Serial.println("The data stored in flash page " str(MY_FLASH_PAGE) " contains: ");
//  Serial.println(blabla);

byte *p = (byte*)ADDRESS_OF_PAGE(MY_FLASH_PAGE - 1);

  Serial.println("The data stored in flash page " str(MY_FLASH_PAGE - 1) " contains: ");
  Serial.println(*p);
//
//  Serial.print("Attempting to erase flash page " str(MY_FLASH_PAGE - 1) ": ");
//  rc = flashPageErase(PAGE_FROM_ADDRESS(p));
//  if (rc == 0)
//    Serial.println("Success");
//  else if (rc == 1)
//    Serial.println("Error - the flash page is reserved");
//  else if (rc == 2)
//    Serial.println("Error - the flash page is used by the sketch");

//  Serial.println("The data stored in flash page " str(MY_FLASH_PAGE - 1) " contains: ");
//  Serial.println(*p);
//
//  while (count<10){
//    RFduino_ULPDelay(SECONDS(1));
//    byte temp = /*RFduino_temperature(CELSIUS)*/analogRead(5) >> 2;
//    Serial.println (count);
//    Serial.println (temp);
//    values[count]=temp;
//    count++;
//  }
//
//  Serial.print("Attempting to write data to flash page " str(MY_FLASH_PAGE - 1) ": ");
//  rc = flashWriteBlock(p, &values, sizeof(values));
//  if (rc == 0)
//    Serial.println("Success");
//  else if (rc == 1)
//    Serial.println("Error - the flash page is reserved");
//  else if (rc == 2)
//    Serial.println("Error - the flash page is used by the sketch");

  Serial.println("The data stored in flash page " str(MY_FLASH_PAGE-1) " contains: ");
  Serial.println(*p);
  for(count = 0; count < sizeof(values); count++){
    Serial.print(p[count]);
    Serial.print("|");
  }

}

void loop() {
}

