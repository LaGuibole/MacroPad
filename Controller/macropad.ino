#include <HijelHID_BLEKeyboard.h>

HijelHID_BLEKeyboard keyboard("MonMacroPad", "DIY", 100);

const int NB_BUTTONS = 9;
const int PINS[NB_BUTTONS] = {13, 21, 4, 27, 25, 5, 32, 22, 18};
bool lastState[NB_BUTTONS];

void setup() {
  Serial.begin(115200);
  for (int i = 0; i < NB_BUTTONS; i++) {
    pinMode(PINS[i], INPUT_PULLUP);
    lastState[i] = HIGH;
  }
  keyboard.begin();
  Serial.println("Démarrage BLE...");
}

void loop() {
  for (int i = 0; i < NB_BUTTONS; i++) {
    bool currentState = digitalRead(PINS[i]);

    if (lastState[i] == HIGH && currentState == LOW) {
      if (keyboard.isConnected()) {
        String msg = String(i + 1) + " OK !";
        for (int c = 0; c < msg.length(); c++) {
          keyboard.tap((uint8_t)msg[c]);
        }
      } else {
        Serial.println("BLE non connecté");
      }
      Serial.print("Bouton ");
      Serial.print(i + 1);
      Serial.println(" appuyé");
    }

    lastState[i] = currentState;
  }
  delay(50);
}