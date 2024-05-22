#include <OneWire.h>
#include <DallasTemperature.h>
#include <WiFi.h>
#include "ESPAsyncWebServer.h"

// Replace with your network credentials
const char* ssid = "REDE IOT";
const char* password = "L@ps1234";

const int SensorDataPin = 4;   
  
OneWire oneWire(SensorDataPin);
DallasTemperature sensors(&oneWire);

float temperature_Celsius;
float temperature_Fahrenheit;

AsyncWebServer server(80);
AsyncEventSource events("/events");

unsigned long lastTime = 0;  
unsigned long timerDelay = 30000;  // send readings timer

void getDS18B20Readings(){
 
  sensors.requestTemperatures(); 
  temperature_Celsius = sensors.getTempCByIndex(0);
  temperature_Fahrenheit = sensors.getTempFByIndex(0);
}

String processor(const String& var){
  getDS18B20Readings();
  //Serial.println(var);
  if(var == "TEMPERATURE_C"){
    return String(temperature_Celsius);
  }\
  else if(var == "TEMPERATURE_F"){
    return String(temperature_Fahrenheit);
  }
}

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>DS18B20 Web Server</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.7.2/css/all.css" integrity="sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr" crossorigin="anonymous">
  <link rel="icon" href="data:,">
  <style>
    html {font-family: Arial; display: inline-block; text-align: center;}
    p {  font-size: 1.2rem;}
    body {  margin: 0;}
    .topnav { overflow: hidden; background-color: #4B1D3F; color: white; font-size: 1.7rem; }
    .content { padding: 20px; }
    .card { background-color: white; box-shadow: 2px 2px 12px 1px rgba(140,140,140,.5); }
    .cards { max-width: 700px; margin: 0 auto; display: grid; grid-gap: 2rem; grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); }
    .reading { font-size: 2.8rem; }
    .card.temperature { color: #0e7c7b; }
  </style>
</head>
<body>
  <div class="topnav">
    <h3>DS18B20 WEB SERVER</h3>
  </div>
  <div class="content">
    <div class="cards">
      <div class="card temperature">
        <h4><i class="fas fa-thermometer-half"></i> TEMPERATURE</h4><p><span class="reading"><span id="temp_celcius">%TEMPERATURE_C%</span> &deg;C</span></p>
      </div>
      <div class="card temperature">
        <h4><i class="fas fa-thermometer-half"></i> TEMPERATURE</h4><p><span class="reading"><span id="temp_fahrenheit">%TEMPERATURE_F%</span> &deg;F</span></p>
      </div>
    </div>
  </div>
<script>
if (!!window.EventSource) {
 var source = new EventSource('/events');
 
 source.addEventListener('open', function(e) {
  console.log("Events Connected");
 }, false);
 source.addEventListener('error', function(e) {
  if (e.target.readyState != EventSource.OPEN) {
    console.log("Events Disconnected");
  }
 }, false);
 
 source.addEventListener('message', function(e) {
  console.log("message", e.data);
 }, false);
 
 source.addEventListener('temperature_Celsius', function(e) {
  console.log("temperature", e.data);
  document.getElementById("temp_celcius").innerHTML = e.data;
 }, false);
 
 source.addEventListener('temperature_Fahrenheit', function(e) {
  console.log("temperature", e.data);
  document.getElementById("temp_fahrenheit").innerHTML = e.data;
 }, false);
 
}
</script>
</body>
</html>)rawliteral";

void setup() {
  Serial.begin(115200);

  // Set the device as a Station and Soft Access Point simultaneously
  //WiFi.mode(WIFI_AP_STA);
  WiFi.setMinSecurity(WIFI_AUTH_WPA_PSK);
  // Set device as a Wi-Fi Station
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Setting as a Wi-Fi Station..");
  }
  Serial.print("Station IP Address: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  
  // Handle Web Server
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });

  // Handle Web Server Events
  events.onConnect([](AsyncEventSourceClient *client){
    if(client->lastId()){
      Serial.printf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
    }
    // send event with message "hello!", id current millis
    // and set reconnect delay to 1 second
    client->send("hello!", NULL, millis(), 10000);
  });
  server.addHandler(&events);
  server.begin();
}

void loop() {
  if ((millis() - lastTime) > timerDelay) {
    
getDS18B20Readings();
    Serial.printf("Temperature = %.2f ºC \n", temperature_Celsius);
    Serial.printf("Temperature = %.2f ºF \n", temperature_Fahrenheit);
    Serial.println();

    // Send Events to the Web Server with the Sensor Readings
    events.send("ping",NULL,millis());
    events.send(String(temperature_Celsius).c_str(),"temperature_Celsius",millis());
    events.send(String(temperature_Fahrenheit).c_str(),"temperature_Fahrenheit",millis());
    
    lastTime = millis();
  }
}
