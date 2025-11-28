#include <WiFi.h>
#include <WebServer.h>

const char* ssid = "LAPTOP-06F9RUHC 2524";
const char* password = "Azerty123";

WebServer server(80);

int Speed = 200;
float diff = 0.15;
float kp = 18;
float ki = 0.002;
float kd = 100;
bool running = false;

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
    <input type="number" id="SpeedInput" style="width:60px;" value=')rawliteral" + String(Speed) + R"rawliteral(' /><br>
    <label for="kpInput">Kp:</label>
    <input type="number" id="kpInput" style="width:60px;" value=')rawliteral" + String(kp) + R"rawliteral(' /><br>
    <label for="kiInput">Ki:</label>
    <input type="number" id="kiInput" style="width:60px;" value=')rawliteral" + String(ki) + R"rawliteral(' /><br>
    <label for="kdInput">Kd:</label>
    <input type="number" id="kdInput" style="width:60px;" value=')rawliteral" + String(kd) + R"rawliteral(' /><br>
    <label for="diffInput">Diff:</label>
    <input type="number" id="diffInput" style="width:60px;" value=')rawliteral" + String(diff) + R"rawliteral(' /><br>
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
      let val = document.getElementById("SpeedInput").value;
      fetch('/setValues?val=' + val);
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
  json += "\"kp\":" + String(kp,4) + ",";
  json += "\"ki\":" + String(ki,4) + ",";
  json += "\"kd\":" + String(kd,4) + ",";
  json += "\"diff\":" + String(diff,4) + ",";
  json += "\"Speed\":" + String(Speed) + ",";
  json += "\"running\":" + String(running ? "true" : "false");
  json += "}";
  server.send(200, "application/json", json);
  Serial.println(json);
}

void handlesetValues() {
  if (!running) { // alleen toestaan als running false is
    if (server.hasArg("val")) {
      Speed = server.arg("val").toInt();
      Serial.printf("Nieuwe speed ingesteld: %d\n", Speed);
    }
    server.send(200, "text/plain", String(Speed));
  }
}

void handleStart() {
  running = true;
  Serial.println("Start commando ontvangen!");
  server.send(200, "text/plain", "Gestart");
}

void handleStop() {
  running = false;
  Serial.println("Stop commando ontvangen!");
  server.send(200, "text/plain", "Gestopt");
}

void setup() {
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
}

void loop() {
  server.handleClient();
  
}
