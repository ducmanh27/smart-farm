
#include "handleOperator.h"

void ledDebug()
{
    digitalWrite(PIN_PB7, 1);
    delay(100);
    digitalWrite(PIN_PB7, 0);
    delay(100);
    digitalWrite(PIN_PB7, 1);
    delay(100);
    digitalWrite(PIN_PB7, 0);
    delay(100);
}

/**
 *  {
    "operator": "register",
    "info": {
     "macAddress": "00:00:5e:00:53:af"
    },
    "status": 0
    }
 */
void registration()
{
    if (!UART2.available())
    {
        StaticJsonDocument<SIZE_OF_JSON> doc;
        doc["operator"] = "register";
        doc["info"]["macAddress"] = macAddress;
        doc["status"] = 0;
        serializeJson(doc, UART2);
    }
}

int switchOperator(String myOperator)
{
    for (int i = 0; i < NUM_OF_OPERATOR; i++)
    {
        if (myOperator == operatorList[i])
        {
            return i;
        }
    }
    return -1;
};

void handleResisterAck(StaticJsonDocument<SIZE_OF_JSON> *doc)
{
    if (macAddress == (*doc)["info"]["macAddress"])
    {
        ledDebug();
        if (((*doc)["info"]["status"] == 1))
        {
            SensorId = (*doc)["info"]["id"];
            serializeJson(*doc, UART2);
        }
        else
        {
            if (SensorId == 0) // re-registration
            {
                registration();
            }
        }
    }
};

void handleResister(StaticJsonDocument<SIZE_OF_JSON> *doc)
{
    if ((*doc)["info"]["status"] == 1)
    {
        strcpy(macAddress, (*doc)["info"]["macAddress"]);
    }
};
void handleKeepAlive(StaticJsonDocument<SIZE_OF_JSON> *doc){};
void handleSensorData(StaticJsonDocument<SIZE_OF_JSON> *doc){};
void handleActuatorData(StaticJsonDocument<SIZE_OF_JSON> *doc){};

void handleSetPoint(StaticJsonDocument<SIZE_OF_JSON> *doc)
{
    if (SensorId == (*doc)["id"])
    {
        run_multispeed((int)(*doc)["info"]["speed"]);
        /**
         * {
            "operator": " setPointAck ",
            "id": 4,
            "info": {
                "status": 1
            }
            }
         */
        if (!UART2.available())
        {
            StaticJsonDocument<SIZE_OF_JSON> docAck;
            docAck["operator"] = "setPointAck";
            docAck["id"] = (*doc)["id"];
            docAck["info"]["time"] = now();
            docAck["info"]["status"] = 1;
            serializeJson(docAck, UART2);
        }
    }
};

void handleSetPointAck(StaticJsonDocument<SIZE_OF_JSON> *doc){};
