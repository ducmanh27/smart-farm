#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <SoftwareSerial.h>

SoftwareSerial UART2(13, 15); // RX, TX. UART2 for ESP07

// led for signalizing
#define LEDPIN 2

// ssid and password wifi
const char *ssid = "Tran Ba Dat";
const char *password = "123456789";

// info you mqtt broker
const char *mqtt_server = "broker.emqx.io";
const int mqtt_port = 1883;
// const char *mqtt_server = "broker.hivemq.com";
// const int mqtt_port = 1883;
WiFiClient espClient;
PubSubClient client(espClient);
#define NumOfOperator 5
const char Operator[NumOfOperator][20] = {"RegistrationAck", "Registration", "keepAlive", "sensorData", "actuatorData"};
char Topic[NumOfOperator][30] = {"farm/1/register", "farm/1/register", "farm/1/alive/1", "farm/1/1", "farm/1/1"};

// function prototype
void ledDebug();
void reconnect();
void callback(char *topic, byte *payload, unsigned int length);
void PublishData();

void setup()
{
  pinMode(LEDPIN, OUTPUT); // led for debug
  Serial.begin(9600);
  UART2.begin(9600);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    ledDebug();
  }
  // Serial.println(WiFi.softAPmacAddress());

  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void loop()
{
  reconnect();
  PublishData();
}

/*******************************************************************************/
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
      String clientId = "ESP32Client-";
      clientId += String(random(0xffff), HEX);

      // Attempt to connect
      if (client.connect(clientId.c_str()))
      {
        /* If connection to MQTT server is successful, turn 2 on */
        digitalWrite(LEDPIN, LOW);
        // Resubscribe to topic
        for (int i = 0; i < NumOfOperator; i++)
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
    StaticJsonDocument<512> doc;
    DeserializationError err = deserializeJson(doc, UART2.readString());

    if (err == DeserializationError::Ok)
    {
      // read sensorid to config topics. operator = "RegistrationAck"
      if (doc["operator"] == "RegistrationAck")
      {
        int SensorId = doc["info"]["id"];
        sprintf(Topic[2], "farm/1/alive/%d", SensorId);
        sprintf(Topic[3], "farm/1/%d", SensorId);
        sprintf(Topic[4], "farm/1/%d", SensorId);
      }
      else
      {
        // publish data to MQTT with right topic
        for (int i = 1; i < NumOfOperator; i++)
        {

          if (doc["operator"] == Operator[i])
          {
            char msg[512];
            serializeJson(doc, msg);
            client.publish(Topic[i], msg);
            ledDebug();
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
}
