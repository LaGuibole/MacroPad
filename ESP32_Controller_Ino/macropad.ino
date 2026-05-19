#include "USB.h"
#include "USBHIDKeyboard.h"
#include <NimBLEDevice.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ---- Pins switches ----
const int NB_BUTTONS = 9;
const int PINS[NB_BUTTONS] = {20, 10, 16, 21, 18, 15, 47, 17, 38};
bool lastState[NB_BUTTONS];

// ---- Pins OLED ----
#define OLED_SDA    8
#define OLED_SCL    9
#define OLED_WIDTH  128
#define OLED_HEIGHT 64
Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);

// ---- Pins encodeur ----
#define ENC_CLK  4
#define ENC_DT   5
#define ENC_SW   6
int lastEncCLK;
unsigned long lastEncDebounce = 0;
unsigned long lastSwDebounce  = 0;
bool lastSwState = HIGH;

// ---- Profils ----
const int NB_PROFILES = 4;
int currentProfile = 0;
String actions[NB_PROFILES][NB_BUTTONS];

const char* profileNames[NB_PROFILES] = {
    "Profile 1",
    "Profile 2", 
    "Profile 3",
    "Profile 4"
};

// ---- UUID UART BLE ----
#define UART_SERVICE_UUID  "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define UART_RX_UUID       "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define UART_TX_UUID       "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

// ---- USB HID ----
USBHIDKeyboard keyboard;

// ---- BLE ----
NimBLEServer*         pServer = nullptr;
NimBLECharacteristic* pTxChar = nullptr;
bool bleConnected = false;
Preferences prefs;

// ============================================================
// OLED
// ============================================================
void updateDisplay()
{
    display.clearDisplay();

    // Header — nom du profil
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.print(profileNames[currentProfile]);

    // Indicateurs profils en haut à droite
    for (int i = 0; i < NB_PROFILES; i++) {
        int x = 128 - (NB_PROFILES - i) * 14;
        if (i == currentProfile) {
            display.fillRect(x, 0, 12, 10, SSD1306_WHITE);
            display.setTextColor(SSD1306_BLACK);
        } else {
            display.drawRect(x, 0, 12, 10, SSD1306_WHITE);
            display.setTextColor(SSD1306_WHITE);
        }
        display.setCursor(x + 3, 1);
        display.print(i + 1);
        display.setTextColor(SSD1306_WHITE);
    }

    // Séparateur
    display.drawLine(0, 12, 128, 12, SSD1306_WHITE);

    // Grille 3x3 des actions
    for (int i = 0; i < NB_BUTTONS; i++) {
        int col = i % 3;
        int row = i / 3;
        int x = col * 42 + 2;
        int y = 15 + row * 16;

        display.setCursor(x, y);
        display.setTextSize(1);

        String label = actions[currentProfile][i];
        if (label.length() == 0)
            label = "---";
        else if (label.length() > 6)
            label = label.substring(0, 6);

        display.print(label);
    }

    display.display();
}

// ============================================================
// USB HID
// ============================================================
void sendKey(uint8_t modifier, uint8_t keycode)
{
    if (modifier) keyboard.press(modifier);
    if (keycode)  keyboard.press(keycode);
    delay(10);
    keyboard.releaseAll();
    delay(10);
}

void executeAction(String action)
{
    action.toUpperCase();
    action.trim();

    uint8_t modifier = 0;
    uint8_t keycode  = 0;

    int lastPlus = action.lastIndexOf('+');
    String mods  = (lastPlus >= 0) ? action.substring(0, lastPlus) : "";
    String key   = (lastPlus >= 0) ? action.substring(lastPlus + 1) : action;

    if (mods.indexOf("CTRL")  >= 0) modifier = KEY_LEFT_CTRL;
    if (mods.indexOf("SHIFT") >= 0) modifier = KEY_LEFT_SHIFT;
    if (mods.indexOf("ALT")   >= 0) modifier = KEY_LEFT_ALT;
    if (mods.indexOf("GUI")   >= 0) modifier = KEY_LEFT_GUI;
    if (mods.indexOf("WIN")   >= 0) modifier = KEY_LEFT_GUI;

    if      (key.length() == 1 && key[0] >= 'A' && key[0] <= 'Z') keycode = key[0];
    else if (key == "F1")    keycode = KEY_F1;
    else if (key == "F2")    keycode = KEY_F2;
    else if (key == "F3")    keycode = KEY_F3;
    else if (key == "F4")    keycode = KEY_F4;
    else if (key == "F5")    keycode = KEY_F5;
    else if (key == "F6")    keycode = KEY_F6;
    else if (key == "F7")    keycode = KEY_F7;
    else if (key == "F8")    keycode = KEY_F8;
    else if (key == "F9")    keycode = KEY_F9;
    else if (key == "F10")   keycode = KEY_F10;
    else if (key == "F11")   keycode = KEY_F11;
    else if (key == "F12")   keycode = KEY_F12;
    else if (key == "ESC")   keycode = KEY_ESC;
    else if (key == "TAB")   keycode = KEY_TAB;
    else if (key == "DEL")   keycode = KEY_DELETE;
    else if (key == "SPACE") keycode = ' ';
    else if (key == "ENTER") keycode = KEY_RETURN;
    else if (key == "UP")    keycode = KEY_UP_ARROW;
    else if (key == "DOWN")  keycode = KEY_DOWN_ARROW;
    else if (key == "LEFT")  keycode = KEY_LEFT_ARROW;
    else if (key == "RIGHT") keycode = KEY_RIGHT_ARROW;

    if (keycode || modifier) sendKey(modifier, keycode);
}

// ============================================================
// Flash — clés "p{profil}b{bouton}"
// ============================================================
void loadConfig()
{
    prefs.begin("macropad", true);
    currentProfile = prefs.getInt("profile", 0);
    for (int p = 0; p < NB_PROFILES; p++)
        for (int b = 0; b < NB_BUTTONS; b++)
            actions[p][b] = prefs.getString(("p" + String(p) + "b" + String(b)).c_str(), "");
    prefs.end();
    Serial.println("Config loaded from flash");
}

void saveConfig()
{
    prefs.begin("macropad", false);
    prefs.putInt("profile", currentProfile);
    for (int p = 0; p < NB_PROFILES; p++)
        for (int b = 0; b < NB_BUTTONS; b++)
            prefs.putString(("p" + String(p) + "b" + String(b)).c_str(), actions[p][b]);
    prefs.end();
    Serial.println("Config saved to flash");
}

// ============================================================
// BLE
// ============================================================
void sendReply(String json)
{
    if (pTxChar && bleConnected) {
        pTxChar->setValue(json.c_str());
        pTxChar->notify();
    }
    Serial.println("BLE TX: " + json);
}

void handleCommand(String payload)
{
    Serial.println("BLE RX: " + payload);
    StaticJsonDocument<1024> doc;
    if (deserializeJson(doc, payload)) {
        sendReply("{\"ok\":false,\"error\":\"json invalid\"}");
        return;
    }

    String cmd = doc["cmd"].as<String>();

    if (cmd == "set") {
        int p   = doc.containsKey("profile") ? (doc["profile"].as<int>() - 1) : currentProfile;
        int btn = doc["btn"].as<int>() - 1;
        if (p < 0 || p >= NB_PROFILES || btn < 0 || btn >= NB_BUTTONS) {
            sendReply("{\"ok\":false,\"error\":\"invalid params\"}");
            return;
        }
        actions[p][btn] = doc["action"].as<String>();
        if (p == currentProfile) updateDisplay();
        sendReply("{\"ok\":true,\"profile\":" + String(p+1) + ",\"btn\":" + String(btn+1) + "}");
    }

    else if (cmd == "get") {
        int p   = doc.containsKey("profile") ? (doc["profile"].as<int>() - 1) : currentProfile;
        int btn = doc["btn"].as<int>() - 1;
        if (p < 0 || p >= NB_PROFILES || btn < 0 || btn >= NB_BUTTONS) {
            sendReply("{\"ok\":false,\"error\":\"invalid params\"}");
            return;
        }
        sendReply("{\"ok\":true,\"profile\":" + String(p+1) + ",\"btn\":" + String(btn+1) + ",\"action\":\"" + actions[p][btn] + "\"}");
    }

    else if (cmd == "getall") {
        String reply = "{\"ok\":true,\"currentProfile\":" + String(currentProfile+1) + ",\"profiles\":[";
        for (int p = 0; p < NB_PROFILES; p++) {
            reply += "{\"profile\":" + String(p+1) + ",\"buttons\":[";
            for (int b = 0; b < NB_BUTTONS; b++) {
                reply += "{\"btn\":" + String(b+1) + ",\"action\":\"" + actions[p][b] + "\"}";
                if (b < NB_BUTTONS - 1) reply += ",";
            }
            reply += "]}";
            if (p < NB_PROFILES - 1) reply += ",";
        }
        reply += "]}";
        sendReply(reply);
    }

    else if (cmd == "switchprofile") {
        int p = doc["profile"].as<int>() - 1;
        if (p < 0 || p >= NB_PROFILES) {
            sendReply("{\"ok\":false,\"error\":\"invalid profile\"}");
            return;
        }
        currentProfile = p;
        updateDisplay();
        sendReply("{\"ok\":true,\"currentProfile\":" + String(currentProfile+1) + "}");
    }

    else if (cmd == "save") {
        saveConfig();
        sendReply("{\"ok\":true,\"saved\":true}");
    }

    else {
        sendReply("{\"ok\":false,\"error\":\"unknown cmd\"}");
    }
}

class ServerCallBacks : public NimBLEServerCallbacks {
    void onConnect(NimBLEServer *pSvr, NimBLEConnInfo &connInfo) override {
        bleConnected = true;
        Serial.println("BLE Client connected");
    }
    void onDisconnect(NimBLEServer *pSvr, NimBLEConnInfo &connInfo) {
        bleConnected = false;
        Serial.println("BLE Client disconnected - restarting advertising");
        NimBLEDevice::startAdvertising();
    }
};

class RxCallBacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic *pChar, NimBLEConnInfo &connInfo) override {
        String val = pChar->getValue().c_str();
        if (val.length() > 0) handleCommand(val);
    }
};

// ============================================================
// Encodeur rotatif
// ============================================================
// void handleEncoder()
// {
//     int clk = digitalRead(ENC_CLK);
//     if (clk != lastEncCLK && millis() - lastEncDebounce > 50) {
//         lastEncDebounce = millis();
//         if (digitalRead(ENC_DT) != clk)
//             currentProfile = (currentProfile + NB_PROFILES - 1) % NB_PROFILES; // anti-horaire
//         else
//             currentProfile = (currentProfile + 1) % NB_PROFILES;        // horaire
//         updateDisplay();
//         Serial.println("Profile -> " + String(currentProfile + 1));
//     }
//     lastEncCLK = clk;
// }

// ---- Variables encodeur (volatile car modifiées en ISR) ----
volatile int encDelta = 0;
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

void IRAM_ATTR encoderISR() {
    portENTER_CRITICAL_ISR(&mux);
    int clk = digitalRead(ENC_CLK);
    int dt  = digitalRead(ENC_DT);
    if (clk == HIGH) {  // front montant
        encDelta += (dt == LOW) ? 1 : -1;
    }
    portEXIT_CRITICAL_ISR(&mux);
}

void handleEncoder() {
    int delta = 0;
    portENTER_CRITICAL(&mux);
    delta    = encDelta;
    encDelta = 0;
    portEXIT_CRITICAL(&mux);

    if (delta > 0)
        currentProfile = (currentProfile + 1) % NB_PROFILES;
    else if (delta < 0)
        currentProfile = (currentProfile + NB_PROFILES - 1) % NB_PROFILES;
    
    if (delta != 0) {
        updateDisplay();
        Serial.println("Profile -> " + String(currentProfile + 1));
    }
}

void handleEncoderSwitch()
{
    bool sw = digitalRead(ENC_SW);
    if (lastSwState == HIGH && sw == LOW && millis() - lastSwDebounce > 50) {
        lastSwDebounce = millis();
        // clic encodeur : sauvegarder le profil courant en flash
        saveConfig();
        // feedback visuel bref
        display.invertDisplay(true);
        delay(80);
        display.invertDisplay(false);
        Serial.println("Profile saved via encoder click");
    }
    lastSwState = sw;
}

// ============================================================
// Setup
// ============================================================
void setup()
{
    Serial.begin(115200);
    loadConfig();

    // Switches
    for (int i = 0; i < NB_BUTTONS; i++) {
        pinMode(PINS[i], INPUT_PULLUP);
        lastState[i] = HIGH;
    }

    // Encodeur
    pinMode(ENC_CLK, INPUT_PULLUP);
    pinMode(ENC_DT,  INPUT_PULLUP);
    pinMode(ENC_SW,  INPUT_PULLUP);
    // lastEncCLK = digitalRead(ENC_CLK);
    attachInterrupt(digitalPinToInterrupt(ENC_CLK), encoderISR, CHANGE);

    // OLED
    Wire.begin(OLED_SDA, OLED_SCL);
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println("OLED init failed");
    } else {
        updateDisplay();
    }

    // USB HID
    keyboard.begin();
    USB.begin();

    // BLE UART
    NimBLEDevice::init("MacroPad");
    NimBLEDevice::setSecurityAuth(false, false, true);
    pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(new ServerCallBacks());

    NimBLEService *pUart = pServer->createService(UART_SERVICE_UUID);
    pTxChar = pUart->createCharacteristic(UART_TX_UUID, NIMBLE_PROPERTY::NOTIFY);
    NimBLECharacteristic *pRxChar = pUart->createCharacteristic(
        UART_RX_UUID,
        NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR
    );
    pRxChar->setCallbacks(new RxCallBacks());
    pUart->start();

    NimBLEAdvertising *adv = NimBLEDevice::getAdvertising();
    adv->addServiceUUID(UART_SERVICE_UUID);
    adv->setName("MacroPad");
    adv->start();

    Serial.println("MacroPad ready — USB HID + BLE UART + OLED");
}

// ============================================================
// Loop
// ============================================================
void loop()
{
    handleEncoder();
    handleEncoderSwitch();

    for (int i = 0; i < NB_BUTTONS; i++) {
        bool currentState = digitalRead(PINS[i]);
        if (lastState[i] == HIGH && currentState == LOW) {
            Serial.println("Button " + String(i+1) + " P" + String(currentProfile+1) + " -> " + actions[currentProfile][i]);
            if (actions[currentProfile][i].length() > 0)
                executeAction(actions[currentProfile][i]);
        }
        lastState[i] = currentState;
    }
    delay(20);
}