#include <Arduino.h>

// Pinii pentru RGB si Butoane
#define PIN_RGB_RED 5
#define PIN_RGB_GREEN 6
#define PIN_BUTTON_START 2
#define PIN_BUTTON_STOP 3

// Definim pinii pentru LEDurile de incarcare
#define PIN_LED_25 7
#define PIN_LED_50 8
#define PIN_LED_75 9
#define PIN_LED_100 10

// definiri de timp
const unsigned long LED_STAGE_TIME = 3000; // 3 secunde pentru fiecare etapa
const unsigned long DEBOUNCE_DELAY = 50;   // delay pt debounce
const unsigned long STOP_FORCE_TIME = 1000; // long press pt stop btn
const unsigned long LED_BLINK_INTERVAL = 600; // 6 sec pt clipire
const int FULL_BLINK_CYCLES = 6;  // 6 tranzitii

// starea butoanelor
int startButtonState = HIGH;
int lastStartButtonState = HIGH;
int stopButtonState = HIGH;
int lastStopButtonState = HIGH;

// debouncing
unsigned long lastDebounceTime = 0;
unsigned long stopButtonHoldTime = 0;
unsigned long chargingStartTime = 0;

// separam incarcarea si starea finala
bool isCharging = false;
bool finalBlinking = false;
bool forceStopping = false; // oprire fortata
int currentLedIndex = 0;
bool ledBlinkState = false;
unsigned long lastBlinkTime = 0;
int chargeBlinkCounter = 0;
int finalBlinkCounter = 0;

// pinii
int ledPins[4] = {PIN_LED_25, PIN_LED_50, PIN_LED_75, PIN_LED_100};

// functie pt led rgb
void setRGB(bool red, bool green) {
  digitalWrite(PIN_RGB_RED, red);
  digitalWrite(PIN_RGB_GREEN, green);
}

// functie pt a stinge toate ledurile
void resetAllLeds() {
  for (int i = 0; i < 4; i++) {
    digitalWrite(ledPins[i], LOW);
  }
}

// functie pt incarcare
void startCharging() {
  setRGB(HIGH, LOW); // RGB ul pe rosu pt incarcare
  chargingStartTime = millis();
  currentLedIndex = 0;
  lastBlinkTime = millis();
  isCharging = true;
  finalBlinking = false;
  chargeBlinkCounter = 0;
  resetAllLeds(); //stingem toate ledurile 
}

// functie pt leduri in timp ce se petrece incarcarea
void handleChargingBlink() {
  // verificare pentru blink
  if (millis() - lastBlinkTime >= LED_BLINK_INTERVAL) {
    ledBlinkState = !ledBlinkState;
    digitalWrite(ledPins[currentLedIndex], ledBlinkState); //blink
    lastBlinkTime = millis();
    chargeBlinkCounter++;
  }

  // 3 blinkuri
  if (chargeBlinkCounter >= FULL_BLINK_CYCLES) {
    digitalWrite(ledPins[currentLedIndex], HIGH);  
    chargeBlinkCounter = 0;  // reset la count
    currentLedIndex++;

    // verificam daca s-au aprins toate ledurile
    if (currentLedIndex >= 4) {
      finalBlinking = true;  // blink final
      lastBlinkTime = millis();
      finalBlinkCounter = 0;  
      resetAllLeds();
    }
  }
}

// functie pt blink final
void handleFinalBlink() {
  if (millis() - lastBlinkTime >= LED_BLINK_INTERVAL) {
    ledBlinkState = !ledBlinkState;
    for (int i = 0; i < 4; i++) {
      digitalWrite(ledPins[i], ledBlinkState);  
    }
    lastBlinkTime = millis();
    finalBlinkCounter++;
  }

  if (finalBlinkCounter >= FULL_BLINK_CYCLES) {
    resetAllLeds(); 
    setRGB(LOW, HIGH);  // Punem RGB ul pe verde
    isCharging = false;
    finalBlinking = false;
    forceStopping = false; // resetam fortat
    finalBlinkCounter = 0;  
  }
}

// functie pt forcestop
void forceStopCharging() {
  if (!finalBlinking && !forceStopping) {  
    resetAllLeds();  
    finalBlinking = true;  
    forceStopping = true;  // Flag pt forcestop setat
    finalBlinkCounter = 0;
    lastBlinkTime = millis(); 
  }
}

// initializarea pinilor pt start
void setup() {
  // pini butoane
  pinMode(PIN_BUTTON_START, INPUT_PULLUP);
  pinMode(PIN_BUTTON_STOP, INPUT_PULLUP);

  // pini RGB
  pinMode(PIN_RGB_RED, OUTPUT);
  pinMode(PIN_RGB_GREEN, OUTPUT);

  // pini led
  for (int i = 0; i < 4; i++) {
    pinMode(ledPins[i], OUTPUT);
  }

  // RGB pe verde initial
  setRGB(LOW, HIGH);
  resetAllLeds();  //stingem toate ledurile pt siguranta
}

// functia main
void loop() {
  // citim butoanele
  int startButtonReading = digitalRead(PIN_BUTTON_START);
  int stopButtonReading = digitalRead(PIN_BUTTON_STOP);

  // deounce pt butonul de start
  if (startButtonReading != lastStartButtonState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY) {
    if (startButtonReading != startButtonState) {
      startButtonState = startButtonReading;
      if (startButtonState == LOW && !isCharging) { 
        startCharging(); // start incarcare
      }
    }
  }

  // debounce pt stop
  if (stopButtonReading != lastStopButtonState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY) {
    if (stopButtonReading != stopButtonState) {
      stopButtonState = stopButtonReading;
      if (stopButtonState == LOW && isCharging) {
        stopButtonHoldTime = millis();  // cautam long press ul
      }
    }

    // daca este apasat stopul incepem oprirea fortata
    if (stopButtonState == LOW && millis() - stopButtonHoldTime >= STOP_FORCE_TIME) {
      forceStopCharging();
    }
  }

  // procesul de incarcare
  if (isCharging && !finalBlinking && currentLedIndex < 4) {
    handleChargingBlink();
  }

  if (finalBlinking) {
    handleFinalBlink();
  }

  // resetam butoanele pt urmatoarea incarcare
  lastStartButtonState = startButtonReading;
  lastStopButtonState = stopButtonReading;
}
