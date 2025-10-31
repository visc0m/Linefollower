const int pinIRenable = 12;  // kies een GPIO voor IR aan/uit (optioneel)
int sensorPins[8] = {13, 14, 15, 16, 17, 18, 19, 20};  // pas aan naar jouw gebruikte GPIO's

void setup() {
  Serial.begin(115200);
  pinMode(pinIRenable, OUTPUT);
  digitalWrite(pinIRenable, HIGH);  // IR-LED’s aanzetten

  for (int i = 0; i < 8; i++) {
    pinMode(sensorPins[i], INPUT);
  }
}

void loop() {
  // Lees alle 8 sensoren
  for (int i = 0; i < 8; i++) {
    int val = analogRead(sensorPins[i]);
    Serial.print("D");
    Serial.print(i+1);
    Serial.print(": ");
    Serial.print(val);
    Serial.print("  ");
  }
  Serial.println();
  delay(200);
}
