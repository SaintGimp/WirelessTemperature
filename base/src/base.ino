// This #include statement was automatically added by the Particle IDE.
#include "LiquidCrystal_I2C_Spark.h"
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

#define TIME_BETWEEN_DISPLAY_UPDATES 2000
#define TIME_BETWEEN_EVENT_PUBLISHING 60000
#define NUMBER_OF_SENSORS 5
#define SENSOR_ACTIVE_WINDOW 10000

/* Hardware configuration: Set up nRF24L01 radio on SPI bus plus pins D6 & A2 */
RF24 radio(D6,A2);
byte sensorAddresses[][6] = {"1Sens", "2Sens", "3Sens", "4Sens", "5Sens"};
byte baseAddress[] = "1Base";

struct SensorData {
    int temperature;
    int voltage;
    unsigned long timestamp;
};

SensorData readings[NUMBER_OF_SENSORS];
unsigned long lastDisplayTime = 0;
unsigned long lastDisplayResetTime = 0;
int lastDisplayedSensor = 0;
unsigned long lastEventPublishTime = 0;
int missedReadings = -1;

LiquidCrystal_I2C *lcd;

SYSTEM_MODE(AUTOMATIC);
SYSTEM_THREAD(ENABLED);

void setup() {
    initializeDisplay();
    initializeRadio();
}

void loop() {
    receiveData();    
    displayReadings();
    publishEvents();
}

void initializeDisplay() {
    lcd = new LiquidCrystal_I2C(0x3F, 16, 2);
    lcd->init();
    lcd->backlight();
    lcd->clear();
    lcd->print("Waiting for data");
}

void resetDisplay() {
    lcd->init();
    lcd->clear();
}

void initializeRadio() {
    radio.begin();

    radio.setPALevel(RF24_PA_MAX);
    radio.setDataRate(RF24_250KBPS);
    radio.setPayloadSize(sizeof(int16_t) * 2);

    radio.openWritingPipe(baseAddress);
    for (int x = 0; x < NUMBER_OF_SENSORS; x++) {
        radio.openReadingPipe(x + 1, sensorAddresses[x]);
    }

    radio.startListening();
}

void receiveData() {
    uint8_t pipe_number;
    int16_t buffer[2];
    
    if (radio.available(&pipe_number)) {
        uint8_t sensorNumber = pipe_number - 1;

        radio.read(buffer, sizeof(buffer));
        
        readings[sensorNumber].temperature = buffer[0];
        readings[sensorNumber].voltage = buffer[1];
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
    
    if (now - lastDisplayTime < TIME_BETWEEN_DISPLAY_UPDATES) {
        return;
    }
    
    if (now - lastDisplayResetTime > 60000) {
        resetDisplay();
        lastDisplayResetTime = now;
    }
    
    int sensorToDisplay = getSensorToDisplay(now, lastDisplayedSensor);
    if (sensorToDisplay < 0) {
        return;
    }
    
    float celsius = readings[sensorToDisplay].temperature / 16.0;
    lcd->setCursor(0,0);
    lcd->print("Temp ");  
    lcd->print(sensorToDisplay + 1);  
    lcd->print(": ");  
    lcd->print(celsius);  
    lcd->print(" C ");
    
    float voltage = readings[sensorToDisplay].voltage / 100.0;
    lcd->setCursor(0,1);
    lcd->print(voltage);
    lcd->print(" V  ");
    lcd->print(missedReadings);  
    lcd->print("  ");

    lastDisplayedSensor = sensorToDisplay;
    lastDisplayTime = now;
}

int getSensorToDisplay(unsigned long now, int startingSensor) {
    // Search for the next sensor that has a recent reading,
    // wrapping if necessary, give up if we checked all of them.
    
    int nextSensor = (startingSensor + 1) % NUMBER_OF_SENSORS;
    for (int x = 0; x < NUMBER_OF_SENSORS; x++) {
        if (readings[nextSensor].timestamp != 0 && now - readings[nextSensor].timestamp < SENSOR_ACTIVE_WINDOW) {
            if (missedReadings == -1)
            {
                missedReadings = 0;
            }

            return nextSensor;
        }
        if (nextSensor == 2 && missedReadings >= 0)
        {
            missedReadings++;
        }
        nextSensor = (nextSensor + 1) % NUMBER_OF_SENSORS;
    }
    
    return -1;
}

void publishEvents() {
    unsigned long now = millis();
    
    if (now - lastEventPublishTime < TIME_BETWEEN_EVENT_PUBLISHING) {
        return;
    }

    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));
    JSONBufferWriter writer(buffer, sizeof(buffer) - 1);
    writer.beginObject();

    for (int x = 0; x < NUMBER_OF_SENSORS; x++) {
        int sensor = x + 1;
        String temperature_name = "t" + String(sensor);
        String voltage_name = "v" + String(sensor);

        if (readings[x].timestamp != 0 && now - readings[x].timestamp < SENSOR_ACTIVE_WINDOW) {
            writer.name(temperature_name).value(readings[x].temperature / 16.0);
            writer.name(voltage_name).value(readings[x].voltage / 100.0);
        }
        else {
            writer.name(temperature_name).value("");
            writer.name(voltage_name).value("");
        }
    }

    writer.endObject();
    
    // First run 'particle webhook create webhook.json' from shell
    // View activity at https://dashboard.particle.io/user/logs
    Particle.publish("temp-sensors", buffer, PRIVATE);
    
    lastEventPublishTime = now;
}
