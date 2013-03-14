 /*
run
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

OneWire  ds(28);  //ds is the core, 28 is the pin
//{"data": {"temp": [322, 302], "relay": [0, 1 ], "setpt": [166, 167]}}
const int MAXCKTS = 12;
const int LOCLU = 100;
const int LOCSETPTS = 10;
const int LOCNUMCKTS =5;
int ctr = 0;
int numckts; //num circuits in this setup	
int numread; //num ckts read this time through
char lu[MAXCKTS][9]; //max sensors is 12, sensor id is 8 bytes + \0
char clu[MAXCKTS][9]; //max sensors is 12, sensor id is 8 bytes + \0
int setpts[MAXCKTS];
int temp[MAXCKTS];
int temps[MAXCKTS];
int hyst[] = {2,2,2,2,2,2,2,2,2,2,2,2}; //hysteresis

byte i; //for all the i's
byte addr[8]; //for ROM ID
//json
int jlen; //length of json
char jsn[200] = "{\"data\": {" ;//should be more than big enough 
//data from server
const int DATALEN = 100; //length of cdate read from server that might have setpts etc
char cdata[DATALEN]; //data coming back from serve is just setpts (reuse this array every 20 sec)
char c1[2]; //a characer sting to hold c data
boolean keepReading = false; //initialize

int relay[MAXCKTS];
int repin[] = {33,35,37,39};

void setup() 
{
  // start serial port:
  Serial.begin(9600);
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    // DHCP failed, so use a fixed IP address:
    Ethernet.begin(mac, ip);
  }  
 EEPROM_readStruc(LOCLU, lu);  
 EEPROM_readStruc(LOCNUMCKTS, numckts);  
 EEPROM_readStruc(LOCSETPTS, setpts);  
 Serial.println();  
 Serial.println(lu[1]);
 Serial.println(numckts);
 Serial.println(setpts[1]);  	
    for (int j=0;j<numckts;j++ ){
        pinMode(repin[j], OUTPUT);  
    } 
}

void loop() {
  // if there's incoming data from the net connection.
    getClientData();

  // if there's no net connection, but there was one last time
  // through the loop, then stop the client:
  if (!client.connected() && lastConnected) {
    Serial.println();
    Serial.println("disconnecting.");
    client.stop();
  }
  // if you're not connected, and ten seconds have passed since
  // your last connection, then connect again and send data:
  if(!client.connected() && (millis() - lastConnectionTime > postingInterval)) {
    Serial.println("meets conditions");
    Serial.println("DOGSHIT ----------------------------------------------------");
    Serial.println(cdata);
    Serial.println();
    updateSetpts();
    cdata[0]='\0';
    getSensorIds();
    readTemps();
    orderTemps();    
    setRelays(); 
    assembleData();
    sendData(); 
    lastConnectionTime = millis();    
  }
  // store the state of the connection for next time through
  // the loop:
  lastConnected = client.connected();
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
    Serial.println(cdata); 
    int datapts;
    String ss;
    int newVal;
    int curVal;
    int idx;
  int pslen = strlen(cdata)-2;
  Serial.print(pslen);
  Serial.println(" is the length of readString");
  if(pslen>4){
    ///*
    char ps[pslen];
    for (int i = 1; i<pslen+1; i++){
        //Serial.print(readString.charAt(i));
        ps[i-1]=cdata[i];
    }  
    Serial.println(ps);
    char *strings[pslen];
    char delims[] = ",";
    int k = 0;
    strings[k] = strtok( ps, delims );
    while( strings[k] != NULL  ) 
    {
        strings[++k] = strtok( NULL, delims );          
    }
    datapts=k;
    for ( int j = 0; j < datapts; j++ ) 
    {
        //Serial.println(strings[j]);
        ss =strings[j];
        idx= ss.substring(0,1).toInt()-1;
        if (idx>-1){//assumes  data is in order 
        	//and in the form[1160,2161,3162,4163,5164,6156] <numckts &&
            curVal = EEPROM.read(idx+LOCSETPTS);
            //Serial.println(curVal);        
            newVal= ss.substring(1,4).toInt();
            //Serial.println(newVal); 
            if(curVal != newVal){
                Serial.println("not the same");
                EEPROM.write(idx+LOCSETPTS, newVal);
                delay(10);
                setpts[j] = newVal;				
            }  else{
                setpts[j] = curVal;
                Serial.println("newVal is same as curVal");
            }		
        }else {
          Serial.println("sensor not implemented");
        }
        Serial.println(setpts[j]);
    }   
  }
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
        strcpy(clu[ctr], dest);
        Serial.print(clu[ctr]);  
        Serial.println(); 
        Serial.print(strlen(clu[ctr])); 
        Serial.println();    
        Serial.print(ctr);
        Serial.println();   
      ctr++;
      ds.select(addr);  
    }   			
}

void readTemps(){ 
	Serial.println("in readTemps");
	//tempstr = "\"temperatures\": \"Device is not a DS18x20 family device.\"";
	//*
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
        	temps[k]=temp[j];
        	j++;
        } else{
        	Serial.println("different");
        	temps[k]=0;
        }
        Serial.println(temps[k]);        
    }		
}

void setRelays(){
    ///*
    Serial.println("in setRelays");
    for (int k=0;k<numckts;k++ ){	
        if(temps[k]>setpts[k]*2) {//if temps reached setpoint, or not valid reading
            relay[k]=0;//record new state
            digitalWrite(repin[k], LOW); //turn off zone
        }
        if(temps[k]<setpts[k]*2-hyst[k]) {
            relay[k]=1;//record new state
            digitalWrite(repin[k], HIGH); //turn on zone	
        if (temps[k]==0){
            relay[k]=0;//record new state
            digitalWrite(repin[k], LOW); //turn off zone        	
        }    		
        }else { //read current state
        relay[k] = digitalRead(repin[k]);
        }//if temp between those points don't do anything	
        //relaystr = relaystr + relay[k]+ ", ";	
        //String rt = "temp";
        //rt += k; rt+=" is "; rt+=temp[k]; rt+=", setpt-delay=";rt+=setpt[k]*2 - hyst[k];
        Serial.println(relay[k]);
        //Serial.println('temp'+ k +'is ' + temp[k] + ', setpt - delay = '+ setpt[k]*2 - hyst[k]) 
    }
    //relaystr = relaystr.substring(0,relaystr.length()-2);//strip the last ,
    //relaystr = "\"relay\": [" +relaystr + "]";
    //Serial.println(relaystr);		
    //*/
     //relaystr = "\"relay\": [1, 0]";	   
}

//{"data": {"temp": [322, 302], "relay": [0, 1 ], "setpt": [166, 167]}}
void assembleData(){
    //jsn = "{\"data\": {" + tempstr + ", " + setptstr + ", " + relaystr + "}}";
    //jlen = jsn.length();
    Serial.println("in assembleData");
    char subst[70] = " ";
    array2json(subst, temps, numckts, "temp");
    Serial.println(subst);
    sprintf(jsn, "%s%s, ",jsn,subst);
    subst[0] = '\0';
    array2json(subst, relay, numckts, "relay");
    Serial.println(subst);
    sprintf(jsn, "%s%s, ",jsn,subst);
    subst[0] = '\0';
    array2json(subst, setpts, numckts, "setpt");
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
  Serial.println(vali[2]);
    sprintf(s, "\"%s\": [", name);
    for (i=0; i < ns; i++){
      sprintf(s, "%s%d, ",s,vali[i]);  
    }
    int sl = strlen(s);
    Serial.println(sl);
    s[sl-2] = '\0';
    Serial.println(sl);
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
        jsn[10]='\0';         
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
