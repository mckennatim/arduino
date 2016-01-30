#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#define deviceId "AAAAA0"
#define cmd "AAAAA0/cmd"
#define wifi_ssid "street_no_vale2"
#define wifi_password "jjjjjjjj"

#define mqtt_server "10.0.1.102"
#define HOAH 4
#define HOAA 13
#define ALED 5
#define CMD 12



WiFiClient espClient;
PubSubClient client(espClient);

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(wifi_ssid);
  WiFi.begin(wifi_ssid, wifi_password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}
char incoming[40];
char rela;
int relay;
char c;
int oldLed;

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i=0;i<length;i++) {
    c = (char)payload[i];
    incoming[i] = c;
  }
  incoming[length] = '\0';
  String sinc = String(incoming).c_str();
  rela = sinc[sinc.indexOf(':')+1];
  relay = rela - '0';
  digitalWrite(ALED, relay);
  oldLed = !digitalRead(ALED);
  Serial.println(sinc + relay);
}
void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    // If you do not want to use a username and password, change next line to
    // if (client.connect("ESP8266Client")) {
    if (client.connect("ESP8266Client")) {
      Serial.println("connected");
      client.subscribe(cmd);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

long lastMsg = 0;
int hoa ;
int haa ;
int hoah = 0;
int hoaa = 0;
int oldHoah;
int oldHoaa;
char astr[80] ;

char status[80];
char payload[100];

void setup() {
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  pinMode(HOAH, INPUT);//pullup
  pinMode(HOAA, INPUT);//pullup
  pinMode(ALED, OUTPUT);
  digitalWrite(ALED, HIGH);
  oldLed = digitalRead(ALED);
  pinMode(CMD, INPUT);
  char arr[ ] = "This is a test";
  String sarr = String(arr).c_str();
  Serial.println(sarr + 456);

}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  long now = millis();
  if (now - lastMsg > 1000) {
    lastMsg = now; //reset timer
    oldHoah = hoah;
    oldHoaa = hoaa;
    hoah=digitalRead(HOAH);
    hoaa=digitalRead(HOAA);
    if (oldHoah != hoah || oldHoaa != hoaa || oldLed != digitalRead(ALED)) {
      oldLed = digitalRead(ALED);
      (hoah==1) ? hoa=0 : hoa=1; //if ho switch is open O else H 
      if(hoaa==0) hoa=2; //if ha is closed A
      digitalRead(ALED) ? haa=1 : haa=0;
      sprintf(astr, "{ \"hoa\":%d, \"auto\":%d }", hoa, digitalRead(ALED));
      Serial.print(astr);
      sprintf(status, "%s/status", deviceId);
      sprintf(payload,"{ payload:%s, topic:'%s' }", astr, status);
      Serial.println(payload);
      client.publish(status, astr, true);
    }
  }
}