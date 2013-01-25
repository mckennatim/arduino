#include <EEPROM.h>
////slow down the loop and try someting
//TEST DATA
String readString = "<0173,1176,3123>";
int nc=2;//number of circuits
int mb =10; //memory begin offset
int me = mb+nc;//memory loc 9 is mb-1 and 
//TEST DATA
unsigned long lastConnectionTime = millis(); // last time you connected to the server, in millis
boolean lastConnected = false;
const unsigned long postingInterval = 2*1000; 
void setup()
{
  Serial.begin(9600);
}
void loop()
{
  if(millis() - lastConnectionTime > postingInterval) {
  	updateSetpts();
    lastConnectionTime=millis();
  }  
}

void updateSetpts(){
	int datapts;
	String ss;
	int newVal;
    int curVal;
    int idx;
  int pslen = readString.length()-2;
  char ps[pslen];
  for (int i = 1; i<pslen+1; i++){
	//Serial.print(readString.charAt(i));
    ps[i-1]=readString.charAt(i);
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
    
    //Serial.println(k);
    //Serial.println(pslen);
    //Serial.println(sizeof(strings));
    ///*

    for ( int j = 0; j < datapts; j++ ) 
    {
    	//Serial.println(strings[j]);
    	ss =strings[j];
    	idx= ss.substring(0,1).toInt();
    	if (idx<nc && idx>-1){//if new data is for sensors in play
	     	curVal = EEPROM.read(idx+mb);
	    	//Serial.println(curVal);        
	        newVal= ss.substring(1,4).toInt();
	    	//Serial.println(newVal); 
	        if(curVal != newVal){
				Serial.println("not the same");
				EEPROM.write(idx+mb, newVal);
                                delay(10);				
	        }  else{
                  Serial.print(ss);
                  Serial.println("newVal is same as curVal");
                }		
    	}else {
          Serial.println("sensor not implemented");
        }
    }
    //*/
}
