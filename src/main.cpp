#include <Arduino.h>
#include <MFRC522_I2C.h>
#include <Wire.h>
#include <WiFiNINA.h>
#include "rgb_lcd.h"
#include "secret.h"

rgb_lcd lcd;
MFRC522 mfrc522(MFRC);

char ssid[] = "msd_2.4ghz";
char pass[] = "admin1234";

int status = WL_IDLE_STATUS;
WiFiServer server(80);

void setup() {
    Serial.begin(9600);
    lcd.begin(16, 2);

    Serial.print("Trying to connect to wifi: ");
    Serial.println(ssid);

    while ( status != WL_CONNECTED) {
        status = WiFi.begin(ssid, pass);
        delay(5000);
    }

    lcd.clear();
    lcd.setRGB(255, 255, 255);
    Wire.begin();
    mfrc522.PCD_Init();

    // Connected to the network
    Serial.print("Connected \nIP: ");
    IPAddress ip = WiFi.localIP();
    Serial.println(ip);
    lcd.clear();

    Serial.println("Setup done");
    lcd.print("Setup done");
}

void loop() {
    if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
        delay(50);
        return;
    }

    lcd.clear();
    String uidBytes;

    for (byte i = 0; i < mfrc522.uid.size; i++) {
        // Save card uid
        uidBytes += String(mfrc522.uid.uidByte[i], HEX);
    }

    Serial.print("Got a card: ");
    Serial.println(uidBytes);

    lcd.print("Scanning card..");
    delay(1000);
    lcd.clear();
    lcd.print("Ready");
}