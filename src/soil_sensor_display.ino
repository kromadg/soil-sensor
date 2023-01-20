#include <SoftwareSerial.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128    // OLED display width, in pixels
#define SCREEN_HEIGHT 64    // OLED display height, in pixels
#define OLED_RESET -1       // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define RE 8
#define DE 7

const byte hum_temp_ec[8] = {0x01, 0x03, 0x00, 0x00, 0x00, 0x03, 0x05, 0xCB};
byte sensorResponse[12] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
byte sensor_values[11];

SoftwareSerial mod(2, 3);

void setup() {
    // put your setup code here, to run once:
    Serial.begin(115200);
    mod.begin(4800);
    pinMode(RE, OUTPUT);
    pinMode(DE, OUTPUT);
    digitalWrite(RE, LOW);
    digitalWrite(DE, LOW);

    // Initializes display
    display.begin(SSD1306_SWITCHCAPVCC, 0x3C); // initialize with the I2C addr 0x3C (128x64)
    delay(500);
    display.clearDisplay();
    display.setCursor(13, 15);
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.println(" THC Soil Sensor");
    display.setCursor(21, 35);
    display.setTextSize(1);
    display.print("Initializing...");
    display.display();
    delay(5000);
}

void loop() {
    // put your main code here, to run repeatedly:

    /**************Soil EC Reading*******************/
    digitalWrite(DE, HIGH);
    digitalWrite(RE, HIGH);
    memset(sensor_values, 0, sizeof(sensor_values));
    delay(100);
    if (mod.write(hum_temp_ec, sizeof(hum_temp_ec)) == 8) {
        digitalWrite(DE, LOW);
        digitalWrite(RE, LOW);
        for (byte i = 0; i < 12; i++) {
            //Serial.print(mod.read(),HEX);
            sensorResponse[i] = mod.read();
            yield();
            Serial.print(sensorResponse[i], HEX);
            Serial.print(",");
        }
        Serial.println();
    }

    delay(250);

    float soil_hum = 0.1 * int(sensorResponse[3] << 8 | sensorResponse[4]);
    float soil_temp = 0.1 * int(sensorResponse[5] << 8 | sensorResponse[6]);
    int soil_ec = int(sensorResponse[7] << 8 | sensorResponse[8]);

    /**
     * Soil VWC correction. Test and use if works for you.
     */
    // change: quadratic aproximation of VWC from CWT sensor to Teros 12 sensor.
    // Just in case or for tests. The VWC of Teros and chinese sensor are very close (see reference spreadsheet).
    //soil_hum = -0.0134 * soil_hum * soil_hum + 1.6659 * soil_hum - 6.1095;

    /**
     * Bulk EC correction. Choose one, test and uncomment if works for you.
     */
    // CHOOSE ONE: cubic aproximation of BULK EC from CWT sensor to Teros 12 sensor (more precise)
    //soil_ec = 0.0000014403 * soil_ec * soil_ec * soil_ec - 0.0036 * soil_ec * soil_ec + 3.7525 * soil_ec - 814.1833;

    // CHOOSE ONE: This equation was obtained from calibration using distilled water and a 1.1178mS/cm solution.
    // Change by @danielfppps >> https://github.com/kromadg/soil-sensor/issues/3#issuecomment-1383959976
    soil_ec = 1.93 * soil_ec - 270.8;

    /**
     * Bulk EC temperature correction. Test and use if works for you.
     */
    // Soil EC temp correction based on the Teros 12 manual. https://github.com/kromadg/soil-sensor/issues/1
    soil_ec = soil_ec / (1.0 + 0.019 * (soil_temp - 25));

    // soil_temp foi deixada a mesma pois os valores do teros e do sensor chines sao parecidos

    // quadratic aproximation
    // the teros bulk_permittivity was calculated from the teros temperature, teros bulk ec and teros pwec by Hilhorst 2000 model
    float soil_apparent_dieletric_constant = 1.3088 + 0.1439 * soil_hum + 0.0076 * soil_hum * soil_hum;

    float soil_bulk_permittivity = soil_apparent_dieletric_constant;  /// hammed 2015 (apparent_dieletric_constant is the real part of permittivity)
    float soil_pore_permittivity = 80.3 - 0.37 * (soil_temp - 20); /// same as water 80.3 and corrected for temperature

    // converting bulk EC to pore water EC
    float soil_pw_ec;
    if (soil_bulk_permittivity > 4.1)
        soil_pw_ec = ((soil_pore_permittivity * soil_ec) / (soil_bulk_permittivity - 4.1) / 1000); /// from Hilhorst 2000.
    else
        soil_pw_ec = 0;

    Serial.print("Humidity: ");
    Serial.print(soil_hum);
    Serial.println(" %");
    Serial.print("Temperature: ");
    Serial.print(soil_temp);
    Serial.println(" °C");
    Serial.print("EC: ");
    Serial.print(soil_ec);
    Serial.println(" us/cm");
    Serial.print("pwEC: ");
    Serial.print(soil_pw_ec);
    Serial.println(" dS/m");
    Serial.print("soil_bulk_permittivity: ");
    Serial.println(soil_bulk_permittivity);
    delay(2000);

    display.clearDisplay();

    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print("Humi:  ");
    display.setTextSize(2);
    display.print(soil_hum);
    display.setTextSize(1);
    display.print(" %");

    display.setTextSize(1);
    display.setCursor(0, 17);
    display.print("Temp:  ");
    display.setTextSize(2);
    display.print(soil_temp);
    display.setTextSize(1);
    display.print(" C");

    display.setTextSize(1);
    display.setCursor(0, 34);
    display.print("EC:    ");
    display.setTextSize(2);
    display.print(soil_ec);
    display.setTextSize(1);
    display.print(" us/cm");

    display.setTextSize(1);
    display.setCursor(0, 50);
    display.print("pwEC:  ");
    display.setTextSize(2);
    display.print(soil_pw_ec);
    display.setTextSize(1);
    display.print(" dS/m");

    display.display();
}
