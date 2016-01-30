# esp8266 development
#### narrative
The installer opens a box with a wifi enabled controller and the relays and power supply needed to control this IOT application of weather condition controlled ice-dam heaters. 

The electrican has already been there, hanging a box on the panel, connecting the 220V roof panels through a 220 volt relay that is switched by a 110V pilot relay. The electrician has added a 220v breaker and a 110v breaker to the panel(or added to an existing 110V circuit). the 110v circuit powers the pilot relay, the 24 volt transformer and a 110v receptacle. The system now runs manually using the HOA switch on the pilot relay. He leaves a 100v receptacle and a low voltage connection block with 5 terminals: two of the terminals are the relay and the other 3 are the HOA digital monitoring signals.

The easisest thing would be a little circuit box with a 5 wire connector between what the electrican left and the microcontroller. Hook it up, plug in the usb power and find it on your laptop from its broadcasting AP. A form comes up asking for the ssid & passwd, and the device ID from the side of the box. Once entered, the mctrller resets and joins the home wifi network. 

Now the installer goes online to check if the device is operating by going to a special page that references the deviceid. If he sees the status and can change it he is done.

### tasks
- read pin 4 and update when changed
- set pin that setup() can read then decide to be AP or St
- store ssid and password in eeprom
- read ssid and password from eeprom

#### read pin 4 and update when changed

basic delay between checking inputs

    long now = millis();
    if (now - lastMsg > 10000) {
        lastMsg = now;
    }

every 1000 read HOA switch and auto relay setting

      if (now - lastMsg > 1000) {
        lastMsg = now; //reset timer
        oldHoah = hoah;
        oldHoaa = hoaa;
        hoah=digitalRead(HOAH);
        hoaa=digitalRead(HOAA);
        if (oldHoah != hoah || oldHoaa != hoaa) {
          digitalWrite(ALED, !digitalRead(ALED)); //temporary toggle sim input
          (hoah==1) ? hoa=0 : hoa=1; //if ho switch is open O else H 
          if(hoaa==0) hoa=2; //if ha is closed A
          digitalRead(ALED) ? haa=1 : haa=0;
          sprintf(astr, "{hoa:%d, auto:%d}", hoa, digitalRead(ALED));
          Serial.print(astr);
          client.publish("presence", astr, true);
        }
      }

#### refs
- <a href="http://www.esp8266.com/wiki/doku.php?id=esp8266_gpio_pin_allocations">gpio_pin_allocations</a> 
- <a href="http://www.forward.com.au/pfod/ESP8266/GPIOpins/index.html">How to use GPIO0, GPIO2 and GPIO15 pins as Outputs and Inputs on ESP8266 modules</a>
- <a href="https://cdn.sparkfun.com/datasheets/Wireless/WiFi/ESP8266-Thing-Dev-v10.pdf">thing dev schematic</a>
- <a href="http://www.wemos.cc/wiki/lib/exe/fetch.php?media=en:d1_mini.pdf">wemos mini schematic</a>