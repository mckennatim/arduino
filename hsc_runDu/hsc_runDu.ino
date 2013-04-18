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

OneWire  ds(28);  //ds is the core, 28 is the pin or 5
//{"data": {"temp": [322, 302], "relay": [0, 1 ], "setpt": [166, 167]}}
const int MAXCKTS = 12;
const int LOCLU = 100;
const int LOCORD = 70;
const int LOCSETPTS = 10;
const int LOCNUMCKTS =5;
byte repin[] = {33,35,37,39,41,43,45,47,49,51,53,52};
byte hyst[] = {16,16,16,16,16,16,16,16,16,16,16,16}; //hysteresis

byte ctr = 0;
byte numckts; //num circuits in this setup reporting
byte numread; //num ckts read this time through
char lu[MAXCKTS][9]; //max sensors is 12, sensor id is 8 bytes + \0
byte setpts[MAXCKTS];
byte sord[MAXCKTS];
int temp[MAXCKTS];
int temps[MAXCKTS];

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
byte relay[MAXCKTS];

void setup() 
{
  Serial.println(freeRAM());
  // start serial port:
  Serial.begin(9600);   
  Serial.println("XXXXXXX reboot XXXXXXX");
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    // DHCP failed, so use a fixed IP address:
    //Ethernet.begin(mac, ip);
  }  
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
}

void loop() {
  getClientData();
  if (!client.connected() && lastConnected) {
  Serial.println();
  Serial.println("disconnecting.");
  client.stop();
  }
  // if you're not connected, and ten seconds have passed since
  // your last connection, then connect again and send data:
  if(!client.connected() && (millis() - lastConnectionTime > postingInterval)) {
    Serial.println("-------");
    Serial.println(cdata);
    Serial.println();
    updateSetpts();
    cdata[0]='\0';
    readTemps();
    setRelays();      
    //orderTemps();
    assembleData();
    sendData(); 
    Serial.println(freeRAM());  
    lastConnectionTime = millis();  
  }
}

void getClientData(){
  if (client.available()) {
    char c = client.read();
    Serial.print(c);
    //*
    if(c=='<'){keepReading=true;}
    if (keepReading){
      c1[0]=c;
      strcat(cdata, c1);
    }
    if(c=='>'){keepReading=false;}  
    //*/
  } 
}

void  updateSetpts(){
  Serial.println("in update setpts"); 
  Serial.println(cdata); //cdata must look like <[166,0,0,0,162,0,156,0,0,0,0,0]>
  //int datapts;
  char ssData[4]; //"123"+/0;
  int newVal;
  int curVal;
  int idx;
  int pslen = strlen(cdata)-4;//take off <[]> 
  Serial.print(pslen);
  Serial.println(" is the length of readString");
  for ( int h = 0; h < MAXCKTS; h++ ) {
    setpts[h]=EEPROM.read(h+LOCSETPTS);
    delay(500);
  }    
  if(pslen>4){
    char ps[pslen];
    for (int i = 2; i<pslen+2; i++){//cdata starts <[, skip them;
      ps[i-2]=cdata[i];//but start ps index at 0
    }  
    Serial.println(ps);
    char *strings[pslen];
    char delims[] = ",";
    int k = 0;
    strings[k] = strtok( ps, delims );
    while( strings[k] != NULL  ) {
      strings[++k] = strtok( NULL, delims );          
    }
    for ( int j = 0; j < MAXCKTS; j++ ) 
    {
      //Serial.println(strings[j]);
      char ssData[4];     
      strcpy(ssData, strings[j]);
      //memcpy( ssData, &ss[1], 3 );
      //ssData[3] = '\0';
      //ssData = "123";          
      newVal= atoi  (ssData);
      //Serial.print(newVal); 
      curVal = setpts[j];
      //Serial.print("<-new|old->");
      //Serial.println(curVal);
      //Serial.print(j);          
      if(curVal != newVal && newVal > 0){//not equal to each other and newVal isn't 0
        Serial.print(" Server has sent new value, setpt changed to ");
        EEPROM.write(j+LOCSETPTS, newVal);
        delay(10);
        setpts[j] = newVal;       
      }  else{
        //Serial.print(" newVal is same as curVal or is 0, stays at ");
      }   
      //Serial.println(setpts[j]);
    }   
  }
}

void readTemps(){ 
  //Serial.println("in readTemps");
  char clu[MAXCKTS][9]; //max sensors is 12, sensor id is 8 bytes + \0
  byte i;
  byte present = 0;
  byte type_s;
  byte data[12];
  byte addr[8];
  ctr = 0;
  ds.reset_search();
  delay(250);
  while ( ds.search(addr)){     
    char dest[9];
    for( i = 0; i < 8; i++) {
      dest[i] = String(addr[i], HEX)[0];      
    } 
    dest[i] ='\0';//add terminator
    strcpy(clu[ctr], dest);
    Serial.print(clu[ctr]);  
    Serial.println();    
    Serial.print(ctr);
    Serial.print(" ");   
    ds.reset();
    ds.select(addr);
    ds.write(0x44,1);         // start conversion, with parasite power on at the end
    delay(1000);     // maybe 750ms is enough, maybe not
    // we might do a ds.depower() here, but the reset will take care of it.
    present = ds.reset();
    ds.select(addr);    
    ds.write(0xBE);         // Read Scratchpad
    for ( i = 0; i < 9; i++) {           // we need 9 bytes
      data[i] = ds.read();
    }
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
    }
    temp[ctr]=raw;
    Serial.print(temp[ctr]);
    Serial.print(" ");
    Serial.println(sord[ctr]);
    ++ctr;
    delay(500);
  } //end of while loop
  //numread=ctr;
  Serial.println(ctr);
  //Serial.println("No more addresses.");
  //Serial.println("in orderTemps");
  int j =0;
  for (int k=0; k<numckts; k++){
    if (!strcmp(clu[j], lu[k])){
      temps[sord[k]]=temp[j];
      j++;
    } else{
      Serial.println("different");
      temps[sord[k]]=0;
    }
  }   
}
void setRelays(){
  ///*
  //Serial.println("in setRelays");
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
  jsn[10]='\0';
  //char jsn[260] = "{\"data\": {" ;//should be more than big enough 
  //Serial.println("in assembleData");
  char subst[100] = " ";
  array2json(subst, temps, MAXCKTS, "temp");
  sprintf(jsn, "%s%s, ",jsn,subst);
  subst[0] = '\0';
  arrayB2json(subst, relay, MAXCKTS, "relay");
  sprintf(jsn, "%s%s, ",jsn,subst);
  subst[0] = '\0';
  arrayB2json(subst, setpts, MAXCKTS, "setpt");
  sprintf(jsn, "%s%s, ",jsn,subst);
  subst[0] = '\0'; 
  jlen = strlen(jsn);
  jsn[jlen-2] = '\0';
  sprintf(jsn, "%s}}",jsn);
  Serial.println(jsn);
}

void array2json(char *s, int vali[], int ns, char name[] ){
  //Serial.println("in array2json");
  sprintf(s, "\"%s\": [", name);
  for (i=0; i < ns; i++){
    sprintf(s, "%s%d, ",s,vali[i]);  
  }
  int sl = strlen(s);
  s[sl-2] = '\0';
  sprintf(s, "%s]",s);
}
void arrayB2json(char *s, byte vali[], int ns, char name[] ){
  sprintf(s, "\"%s\": [", name);
  for (i=0; i < ns; i++){
    sprintf(s, "%s%d, ",s,vali[i]);  
  }
  int sl = strlen(s);
  s[sl-2] = '\0';
  sprintf(s, "%s]",s);
}

void sendData() {
  // if there's a successful connection:
  if (client.connect(cosmserver, 80)) {
  Serial.println("connecting...");
  // send the HTTP PUT request:
  client.print("PUT /hsc/feed/80302");
  //client.print(FEEDID);
  client.println(" HTTP/1.1");
  client.println("Host: api.cosm.com");
  client.print("X-ApiKey: ");
  client.println(APIKEY);
  client.print("User-Agent: ");
  client.println(USERAGENT);
  client.print("Content-Length: ");
  //char tempjsn[] = "{\"data\": {\"temp\": [322, 302], \"relay\": [0, 1 ], \"setpt\": [166, 167]}}";
  jlen = strlen(jsn);
  client.println(jlen);
  // last pieces of the HTTP PUT request:
  client.println("Content-Type: application/json");
  client.println("Connection: close");
  client.println();
  client.println(jsn);
    client.println();
      client.println();
    //jsn[10]='\0';         
  } 
  else {
  // if you couldn't make a connection:
  Serial.println("connection failed");
  Serial.println();
  Serial.println("disconnecting."); 
  client.stop();
  }
   // note the time that the connection was made or attempted:
}


int freeRAM(){
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}