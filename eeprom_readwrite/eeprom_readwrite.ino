#include <EEPROM.h>
int addr = 0;
int val;
byte value;
void setup()
{
  Serial.begin(9600);
}
void loop()
{
  val = 155;
  EEPROM.write(addr, val);
  delay(500);
  value = EEPROM.read(addr);
  addr = addr + 1;
  if (addr == 50) addr = 0;  
  Serial.print(addr);
  Serial.print("\t");
  Serial.print(value);
  Serial.println();  
}
