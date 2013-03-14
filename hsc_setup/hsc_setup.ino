 /*

 */
#include <EEPROM.h>
#include <OneWire.h>
#include <EEPROMstruc.h>
OneWire  ds(28);  //ds is the core, 28 is the pin
const int MAXCKTS = 12;
const int LOCLU = 100;
const int LOCSETPTS = 10;
const int LOCNUMCKTS =5;
int ctr = 0;
int numckts; //num circuits in this setup	
char lu[MAXCKTS][9]; //max sensors is 12, sensor id is 8 bytes + \0
static int DEFSETPT = 150;
int setpts[MAXCKTS];


byte i;
byte addr[8];
byte defsetpt =150;

unsigned long lastConnectionTime = 0; // last time you connected to the server, in millis
boolean lastConnected = false; // state of the connection last time through the main loop
const unsigned long postingInterval = 20*1000; //delay between updates

void setup() {
  // start serial port:
  Serial.begin(9600);
  numckts = countSensors();
  getSensorIds();  

	for (i=0;i < numckts; i++)
	{
		setpts[i]=DEFSETPT;
	}
	//config.numckts= numckts;
	//config.lu= lu;
	Serial.println(numckts);
	//Serial.println(config.lu[0]);

     
    writeSensorIds();
   
    installDefaultSetpts();	
}

void loop() {

}
int countSensors(){
  Serial.println("in countSensors");
  ctr = 0;
  ds.reset_search();
  delay(250);
  while ( ds.search(addr)){
      Serial.print("ROM =");
      for( i = 0; i < 8; i++) {
        Serial.write(' ');
        Serial.print(addr[i], HEX);
      }
	Serial.println();         
      ctr++; 
      ds.select(addr);  
  } 
  return ctr; 
}

void getSensorIds(){
    Serial.println("in getSensorIds");
    ds.reset(); 
	ctr = 0;
    while ( ds.search(addr)){ 
        char dest[9];
	    for( i = 0; i < 8; i++) {
	    dest[i] = String(addr[i], HEX)[0];	    
	    } 
	    dest[i] ='\0';//add terminator
	    strcpy(lu[ctr], dest);
	    Serial.print(lu[ctr]);  
		Serial.println(); 
        Serial.print(strlen(lu[ctr])); 
        Serial.println();    
        Serial.print(ctr);
        Serial.println();   
      ctr++;
      ds.select(addr);  
	}   			
}
void writeSensorIds(){
    Serial.println("in writeSensorIds");	
    Serial.print(lu[1]);  
	Serial.println();
	EEPROM_writeStruc(LOCLU, lu);  	
	EEPROM_writeStruc(LOCNUMCKTS, numckts);  
	EEPROM_writeStruc(LOCSETPTS, setpts);  	
 		
}
void installDefaultSetpts(){
    Serial.println("in installDefaultSetpts");		
}
