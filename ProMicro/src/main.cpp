#include <Arduino.h>

const uint8_t buttonPin = A7;
const uint8_t NUM_SLIDERS = 5;
const uint16_t analogInputs[NUM_SLIDERS] = {A6, A1, A2, A3, A0};

uint16_t analogSliderValues[NUM_SLIDERS];
uint16_t oldAnalogSliderValues[NUM_SLIDERS];
bool buttonPressed = false;

char serialBuffer[256];
uint8_t bufferIndex = 0;

unsigned long lastSliderCheckTime = 0;
const unsigned long sliderCheckInterval = 2; 
const uint8_t NUM_SAMPLES = 3; 
uint16_t avgSliderValues[NUM_SLIDERS];

const uint16_t TOLERANCE = 1; 

void setup() {
  for (uint8_t i = 0; i < NUM_SLIDERS; i++) {
    pinMode(analogInputs[i], INPUT);
  }
  pinMode(buttonPin, INPUT_PULLUP);
  Serial.begin(9600); 
  Serial1.begin(9600); 
}

void readSliderValues() {
  for (uint8_t i = 0; i < NUM_SLIDERS; i++) {
    analogSliderValues[i] = analogRead(analogInputs[i]);
  }
}

void calculateMovingAverage() {
  for (uint8_t i = 0; i < NUM_SLIDERS; i++) {
    uint32_t sum = 0;
    for (uint8_t j = 0; j < NUM_SAMPLES; j++) {
      sum += analogRead(analogInputs[i]);
      delayMicroseconds(100); 
    }
    avgSliderValues[i] = sum / NUM_SAMPLES;
  }
}

void applyWeightedAverage() {
  for (uint8_t i = 0; i < NUM_SLIDERS; i++) {
    avgSliderValues[i] = (avgSliderValues[i] + oldAnalogSliderValues[i]) / 2;
  }
}

bool hasSliderValuesChanged() {
  for (uint8_t i = 0; i < NUM_SLIDERS; i++) {
    if (abs(avgSliderValues[i] - oldAnalogSliderValues[i]) > TOLERANCE) {
      return true; 
    }
  }
  return false;
}

bool isWithinTolerance(uint16_t value1, uint16_t value2) {
  return (abs(value1 - value2) <= TOLERANCE);
}

void sendChangedSliderValues() {
  String builtString = "";
  for (uint8_t i = 0; i < NUM_SLIDERS; i++) {
    builtString += "V" + String(i) + ":" + String((uint16_t)avgSliderValues[i]);
    if (i < NUM_SLIDERS - 1) {
      builtString += "|"; // Add separator unless it's the last value
    }
  }
  builtString += "|"; // Add final separator after the last value
  Serial.println(builtString);
}

void processSerialCommand(const char* command) {
  for (size_t i = 0; command[i]; i++) {
    Serial.write(toupper(command[i]));
  }
  Serial.println();
}

void sendAcknowledge() {
  Serial1.println("ACK");
}

void forwardSerialData(const char* message) {
  if (strstr(message, "Setup") != nullptr) {
    delay(10);
    Serial1.println(message);
  }
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
      bufferIndex = 0;
      sendAcknowledge();
    }
  }
}

void handleSerialInput() {
  while (Serial.available() > 0) {
    char incomingByte = Serial.read();
    if (incomingByte != '\n') {
      serialBuffer[bufferIndex++] = incomingByte;
      if (bufferIndex >= sizeof(serialBuffer)) {
        bufferIndex = sizeof(serialBuffer) - 1;
      }
    } else {
      serialBuffer[bufferIndex] = '\0';
      forwardSerialData(serialBuffer);
      bufferIndex = 0;
    }
  }
}

void loop() {
  unsigned long currentMillis = millis();
  if (currentMillis - lastSliderCheckTime >= sliderCheckInterval) {
    readSliderValues();
    calculateMovingAverage();
    applyWeightedAverage();
    if (hasSliderValuesChanged()) {
      sendChangedSliderValues();
      memcpy(oldAnalogSliderValues, avgSliderValues, sizeof(avgSliderValues));
    }
    lastSliderCheckTime = currentMillis;
  }
  readSerialData();
  handleSerialInput();
}
