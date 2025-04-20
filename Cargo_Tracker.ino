#include <WiFi.h>
#include <ThingSpeak.h>
#include <DHT.h>
#include <Wire.h>
#include <BH1750.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_SSD1306.h>
#include <TinyGPS++.h>
#include <HardwareSerial.h>
#include <SPI.h>
#include <LoRa.h>

#define DHTPIN 4
#define DHTTYPE DHT11
#define VIBRATION_SENSOR_PIN 32
#define MQ135_PIN 34
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

const char* ssid = "lol";
const char* password = "xyz12345";
unsigned long channelID = 2906008;
const char* writeAPIKey = "1RZYXR9MU7430ZY7";

WiFiClient client;
DHT dht(DHTPIN, DHTTYPE);
BH1750 lightMeter;
Adafruit_BMP280 bmp;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

TinyGPSPlus gps;
HardwareSerial gpsSerial(2);
HardwareSerial sim800lSerial(1);

#define LORA_SCK 5
#define LORA_MISO 19
#define LORA_MOSI 27
#define LORA_SS 18
#define LORA_RST 14
#define LORA_DIO0 26

void setup() {
    Serial.begin(115200);

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
    }
    ThingSpeak.begin(client);

    dht.begin();
    Wire.begin();
    gpsSerial.begin(9600, SERIAL_8N1, 16, 17);
    sim800lSerial.begin(9600, SERIAL_8N1, 33, 32);
    lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE);
    bmp.begin(0x76);

    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        while (1);
    }
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_SS);
    LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);
    if (!LoRa.begin(915E6)) {
        while (1);
    }

    pinMode(VIBRATION_SENSOR_PIN, INPUT);
}

void loop() {
    display.clearDisplay();

    float temp = dht.readTemperature();
    float hum = dht.readHumidity();
    float lux = lightMeter.readLightLevel();
    int vibration = digitalRead(VIBRATION_SENSOR_PIN);
    int air_quality = analogRead(MQ135_PIN);
    float pressure = bmp.readPressure() / 100.0;

    while (gpsSerial.available()) {
        gps.encode(gpsSerial.read());
    }
    float latitude = 0.0;
    float longitude = 0.0;
    if (gps.location.isValid()) {
        latitude = gps.location.lat();
        longitude = gps.location.lng();
    }

    Serial.printf("Temp: %.2f°C, Humidity: %.2f%%, Light: %.2f lx, Vibration: %d, AirQ: %d, Pressure: %.2f hPa\n",
                  temp, hum, lux, vibration, air_quality, pressure);
    Serial.printf("Lat: %.6f, Lon: %.6f\n", latitude, longitude);

    display.setCursor(0, 0); display.print("T:"); display.print(temp); display.print("C ");
    display.setCursor(0, 10); display.print("H:"); display.print(hum); display.print("% ");
    display.setCursor(0, 20); display.print("L:"); display.print(lux); display.print("lx ");
    display.setCursor(0, 30); display.print(vibration ? "Vibration!" : "No Vibration");
    display.setCursor(0, 40); display.print("A:"); display.print(air_quality);
    display.setCursor(0, 50); display.print("P:"); display.print(pressure); display.print("hPa");
    display.display();

    ThingSpeak.setField(1, temp);
    ThingSpeak.setField(2, hum);
    ThingSpeak.setField(3, lux);
    ThingSpeak.setField(4, vibration);
    ThingSpeak.setField(5, air_quality);
    ThingSpeak.setField(6, pressure);
    ThingSpeak.setField(7, latitude);
    ThingSpeak.setField(8, longitude);

    int x = ThingSpeak.writeFields(channelID, writeAPIKey);
    if (x == 200) {
        Serial.println("ThingSpeak update successful.");
    } else {
        Serial.print("ThingSpeak update failed. Code: ");
        Serial.println(x);
    }

    LoRa.beginPacket();
    LoRa.print("T:"); LoRa.print(temp);
    LoRa.print(" H:"); LoRa.print(hum);
    LoRa.print(" L:"); LoRa.print(lux);
    LoRa.print(" V:"); LoRa.print(vibration);
    LoRa.print(" AQ:"); LoRa.print(air_quality);
    LoRa.print(" P:"); LoRa.print(pressure);
    LoRa.print(" Lat:"); LoRa.print(latitude);
    LoRa.print(" Lon:"); LoRa.print(longitude);
    LoRa.endPacket();

    sendSMS(temp, hum, air_quality);

    delay(20000);
}

void sendSMS(float temp, float hum, int airq) {
    sim800lSerial.println("AT+CMGF=1");
    delay(100);
    sim800lSerial.println("AT+CMGS=\"+94716517254\"");
    delay(100);
    sim800lSerial.print("ALERT! Temp:");
    sim800lSerial.print(temp);
    sim800lSerial.print("C Humidity:");
    sim800lSerial.print(hum);
    sim800lSerial.print("% AirQ:");
    sim800lSerial.print(airq);
    sim800lSerial.write(26);
    delay(5000);
    Serial.println("SMS Sent.");
}
