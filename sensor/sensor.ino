#include <avr/sleep.h>
#include <avr/wdt.h>
#include "MCP9808.h"
#include <SPI.h>
#include "RF24.h"
#include <EEPROM.h>

#define LED_PIN 0

// Node addresses need to be unique per device and should
// vary only in the first byte.  We're going to load the first
// byte from EEPROM (set by another sketch) and modify the address
byte nodeAddress[] = "*Sens";
byte baseAddress[] = "1Base";
RF24 radio(7,8);

MCP9808 sensor = MCP9808();

void setup(void)
{
  initializePins();
  loadNodeAddress();
  initializePeripherals();
}

void loop(void)
{ 
  wakePeripherals();
  sendTemperatureData(); 
  shutDownPeripherals();
  longSleep();
}

void initializePins()
{
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
}

void initializePeripherals()
{
  sensor.begin();
  
  radio.begin();
  radio.setRetries(15,15);

  // TODO: can we start with a lower power level and boost if we're having issues?
  radio.setPALevel(RF24_PA_MAX);
  radio.setDataRate(RF24_250KBPS);
  radio.setPayloadSize(sizeof(int16_t));
  
  radio.openWritingPipe(nodeAddress);
  radio.openReadingPipe(1, baseAddress);

  // Any communication to us is going to piggyback on ACKs
  radio.stopListening();
}

void loadNodeAddress()
{
  byte nodeId = EEPROM.read(0);

  if (nodeId < 1 || nodeId > 9)
  {
    blinkError();  
  }
  else
  {
    blinkIdCode(nodeId);
  }

  // convert id to ASCII digit
  nodeAddress[0] = nodeId + 48;
}

void blinkError()
{
  while (1)
  {
    blinkIndicatorLed();
    delay(200);
  }
}

void sendTemperatureData()
{
  int16_t temperatureAsInteger = sensor.readTempC() * 16;

  // TODO: get voltage and send it too
  bool success = radio.write(&temperatureAsInteger, sizeof(int16_t));
  if (!success)
  {
    blinkIndicatorLed();
  }
}

void shutDownPeripherals()
{
  sensor.shutdown();
  radio.powerDown();
}

void wakePeripherals()
{
  // When we wake the sensor it takes time to do another conversion.
  // The time depends on the configured resolution (250ms for highest res.). We need to wait
  // until the sensor has had time to do another conversion after
  // waking or we'll just end up reading the previous conversion.
  sensor.wake();
  shortSleep();
  radio.powerUp();
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

// For sleep info see http://www.gammon.com.au/power

void shortSleep()
{
  // 256 ms then 32 ms
  doSleep(bit (WDP2));
  doSleep(bit (WDP0));
}

void longSleep()
{
  // 8 seconds
  doSleep(bit (WDP3) | bit (WDP0));
}

// watchdog interrupt
ISR (WDT_vect) 
{
   wdt_disable();  // disable watchdog
}

void doSleep(const byte interval)
{
  byte old_ADCSRA = ADCSRA;
  ADCSRA = 0;  
 
  // clear various "reset" flags
  MCUSR = 0;     
  // allow changes, disable reset
  WDTCSR = bit (WDCE) | bit (WDE);
  // set interrupt mode and an 0.5-second interval 
  WDTCSR = bit (WDIE) | interval;
  wdt_reset();  // pat the dog

  // timed sequence follows, must not add additional code in here
  noInterrupts();
  set_sleep_mode (SLEEP_MODE_PWR_DOWN);  
  sleep_enable();
 
  // turn off brown-out enable in software
  MCUCR = bit (BODS) | bit (BODSE);
  MCUCR = bit (BODS); 
  // guarantees next instruction executed
  interrupts();
  sleep_cpu();  
  
  // cancel sleep as a precaution
  sleep_disable();

  ADCSRA = old_ADCSRA;
}

