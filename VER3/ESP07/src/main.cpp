#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <SoftwareSerial.h>

// Config
#define RX2 13
#define TX2 15
#define LEDPIN 2
#define BAUD 9600

// ssid and password wifi
const char *ssid = "Tran Ba Dat";
const char *password = "123456789";
// const char *ssid = "BKEM";
// const char *password = "12345678";

// info you mqtt broker
const char *mqtt_server = "broker.emqx.io";
const int mqtt_port = 1883;
// const char *mqtt_server = "27.71.227.1";
// const int mqtt_port = 1883;
WiFiClient espClient;
PubSubClient client(espClient);
#define SIZE_OF_MSG 256
#define NUM_OF_OPERATOR 7
const char operatorList[NUM_OF_OPERATOR][20] = {"registerAck",
                                                "register",
                                                "keepAlive",
                                                "sensorData",
                                                "actuatorData",
                                                "setPoint",
                                                "setPointAck"};
char Topic[NUM_OF_OPERATOR][30] = {"farm/1/register",
                                   "farm/1/register",
                                   "farm/1/alive",
                                   "farm/1/sensor",
                                   "farm/1/actuator",
                                   "farm/1/actuator",
                                   "farm/1/actuator"};

SoftwareSerial UART2(RX2, TX2); // UART2 for ESP07
// function prototype
void ledDebug();
void reconnect();
void callback(char *topic, byte *payload, unsigned int length);
void PublishData();

void setup()
{
  pinMode(LEDPIN, OUTPUT); // led for debug
  Serial.begin(BAUD);
  UART2.begin(BAUD);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    ledDebug();
  }
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void loop()
{
  reconnect();
  PublishData();
}

/*******************************************************************************/

// callback function when MQTT has new data, it will be sent to ATMEGA128 by Serial(UART1)
void callback(char *topic, byte *payload, unsigned int length)
{
  for (unsigned int i = 0; i < length; i++)
  {
    Serial.print((char)payload[i]);
  }
}

// reconect function
void reconnect()
{

  if (!client.connected())
  {
    // Loop until we're reconnected
    while (!client.connected())
    {
      // Signalizing a MQTT connection
      ledDebug();

      // Create a random client ID
      String clientId = "smart_farm_things_";
      clientId += String(random(0xffff), HEX);

      // Attempt to connect
      if (client.connect(clientId.c_str()))
      {
        /* If connection to MQTT server is successful, turn 2 on */
        digitalWrite(LEDPIN, LOW);
        // Resubscribe to topic
        for (int i = 0; i < NUM_OF_OPERATOR; i++)
        {
          client.subscribe((const char *)Topic[i]);
        }
      }
      else
      {
        /* If connection to MQTT server is failed, turn 2 off */
        digitalWrite(LEDPIN, HIGH);
        // Wait 5 seconds before retrying
        delay(5000);
      }
    }
  }
  client.loop();
}

// receive data from ATMEGA128
void PublishData()
{
  if (UART2.available())
  {
    StaticJsonDocument<SIZE_OF_MSG> doc;
    DeserializationError err = deserializeJson(doc, UART2.readString());

    if (err == DeserializationError::Ok)
    {
      ledDebug();
      // add mac address into message "register"
      if (doc["operator"] == "register")
      {
        doc["info"]["macAddress"] = WiFi.softAPmacAddress();
        doc["info"]["status"] = 1;
        serializeJson(doc, Serial);
        doc["info"]["status"] = 0;
      }

      // publish data to MQTT with right topic
      for (int i = 1; i < NUM_OF_OPERATOR; i++)
      {

        if (doc["operator"] == operatorList[i])
        {
          char msg[SIZE_OF_MSG];
          serializeJson(doc, msg);
          client.publish(Topic[i], msg);
        }
      }
    }
  }
  else
  {
    // Flush all bytes in the "link" serial port buffer
    while (UART2.available() > 0)
      UART2.read();
  }
}

// led blink to debug
void ledDebug()
{
  digitalWrite(LEDPIN, LOW);
  delay(100);
  digitalWrite(LEDPIN, HIGH);
  delay(100);
  digitalWrite(LEDPIN, LOW);
  delay(100);
  digitalWrite(LEDPIN, HIGH);
  delay(100);
}
