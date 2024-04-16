#include <Arduino.h>
#include <MFRC522_I2C.h>
#include <Wire.h>
#include <WiFiNINA.h>
#include "rgb_lcd.h"
#include "secret.h"
#include <PubSubClient.h>

IPAddress server(SERVER_OCTETS);
MFRC522 mfrc522(MFRC);
rgb_lcd lcd;
WiFiClient net;
PubSubClient client(server, 1883, net);

String lastScannedCard;
int status = WL_IDLE_STATUS;

void connectToWifi(char ssid[], char pass[]);
void handleCallback(char *topic, byte *payload, unsigned int length);
void scanCard();
void verifyCard(String uid);
void connectClient();
void resetToDefault();

void setup() {
    // Setup Serial, LCD, MFRC/RFID scanner
    Serial.begin(9600);
    Serial.println("Setup started..");
    lcd.begin(16, 2);
    Wire.begin();
    mfrc522.PCD_Init();

    // Setup LEDs
    pinMode(RED_LED, OUTPUT);
    pinMode(GREEN_LED, OUTPUT);

    digitalWrite(GREEN_LED, HIGH);
    digitalWrite(RED_LED, HIGH);
    delay(150);
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(RED_LED, LOW);

    lcd.clear();
    lcd.setRGB(255, 255, 255);

    // Connect to the wifi
    connectToWifi(SECRET_SSID, SECRET_PASS);

    // Connect to MQTT
    client.setServer(server, 1883);
    connectClient();
    delay(500);
    lcd.clear();

    // Ready..
    lcd.print("Setup done");
    Serial.println("Setup done");
    delay(500);
    lcd.clear();
    lcd.print("Scan a card");
}

void loop() {
    scanCard();

    // Refresh MQTT connection
    client.loop();
}

void scanCard() {
    // Return if no new card is present or getting read
    if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
        delay(50);
        return;
    }

    String uidBytes;

    for (byte i = 0; i < mfrc522.uid.size; i++) {
        // Save card uid
        uidBytes += String(mfrc522.uid.uidByte[i], HEX);
    }

    Serial.print("A card was scanned: ");
    Serial.println(uidBytes);

    if (lastScannedCard == uidBytes) {
        lcd.clear();
        Serial.println("This card has just been scanned! Try again later..");
        lcd.print("This card is");
        lcd.setCursor(0, 1);
        lcd.print("on cooldown");
        lcd.setRGB(255,255,0);
    } else {
        lcd.clear();
        lcd.print("Verifying card..");
        lcd.setRGB(255,255,0);

        verifyCard(uidBytes);
        lastScannedCard = uidBytes;
    }

    // After 1 second reset back to default afterwards
    delay(1000);
    resetToDefault();
}

void resetToDefault() {
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(RED_LED, LOW);
    lcd.clear();
    lcd.setRGB(255,255,255);
    lcd.print("Scan a card");
}

// Connect to the wifi
void connectToWifi(char ssid[], char pass[]) {
    lcd.print("Connecting to");
    lcd.setCursor(0, 1);
    lcd.print(ssid);

    Serial.print("Trying to connect to wifi: ");
    Serial.println(ssid);

    while (status != WL_CONNECTED) {
        status = WiFi.begin(ssid, pass);
        delay(5000);
    }

    // Connected to the network & display it to the user
    IPAddress ip = WiFi.localIP();
    lcd.clear();
    lcd.print("Connected");
    lcd.setCursor(0, 1);
    lcd.print(ip);
    delay(300);
    Serial.print("Connected \nIP: ");
    Serial.println(ip);
}

// Connect to MQTT;
void connectClient() {
    int attempts = 0;
    // Loop until we're reconnected
    while (!client.connected()) {
        if (attempts < 5) {
            Serial.println("Attempting MQTT connection...");
            // Attempt to connect
            if (client.connect("wifiDoorClient", MQTT_USER, MQTT_PASS)) {
                Serial.println("MQTT Connected");
                delay(500);
                lcd.clear();
                lcd.print("MQTT Connected");
                // Once connected, publish an announcement...
                client.publish("/mqtt/", "Door arduino connected over wifi");
            } else {
                Serial.print("failed, rc=");
                Serial.print(client.state());
                Serial.println(" try again in 5 seconds");
                // Wait 5 seconds before retrying
                delay(5000);
                attempts++;
            }
        } else {
            Serial.println("MQTT Connection failed after 5 attempts exiting...");
            break;
        }
    }
    delay(400);
}

void handleCallback(char *topic, byte *payload, unsigned int length) {
    Serial.print("Handling response | Topic: ");
    Serial.print(topic);
    Serial.print("| Payload: ");

    // Since payload is byte we need to for loop through it
    for (int i = 0; i < length; i++) {
        Serial.println((char) payload[i]);

        lcd.clear();
        if ((char) payload[i] == '1') {
            lcd.print("Door opening..");
            Serial.println("Door opening..");
            digitalWrite(GREEN_LED, HIGH);
            lcd.setRGB(0, 255, 0);
        } else {
            lcd.print("No entry!");
            Serial.println("Card not allowed");
            digitalWrite(RED_LED, HIGH);
            lcd.setRGB(255, 0, 0);
        }
    }

    delay(1000);
    resetToDefault();
}

// Verify a card over MQTT
void verifyCard(String uid) {
    char topic[30] = {0};
    char message[50] = {0};
    char returnAddress[50] = {0};

    // Setup topic either for pin code or keycard
    strcat(topic, "/mqtt/room/doors/4/keycard");

    // Create return address from topic
    strcat(returnAddress, topic);
    strcat(returnAddress, "/return");

    // Create message to publish
    strcat(message, "!");
    strcat(message, uid.c_str());
    strcat(message, "+");
    strcat(message, returnAddress);

    Serial.println(topic);
    Serial.println(message);
    // Publish message and setup callback to handle response from server
    client.publish(topic, message);
    client.subscribe(returnAddress);
    client.setCallback(handleCallback);
}