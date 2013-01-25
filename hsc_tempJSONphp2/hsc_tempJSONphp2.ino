 /*

 */
#include <EEPROM.h>
#include <OneWire.h>
#include <SPI.h>
#include <Ethernet.h>

#define APIKEY         "xxxxxxxxxxxxxxxxxxx1NVRDhPeSAKxpVkhjNUI5RnlEVT0g" // your sitebuilt api key
#define FEEDID         "02130"// your feed ID
#define USERAGENT      "sitebuilt Arduino Example (83080)" // user agent is the project name

// assign a MAC address for the ethernet controller.
// Newer Ethernet shields have a MAC address printed on a sticker on the shield
// fill in your address here:
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};

IPAddress ip(10,0,1,79);//<<< ENTER YOUR IP ADDRESS HERE!!!

// Initialize the Ethernet server library
// with the IP address and port you want to use 
// (port 80 is default for HTTP):
EthernetServer server(7777);

// initialize the library instance:
EthernetClient client;

// if you don't want to use DNS (and reduce your sketch size)
// use the numeric IP instead of the name for the server:
IPAddress cosmserver(198,23,156,78);    // numeric IP for api.cosm.com
//char server[] = "api.cosm.com";   // name address for cosm API

unsigned long lastConnectionTime = 0;          // last time you connected to the server, in milliseconds
boolean lastConnected = false;                 // state of the connection last time through the main loop
const unsigned long postingInterval = 10*1000; //delay between updates to Cosm.com

OneWire  ds(28);  // for due on pin 15 analog 1
float celsius, fahrenheit;
String jsn = ""; 
int jlen;
int nc=2;
String tempstr = "";
String setptstr;
String relaystr;
String ostr = "";
int temp[7];
int setpt[7];

char tempBuf[32] = {0};
int ctr = 0;

void setup() {
  // start serial port:
  Serial.begin(9600);
 // start the Ethernet connection:
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    // DHCP failed, so use a fixed IP address:
    Ethernet.begin(mac, ip);
  }
}

void loop() {
  // read the analog sensor:
  // if there's incomi ng data from the net connection.
  // send it out the serial port.  This is for debugging
  // purposes only:
  if (client.available()) {
    char c = client.read();
    Serial.print(c);
  }

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
   readTemps();
    readSetpts(); 
    setRelays();
    //readOtherStuff();
    assembleData();
    sendData();
    lastConnectionTime = millis();
  }
  // store the state of the connection for next time through
  // the loop:
  lastConnected = client.connected();
}

void readTemps(){ 
    //tempstr = "\"temperatures\": \"Device is not a DS18x20 family device.\"";
    //*
    byte i;
    byte present = 0;
    byte type_s;
    byte data[12];
    byte addr[8];
    tempstr = ""; //empty the tempstr
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
          tempstr = "\"temp\": \"CRC is not valid!\"";
          return;
        }
        Serial.println();
        // the first ROM byte indicates which chip
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
          tempstr = "\"temp\": \"Device is not a DS18x20 family device.\"";
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
      ++ctr;
      tempstr= tempstr + raw + ", ";
      //do any formatting and float stuff on sitebuilt server
    }
    //out of while loop, no more addresses to read
    Serial.println("No more addresses.");
    Serial.println();
    tempstr = tempstr.substring(0,tempstr.length()-2);//strip the last ,
    tempstr = "\"temp\": [" +tempstr + "]";
    Serial.println(tempstr);	
    //*/
        //tempstr = "\"temp\": [356, 344 ]";	
}   

void readSetpts(){
	
	setptstr="";
	if (EEPROM.read(0)!=1){//if eeprom isn't written to yet
		EEPROM.write(0,1);
		for (int k=1;k<=nc;k++ ){//set default values
			EEPROM.write(k,128); //*2/16*9/5+32~60F
		}				
	}
	for (int k=1;k<=nc;k++ ){
		setpt[k]=EEPROM.read(k);
		setptstr = setptstr + setpt[k]+ ", ";
	}	
	setptstr = setptstr.substring(0,setptstr.length()-2);//strip the last ,
    setptstr = "\"setpt\": [" +setptstr + "]";	
    
	//setptstr = "\"setpt\": [300, 310]";    
     Serial.println(setptstr); 
} 

void setRelays(){
	/*
	relaystr="";
	for (int k=0;k<nc;k++ ){	
		if(temp[k]>setpt[k]*2) {//if temps reached setpoint
			relay[k]=0;//record new state
			digitalWrite(repin[k], LOW); //turn off zone
		}
		if(temp[k]<setpt[k]*2-hyst[k]) {
			relay[k]=1;//record new state
			digitalWrite(repin[k], HIGH); //turn on zone			
		}//if temp between those points don't do anything	
		relaystr = relaystr + relay[k]+ ", ";	
	}
	relaystr = relaystr.substring(0,relaystr.length()-2);//strip the last ,
    relaystr = "\"relay\": [" +relaystr + "]";		
    */
     relaystr = "\"relay\": [1, 0]";	   
}

void readOtherStuff(){
    ostr = "\"noise\": {\"sensor-1\": 42, \"sensor-2\": 43}" ; 
    /*
    ostr = "\"noise\": {"; 
    // here's the actual content of the PUT request:
    for (int analogChannel = 2; analogChannel < 6; analogChannel++) {
      int sensorReading = analogRead(analogChannel);    
      String schan = "\"sensor";
      schan = schan + analogChannel;
     schan= schan + "\":";
      ostr = ostr + schan;
      ostr = ostr + sensorReading +", ";
    }
    ostr = ostr.substring(0,ostr.length()-2);//strip the last ,
    ostr = ostr + "} " ;	
    //should look like \"noise\": {\"sensor1\": 124,  \"sensor2\": 2224}
    */
}

void assembleData(){
    jsn = "{\"data\": {" + tempstr + ", " + setptstr + ", " + relaystr + "}}";
    jlen = jsn.length();
    Serial.println(jsn);
}

// this method makes a HTTP connection to the server:
void sendData() {
  // if there's a successful connection:
  if (client.connect(cosmserver, 80)) {
    Serial.println("connecting...");
    // send the HTTP PUT request:
    client.print("PUT /feeds/getnoise2.php");
    //client.print(FEEDID);
    client.println(" HTTP/1.1");
    client.println("Host: api.cosm.com");
    client.print("X-ApiKey: ");
    client.println(APIKEY);
    client.print("User-Agent: ");
    client.println(USERAGENT);
    client.print("Content-Length: ");
    // calculate the length of the sensor reading in bytes:
    // 8 bytes for "sensor1," + number of digits of the data:
    //int thisLength = 8 + getLength(thisData);
    client.println(jlen);
    // last pieces of the HTTP PUT request:
    client.println("Content-Type: application/json");
    client.println("Connection: close");
    client.println();
    client.println(jsn);
      
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



