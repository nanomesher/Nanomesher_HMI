// ---------------------------------------------------------------------------

#include <ESP8266WiFi.h>  
#include <Nextion.h>
#include <NextionPage.h>
#include <NextionText.h>
#include <NextionButton.h>
#include <NextionGauge.h>
#include <NextionNumber.h>
#include <NextionPicture.h>
#include <NextionSlider.h>
#include <NextionProgressBar.h>
#include <NextionSlidingText.h>
#include <ESP8266mDNS.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include "WiFiManager.h"  
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include "Adafruit_SHT31.h"
#include <SFE_BMP180.h>
#include <SoftwareSerial.h>

//Below for hardware serial
//#define nextion Serial

SoftwareSerial nextion(D6, D4); // RX, TX

Adafruit_SHT31 sht31 = Adafruit_SHT31();


//Setup our own MQTT Client
WiFiClient myclient;
String eventtopic = "user/" +  String(ESP.getChipId(),HEX) + "/event";
 
#define MQTT_SERVER      "io.adafruit.com"

Adafruit_MQTT_Client mymqtt = Adafruit_MQTT_Client(&myclient, MQTT_SERVER, 1883, "", "");
Adafruit_MQTT_Publish mymqttpublisher = Adafruit_MQTT_Publish(&mymqtt,  eventtopic.c_str());


bool objectDetected = false;


void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());
}

ESP8266WebServer server ( 80 );

//Display
Nextion nex(nextion);

//Buzzer
const int BuzzerPin = D0;

//LED
const int LEDPin = D5;

bool enabledMQTT = false;

void disableMQTT()
{
  enabledMQTT = false;
}

void enableMQTT() {
  if(server.hasArg("host")) {
    String host = server.arg("host");
    String port = server.arg("port");

    char charBuf[50];
    host.toCharArray(charBuf,50);
    mymqtt = Adafruit_MQTT_Client(&myclient, charBuf, (uint16_t)  (atoi(port.c_str())), "", "");
    mymqttpublisher = Adafruit_MQTT_Publish(&mymqtt,  eventtopic.c_str());
    enabledMQTT = true;
    server.send ( 200, "text/plain", "OK");
  }
  server.send ( 200, "text/plain", "No Argument was specified" );
}


void handleRoot() {
  server.send ( 200, "text/plain", "Nanomesher Wireless HMI" );
}

void ledon(){
  digitalWrite(LEDPin, HIGH);
  server.send ( 200, "text/plain", "OK");
}

void ledoff(){
  digitalWrite(LEDPin, LOW);
  server.send ( 200, "text/plain", "OK");
}

void dimmer(){
  sendCommand("dim=dim-10");
  server.send ( 200, "text/plain", "OK");
}

void brighter(){
  sendCommand("dim=dim+10");
  server.send ( 200, "text/plain", "OK");
}


void sendCommand(char* command){
  nextion.print (command);
  nextion.write (0xff);
  nextion.write (0xff);
  nextion.write (0xff);
}

void blinkled(){
  if(server.hasArg("t")) {
    int blinktimes = atoi(server.arg("t").c_str());

    if(blinktimes<=5)
    {
      for(int i=0;i<blinktimes;i++)
      {
        digitalWrite(LEDPin, HIGH);
        delay(250);
        digitalWrite(LEDPin, LOW);
        delay(250);
      }
    }
    server.send ( 200, "text/plain", "OK");
  }
  server.send ( 200, "text/plain", "No Argument was specified" );
}




void beep(){
  if(server.hasArg("s")) {
    String signalkhz = server.arg("s");
    String duration = server.arg("d");

    tone(BuzzerPin, atoi(signalkhz.c_str())); 
    delay(atoi(duration.c_str()));       
    noTone(BuzzerPin);     
    server.send ( 200, "text/plain", "OK");
  }
  server.send ( 200, "text/plain", "No Argument was specified" );
}

void localSetText(int page, int componentid, char* componentname, char* displayText){
  NextionText text(nex, page, componentid, componentname);
  text.setText(displayText);  
}

void setNextionPicture(){
  if(server.hasArg("p")) {
    String page = server.arg("p");
    String componentid = server.arg("c");
    String componentname = server.arg("n");
    char charBuf[50];
    componentname.toCharArray(charBuf,50);

    
    NextionPicture picture(nex, atoi(page.c_str()), atoi(componentid.c_str()), charBuf);
    
    if(server.hasArg("pid"))
    {
      String picturepid = server.arg("pid");
      picture.setPictureID(atoi(picturepid.c_str()));
    }
    
    server.send ( 200, "text/plain", "OK");
  }
  server.send ( 200, "text/plain", "No Argument was specified" );
  
}

void setNextionText(){
  if(server.hasArg("p")) {
    String page = server.arg("p");
    String componentid = server.arg("c");
    String componentname = server.arg("n");
    char charBuf[50];
    componentname.toCharArray(charBuf,50);

    
    NextionText text(nex, atoi(page.c_str()), atoi(componentid.c_str()), charBuf);
    
    if(server.hasArg("v"))
    {
      String displaytext = server.arg("v");
      char displayTextArray[100];
      displaytext.toCharArray(displayTextArray,100);
      text.setText(displayTextArray);
  
    }
    
    if(server.hasArg("fc"))
    {
      String colourvalue = server.arg("fc");
      text.setForegroundColour(atoi(colourvalue.c_str()));      
    }

    if(server.hasArg("bc"))
    {
      String bgcolourvalue = server.arg("bc");
      text.setBackgroundColour(atoi(bgcolourvalue.c_str()));      
    }

    
    server.send ( 200, "text/plain", "OK");
  }
  server.send ( 200, "text/plain", "No Argument was specified" );
}

void getTemperatureC()
{
    Adafruit_SHT31 sht31 = Adafruit_SHT31();
    if (! sht31.begin(0x44)) {
      server.send ( 200, "text/plain", "SHT31 not found");
    }
    else
    {
        float t = sht31.readTemperature();
        char temp[10];
        dtostrf(t,1,1,temp);
        server.send ( 200, "text/plain", temp);
    }
}


void getTemperatureF()
{
    Adafruit_SHT31 sht31 = Adafruit_SHT31();
    if (! sht31.begin(0x44)) {
      server.send ( 200, "text/plain", "SHT31 not found");
    }
    else
    {
        float t = sht31.readTemperature();
        t = (t*1.8) + 32;
        char temp[10];
        dtostrf(t,1,1,temp);
        server.send ( 200, "text/plain", temp);
    }
}

void getHumidity()
{
    Adafruit_SHT31 sht31 = Adafruit_SHT31();
    if (! sht31.begin(0x44)) {
      server.send ( 200, "text/plain", "SHT31 not found");
    }
    else
    {
        float h = sht31.readHumidity();
        char humid[10];
        dtostrf(h,1,1,humid);
        server.send ( 200, "text/plain", humid);
    }
   
  
}

void getPressure()
{
  SFE_BMP180 pressure;
  pressure.begin();

  
  char status;
  double T,P;
  status = pressure.startTemperature();
  if (status != 0)
  {
    // Wait for the measurement to complete:
    delay(status);
    status = pressure.getTemperature(T);
    if (status != 0)
    {
      status = pressure.startPressure(3);
      if (status != 0)
      {

        delay(status);

        status = pressure.getPressure(P,T);
        if (status != 0)
        {
          char pressure[10];
          dtostrf(P,1,1,pressure);
          server.send ( 200, "text/plain", pressure);
          return;
        }
      }
    }
  }

  server.send ( 200, "text/plain", "Cannot read pressure. Please check if BMP180 sensor is connected");
}

void setNextionSlidingText(){
  if(server.hasArg("p")) {
    String page = server.arg("p");
    String componentid = server.arg("c");
    String componentname = server.arg("n");
    char charBuf[50];
    componentname.toCharArray(charBuf,50);

    
    NextionSlidingText text(nex, atoi(page.c_str()), atoi(componentid.c_str()), charBuf);
    
    if(server.hasArg("v"))
    {
      String displaytext = server.arg("v");
      char displayTextArray[100];
      displaytext.toCharArray(displayTextArray,100);
      text.setText(displayTextArray);
  
    }
    
    if(server.hasArg("fc"))
    {
      String colourvalue = server.arg("fc");
      text.setForegroundColour(atoi(colourvalue.c_str()));      
    }

    if(server.hasArg("bc"))
    {
      String bgcolourvalue = server.arg("bc");
      text.setBackgroundColour(atoi(bgcolourvalue.c_str()));      
    }

    
    server.send ( 200, "text/plain", "OK");
  }
  server.send ( 200, "text/plain", "No Argument was specified" );
}


void setNextionNumber(){
  if(server.hasArg("p")) {
    String page = server.arg("p");
    String componentid = server.arg("c");
    String componentname = server.arg("n");
    char charBuf[50];
    componentname.toCharArray(charBuf,50);

    
    NextionNumber number(nex, atoi(page.c_str()), atoi(componentid.c_str()), charBuf);
    
    if(server.hasArg("v"))
    {
      String value = server.arg("v");
      number.setValue(atoi(value.c_str()));  
    }
    
    if(server.hasArg("fc"))
    {
      String colourvalue = server.arg("fc");
      number.setForegroundColour(atoi(colourvalue.c_str()));      
    }

    if(server.hasArg("bc"))
    {
      String bgcolourvalue = server.arg("bc");
      number.setBackgroundColour(atoi(bgcolourvalue.c_str()));      
    }

    
    server.send ( 200, "text/plain", "OK");
  }
  server.send ( 200, "text/plain", "No Argument was specified" );
}

void setNextionGauge(){
  if(server.hasArg("p")) {
    String page = server.arg("p");
    String componentid = server.arg("c");
    String componentname = server.arg("n");
    String value = server.arg("v");

    char componentNameBuf[50];
    componentname.toCharArray(componentNameBuf,50);

        
    NextionGauge gauge(nex, atoi(page.c_str()), atoi(componentid.c_str()), componentNameBuf);
    gauge.setValue(atoi(value.c_str()));
    server.send ( 200, "text/plain", "OK");
  }
  server.send ( 200, "text/plain", "No Argument was specified" );
}

void setNextionProgressBar(){
  if(server.hasArg("p")) {
    String page = server.arg("p");
    String componentid = server.arg("c");
    String componentname = server.arg("n");
    String value = server.arg("v");

    char componentNameBuf[50];
    componentname.toCharArray(componentNameBuf,50);

        
    NextionProgressBar progressBar(nex, atoi(page.c_str()), atoi(componentid.c_str()), componentNameBuf);
    progressBar.setValue(atoi(value.c_str()));

    server.send ( 200, "text/plain", "OK");
  }
  server.send ( 200, "text/plain", "No Argument was specified" );
}


void setNextionSlider(){
  if(server.hasArg("p")) {
    String page = server.arg("p");
    String componentid = server.arg("c");
    String componentname = server.arg("n");
    String value = server.arg("v");

    char componentNameBuf[50];
    componentname.toCharArray(componentNameBuf,50);

        
    NextionSlider nextionSlider(nex, atoi(page.c_str()), atoi(componentid.c_str()), componentNameBuf);
    nextionSlider.setValue(atoi(value.c_str()));

    server.send ( 200, "text/plain", "OK");
  }
  server.send ( 200, "text/plain", "No Argument was specified" );
}

void getStatus(){

//      server.send ( 200, "text/plain", WiFi.localIP());
}

  
void setup() {
  

  WiFiManager wifiManager;
  
  wifiManager.setAPCallback(configModeCallback);
  if(!wifiManager.autoConnect("NanomesherHMI")) {
    //Serial.println("failed to connect and hit timeout");
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(1000);
  } 

  
  pinMode(BuzzerPin, OUTPUT);
  pinMode(LEDPin, OUTPUT);
  
  //Register root
  server.on ( "/", handleRoot );
  
  server.on ( "/SetNextionText", setNextionText);
  server.on ( "/SetNextionGauge", setNextionGauge);
  server.on ( "/SetNextionProgressBar", setNextionProgressBar);
  server.on ( "/SetNextionNumber", setNextionNumber);
  server.on ( "/SetNextionPicture", setNextionPicture);
  server.on ( "/SetNextionSlidingText", setNextionSlidingText);
  server.on ( "/SetNextionSlider", setNextionSlider);
  server.on ( "/Beep",beep);
  server.on ( "/LedOn",ledon);
  server.on ( "/LedOff",ledoff);
  server.on ( "/BlinkLed",blinkled);
  server.on ( "/Dimmer",dimmer);
  server.on ( "/Brighter",brighter);
  server.on ( "/DisableMQTT",disableMQTT);
  server.on ( "/EnableMQTT",enableMQTT);
  server.on ( "/Status",getStatus);
  server.on ( "/GetPressure",getPressure);
  server.on ( "/GetTemperatureC",getTemperatureC);  
  server.on ( "/GetTemperatureF",getTemperatureF);  
  server.on ( "/GetHumidity",getHumidity);  
  server.begin();


  nextion.begin(9600);
  nex.init();

  //Show IP address
  String wifist = ipToString(WiFi.localIP());

  char wifistBuf[50];
  wifist.toCharArray(wifistBuf,50);
  NextionText text(nex, 0, 3, "txt");
  text.setText(wifistBuf);



}

String ipToString(IPAddress ip){
  String s="";
  for (int i=0; i<4; i++)
    s += i  ? "." + String(ip[i]) : String(ip[i]);
  return s;
}

int connection_slow =0;

float t=0;
float h=0;


void loop() {

  if(enabledMQTT)
  {
    myMQTT_connect();
  }
  
  server.handleClient();

  while (nextion.available() > 0)
  {
    char c = nextion.read();

    if (c == NEX_RET_EVENT_TOUCH_HEAD)
    {
      delay(10);

      if (nextion.available() >= 6)
      {
        static uint8_t buffer[8];
        buffer[0] = c;

        uint8_t i;
        for (i = 1; i < 7; i++)
          buffer[i] = nextion.read();
        buffer[i] = 0x00;

        if (buffer[4] == 0xFF && buffer[5] == 0xFF && buffer[6] == 0xFF)
        {
          mymqttpublisher.publish(buffer[2]);
         
          /*
          ITouchableListItem *item = m_touchableList;
          while (item != NULL)
          {
            item->item->processEvent(buffer[1], buffer[2], buffer[3]);
            item = item->next;
          }*/
        }
      }
    }
  }
  
}


//-------------------- my MQTT io code
void myMQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mymqtt.connected()) {
    return;
  }

  //Serial.print("Connecting to My MQTT... ");

  bool myConnectionSuccess=true;
  uint8_t retries = 1;
  while ((ret = mymqtt.connect()) != 0) { // connect will return 0 for connected
       //Serial.println(mymqtt.connectErrorString(ret));
       //Serial.println("Retrying my MQTT connection in 5 seconds...");
       mymqtt.disconnect();
       retries--;
       if (retries == 0) {
          myConnectionSuccess = false;
          break;
       }
  }

}
