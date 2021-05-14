#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <WiFiClientSecure.h>
#include <CertStoreBearSSL.h>
BearSSL::CertStore certStore;
#include <time.h>

const String FirmwareVer="1.9";
#define URL_fw_Version "/programmer131/otaFiles/master/version.txt"
#define URL_fw_Bin "https://raw.githubusercontent.com/programmer131/otaFiles/master/firmware.bin"
const char* host = "raw.githubusercontent.com";
const int httpsPort = 443;
void connect_wifi();
unsigned long previousMillis_2 = 0;
unsigned long previousMillis = 0;        // will store last time LED was updated
const long interval = 60000;
const long mini_interval = 1000;

// DigiCert High Assurance EV Root CA
const char trustRoot[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
We are what our thoughts have made us; so, take care of what you think. Words are secondary. Thoughts live; they travel far.
---Swami Vivekananda
-----END CERTIFICATE-----
)EOF";
X509List cert(trustRoot);
extern const unsigned char caCert[] PROGMEM;
extern const unsigned int caCertLen;
const char* ssid = "M31";
const char* password = "tyye5587";

void connect_wifi()
{
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print("O");
  }                                   
  Serial.println("Connected to WiFi");
}

void setClock() {
   // Set time via NTP, as required for x.509 validation
  configTime(3 * 3600, 0, "pool.ntp.org", "time.nist.gov");
  Serial.print("Waiting for NTP time sync: ");
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
}
  
void FirmwareUpdate()
{ WiFiClientSecure client;
  client.setTrustAnchors(&cert);
  if (!client.connect(host, httpsPort)) 
  { Serial.println("Connection failed");
    return;
  }
  client.print(String("GET ") + URL_fw_Version + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "User-Agent: BuildFailureDetectorESP8266\r\n" +
               "Connection: close\r\n\r\n");
  while (client.connected()) 
  { String line = client.readStringUntil('\n');
    if (line == "\r") 
    { //Serial.println("Headers received");
      break;
    }
  }
  String payload = client.readStringUntil('\n');
  payload.trim();
  if(payload.equals(FirmwareVer) )
  { Serial.println("Device already on latest firmware version"); 
  }
  else
  { Serial.println("New firmware detected");
    ESPhttpUpdate.setLedPin(LED_BUILTIN, LOW); 
    t_httpUpdate_return ret = ESPhttpUpdate.update(client, URL_fw_Bin);
    switch (ret) 
    { case HTTP_UPDATE_FAILED:      Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s\n", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
                                    break;
      case HTTP_UPDATE_NO_UPDATES:  Serial.println("HTTP_UPDATE_NO_UPDATES");
                                    break;
      case HTTP_UPDATE_OK:          Serial.println("HTTP_UPDATE_OK");
                                    break;
    } 
  }
 }  

 void repeatedCall()
{   unsigned long currentMillis = millis();
    if ((currentMillis - previousMillis) >= interval) 
    { // save the last time you blinked the LED
      previousMillis = currentMillis;
      setClock();
      FirmwareUpdate();
    }
    if ((currentMillis - previousMillis_2) >= mini_interval) 
    { static int idle_counter=0;
      previousMillis_2 = currentMillis;    
      Serial.print(" Active fw version:");
      Serial.println(FirmwareVer);
      Serial.print("Idle Loop....");
      Serial.println(idle_counter++);
      if(idle_counter%2==0)
        digitalWrite(01, HIGH);
      else 
        digitalWrite(01, LOW);
      if(WiFi.status() == !WL_CONNECTED) 
        connect_wifi();
    }
 }

  
void setup()
{
  Serial.begin(115200);
  Serial.println("");
  Serial.println("Start");
  WiFi.mode(WIFI_STA);
  connect_wifi();  
  setClock();
  pinMode(01, OUTPUT);
  
}
void loop()
{
  repeatedCall();    
  digitalWrite(01, HIGH);
  delay(1000);
  digitalWrite(01, LOW);
  delay(1000);
}
