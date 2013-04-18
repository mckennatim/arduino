 /*

 */
#include <EEPROM.h>
#include <OneWire.h>
#include <EEPROMstruc.h>
OneWire  ds(28);  //ds is the core, 28 is the pin

const int MAXCKTS = 12;
const int LOCLU = 100;
const int LOCORD = 70;
const int LOCSETPTS = 10;
const int LOCNUMCKTS =5;
int ctr = 0;
int numckts; //num circuits in this setup	
char lu[MAXCKTS][9]; //max sensors is 12, sensor id is 8 bytes + \0
int setpts[MAXCKTS];

byte i;
byte addr[8];
byte defsetpt =150;

void setup() {
	// start serial port:
	Serial.begin(9600);
	EEPROM_readStruc(100, lu);  
	EEPROM_readStruc(5, numckts);  
	EEPROM_readStruc(10, setpts);    
	for (int i = 0; i<numckts;i++) {
		Serial.println(lu[i]);
		Serial.println(setpts[i]);  	
	}
	Serial.println(numckts);	
	for ( int h = 0; h < MAXCKTS; h++ ) {
		Serial.print(EEPROM.read(h+LOCORD));
		Serial.print(", ");
	}
	Serial.println();
	for ( int h = 0; h < MAXCKTS; h++ ) {
		Serial.print(EEPROM.read(h+LOCSETPTS));
		Serial.print(", ");
	}  	  
}

void loop() 
{
}

