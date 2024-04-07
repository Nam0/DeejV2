#include <Arduino.h>
#include <string.h>

const uint8_t buttonPin = A7;
const uint8_t NUM_SLIDERS = 5;
const uint16_t analogInputs[NUM_SLIDERS] = {A6, A1, A2, A3, A0};

uint16_t analogSliderValues[NUM_SLIDERS];
bool buttonPressed = false;

char serialBuffer[256];
uint8_t bufferIndex = 0;

void setup() { 
  for (uint8_t i = 0; i < NUM_SLIDERS; i++) {
    pinMode(analogInputs[i], INPUT);
  }
  pinMode(buttonPin, INPUT_PULLUP);
  Serial.begin(9600); // USB Serial
  //Serial1.begin(9600); // Serial1 (RX/TX pins)
}

void checkForHeadset() {
  if (digitalRead(buttonPin) == LOW) {
    Serial.println("Headset|1");
  } else {
    Serial.println("Headset|0");
  }
}

void updateSliderValues() {
  for (uint8_t i = 0; i < NUM_SLIDERS; i++) {
     analogSliderValues[i] = analogRead(analogInputs[i]);
  }
}

void sendSliderValues() {
  String builtString = String("");

  for (uint8_t i = 0; i < NUM_SLIDERS; i++) {
    builtString += String((uint16_t)analogSliderValues[i]);

    if (i < NUM_SLIDERS - 1) {
      builtString += String("|");
    }
  }
  
  Serial.println(builtString);
}

void processSerialCommand(const char* command) {

  for (size_t i = 0; command[i]; i++) {
    Serial.write(toupper(command[i]));
  }
  Serial.println();
}
void sendAck() {
  Serial1.println("ACK"); 
}

void readSerialData() {
  while (Serial1.available() > 0) {
    char incomingByte = Serial1.read();
    if (incomingByte != '\n') {
      serialBuffer[bufferIndex++] = incomingByte;
      if (bufferIndex >= sizeof(serialBuffer)) {
        bufferIndex = sizeof(serialBuffer) - 1;
      }
    } else {
      serialBuffer[bufferIndex] = '\0'; 
      processSerialCommand(serialBuffer); 
      sendAck(); 
      bufferIndex = 0; 
    }
  }
}

void loop() {
  updateSliderValues();
  checkForHeadset();
  sendSliderValues(); 
  //readSerialData(); 
  delay(1000);
}
