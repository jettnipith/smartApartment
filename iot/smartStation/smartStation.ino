#include <EmonLib.h>
EnergyMonitor emon1;                   // Create an instance

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

#define D0 2
#define LED D0
#define relay1 D2

const char* ssid = "Godemo";
const char* password = "gl123456";

const char* host = "http://vrsim4learning.com/smartcafe"; //ใส่ IP หรือ Host ของเครื่อง Database ก็ได้

void setup() {
  pinMode(LED, OUTPUT);
  pinMode(relay1, OUTPUT);
  emon1.current(A0, 60);             // Current: input pin, calibration.
  digitalWrite(LED, LOW);
  digitalWrite(relay1, HIGH);

  Serial.begin(115200);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {

    delay(1000);
    Serial.print("Connecting..");

  }

}

void loop() {
  digitalWrite(LED, HIGH);
  delay(250);
  digitalWrite(LED, LOW);
  delay(250);




  //Relay process------------------------------------------------------------------------
  if (WiFi.status() == WL_CONNECTED) { //Check WiFi connection status

    HTTPClient http;  //Declare an object of class HTTPClient

    http.begin("http://vrsim4learning.com/smartcafe/get_relayStat.php?dif_id=1");  //Specify request destination
    int httpCode = http.GET();                                                                  //Send the request

    if (httpCode > 0) { //Check the returning code

      String payload = http.getString();   //Get the request response payload
      Serial.println(payload);                  //Print the response payload

      if (payload == "dif1Open") {
        digitalWrite(relay1, LOW);

        //Current process after open relay------------------------------------------------------------------------
        double Irms = emon1.calcIrms(1480);  // Calculate Irms only

        Serial.print(Irms * 220.0);       // Apparent power
        Serial.print(" ");
        Serial.println(Irms * 0.35);        // Irms

        float ws = Irms * 220.0;
        String txtws = String(ws);

        if (WiFi.status() == WL_CONNECTED) { //Check WiFi connection status
          HTTPClient http;  //Declare an object of class HTTPClient
          http.begin("http://vrsim4learning.com/smartcafe/save_wh.php?relay_stat=1&dif_id=1&ws=" + txtws); //Specify request destination
          int httpCode = http.GET();                                                                  //Send the request

          if (httpCode > 0) { //Check the returning code

            String checkVar = http.getString();   //Get the request response payload
            Serial.println(checkVar);                  //Print the response payload
          }

        }
      }
      else if (payload == "dif1Close") {
        digitalWrite(relay1, HIGH);
        //stop send data after close relay------------------------------------------------------------------------
        if (WiFi.status() == WL_CONNECTED) { //Check WiFi connection status
          HTTPClient http;  //Declare an object of class HTTPClient
          http.begin("http://vrsim4learning.com/smartcafe/save_wh.php?relay_stat=0"); //Specify request destination
          int httpCode = http.GET();                                                                  //Send the request
          if (httpCode > 0) { //Check the returning code

            String checkVar = http.getString();   //Get the request response payload
            Serial.println(checkVar);                  //Print the response payload
          }

        }


      }


    }

    http.end();   //Close connection

  }

  delay(500);    //Send a request every 30 seconds

}


