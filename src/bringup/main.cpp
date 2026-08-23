#include <Arduino.h>

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
  digitalWrite(LED_BUILTIN, (millis() / 500) & 1);
  static uint32_t last = 0;
  if (millis() - last >= 1000) {
    last = millis();
    Serial.print("alive ");
    Serial.println(millis());
  }
}
