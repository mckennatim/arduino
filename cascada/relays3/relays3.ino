#include <EtherCard.h>

#define RELAY_PIN 2

static byte mymac[]  = {0xDD,0xDD,0xDD,0x00,0x00,0x01};
static byte myip[]   = {10,0,1,186};
byte Ethernet::buffer[700];
static uint32_t timer[3];
char til[4];
char rel[2];
byte repin[] = {2,3,4};
byte reon[] = {1,0,0};
byte reoff[] = {0,1,1};

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

  pinMode(repin[0], OUTPUT);
  digitalWrite(repin[0], reoff[0]);  
  pinMode(repin[1], OUTPUT);
  digitalWrite(repin[1], reoff[1]);  
  pinMode(repin[2], OUTPUT);
  digitalWrite(repin[2], reoff[2]);  
  relayStatus = false;
  relayLabel = off;
  linkLabel = on;
  timer[0] = millis() + 5000;
}
  
void loop() {
  word len = ether.packetReceive();
  word pos = ether.packetLoop(len);
 if (millis() > timer[0]) {
   digitalWrite(repin[0], reoff[0]);
 } 
  if (millis() > timer[1]) {
   digitalWrite(repin[1], reoff[1]);
 } 
  if (millis() > timer[2]) {
   digitalWrite(repin[2], reoff[2]);
 } 
  
  if(pos) {
    char* data = (char *) Ethernet::buffer + pos;
    Serial.println(data); 
    ether.findKeyVal(data + 6, rel , sizeof rel , "relay");
    byte rela = atoi(rel);   
    Serial.println(rel); 
    if(strstr(data, "GET /?status=ON") != 0) {
      ether.findKeyVal(data + 6, til , sizeof til , "til");
      Serial.print("Will stay on for ");
      Serial.print(til);
      Serial.println( " minutes.\n");
      Serial.println(freeRAM());
      int until = atoi(til);
      //Serial.println(until);    
      timer[rela] = millis() + until*60000;
      relayStatus = reon[rela];
      relayLabel = on;
      linkLabel = off;
    } else if(strstr(data, "GET /?status=OFF") != 0) {
      relayStatus = reoff[rela];
      relayLabel = off;
      linkLabel = on;
    }
    digitalWrite(repin[rela], relayStatus);
    
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

int freeRAM(){
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}
