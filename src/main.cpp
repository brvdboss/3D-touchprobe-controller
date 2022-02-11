#include <Arduino.h>
#include <CAN.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <sstream>
#include <iostream>
#include <queue>

// create this file and fill your wifi credentials
#include <wifi/credentials.h>

int probeState = -1;
int oldState = -1;

bool event = true;
long cleanupTimeout = 0;
long msgTimeout = 0;

std::queue<String> msgq;


// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
// Create websocket
AsyncWebSocket ws("/ws");

//convenience methods to convert some data to hex values
std::string toHex(int d)
{
  char tmp[4];
  sprintf(tmp, "%02X", d);
  return std::string(tmp);
}

std::string toHex(long d)
{
  char tmp[32];
  sprintf(tmp, "%08lX", d);
  return std::string(tmp);
}


/**
 * @brief notify all listening clients
 *
 */
void notifyClients(String s)
{
  ws.textAll(s);
}

/**
 * @brief handle incoming websocket messages
 *
 * @param arg
 * @param data
 * @param len
 */
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len)
{
  AwsFrameInfo *info = (AwsFrameInfo *)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT)
  {
    data[len] = 0;
    if (strcmp((char *)data, "toggle") == 0)
    {
      //If we want to listen to incoming messages: parse and fill in logic here.
      //Currently not used.
      event = true;
      //notifyClients();
    }
  }
}

/**
 * @brief Listen to incoming websocket messages
 *
 * @param server
 * @param client
 * @param type
 * @param arg
 * @param data
 * @param len
 */
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len)
{
  switch (type)
  {
  case WS_EVT_CONNECT:
    Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
    break;
  case WS_EVT_DISCONNECT:
    Serial.printf("WebSocket client #%u disconnected\n", client->id());
    break;
  case WS_EVT_DATA:
    handleWebSocketMessage(arg, data, len);
    break;
  case WS_EVT_PONG:
  case WS_EVT_ERROR:
    break;
  }
}

/**
 * @brief Initialize websocket and define the handler methods
 *
 */
void initWebSocket()
{
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}

/**
 * @brief Group everything that is webrelated and needs to be in setup()
 *
 */
void webSetup()
{

  pinMode(LED_BUILTIN, OUTPUT);

  if (!SPIFFS.begin(true))
  {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  // Initialize SPIFFS
  if (!SPIFFS.begin(true))
  {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }

  // Print ESP Local IP Address
  Serial.println(WiFi.localIP());

  initWebSocket();

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/index.html", "text/html"); });
  // Route to load style.css file
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/style.css", "text/css"); });

  // Route to load script.js file
  server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/script.js", "application/javascript"); });

  server.begin();
}

/**
 * @brief Read Can Bus and print to Serial
 *
 * if not connected to a handler, you can call it in loop() as well, then packet size doesn't need to be provided
 */
void canReadLoop(int packetSize)
{

  // int packetSize = CAN.parsePacket();

  if (packetSize)
  {
    // received a packet
    Serial.print("Received ");

    if (CAN.packetExtended())
    {
      Serial.print("extended ");
    }

    if (CAN.packetRtr())
    {
      // Remote transmission request, packet contains no data
      Serial.print("RTR ");
    }

    Serial.print("packet with id 0x");
    Serial.print(CAN.packetId(), HEX);

    if (CAN.packetRtr())
    {
      Serial.print(" and requested length ");
      Serial.println(CAN.packetDlc());
    }
    else
    {
      Serial.print(" and length ");
      Serial.println(packetSize);
    }
    while (CAN.available())
    {
      Serial.print(" ");
      Serial.print(CAN.read(), HEX);
    }
    Serial.println("");
  }
}

/**
 * @brief Read messages from CAN bus and transform it into a JSON object.
 *
 * @param size
 */
void canReadJSON(int size)
{
  if (size)
  {
    DynamicJsonDocument doc(1024);

    doc["length"] = size;
    if (CAN.packetExtended())
    {
      doc["type"] = "extended";
    }
    else if (CAN.packetRtr())
    {
      doc["type"] = "RTR";
      doc["rtrlength"] = CAN.packetDlc();
    }
    else
    {
      doc["type"] = "normal";
    }
    doc["id"] = toHex(CAN.packetId());

    std::stringstream os;
    while (CAN.available())
    {
      os << " " << toHex(CAN.read());
    }
    doc["data"] = os.str();
    String res = "";
    serializeJson(doc, res);
    msgq.push(res);
    // notifyClients(res);
  }
}



/**
 * @brief Testmethod to debug UI stuff. can generate artifical CAN JSON objects
 *
 */
void testJSON()
{

  DynamicJsonDocument doc(1024);

  doc["length"] = 12;
  if (random(0, 2))
  {
    doc["type"] = "normal";
  }
  else
  {
    doc["type"] = "RTR";
  }
  doc["rtrlength"] = 12;
  doc["id"] = toHex((long)(random(5)));

  std::stringstream os;

  os << toHex((int)random(5,15)) << " ";
  os << toHex(10) << " ";
  os << toHex(20) << " ";
  os << toHex(30) << " ";
  os << toHex(40);

  doc["data"] = os.str();
  String res = "";
  serializeJson(doc, res);
  msgq.push(res);
  // notifyClients(res);
}

/**
 * @brief Setup CAN bus
 *
 * everything CAN releted that needs to be in setup()
 *
 */
void canSetup()
{
  Serial.println("CAN Receiver");
  // start the CAN bus at 500 kbps
  if (!CAN.begin(500E3))
  {
    Serial.println("Starting CAN failed!");
    while (1)
      ;
  }
  // CAN.onReceive(canReadLoop);
  CAN.onReceive(canReadJSON);
}

/**
 * @brief Action to take when touchprobe detects edge
 * IRAM_ATTR identifier to keep it in RAM and have faster execution
 */
void IRAM_ATTR triggerEnd()
{
  int state = digitalRead(GPIO_NUM_33);
  if (state != probeState)
  {
    probeState = state;
    // should be between IFDEBUGS or something
    Serial.print("!!!!!!!! Touch Change : ");
    Serial.print(state);
    Serial.println(" !!!!!!!!");

    // Sendin a CAN message from within the interrupt didn't work,
    // so just changing the state and sending the message in the main loop
    /*
    CAN.beginPacket(0x604);
    CAN.write(state);  //0 triggered, 1 when not triggered
    CAN.endPacket();
    */
  }
}

/**
 * @brief Attach interrupt to take action when touchprobe detects edge
 *
 */
void pinInterruptSetup()
{
  pinMode(GPIO_NUM_33, INPUT_PULLDOWN);
  // It is not possible to have an interrupt on both FALLING and RISING, so declare one on CHANGE
  // and read the state in the interrupt method itself
  attachInterrupt(digitalPinToInterrupt(GPIO_NUM_33), triggerEnd, CHANGE);
}

/**
 * @brief Set everything up
 *
 */
void setup()
{
  Serial.begin(115200);
  while (!Serial)
    ;
  canSetup();
  pinInterruptSetup();
  webSetup();
}

/**
 * @brief loop
 *
 */
void loop()
{
  // canReadLoop();

  // id 604 and 605 have been seen for probe, value is 0 when bed detected, 1 when moving away
  // only use with printhead connected for now
  if (probeState != oldState)
  {
    oldState = probeState;
    CAN.beginPacket(0x605);
    CAN.write(probeState); // 0 triggered, 1 when not triggered
    CAN.endPacket();
  }

  //Occasionally clean up old websocket clients to free up resources
  long t = millis();
  if (t > cleanupTimeout)
  {
    cleanupTimeout = t + 1000;
    ws.cleanupClients();
    //testJSON(); //hitch along on this method to create test messages when uncommented
  }

  if (event)
  {
    event = false;
    //Action to take on incoming message
  }

  //We can't send every CAN message immediately through the websocket as the rate is too high
  //every half second we group everything that was in the queue and send them as a combined
  //JSON.
  if(t > msgTimeout) {
    msgTimeout = t + 500;
    if (!msgq.empty())
    {
      String result = "{ \"combined\": ["+msgq.front();
      msgq.pop();
      int size = msgq.size();
      for (int i=0;i<size;i++)
      {
        result+=","+msgq.front();
        msgq.pop();
      }
      result +="]}";
      notifyClients(result);
    }
  }
}
