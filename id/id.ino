#include <EEPROM.h>

// Expected fuses: http://www.engbedded.com/fusecalc
// 8MHz internal clock
// Preserve EEPROM memory
// BOD 2.7V
// avrdude -c usbtiny -p m328p -U lfuse:w:0xe2:m -U hfuse:w:0xd1:m -U efuse:w:0x05:m

#define LED_PIN 0

byte nodeId = 0;

void setup() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  EEPROM.write(0, nodeId);
}

void loop() {
  blinkIdCode(nodeId);

  delay(1000);
}

void blinkIdCode(byte id)
{
  for (byte i = 0; i < id; i++)
  {
    blinkIndicatorLed();
    delay(350);
  }
}

void blinkIndicatorLed()
{
  digitalWrite(LED_PIN, HIGH);
  delay(10);
  digitalWrite(LED_PIN, LOW);
}

