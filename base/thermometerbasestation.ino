// https://github.com/stewarthou/Particle-RF24
#include "particle-rf24.h"

/*
  PINOUTS
  http://docs.spark.io/#/firmware/communication-spi
  http://maniacbug.wordpress.com/2011/11/02/getting-started-rf24/
  SPARK CORE    SHIELD SHIELD    NRF24L01+
  GND           GND              1 (GND)
  3V3 (3.3V)    3.3V             2 (3V3)
  D6 (CSN)      9  (D6)          3 (CE)
  A2 (SS)       10 (SS)          4 (CSN)
  A3 (SCK)      13 (SCK)         5 (SCK)
  A5 (MOSI)     11 (MOSI)        6 (MOSI)
  A4 (MISO)     12 (MISO)        7 (MISO)
  NOTE: Also place a 10-100uF cap across the power inputs of
        the NRF24L01+.  I/O of the NRF24 is 5V tolerant, but
        do NOT connect more than 3.3V to pin 2(3V3)!!!
 */

#define TIME_BETWEEN_UPDATES 2000
#define NUMBER_OF_SENSORS 5
#define SENSOR_ACTIVE_WINDOW 10000

/* Hardware configuration: Set up nRF24L01 radio on SPI bus plus pins D6 & A2 */
RF24 radio(D6,A2);
byte sensorAddresses[][6] = {"1Sens", "2Sens", "3Sens", "4Sens", "5Sens"};
byte baseAddress[] = "1Base";

struct SensorData {
    int temperature;
    unsigned long timestamp;
};

SensorData readings[NUMBER_OF_SENSORS];
unsigned long lastDisplayTime = 0;
int lastDisplayedSensor = 0;

void setup() {
    initializeDisplay();
    initializeRadio();
    initializeCloud();
}

void loop() {
    receiveData();    
    displayReadings();
}

void initializeDisplay() {
    Serial1.begin(9600);

    Serial1.write(0xFE);
    Serial1.write(0x01);
    Serial1.print("Waiting for data");
}

void initializeRadio() {
    radio.begin();

    radio.setPALevel(RF24_PA_MAX);
    radio.setDataRate(RF24_250KBPS);
    radio.setPayloadSize(sizeof(int16_t));

    radio.openWritingPipe(baseAddress);
    for (int x = 0; x < NUMBER_OF_SENSORS; x++) {
        radio.openReadingPipe(x + 1, sensorAddresses[x]);
    }

    radio.startListening();
}

void initializeCloud() {
    for (int x = 0; x < NUMBER_OF_SENSORS; x++) {
        readings[x].temperature = -16000;
        String name = String("temp") + String(x + 1);
        Particle.variable(name, readings[x].temperature);
    }
}

void receiveData() {
    int16_t temperature;
    
    uint8_t pipe_number;
    if (radio.available(&pipe_number)) {
        radio.read(&temperature, sizeof(int16_t));
        uint8_t sensorNumber = pipe_number - 1;
        
        readings[sensorNumber].temperature = temperature;
        readings[sensorNumber].timestamp = millis();
        
        flashLed();
    }
}

void flashLed() {
    RGB.control(true);
    RGB.color(255, 0, 0);
    delay(50);
    RGB.control(false);
}

void displayReadings() {
    unsigned long now = millis();
    
    if (now - lastDisplayTime < TIME_BETWEEN_UPDATES) {
        return;
    }
    
    int sensorToDisplay = getSensorToDisplay(now, lastDisplayedSensor);
    if (sensorToDisplay < 0) {
        return;
    }
    
    float celsius = readings[sensorToDisplay].temperature / 16.0;
    Serial1.write(0xFE);
    Serial1.write(0x01);
    Serial1.print("Temp ");  
    Serial1.print(sensorToDisplay + 1);  
    Serial1.print(": ");  
    Serial1.print(celsius);  
    Serial1.print(" C");
    
    lastDisplayedSensor = sensorToDisplay;
    lastDisplayTime = now;
}

int getSensorToDisplay(unsigned long now, int startingSensor) {
    // Search for the next sensor that has a recent reading,
    // wrapping if necessary, give up if we checked all of them.
    
    int nextSensor = (startingSensor + 1) % NUMBER_OF_SENSORS;
    while (nextSensor != startingSensor) {
        if (readings[nextSensor].timestamp != 0 && now - readings[nextSensor].timestamp < SENSOR_ACTIVE_WINDOW) {
            return nextSensor;
        }
        
        nextSensor = (nextSensor + 1) % NUMBER_OF_SENSORS;
    }
    
    return -1;
}