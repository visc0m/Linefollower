// DRV8833 testprogramma - ESP32-S3
// Motor A: AIN1 = GPIO4, AIN2 = GPIO5
// Motor B: BIN1 = GPIO6, BIN2 = GPIO7

#define AIN1 4
#define AIN2 5
#define BIN1 6
#define BIN2 7
#define Stop 8
#define Start 46
#define On 3

boolean aan=false;
// PWM-instellingen
#define FREQ     20000   // 20 kHz = stil voor motoren
#define RESOLUTION 8     // 8 bit (0-255)

unsigned long vorigeTijd = 0;
const long interval = 100;

void setup() {
  Serial.begin(115200);
  Serial.println("DRV8833 test start");
  
  // Attach pinnen aan kanalen
  pinMode(AIN1, OUTPUT);
  pinMode(AIN2, OUTPUT);
  pinMode(BIN1, OUTPUT);
  pinMode(BIN2, OUTPUT);
  pinMode(On, OUTPUT);
  pinMode(Stop, INPUT_PULLUP);
  pinMode(Start, INPUT_PULLUP);

  stopMotors();
}

void loop() {
  if(!digitalRead(Start)){aan=true;}
  if(!digitalRead(Stop)){aan=false;}
  digitalWrite(On, aan);
  if(aan){testmotor();}
  else{stopMotors();}
}

// ------------------- Functies -------------------
void testmotor(){
  uint8_t A=0;
  uint8_t B=0;
  while (A<255){
    if(millis()- vorigeTijd >= interval){
      Serial.println(A);
      A++;
      B++;
      motorAForward(A);
      motorBForward(B);
      vorigeTijd = millis();
      if(!digitalRead(Stop)){
        aan=false;
        return;
        }
    }
  }
  Serial.println("back");
  while (A>0){
    if(millis()- vorigeTijd >= interval){
      Serial.println(A);
      A--;
      B--;
      motorABackward(A);
      motorBBackward(B);
      vorigeTijd = millis();
      if(!digitalRead(Stop)){
        aan=false;
        return;
        }
    }
  }

}
void motorAForward(uint8_t speed) {
  analogWrite(AIN1, speed);
  analogWrite(AIN2, 0);
}

void motorABackward(uint8_t speed) {
  analogWrite(AIN1, 0);
  analogWrite(AIN2, speed);
}

void motorBForward(uint8_t speed) {
  analogWrite(BIN1, speed);
  analogWrite(BIN2, 0);
}

void motorBBackward(uint8_t speed) {
  analogWrite(BIN1, 0);
  analogWrite(BIN2, speed);
}

void stopMotors() {
  analogWrite(AIN1, 0);
  analogWrite(AIN2, 0);
  analogWrite(BIN1, 0);
  analogWrite(BIN2, 0);
}
