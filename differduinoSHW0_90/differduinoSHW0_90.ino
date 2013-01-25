/*

 Differduino Solar Hot Water ver 0.90.1
 Works with the PCB Differduino ver 0.90
 
 Open Source solar hot water differential controller.
 
 www.nateful.com
 
 Uses Dallas 1-wire DS18B20 temperature sensors.
 Decides when to circulate based on collector vs tank temperature.
 Turns on circulator pump via digital output hooked to a relay circuit.
 
 Posts data to the free graphing site www.cosm.com (formerly pachube.com) via a wiz812mj ethernet module
 using the standard Arduino ethernet library. Also gets a control feed from Cosm
 to remotely change the solar hot water control variables.
 
 All temperatures are in fahrenheit
 
 Changes from ver 0.90:
 Corrected fault condition to post invalidData bool AND pumpOn bool to Cosm.
 Was only posting invalid bool, and not updating the pump state, which graphed the pump as on, 
 even though controller shut it down due to excessive bad thermometer reads.
 
 
 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details. <http://www.gnu.org/licenses/>.
 
 */



//Libraries needed for Dallas 1-wire thermometers
#include <OneWire.h>
#include <DallasTemperature.h>

//Libraries needed for ethernet
#include <SPI.h>
#include <Ethernet.h>

//library to simplify put and get from pachube (cosm).
#include "ERxPachube.h"

//Library needed for the lcd screen
//#include <LiquidCrystal.h>







//ADJUSTMENTS!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//These variables are where the adjustments are made that effect when system will cycle.
//These are the defaults that are used when the differential controller first powers up, and should work ok for most systems. 
//If a cosm 'control' feed is used, these variables can be adjustable through the cosm interface,
//but system defaults to the values below upon restart/reset of the controller.

//set maximum tank temperature
int tankMax = 140;

//if tankMax is met, system won't cycle back on till tank cools down a bit. Keeps from overcycling.
float tankCooldown = 1.0;

//set how much hotter collector should be than tank before turning on pump
int setDifferential = 30;

//set the minumum differential to decide when to turn off pump 
int minDifferential = 10;
////////////////////////////////////////////////////////////////////////////////////////////////







//SET YOUR THERMOMETER ADDRESSES!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//The 1-wire thermometers are identified by their unique hexidecimal identifier
//use the example sketches provided with the DallasTemperature library 
//to get each device address.

//I used my own as the placeholders. You must change these to your own.
byte tanktopThermometer[8] = {0x28, 0x74, 0x28, 0xe3, 0x02, 0x0, 0x0, 0x12};
byte tankbottomThermometer[8] =  {0x28, 0x1e, 0x99, 0xe3, 0x02, 0x0, 0x0, 0x9c};
byte collectorThermometer[8] = {0x28, 0xda, 0x26, 0xe3, 0x02, 0x0, 0x0, 0xff};
////////////////////////////////////////////////////////////////////////////////////////

//define input pin for 1-wire bus. The Differduino PCB uses digital pin 9
#define ONE_WIRE_BUS 9
// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(ONE_WIRE_BUS);
// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);







////physical mac address of ethernet. The wiz812mj does not come with an assigned mac address! 
//Give it any mac (hexidecimal) you wish, AS LONG AS device is connected to your lan, NOT directly to the internet.
//if you wish to connect directly to the internet, with a public IP address, you must find a valid
//assigned mac address, for example, you could use a mac address from some other broken ethernet devise.
//if you have any doubt what this means, do some reasearch!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! 
byte    mac[] =     { 0x00, 0x00, 0x00, 0x00, 0x11, 0x11 }; 


//SET ETHERNET ADDRESS!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// Network address for the wiznet ethernet module is assigned statically. 
// DHCP is possible with the arduino libraries, but I static is much simpler and smoother.
// Pick an ip address from your own local area network (LAN) that is not in use, and is 
// not in the DHCP pool of your router.
byte    ip[] =      { 192, 168, 0, 99 };







//SET UP COSM!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//To graph to Cosm.com, a cosm account must be set up, and at least one "feed" created. 
//Fill in below, the API key and Feed ID(s) from your own account.
//fill in your own API key here within the quotes 
#define PACHUBE_API_KEY				"paste your API key here"
// replace 00000 with your feed id's
#define PutFeed			00000 //the data we will be posting to cosm
#define GetFeed                 00000 //the control variables we get from cosm


//other cosm stuff
ERxPachubeDataOut dataout(PACHUBE_API_KEY, PutFeed);
ERxPachubeDataIn datain(PACHUBE_API_KEY, GetFeed);
void PrintDataStream(const ERxPachube& pachube);






//timings. Puts to cosm every 30sec, takes temp readings every 5 sec. 
int pachubeDelay = 30000; 
long pachubeWait =10000; 

int readingDelay = 5000;
long readingWait =1000;



//define pin for pump relay
const int pump = A0;

//define pins used by lcd display. 
//LiquidCrystal lcd (3, 4, 5, 6, 7, 8 );




//declare some variables

//used for control logic
static bool pumpOn;
static bool pumpOverride;
static bool tankMaxed;
static bool validData;
int invalidCount=0;

//temperature variables
float tank;
float collector;
float tankbottom; 
float differential;


//charactar arrays to hold values as strings. For printing to cosm without so many decimal places.
char tank_str[8];
char collector_str[8];
char tankbottom_str[8];
char differential_str[8];

// Number of temperature devices found
int numberOfDevices; 









//this part of the code runs once to get everything setup and started.
void setup ()
{

  // start the ethernet connection
  Ethernet.begin(mac, ip);
 
  // Start up the 1-wire library
  sensors.begin();
 
  //initialize lcd screen
  //lcd.begin(16,2);
 
  //set pins to output
  pinMode (pump, OUTPUT);

  //start serial port and print information for debugging
  Serial.begin(9600);
  Serial.println ("Diffduino Ver 0.90");
  Serial.println ("open source");
  Serial.println ("Solar hot water Differential Controller");
  Serial.println ("www.nateful.com");
  Serial.println();
  
  //lcd.print ("Differduino 0.90");
  //lcd.setCursor(0,1);
  //lcd.print ("www.nateful.com");

  
  // locate devices on the bus
  Serial.println("");
  Serial.print("Locating 1-wire devices...");
  Serial.println ("");
  Serial.print("Found ");
  Serial.print(sensors.getDeviceCount(), DEC);
  Serial.println(" devices.");

  // report parasite power requirements
  Serial.print("Parasite power is: "); 
  if (sensors.isParasitePowerMode()) Serial.println("ON");
  else Serial.println("OFF");
  Serial.println ("");

  // set the sensor resolution..
  //Resolution can be set from 9 to 12 bit. The higher the resolution, the slower the read time
  //12 bit resolution is still plenty fast for most applications. We're talking about milliseconds differances
  sensors.setResolution(tanktopThermometer, 12);
  sensors.setResolution(collectorThermometer, 12);
  sensors.setResolution(tankbottomThermometer, 12);

  //define datastreams for posting to Cosm
  dataout.addData(0);
  dataout.addData(1);
  dataout.addData(2);
  dataout.addData(3);
  dataout.addData(4);
  dataout.addData(5);

  //delay a bit to give everything time to start
  delay (3000);
}



//the loop does just that, loops over and over again.
void loop ()
{



  //if the pumpOverride bool is set to 1 (via cosm control feed), pump runs no matter what.
  if (pumpOverride) 
  {
    if (!pumpOn)
    {
      digitalWrite (pump, HIGH);
      pumpOn = true; 
    } 
  }
  
  
  if (millis() >= readingWait) 
  {
    //get the values from the DS8B20's 
    sensors.requestTemperatures();

    //assign value to variables
    tank = (sensorValue(tanktopThermometer));
    collector = (sensorValue(collectorThermometer));
    tankbottom = (sensorValue(tankbottomThermometer));


    //I've set this to be pretty tolerant of bad sensor reads. It must get 20 bad reads in a row before the system is shut down.
    if (invalidCount > 20)
    {
      Serial.println ("check sensors...too many failed reads!!");
      
      
      //print invalid warning to lcd
      //lcd.clear();
      //lcd.setCursor (0,0);
      //lcd.print ("sensors failed");
      //delay(3000);
      //printLCD();
      
      
      if (!pumpOverride) 
      {
        digitalWrite (pump, LOW);
        pumpOn =false;
      }
    }



    //here is where we decide if the reading from the DS18B20 is valid. I use simple comparisons here.
    //If thermometer is hooked up wrong, or wires cross etc, they can return a temp of exactly
    //185.0 and 32.0 degrees F. This would indicate a bad sensor reading. 
    Serial.println ("validating sensor readings...");

    if ( tank < 35 || tank > 180 ) {
      validData = false;
    }   
    else 
      if ( tankbottom < 35 || tankbottom > 180 ) {
      validData = false;
    }
    else
      if (collector <-30 || collector > 250 || collector == 185.0 || collector ==32.0) {
        validData = false;
      } 
      else {
        validData = true;
      }  


    //if sensor readings are invalid, print to serial for debugging
    // and put an invalid data bool to cosm feed
    if (!validData)
    {

      Serial.println ("failed to validate sensor readings. Check connections and sensors");
      Serial.print ("tank ");
      Serial.println (tank);
      Serial.print ("tank bottom ");
      Serial.println (tankbottom);
      Serial.print ("collector ");
      Serial.println (collector);
      Serial.println("");
      
      //printLCD();

      //post invalid bool to cosm
      Serial.println("Posting invalid bool and pump state to Cosm...........");
      dataout.updateData(10, !validData );
      dataout.updateData(4, pumpOn);
      int status = dataout.updatePachube();
      Serial.print("sync status code <OK == 200> => ");
      Serial.println(status);
      invalidCount++; //incriment the invalid counter

    }

    else
    {
      //if sensor readings are valid, make pump decisions

      Serial.println ("sensor readings are valid");
      invalidCount = 0; //we got a good read, so invalid count is reset to zero. 

      //this should be self explainatory!
      differential=collector-tankbottom;   

      //prints values to serial port 
      Serial.print ("tank ");
      Serial.println (tank);
      Serial.print ("tank bottom ");
      Serial.println (tankbottom);
      Serial.print ("collector ");
      Serial.println (collector);
      Serial.print ("differential ");
      Serial.println (differential);
      Serial.print ("pump state ");
      Serial.println (pumpOn);
      Serial.print ("pump override ");
      Serial.println (pumpOverride);
      Serial.println ("");
      Serial.println ("");
      Serial.println ("");
      Serial.println ("");
      
      //printLCD();
      

      /*CONTROL!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
       Here is the logic that decides when to turn on and off the circulator pump
       
       The first part checks on the max tank temperature. If max tank temp is reached. the bool tankMaxed is set to true
       amd the pump turns off. tankMaxed will not be set to false until tank cools down by tankCooldown amount
       
       
       If pump is not over ridden, and
       If tank is not maxed:
       
       If the pump is ON:
       if the differential drops below the minDifferential, then turn pump off
       
       If the pump is off:
       if the differentual is greater than the setDifferential, then turn pump On
       */

      if (!pumpOverride)
      {
        
        if (tankMaxed)
        {
          if  (tank < (tankMax - tankCooldown ) ) 
          { 
            tankMaxed =false;
          }
        }
        else
        {
          if ( tank > tankMax )       
          { 
            tankMaxed = true;
            digitalWrite (pump, LOW);
            pumpOn =false;

          }
        }


        if (!tankMaxed)
        {
          if (pumpOn)
          {
           
            if ( (differential < minDifferential ) )
            {
              digitalWrite (pump, LOW);  
              pumpOn =false;
            }
          }
          else
          {
            if ( (differential > setDifferential ) )       
            {
              digitalWrite (pump, HIGH); 
              pumpOn = true;  
            }
          } 
        }
      }
    }
    readingWait = millis() + readingDelay;
  }



  //put and get data to/from pachube
  if (millis() >= pachubeWait)
  { 
    Serial.println("Getting control stream");
    int status = datain.syncPachube();

    //only get data if there is a connection and a valid control stream
    if (status == 200) 
    {
      tankMax = datain.getValueInt(0);
      tankCooldown = datain.getValueFloat(1);
      setDifferential = datain.getValueInt(2);
      minDifferential = datain.getValueInt(3);
      pumpOverride = datain.getValueInt(4);

    }
    Serial.print("sync status code <OK == 200> => ");
    Serial.println(status);

    PrintDataStream(datain);
    
    //only post to Cosm feed if we have valid data from sensors.
    if (validData)
    {
      
      Serial.println("Posting sensor data to Pachube...........");

      //convert floats to string for 2 decimal posting to cosm  
      dtostrf(tank, -7, 2, tank_str);
      dtostrf(collector, -7, 2, collector_str);
      dtostrf(tankbottom, -7, 2, tankbottom_str);
      dtostrf(differential, -6, 2, differential_str);
      
      dataout.updateData(0, tank_str);
      dataout.updateData(1, tankbottom_str);
      dataout.updateData(2, collector_str);
      dataout.updateData(3, differential_str);
      dataout.updateData(4, pumpOn);
      dataout.updateData(5, !validData);

      int status = dataout.updatePachube();

      Serial.print("sync status code <OK == 200> => ");
      Serial.println(status);

      PrintDataStream(dataout);

      Serial.println("");

    }

    pachubeWait = millis() + pachubeDelay;    
  }

}
//And thats the end of the loop.


//Here are a couple simple functions used in the code.





//sensorValue function
//reads temp from sensors and returns as floating point value in F.
//To use celsius instead, the last line should be changed to "return tempC".

float sensorValue (byte deviceAddress[])
{
  float tempC = sensors.getTempC (deviceAddress);
  float tempF = (DallasTemperature::toFahrenheit(tempC));
  return tempF;
}



/*
//function to print sensor values to lcd screen
void printLCD ()
{
 //refresh screen
 lcd.clear();
  
 //print values to lcd screen
 lcd.setCursor (0,0);
 lcd.print ("c");
 lcd.setCursor (1,0);
 lcd.print (collector);
 lcd.setCursor (9,0);
 lcd.print ("t");
 lcd.setCursor (10,0);
 lcd.print (tank);
 lcd.setCursor (9,1);
 lcd.print ("b");
 lcd.setCursor (10,1);
 lcd.print (tankbottom);
 lcd.setCursor (0,1);
 lcd.print ("d");
 lcd.setCursor (1,1);
 lcd.print (differential);
}  
*/



//function to print to serial the data stream that will be posted to Cosm
void PrintDataStream(const ERxPachube& pachube)
{
  unsigned int count = pachube.countDatastreams();
  Serial.print("data count=> ");
  Serial.println(count);

  Serial.println("<id>,<value>");
  for(unsigned int i = 0; i < count; i++)
  {
    Serial.print(pachube.getIdByIndex(i));
    Serial.print(",");
    Serial.print(pachube.getValueByIndex(i));
    Serial.println();
  }
}





