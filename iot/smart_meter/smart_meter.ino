#include <string> 
#include <EmonLib.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include<SPI.h>
#include<SD.h>
#if defined(ESP8266)
#include <pgmspace.h>
#else
#include <avr/pgmspace.h>
#endif
#include <Wire.h>
#include <RtcDS3231.h>
#define countof(a) (sizeof(a) / sizeof(a[0]))
RtcDS3231<TwoWire> Rtc(Wire);
EnergyMonitor emon1;
String device_id = "Bjf825qW";
WiFiServer server(80);
char tempAPname[15] = {};
bool interval=false;
bool firstTime=true;
int checkk;
int tempp;
int cnt; 
String uri;
char datestring[20];
double Irms ;
unsigned long timeout;
String req2;
String limit;
float lim;
void setup() {
  Serial.begin(9600);
  Rtc.Begin();
  emon1.current(A0,7.458);
  pinMode(D8,OUTPUT);
  Serial.println("\n\nInnitialing SD card");
  if (SD.begin(D3))
  {
    Serial.println("Innitialing SD card succeed");
  }
  else
  {
    Serial.println("Innitialing SD card failed");
  }
  if (!SD.exists("/status.txt"))
  {
    WiFi.mode(WIFI_AP);         //---------------<Make Access point part>------------------
    uint8_t mac[WL_MAC_ADDR_LENGTH];
    WiFi.softAPmacAddress(mac);
    String macID = String(mac[WL_MAC_ADDR_LENGTH - 2], HEX) + String(mac[WL_MAC_ADDR_LENGTH - 1], HEX);
    macID.toUpperCase();
    String AP_nameString = "SmartMeter" + macID;
    char AP_nameChar[AP_nameString.length() + 1];
    memset(AP_nameChar, 0 , AP_nameString.length() + 1);
    for (int i = 0; i < AP_nameString.length(); i++)
    {
      AP_nameChar[i] = AP_nameString.charAt(i);
    }
    strcpy(tempAPname, AP_nameChar);
    if (WiFi.softAP(AP_nameChar))
    {
      Serial.println("Set Wifi Success");
      Serial.print("SSID:");
      Serial.println(AP_nameChar);
    }
    else
    {
      Serial.println("Set Wifi failed");
    }
    server.begin();
    Serial.println("Server start success");
  }
  else
  {
    WiFi.mode(WIFI_STA);  //---------------<Connect to wifi part>------------------
    String data = "";
    File myFile = SD.open("/status.txt");
    if (myFile)
    {
      Serial.println("Open status.txt success");
      while (myFile.available())
      {
        data += (char)myFile.read();
      }
      myFile.close();
      Serial.print(data);
      int index = 10;
      String wifiName = "";
      String wifiPass = "";
      while (data[index] != '\r')
      {
        wifiName += data.charAt(index);
        index++;
      }
      index = 28;
      while (data[index] != '\r')
      {
        wifiPass += data.charAt(index);
        index++;
      }
      WiFi.begin( wifiName.c_str(),wifiPass.c_str());//---------------<function connect to wifi >------------------
      while (WiFi.status() != WL_CONNECTED)  
      {
            delay(500);
            Serial.print(".");
      }
        Serial.println("Set Wifi success");
        Serial.print("SSID:");
        Serial.println(WiFi.SSID());

    }
    else
    {
      Serial.println("Open status.txt failed");
    }
    delay(1000);
  }

}

void loop() {
  if (SD.exists("/status.txt"))
  {
    WiFiClient upserv;            //--------------------------------------Object For Connect To Server
    upserv.flush();
    Serial.println("connecting to:vrsim4learning.com");
    cnt = 0;
    while (!upserv.connect("vrsim4learning.com", 80)) //-------------------Function Connect To Server
    {
      Serial.print(".");
      delay(100);
      cnt++;
      if (cnt > 50)
      {
        Serial.println("\r\n connected failed");
        return;
      }
    }
    Serial.println("Connect to server success");
    uri = "/smartmeter/api.php?id=";
    uri += device_id + "&";
    RtcDateTime now = Rtc.GetDateTime();
    snprintf_P(datestring,
               countof(datestring),
               PSTR("%02u/%02u/%04u.%02u:%02u:%02u"),
               now.Day(),
               now.Month(),
               now.Year(),
               now.Hour(),
               now.Minute(),
               now.Second());
    uri += "t=" + String(datestring);
    Irms = emon1.calcIrms(1500);
    uri += "&d=" +String(Irms*1.127272727*239);
    uri += "&nd=" +String(Irms*1.127272727);
    Serial.print("uri:");
    Serial.println(uri);
    upserv.print(String("GET ") + uri + " HTTP/1.1\r\n" +   //-------------------Function sent data to server
                 "Host: vrsim4learning.com\r\n" +
                 "Connection: close\r\n\r\n");
    timeout = millis();
    while (upserv.available() == 0)
    {
      if (millis() - timeout > 5000)
      {
        Serial.println(" >>> Client Timeout !");
        upserv.stop();
        return;
      }

    }
    req2 ="";
    while(upserv.available())            //-------------------Function get response from server
    {
       req2+=upserv.readStringUntil('\r');
    }
    int len = req2.indexOf("limit:");
    limit ="";
    for(int i=len+6;req2[i]!='\n';i++)
    {
      limit+=req2[i];
    }
    lim = ::atof(limit.c_str());
    Serial.println(lim);
    if(lim!=-1)
    {
      if(Irms*1.127272727>lim)
      {
        digitalWrite(D8,HIGH);
      }
      else
      {
        digitalWrite(D8,LOW);
      }
    }
    else
    {
      digitalWrite(D8,LOW);
    }
    Serial.println("Sent success");
    delay(10);
  }
  else
  {
    WiFiClient client = server.available();  //-------------------Make Web Server part
    if (!client)
    {
      return ;
    }
    else
    {
      Serial.println("Client requese");
      String req = client.readStringUntil('\r');
      Serial.println(req);
      client.flush();
      int index = req.indexOf("id");
      String res = "";
      if (index != -1)
      {
        String WifiName = "";
        String WifiPass = "";
        index += 3;
        while (req[index] != '&')
        {
          WifiName += req[index];
          index++;
        }
        index += 6;
        while (req[index] != ' ')
        {
          WifiPass += req[index];
          index++;
        }
        int Status;
        WiFi.begin(WifiName.c_str(), WifiPass.c_str());
        unsigned long timeout2 = millis();
        bool ck=false;
        while (WiFi.status() != WL_CONNECTED)  
        {
            delay(500);
            Serial.print(".");
            if(millis()-timeout2>50000)
            {
              ck=true;
              break;
            }
         }
        if (!ck)
        {
          Serial.println("Connected success");
          Serial.println(WiFi.SSID());
          res = "HTTP/1.1 200 OK\r\n";
          res += "Content-Type: text/html\r\n\r\n";
          res += "<!DOCTYPE HTML>\r\n<html>\r\n";
          res += "<p>Connected succeed</p>\r\n";
          res += "</html>\r\n";
          client.print(res);
          delay(100);
          client.stop();
          Serial.println("Client disconnected");
          File myFile = SD.open("/status.txt", FILE_WRITE);
          if (myFile)
          {
            Serial.println("Create status.txt success");
            myFile.print("WiFiName: ");
            myFile.println(WifiName);
            myFile.print("Password: ");
            myFile.println(WifiPass);
            myFile.close();
            WiFi.mode(WIFI_STA);
            delay(100);
            return ;
          }
          else
          {
            Serial.println("Create status.txt failed");
            return ;
          }
        }
        else
        {
          Serial.println("Connected failed");
          res = "HTTP / 1.1 200 OK\r\n";
          res += " < Content - Type: text / html\r\n\r\n";
          res += " < !DOCTYPE HTML > \r\n<html>\r\n";
          res += "<p>Connected failed < / p > \r\n";
          res += "</html>\r\n";
          client.print(res);
          delay(100);
          Serial.println("Client disconnected");
        }




      }
      else
      {
        res = "HTTP/1.1 200 OK\r\n";
        res += "<Content-Type: text/html\r\n\r\n";
        res += "<!DOCTYPE HTML>\r\n<html>\r\n";
        res += "<form action = \"\"method=\"get\">\r\n";
        res += "WifiName:<input type=\"text\"name=\"id\"required=\"required\"/>\r\n";
        res += "<br/>Password:<input type=\"text\"name=\"pass\"/>\r\n";
        res += "<br/><input type=\"submit\"value=\"save\"/>\r\n";
        res += "</form>\r\n";
        res += "</html>\n";
        client.print(res);
        delay(1);
        Serial.println("Client disconnected");
    
      }

    }
  }

}
