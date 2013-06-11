#include <EtherCard.h>

#define RELAY_PIN	2

static byte mymac[]  = {0xDD,0xDD,0xDD,0x00,0x00,0x01};
static byte myip[]   = {10,0,1,186};
byte Ethernet::buffer[700];
static uint32_t timer;
char til[10];

char* on  = "ON";
char* off = "OFF";

boolean relayStatus;
char* relayLabel;
char* linkLabel;

void setup () {
 
  Serial.begin(57600);
  Serial.println("WebRelay Demo");

  if(!ether.begin(sizeof Ethernet::buffer, mymac))
    Serial.println( "Failed to access Ethernet controller");
  else
    Serial.println("Ethernet controller initialized");

  if(!ether.staticSetup(myip))
    Serial.println("Failed to set IP address");

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);  
  relayStatus = false;
  relayLabel = off;
  linkLabel = on;
  timer = millis() + 5000;
}
  
void loop() {
  word len = ether.packetReceive();
  word pos = ether.packetLoop(len);
 if (millis() > timer) {
   digitalWrite(RELAY_PIN, false);
 } 
  
  if(pos) {
  	char* data = (char *) Ethernet::buffer + pos;
	Serial.println(data);    
	if(strstr(data, "GET /?status=ON") != 0) {
		ether.findKeyVal(data + 6, til , sizeof til , "til");
		Serial.print("Will stay on for ");
		Serial.print(til);
		Serial.println( " minutes.\n");
		int until = atoi(til);
		//Serial.println(until);		
		timer = millis() + until*60000;
		relayStatus = true;
		relayLabel = on;
		linkLabel = off;
	} else if(strstr(data, "GET /?status=OFF") != 0) {
		relayStatus = false;
		relayLabel = off;
		linkLabel = on;
	}

      digitalWrite(RELAY_PIN, relayStatus);
 
		
    BufferFiller bfill = ether.tcpOffset();
    bfill.emit_p(PSTR("HTTP/1.0 200 OK\r\n"
      "Content-Type: text/html\r\nPragma: no-cache\r\n\r\n"
      "<html><head><meta name='viewport' content='width=200px'/></head><body>"
      "<div style='position:absolute;width:200px;height:200px;top:50%;left:50%;margin:-100px 0 0 -100px'>"
      "<div style='font:bold 14px verdana;text-align:center'>Relay is $S</div>"
      "<br><div style='text-align:center'>"
      "<a href='/?status=$S'><img src='http://www.lucadentella.it/files/bt_$S.png'></a>"
      "</div></div></body></html>"
      ), relayLabel, linkLabel, linkLabel);

      ether.httpServerReply(bfill.position());
    }
}
