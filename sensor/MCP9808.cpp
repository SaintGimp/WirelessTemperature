// Based on Adafruit_MCP9808 (https://github.com/adafruit/Adafruit_MCP9808_Library)

#if ARDUINO >= 100
 #include "Arduino.h"
#else
 #include "WProgram.h"
#endif

#ifdef __AVR_ATtiny85__
 #include "TinyWireM.h"
 #define Wire TinyWireM
#else
 #include <Wire.h>
#endif

#include "MCP9808.h"

MCP9808::MCP9808()
  : i2c_address(0)
{
}

boolean MCP9808::begin(uint8_t address) {
  i2c_address = address;
  Wire.begin();

  if (read16(MCP9808_REG_MANUF_ID) != 0x0054) return false;
  if (read16(MCP9808_REG_DEVICE_ID) != 0x0401) return false;

  return true;
}
 
uint16_t MCP9808::readTempRaw()
{
  return read16(MCP9808_REG_AMBIENT_TEMP);
}

float MCP9808::readTempC()
{
  uint16_t rawValue = readTempRaw();

  // Mask out 4 high bits, convert to decimal
  float temperature = (rawValue & 0x0FFF) / 16.0;

  // The raw value is 13-bit twos-complement value,
  // if the negation bit is set, it's a negative number
  // so we take the twos-complement of it
  if (rawValue & 0x1000) {
    temperature -= 256;
  }

  return temperature;
}

float MCP9808::readTempF()
{
  return readTempC() * 9.0 / 5.0 + 32;
}
 
boolean MCP9808::shutdown()
{
  uint16_t conf_register = read16(MCP9808_REG_CONFIG);

  if (conf_register & MCP9808_REG_CONFIG_CRITLOCKED == MCP9808_REG_CONFIG_CRITLOCKED || 
      conf_register & MCP9808_REG_CONFIG_WINLOCKED == MCP9808_REG_CONFIG_WINLOCKED) {
    return false;
  }
 
  uint16_t conf_shutdown = conf_register | MCP9808_REG_CONFIG_SHUTDOWN ;
  write16(MCP9808_REG_CONFIG, conf_shutdown);

  return true;
 }

void MCP9808::wake()
{
  uint16_t conf_register = read16(MCP9808_REG_CONFIG);
  
  
  uint16_t conf_wake = 0;
  //uint16_t conf_wake = conf_register & ~MCP9808_REG_CONFIG_SHUTDOWN;
  //uint16_t conf_wake = conf_register ^ MCP9808_REG_CONFIG_SHUTDOWN;
  write16(MCP9808_REG_CONFIG, conf_wake);
}

void MCP9808::write16(uint8_t reg, uint16_t value) {
    Wire.beginTransmission(i2c_address);
    Wire.write((uint8_t)reg);
    Wire.write(value >> 8);
    Wire.write(value & 0xFF);
    Wire.endTransmission();
}

uint16_t MCP9808::read16(uint8_t reg) {
  uint16_t val;

  Wire.beginTransmission(i2c_address);
  Wire.write((uint8_t)reg);
  Wire.endTransmission();
  
  Wire.requestFrom((uint8_t)i2c_address, (uint8_t)2);
  val = Wire.read();
  val <<= 8;
  val |= Wire.read();  
  return val;  
}
