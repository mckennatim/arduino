 /*

 */
#include <EEPROM.h>
#include <OneWire.h>
#include <EEPROMstruc.h>
#include <SPI.h>
#include <Ethernet.h>

#define APIKEY "xxxxxxxxxxxxxxxxxxx1NVxpVkhjNUI5RnlEVT0g" // your sitebuilt api key
#define FEEDID         "02130"// your feed ID
#define USERAGENT      "sitebuilt Arduino Example (83080)" // user agent is the project 
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
IPAddress ip(10,0,1,79);//<<< ENTER YOUR IP ADDRESS HERE!!!
EthernetServer server(7777);
EthernetClient client;
IPAddress cosmserver(198,23,253,29);    // numeric IP for api.cosm.com
unsigned long lastConnectionTime = 0; // last time you connected to the server, in millis
boolean lastConnected = false; // state of the connection last time through the main loop
const unsigned long postingInterval = 20*1000; //delay between updates to Cosm.com

OneWire  ds(28);  //ds is the core, 28 is the pin or 5
OneWire  ds2(26);  //ds is the core, 28 is the pin or 5
//{"data": {"temp": [322, 302], "relay": [0, 1 ], "setpt": [166, 167]}}
const int MAXCKTS = 12;
const int LOCLU = 100;
const int LOCORD = 70;
const int LOCSETPTS = 10;
const int LOCNUMCKTS =5;
static byte DEFSETPT = 150;
int ctr = 0;
int numckts; //num circuits in this setup reporting
byte numread; //num ckts read this time through
char lu[MAXCKTS][9]; //max sensors is 12, sensor id is 8 bytes + \0
char clu[MAXCKTS][9]; //max sensors is 12, sensor id is 8 bytes + \0
byte setpts[MAXCKTS];
int temp[MAXCKTS];
int temps[MAXCKTS];
byte hyst[] = {16,16,16,16,16,16,16,16,16,16,16,16}; //hysteresis
byte sord[] = {1,4,5,2,0,3,6,7,8,9,10,11};
byte i; //for all the i's
byte addr[8]; //for ROM ID
//json
int jlen; //length of json
char jsn[210] = "{\"data\": {" ;//should be more than big enough 
//data from server
const int DATALEN = 100; //length of cdate read from server that might have setpts etc
char cdata[DATALEN]; //data coming back from serve is just setpts (reuse this array every 20 sec)
char c1[2]; //a characer sting to hold c data
boolean keepReading = false; //initialize
#define RELAY_ON 0
#define RELAY_OFF 1
int relay[MAXCKTS];
byte repin[] = {33,35,37,39,41,43,45,47,49,51,53,52};

void setup() 
{
  // start serial port:
  Serial.begin(9600);
  initializeArrays();
  numckts = getSensorIds(0,ds);  

  Serial.println(numckts);
  numckts = getSensorIds(numckts,ds2);  

  Serial.println(numckts);  
  //Serial.println(config.lu[0]);
  installDefaultSetpts();     
  writeSensorIds();
 
  EEPROM_readStruc(LOCSETPTS, setpts); 
  for ( int h = 0; h < MAXCKTS; h++ ) {
    Serial.print(setpts[h]);
    Serial.print(", ");
  }
  Serial.println();
  EEPROM_readStruc(LOCORD, sord);   
  for ( int h = 0; h < MAXCKTS; h++ ) {
    Serial.print(sord[h]);
    Serial.print(", ");
  }
  Serial.println();

  EEPROM_readStruc(LOCLU, lu);   
  for ( int h = 0; h < MAXCKTS; h++ ) {
    Serial.print(lu[h]);
    Serial.print(", ");
  }
  Serial.println();
  Serial.println(freeRAM());
}



void loop() {

}
void initializeArrays(){

}

int getSensorIds(int ctr, OneWire ds){
  Serial.println("in getSensorIds");
  ds.reset_search();
  delay(250);
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
      delay(250);
      ds.select(addr);  
  } 
  return ctr; 
}

void writeSensorIds(){
    Serial.println("in writeSensorIds");  
    Serial.print(lu[0]);  
  Serial.println();
  EEPROM_writeStruc(LOCLU, lu);   
  EEPROM_writeStruc(LOCNUMCKTS, numckts);  
  EEPROM_writeStruc(LOCSETPTS, setpts);   
  EEPROM_writeStruc(LOCORD, sord);   
    
}
void installDefaultSetpts(){
  Serial.print(DEFSETPT);
  Serial.println("in installDefaultSetpts");    
  for ( int h = 0; h < MAXCKTS; h++ ) {
    setpts[h]=DEFSETPT;
  }    
}

int freeRAM(){
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}