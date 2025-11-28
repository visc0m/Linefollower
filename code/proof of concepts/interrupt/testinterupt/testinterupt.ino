#define Start 46
#define Stop  8
#define On   3

volatile bool run = false;
volatile unsigned long lastInterrupt = 0;

void IRAM_ATTR handleStart() {
  unsigned long now = millis();
  if (now - lastInterrupt > 150) {   // debounce
      run = true;
      digitalWrite(On, HIGH);   // LED AAN
  }
  lastInterrupt = now;
}

void IRAM_ATTR handleStop() {
  unsigned long now = millis();
  if (now - lastInterrupt > 150) {   // debounce
      run = false;
      digitalWrite(On, LOW);    // LED UIT
  }
  lastInterrupt = now;
}

void setup() {
  Serial.begin(115200);

  pinMode(Start, INPUT_PULLUP);
  pinMode(Stop, INPUT_PULLUP);
  pinMode(On, OUTPUT);

  attachInterrupt(digitalPinToInterrupt(Start), handleStart, FALLING);
  attachInterrupt(digitalPinToInterrupt(Stop), handleStop, FALLING);
}

void loop() {
  // laat de robot het "run" signaal gebruiken
  if (run) {
    Serial.println("RUNNING");
  } else {
    Serial.println("STOPPED");
  }
  delay(500);
}
