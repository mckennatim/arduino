// simple starting sketch for using the ethercard to send/receive info over the internet
// it displays the reading from input pin A0 as a demonstration and the led can be turned on or off via ethernet

// send data to the arduino by adding a command onto the end of the URL   e.g.  HTTP://172.22.139.5/option1  



#include <EtherCard.h>

#define DEBUG 1               // set to 1 to display debug info via seral link

int sensorPin = A0;           // select the input pin for the potentiometer to display as test data

int LEDpin = 2;               // pin the LED is on   (you can't use the onboard one as the ethernet card uses it)

char TextBox1[10];            // Data in text box


// web page buffe
static byte mymac[]  = {0xDD,0xDD,0xDD,0x00,0x00,0x01};
static byte myip[]   = {10,0,1,186};
  byte Ethernet::buffer[1000];
  BufferFiller bfill;


// store html header in flash to save memory
char htmlHeader[] PROGMEM = 
      "HTTP/1.0 503 test page\r\n"
      "Content-Type: text/html\r\n"
      "Retry-After: 600\r\n\r\n"
      "<html><head><title>Arduino test page</title></head>"
      "<body style='background-color:rgb(255,255,102);'>"
      "<h2 style='text-align: center;'><em><span style='color: rgb(153, 0, 0);'>This is a test page on my Arduino</span></em></h2>"
;


// ----------------------------------------------


// HTML page to display

  static word homePage() {

    bfill = ether.tcpOffset();

    // read A0 status
    word sensorValue = analogRead(sensorPin);    
    
 
    // read statues of the LED
    char* ledstat;
    if ( digitalRead(LEDpin) == HIGH ) {
      ledstat = "On" ; }
    else {
      ledstat = "Off"; }
    

    // This is html code with embedded variables 
    //    put variables in the HTML area as $D for a number and $S for text then add the actual values after the comma at the end
    //
    // Notes  "<meta http-equiv=refresh content=5>"   tells the page to auto refresh every 5 seconds
    //        if you increase the size of the HTML code you may need to increase the buffer size (or it will stop working)
  
    bfill.emit_p( PSTR ( 
      "$F<p><em>"    // $F = htmlheader in flash memory
      "Reading of A0 input pin = $D <br/><br/>"
      "The LED is $S <br/><br/>"
      "Entered in text box = $S <br/>"
      "</em></p><div style='text-align: center;'><p><em>"
      "<A HREF='?cmd=on'>Turn on</A> <br/>"
      "<A HREF='?cmd=off'>Turn off</A> <br/>"
      "<A HREF='?cmd=blank'>Refresh Page</A> <br/>"
      "<FORM>Test input box <input type=text name=boxa size=10> <input type=submit value=Enter> </form> <br/><br/>"
      "<FORM METHOD=LINK ACTION='10.0.1.186'> <INPUT TYPE=submit VALUE='More Info Here'> </FORM>"
      "</em></p></body></html>" 
    ) , htmlHeader , sensorValue , ledstat , TextBox1 ) ;

    return bfill.position(); 

  }


// ----------------------------------------------


void setup () {

  // set LED pin 13 as output (so the led can be turned on and off)
  pinMode(LEDpin, OUTPUT);  
  
  Serial.begin(57600);
  Serial.println("WebRelay Demo");

  if(!ether.begin(sizeof Ethernet::buffer, mymac))
    Serial.println( "Failed to access Ethernet controller");
  else
    Serial.println("Ethernet controller initialized");

  if(!ether.staticSetup(myip))
    Serial.println("Failed to set IP address");
  
}


// ----------------------------------------------


void loop () {
  // check if anything has come in via ethernet
  word len = ether.packetReceive();
  word pos = ether.packetLoop(len);
  
  if (pos) { // check if valid tcp data is received
    // data received from ethernet

       char* data = (char *) Ethernet::buffer + pos;

       #if DEBUG       // display incoming data    
         Serial.println("----------------");
         Serial.println("data received:");
         Serial.println(data);
         Serial.println("----------------");
       #endif

       // "on" command received     
       if (strncmp( "GET /?cmd=on" , data , 12 ) == 0) {
         digitalWrite(LEDpin, HIGH);   // set the LED on
         #if DEBUG
           Serial.println("on received");
         #endif
       }

       // "off" command received     
       if (strncmp( "GET /?cmd=off" , data , 13 ) == 0) {
         digitalWrite(LEDpin, LOW);   // set the LED on
         #if DEBUG
           Serial.println("off received");
         #endif
       }

       // read data from text box
       if (strncmp( "GET /?boxa=" , data , 11 ) == 0) {
           Serial.print ( "text box input received - " );
           if (ether.findKeyVal(data + 6, TextBox1 , sizeof TextBox1 , "boxa") > 0) {
             // int value = atoi(TextBox1);   // command to convert a string to number
         #if DEBUG
             Serial.print ( "input = " );
             Serial.println ( TextBox1 );
         #endif
           }
       }



       ether.httpServerReply(homePage()); // send web page data
   }
  
}



// ----------------------------------------------
// end