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
IPAddress cosmserver(198,23,156,78);    // numeric IP for api.cosm.com
unsigned long lastConnectionTime = 0; // last time you connected to the server, in millis
boolean lastConnected = false; // state of the connection last time through the main loop
const unsigned long postingInterval = 20*1000; //delay between updates to Cosm.com

OneWire  ds(5);  //ds is the core, 28 is the pin
//{"data": {"temp": [322, 302], "relay": [0, 1 ], "setpt": [166, 167]}}
const int MAXCKTS = 12;
const int LOCLU = 100;
const int LOCORD = 70;
const int LOCSETPTS = 10;
const int LOCNUMCKTS =5;
int ctr = 0;
int numckts; //num circuits in this setup reporting
int datapts; //num setpoints being sent from server
int numread; //num ckts read this time through
char lu[MAXCKTS][9]; //max sensors is 12, sensor id is 8 bytes + \0
char clu[MAXCKTS][9]; //max sensors is 12, sensor id is 8 bytes + \0
byte setpts[MAXCKTS];
int sord[MAXCKTS];
int temp[MAXCKTS];
int temps[MAXCKTS];
int hyst[] = {16,16,16,16,16,16,16,16,16,16,16,16}; //hysteresis

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
int repin[] = {33,35,37,39,41,43,45,47,49,51,53,52};

void setup() 
{
  Serial.println(freeRAM());
  // start serial port:
  Serial.begin(9600);
  //if (Ethernet.begin(mac) == 0) {
    Serial.println("XXXXXXXXXXXXXXXXXXXXXXXXXX reboot XXXXXXXXXXXXXXXXXXX");
    // DHCP failed, so use a fixed IP address:
    //Ethernet.begin(mac, ip);
  //}  
  EEPROM_readStruc(LOCLU, lu); 
  delay(500); 
  EEPROM_readStruc(LOCNUMCKTS, numckts);
  delay(500);
  EEPROM_readStruc(LOCSETPTS, setpts);

  delay(500);
  EEPROM_readStruc(LOCORD, sord);  
  Serial.println(lu[0]);
  Serial.println(numckts);
  Serial.println(setpts[0]);    
  for (int j=0;j<MAXCKTS;j++ ){
    digitalWrite(repin[j], RELAY_OFF);
    pinMode(repin[j], OUTPUT);  
  } 
  Serial.println(freeRAM());  
  getSensorIds();
}



void loop() {
  if(millis() - lastConnectionTime > postingInterval) { 
    Serial.println("meets conditions");
    Serial.println(" ----------------------------------------------------");
    Serial.println(freeRAM());  
    getSensorIds();
    Serial.println(freeRAM());  
    readTemps();
    Serial.println(freeRAM());
    setRelays();      
    Serial.println(freeRAM());
    orderTemps();
    Serial.println(freeRAM());
    assembleData();
    Serial.println(freeRAM());  
    lastConnectionTime = millis();  
  }
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
  ds.reset_search();
  delay(250);
	ctr = 0;
    while ( ds.search(addr)){ 
        char dest[9];
	    for( i = 0; i < 8; i++) {
	    dest[i] = String(addr[i], HEX)[0];	    
	    } 
	    dest[i] ='\0';//add terminator
	    strcpy(clu[ctr], dest);
	    Serial.print(clu[ctr]);  
		Serial.println(); 
        Serial.print(strlen(clu[ctr])); 
        Serial.println();    
        Serial.print(ctr);
        Serial.println();   
      ctr++;
      delay(250);
      ds.select(addr);  
	}   			
}
void readTemps(){ 
  Serial.println("in readTemps");
  byte i;
  byte present = 0;
  byte type_s;
  byte data[12];
  byte addr[8];
  ctr = 0;
  ds.reset_search();
  delay(250);
  while ( ds.search(addr)){      
    Serial.print("ROM =");
    for( i = 0; i < 8; i++) {
      Serial.write(' ');
      Serial.print(addr[i], HEX);
    }
    if (OneWire::crc8(addr, 7) != addr[7]) {
      Serial.println("CRC is not valid!");
      //tempstr = "\"temp\": \"CRC is not valid!\"";
      return;
    }
    Serial.println();
    // the first ROM byte indicates which chipi
    switch (addr[0]) {
      case 0x10:
        Serial.println("  Chip = DS18S20");  // or old DS1820
        type_s = 1;
        break;
      case 0x28:
        Serial.println("  Chip = DS18B20");
        type_s = 0;
        break;
      case 0x22:
        Serial.println("  Chip = DS1822");
        type_s = 0;
        break;
      default:
        Serial.println("Device is not a DS18x20 family device.");
        //tempstr = "\"temp\": \"Device is not a DS18x20 family device.\"";
        return;
    }  
    ds.reset();
    ds.select(addr);
    ds.write(0x44,1);         // start conversion, with parasite power on at the end
    delay(1000);     // maybe 750ms is enough, maybe not
    // we might do a ds.depower() here, but the reset will take care of it.
    present = ds.reset();
    ds.select(addr);    
    ds.write(0xBE);         // Read Scratchpad
    Serial.print("  Data = ");
    Serial.print(present,HEX);
    Serial.print(" ");
    for ( i = 0; i < 9; i++) {           // we need 9 bytes
      data[i] = ds.read();
      Serial.print(data[i], HEX);
      Serial.print(" ");
    }
    Serial.print(" CRC=");
    Serial.print(OneWire::crc8(data, 8), HEX);
    Serial.println();
    // convert the data to actual temperature
    unsigned int raw = (data[1] << 8) | data[0];
    if (type_s) {
      raw = raw << 3; // 9 bit resolution default
      if (data[7] == 0x10) {
      // count remain gives full 12 bit resolution
      raw = (raw & 0xFFF0) + 12 - data[6];
    }
    } else {
      byte cfg = (data[4] & 0x60);
      if (cfg == 0x00) raw = raw << 3;  // 9 bit resolution, 93.75 ms
      else if (cfg == 0x20) raw = raw << 2; // 10 bit res, 187.5 ms
      else if (cfg == 0x40) raw = raw << 1; // 11 bit res, 375 ms
      // default is 12 bit resolution, 750 ms conversion time
    }
    // divide raw/16 for celsius and *9/5+32 for F but do it on sitebuilt
    //appends raw to json
    temp[ctr]=raw;
    Serial.println(temp[ctr]);
    ++ctr;
    delay(500);
    //tempstr= tempstr + raw + ", ";
    //do any formatting and float stuff on sitebuilt server
  } //end of while loop
  numread=ctr;
  //out of while loop, no more addresses to read
  Serial.println(numread);
  Serial.println("No more addresses.");
  Serial.println();
  //tempstr = tempstr.substring(0,tempstr.length()-2);//strip the last ,
  //tempstr = "\"temp\": [" +tempstr + "]";
  //Serial.println(tempstr);  
  //*/
  //tempstr = "\"temp\": [356, 344 ]";  
} 
void orderTemps(){
  Serial.println("in orderTemps");
  int j =0;
  for (int k=0; k<numckts; k++){
        Serial.println(clu[j]);
        Serial.println(lu[k]);
    if (!strcmp(clu[j], lu[k])){
      temps[sord[k]]=temp[j];
      j++;
    } else{
      Serial.println("different");
      temps[sord[k]]=0;
    }
    //Serial.println(temps[k]);        
  }   
}
void setRelays(){
  ///*
  Serial.println("in setRelays");
  for (int k=0;k<MAXCKTS;k++ ){ 
    if(temps[k]>setpts[k]*2 || temps[k]==0) {//if temps reached setpoint, or not valid reading
      relay[k]=RELAY_OFF;//record new state
      digitalWrite(repin[k], RELAY_OFF); //turn off zone
    } else if(temps[k]<(setpts[k]*2-hyst[k])) {
      relay[k]=RELAY_ON;//record new state
      digitalWrite(repin[k], RELAY_ON); //turn on zone
    } 
    relay[k] = digitalRead(repin[k]);
  }   
}
void assembleData(){
  //jsn = "{\"data\": {" + tempstr + ", " + setptstr + ", " + relaystr + "}}";
  //jlen = jsn.length();
  //jsn[0] = '\0';
  char jsn[260] = "{\"data\": {" ;//should be more than big enough 

  Serial.println("in assembleData");
  char subst[100] = " ";
  array2json(subst, temps, MAXCKTS, "temp");
  Serial.println(subst);
  sprintf(jsn, "%s%s, ",jsn,subst);
  subst[0] = '\0';
  array2json(subst, relay, MAXCKTS, "relay");
  Serial.println(subst);
  sprintf(jsn, "%s%s, ",jsn,subst);
  subst[0] = '\0';
  Serial.println(subst);
  //Serial.println(printf("num of datapts = %d", datapts ));
  arrayB2json(subst, setpts, MAXCKTS, "setpt");
  Serial.println(subst);
  sprintf(jsn, "%s%s, ",jsn,subst);
  subst[0] = '\0'; 
  jlen = strlen(jsn);
  jsn[jlen-2] = '\0';
  sprintf(jsn, "%s}}",jsn);
  Serial.println(jsn);

}

void array2json(char *s, int vali[], int ns, char name[] ){
  Serial.println("in array2json");
  //Serial.println(vali[2]);
  sprintf(s, "\"%s\": [", name);
  for (i=0; i < ns; i++){
    sprintf(s, "%s%d, ",s,vali[i]);  
  }
  int sl = strlen(s);
  //Serial.println(sl);
  s[sl-2] = '\0';
  Serial.println(sl);
  sprintf(s, "%s]",s);
}
void arrayB2json(char *s, byte vali[], int ns, char name[] ){
  Serial.println("in array2json");
  //Serial.println(vali[2]);
  sprintf(s, "\"%s\": [", name);
  for (i=0; i < ns; i++){
    sprintf(s, "%s%d, ",s,vali[i]);  
  }
  int sl = strlen(s);
  //Serial.println(sl);
  s[sl-2] = '\0';
  Serial.println(sl);
  sprintf(s, "%s]",s);
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
    Serial.println("in installDefaultSetpts");		
}

int freeRAM(){
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}