#include <WiFi.h>
#include <ThingSpeak.h>
#include <DHT.h>
#include <Wire.h>
#include <BH1750.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_SSD1306.h>
#include <TinyGPS++.h>
#include <HardwareSerial.h>

#define DHTPIN 4
#define DHTTYPE DHT11
#define VIBRATION_SENSOR_PIN 32
#define MQ135_PIN 34
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

const char* ssid = "lol";         
const char* password = "xyz12345"; 
unsigned long channelID =  2906008;   
const char* writeAPIKey = "1RZYXR9MU7430ZY7";    

WiFiClient client;
DHT dht(DHTPIN, DHTTYPE);
BH1750 lightMeter;
Adafruit_BMP280 bmp;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);


TinyGPSPlus gps;
HardwareSerial gpsSerial(2);  

void setup() {
    Serial.begin(115200);

    
    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi...");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected.");
    ThingSpeak.begin(client);

    dht.begin();
    Wire.begin();
    
    gpsSerial.begin(9600, SERIAL_8N1, 16, 17); // GPS Serial

   
    lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE);

    
    bmp.begin(0x76);

    
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println("SSD1306 allocation failed");
        while (1);
    }
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    pinMode(VIBRATION_SENSOR_PIN, INPUT);
    Serial.println("Sensors Initialized");
}

void loop() {
    display.clearDisplay();

   
    float temp = dht.readTemperature();
    float hum = dht.readHumidity();
    float lux = lightMeter.readLightLevel();
    int vibration = digitalRead(VIBRATION_SENSOR_PIN);
    int air_quality = analogRead(MQ135_PIN);
    float pressure = bmp.readPressure();

   
    while (gpsSerial.available()) {
        gps.encode(gpsSerial.read());
    }

    float latitude = 0.0;
    float longitude = 0.0;

    if (gps.location.isValid()) {
        latitude = gps.location.lat();
        longitude = gps.location.lng();
    } else {
        Serial.println("Waiting for GPS signal...");
    }

    
    Serial.printf("Temp: %.2f°C, Humidity: %.2f%%, Light: %.2f lx, Vibration: %d, Air Quality: %d, Pressure: %.2f Pa\n",
                  temp, hum, lux, vibration, air_quality, pressure);
    Serial.printf("Latitude: %.6f, Longitude: %.6f\n", latitude, longitude);

   
    display.setCursor(0, 0); display.print("Temp: "); display.print(temp); display.print(" C");
    display.setCursor(0, 10); display.print("Humid: "); display.print(hum); display.print(" %");
    display.setCursor(0, 20); display.print("Light: "); display.print(lux); display.print(" lx");
    display.setCursor(0, 30); display.print(vibration ? "Vibration!" : "No vibration");
    display.setCursor(0, 40); display.print("Air Q: "); display.print(air_quality);
    display.setCursor(0, 50); display.print("Pres: "); display.print(pressure / 100.0); display.print(" hPa");
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

    delay(20000); 
}
