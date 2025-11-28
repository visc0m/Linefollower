#include "SerialCommand.h"
#include "EEPROMAnything.h"
#include <WiFi.h>
#include <WebServer.h>

const char* ssid = "LAPTOP-06F9RUHC 2524";
const char* password = "Azerty123";

WebServer server(80);

#define SerialPort Serial
#define Baudrate 115200

SerialCommand sCmd(SerialPort);
#define AIN1 4
#define AIN2 5
#define BIN1 6
#define BIN2 7
#define Stop 8
#define Start 46
#define On 3
bool lost=false;
volatile bool run = false;
volatile unsigned long lastInterrupt = 0;
// PWM-instellingen
#define FREQ     20000   // 20 kHz = stil voor motoren
#define RESOLUTION 8     // 8 bit (0-255)

unsigned long previous, calculationTime;
int normalised[8];
const int pinIRenable = 12;  // kies een GPIO voor IR run/uit (optioneel)
int sensor[8] = {20, 19, 18, 17, 16, 15, 14, 13};  // pas run naar jouw gebruikte GPIO's
float position;
float debugposition;

int powerLeft = 0;
int powerRight = 0;

float iTerm, lastErr;

struct param_t
{
  unsigned long cycleTime;
  int black[8];
  int white[8];
  int power = 200;
  float diff = 0.15;
  float kp = 18;
  float ki = 0.002;
  float kd = 100;
  /* andere parameters die in het eeprom geheugen moeten opgeslagen worden voeg je hier toe ... */
} params;

///----------------------------------------------------interupt------------------------------------------
void IRAM_ATTR intHandleStart() {
  unsigned long now = millis();
  if (now - lastInterrupt > 150) {   // debounce
      run = true;
      digitalWrite(On, HIGH);   // LED AAN
  }
  lastInterrupt = now;
}

void IRAM_ATTR intHandleStop() {
  unsigned long now = millis();
  if (now - lastInterrupt > 150) {   // debounce
      run = false;
      digitalWrite(On, LOW);    // LED UIT
  }
  lastInterrupt = now;
}
///----------------------------------------------------setup------------------------------------------
void setup() {
   EEPROM_readAnything(0, params);
  //------------------server-------
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  Serial.print("Verbinden met WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi verbonden!");
  Serial.print("IP adres: ");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/status", handleStatus);
  server.on("/setValues", handlesetValues);
  server.on("/start", handleStart);
  server.on("/stop", handleStop);

  server.begin();
  //------------------------serialcommands---------------------------
  SerialPort.begin(Baudrate); // set serial baudrate at 115200
  sCmd.setDefaultHandler(onUnknownCommand);
  sCmd.addCommand("calibrate", onCalibrate);

  SerialPort.println("ready");
  
  // Attach pinnen run kanalen
  pinMode(AIN1, OUTPUT);
  pinMode(AIN2, OUTPUT);
  pinMode(BIN1, OUTPUT);
  pinMode(BIN2, OUTPUT);
  pinMode(On, OUTPUT);
  pinMode(Stop, INPUT_PULLUP);
  pinMode(Start, INPUT_PULLUP);

  stopMotors();

  pinMode(pinIRenable, OUTPUT);
  digitalWrite(pinIRenable, HIGH);  // IR-LED’s runzetten

  for (int i = 0; i < 8; i++) {
    pinMode(sensor[i], INPUT);
  }
  attachInterrupt(digitalPinToInterrupt(Start), intHandleStart, FALLING);
  attachInterrupt(digitalPinToInterrupt(Stop), intHandleStop, FALLING);
}

void loop() 
{
 sCmd.readSerial();
 server.handleClient();

 unsigned long current = micros();
 if (current - previous >= params.cycleTime)
 {
  previous = current;

  lost=true;
  //Waardes normaliseren: 
  for (int i = 0; i < 8; i++) {
    normalised[i] = map(analogRead(sensor[i]), params.black[i], params.white[i], 0, 1000);
    if(analogRead(sensor[i])>=(params.black[i]-1500)) lost=false;
    //Serial.println(sensor[i]);
  }
  
  //Kwadratische interpolatie:
  int index = 0;
  float position;
  for (int i=1; i < 8; i++) if (normalised[i] < normalised[index]) index = i;

  //if (normalised[index] > 3000) run = false;

  if (index == 0) position = -20;
  else if (index == 7) position = 20;
  else
  {
    int sNul = normalised[index];
    int sMinEen = normalised[index-1];
    int sPlusEen = normalised[index+1];

    float b = sPlusEen - sMinEen;
    b = b / 2;
    float a = sPlusEen - b - sNul;
    position = -b /(2 * a);
    position += index;
    position -= 3.5;
    position *= 10;
  }
  debugposition = position;

  //PID
  float error = -position;
  float output = error * params.kp;
  iTerm += params.ki * error;
  iTerm = constrain(iTerm, -510, 510);
  output += iTerm;
  output += params.kd * (error - lastErr);
  lastErr = error;
  output = constrain(output, -510, 510);
  
  int powerLeft = 0;
  int powerRight = 0;
  if (run)if(!lost)
  {
      if (output >= 0)
    {
      powerLeft = constrain(params.power + params.diff * output, -255, 255);
      powerRight = constrain(powerLeft - output, -255, 255);
      powerLeft = powerRight + output;
    } 
    else
    {
      powerRight = constrain(params.power - params.diff * output, -255, 255);
      powerLeft = constrain(powerRight + output, -255, 255);
      powerRight = powerLeft - output;
    }
 }
 else{
  powerRight = -255;
  powerLeft = 255;
  Serial.println(powerLeft);
 }
  powerRight = map(powerRight, -255, 255, -params.power, params.power);
  powerLeft = map(powerLeft, -255, 255, -params.power, params.power);

  //Serial.println(powerLeft);
  //Serial.println(powerRight);
  analogWrite(AIN1, powerLeft > 0 ? powerLeft : 0);
  analogWrite(AIN2, powerLeft < 0 ? -powerLeft : 0);
  analogWrite(BIN1, powerRight > 0 ? powerRight : 0);
  analogWrite(BIN2, powerRight < 0 ? -powerRight : 0);

 }
 unsigned long difference = micros() - current;
 if (difference > calculationTime) calculationTime = difference;
}


//---------------------motor----------------------
void stopMotors() {
  analogWrite(AIN1, 0);
  analogWrite(AIN2, 0);
  analogWrite(BIN1, 0);
  analogWrite(BIN2, 0);
}
//--------------------commands-----------------------
void onCalibrate()
{
  SerialPort.println("calibratie");
  char* parameter = sCmd.next();
  if (strcmp(parameter, "black") == 0)
  {
    Serial.print("start calibrating black... ");
    for (int i = 0; i < 8; i++){
      params.black[i] = analogRead(sensor[i]);
      Serial.println(analogRead(sensor[i]));
      }
    Serial.print("done");
  }
  else if (strcmp(parameter, "white") == 0)
  {
    Serial.print("start calibrating white... ");
    for (int i = 0; i < 8; i++) {
      params.white[i] = analogRead(sensor[i]);
      Serial.println(analogRead(sensor[i]));
    }
    Serial.print("done");
  }
  EEPROM_writeAnything(0, params);
}

void onUnknownCommand(char* command)
{
  SerialPort.print("Unknown Command: \"");
  SerialPort.print(command);
  SerialPort.println("\"");
}

//--------------------------------------------webpagina---------------------------
void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="utf-8">
  <title>Robot Control</title>
  <style>
    body { font-family: Arial; text-align: center; margin-top: 40px; }
    .values-box { font-size: 1.2em; margin: 10px; }
    button { font-size: 0.9em; padding: 10px 20px; margin: 10px; }
    input { font-size: 0.9em; padding: 5px 10px; margin: 5px; width: 50px;}
  </style>
</head>
<body>
  <h2>Robot Control</h2>
  <div class="values-box">
    Status: <span id="status">...</span><br>
    Speed: <span id="currentSpeed">0</span><br>
    kp: <span id="kp">0</span><br>
    ki: <span id="ki">0</span><br>
    kd: <span id="kd">0</span><br>
    diff: <span id="diff">0</span>
  </div>

  <div style="margin:10px;">
    <label for="SpeedInput">Speed:</label>
    <input type="number" id="SpeedInput" style="width:60px;" value=')rawliteral" + String(params.power) + R"rawliteral(' /><br>
    <label for="kpInput">Kp:</label>
    <input type="number" id="kpInput" style="width:60px;" value=')rawliteral" + String(params.kp) + R"rawliteral(' /><br>
    <label for="kiInput">Ki:</label>
    <input type="number" id="kiInput" style="width:60px;" value=')rawliteral" + String(params.ki) + R"rawliteral(' /><br>
    <label for="kdInput">Kd:</label>
    <input type="number" id="kdInput" style="width:60px;" value=')rawliteral" + String(params.kd) + R"rawliteral(' /><br>
    <label for="diffInput">Diff:</label>
    <input type="number" id="diffInput" style="width:60px;" value=')rawliteral" + String(params.diff) + R"rawliteral(' /><br>
  </div>
  
  <button onclick="setValues()">Verstuur</button><br>

  <button onclick="start()">Start</button>
  <button onclick="stop()">Stop</button>

  <script>
    // Live updates elke seconde
    setInterval(function() {
      fetch('/status')
        .then(r => r.json())
        .then(data => {
          document.getElementById("status").innerText = data.running ? "RUNNING" : "STOPPED";
          document.getElementById("kp").innerText = data.kp;
          document.getElementById("ki").innerText = data.ki;
          document.getElementById("kd").innerText = data.kd;
          document.getElementById("diff").innerText = data.diff;
          document.getElementById("currentSpeed").innerText = data.Speed;
          
          // input en knop in-/uitschakelen
          document.getElementById("SpeedInput").disabled = data.running;
          document.querySelector("button[onclick='setValues()']").disabled = data.running;
        });
    }, 1000);

    function setValues() {
      let speed = document.getElementById("SpeedInput").value;
      let kp    = document.getElementById("kpInput").value;
      let ki    = document.getElementById("kiInput").value;
      let kd    = document.getElementById("kdInput").value;
      let diff  = document.getElementById("diffInput").value;

      fetch(`/setValues?speed=${speed}&kp=${kp}&ki=${ki}&kd=${kd}&diff=${diff}`);
    }
  
    function start() {
      fetch('/start');
    }
    function stop() {
      fetch('/stop');
    }
  </script>
</body>
</html>
)rawliteral";

  server.send(200, "text/html", html);                                                           
}

void handleStatus() {
  String json = "{";
  json += "\"kp\":" + String(params.kp,4) + ",";
  json += "\"ki\":" + String(params.ki,4) + ",";
  json += "\"kd\":" + String(params.kd,4) + ",";
  json += "\"diff\":" + String(params.diff,4) + ",";
  json += "\"Speed\":" + String(params.power) + ",";
  json += "\"running\":" + String(run ? "true" : "false");
  json += "}";
  server.send(200, "application/json", json);
  //Serial.println(json);
}

void handlesetValues() {
  if (!run) {  // Alleen toestaan als robot stil staat

    if (server.hasArg("speed")) {
      params.power = server.arg("speed").toInt();
    }

    if (server.hasArg("kp")) {
      params.kp = server.arg("kp").toFloat();
    }

    if (server.hasArg("ki")) {
      params.ki = server.arg("ki").toFloat();
    }

    if (server.hasArg("kd")) {
      params.kd = server.arg("kd").toFloat();
    }

    if (server.hasArg("diff")) {
      params.diff = server.arg("diff").toFloat();
    }
    EEPROM_writeAnything(0, params);
    Serial.printf("Nieuwe waardes:\n");
    Serial.printf("Speed = %d\n", params.power);
    Serial.printf("Kp = %f\n", params.kp);
    Serial.printf("Ki = %f\n", params.ki);
    Serial.printf("Kd = %f\n", params.kd);
    Serial.printf("Diff = %d\n", params.diff);

    server.send(200, "text/plain", "OK");
  }
}


void handleStart() {
  run = true;
  digitalWrite(On, HIGH);
  Serial.println("Start commando ontvangen!");
  server.send(200, "text/plain", "Gestart");
}

void handleStop() {
  run = false;
  digitalWrite(On, LOW);
  Serial.println("Stop commando ontvangen!");
  server.send(200, "text/plain", "Gestopt");
}
