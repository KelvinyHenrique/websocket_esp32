#ifdef ESP8266
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#elif defined(ESP32)
#include <WiFi.h>
#include <ESPmDNS.h>
#else
#error "Board not found"
#endif

#include <WebSocketsServer.h>
#include <ArduinoJson.h>

#define LED1 15
#define LED2 12

char webpage[] PROGMEM = R"=====(

<!DOCTYPE html>
<html lang="en">

<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32</title>
    <style>
        .container_home {
            display: flex;
            align-items: center;
            justify-content: center;
            flex-direction: column;
        }

        .ledred {
            display: flex;
            flex-direction: column;
            align-items: center;
            justify-content: center;
        }

        .btn-group {
            display: flex;
        }

        .btn {
            background-color: #7159c1;
            border: unset;
            color: #FFF;
            width: 100px;
            height: 30px;
            margin: 10px;
        }
    </style>
    <script>
        var connection = new WebSocket('ws://' + location.hostname + ':81/');
        var button_1_status = 0;
        var button_2_status = 0
        function button_1_on() {
            button_1_status  = 1;
            console.log("LED 1 is ON");
            send_data();
        }
        function button_1_off() {
          button_1_status = 0;
            console.log("LED 1 is OFF ");
                 connection.send("LED 1 is OFF");
                    send_data();
        }

              function button_2_on() {
            button_2_status  = 1;
            console.log("LED 2 is ON");
            send_data();
        }
        function button_2_off() {
          button_2_status = 0;
            console.log("LED 2 is OFF ");
                 connection.send("LED 2 is OFF");
                    send_data();
        }

        function send_data() {
             var full_data = '{"LED1":'+button_1_status+',"LED2":'+button_2_status+'}';
             connection.send(full_data);       
        }
    </script>
</head>

<body>
    <div class="container_home">
        <h1>Websocket com ESP32</h1>
        <div class="ledred">
            <h1>LED VERMELHO</h1>
            <div class="btn-group">
                <button onclick="button_1_on()" class="btn">ON</button>
                <button onclick="button_1_off()" class="btn">OFF</button>
            </div>
        </div>
        <div class="ledred">
            <h1>LED AZUL</h1>
            <div class="btn-group">
                <button onclick="button_2_on()" class="btn">ON</button>
                <button onclick="button_2_off()" class="btn">OFF</button>
            </div>
        </div>
    </div>
</body>
  
</html>
)=====";

#include <ESPAsyncWebServer.h>

AsyncWebServer server(80); // server port 80
WebSocketsServer websockets(81);
void notFound(AsyncWebServerRequest *request)
{
  request->send(404, "text/plain", "Page Not found");
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length)
{

  switch (type)
  {
  case WStype_DISCONNECTED:
    Serial.printf("[%u] Disconnected!\n", num);
    break;
  case WStype_CONNECTED:
  {
    IPAddress ip = websockets.remoteIP(num);
    Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);

    // send message to client
    websockets.sendTXT(num, "Connected from server");
  }
  break;
  case WStype_TEXT:
    Serial.printf("[%u] get Text: %s\n", num, payload);
    String message = String((char *)(payload));
    Serial.println(message);

    DynamicJsonDocument doc(200);
    // deserialize data
    DeserializationError error = deserializeJson(doc, message);
    // parse the parameters we expect to receive (TO-DO: error handling)
    // Test if parsopm sicceeds.
    if (error)
    {
      Serial.print("deserializeJson() failed: ");
      Serial.println(error.c_str());
      return;
    }
    int LED1_status = doc["LED1"];
    int LED2_status = doc["LED2"];
    digitalWrite(LED1, LED1_status);
    digitalWrite(LED2, LED2_status);
  }
}

void setup(void)
{

  Serial.begin(115200);
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);

  WiFi.softAP("techiesms", "");
  Serial.println("softap");
  Serial.println("");
  Serial.println(WiFi.softAPIP());

  if (MDNS.begin("ESP"))
  { //esp.local/
    Serial.println("MDNS responder started");
  }

  server.on("/", [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", webpage);
  });

  server.on("/led1/on", HTTP_GET, [](AsyncWebServerRequest *request) {
    digitalWrite(LED1, HIGH);
    request->send_P(200, "text/html", webpage);
  });

  server.on("/led1/off", HTTP_GET, [](AsyncWebServerRequest *request) {
    digitalWrite(LED1, LOW);
    request->send_P(200, "text/html", webpage);
  });

  server.onNotFound(notFound);

  server.begin(); // it will start webserver
  websockets.begin();
  websockets.onEvent(webSocketEvent);
}

void loop(void)
{
  websockets.loop();
}