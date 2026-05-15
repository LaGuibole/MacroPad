#include "USB.h"
#include "USBHIDKeyboard.h"
#include <NimBLEDevice.h>
#include <Preferences.h>
#include <ArduinoJson.h>

// pins
const int NB_BUTTONS = 9;
const int PINS[NB_BUTTONS] = {1, 2, 3, 4, 5, 6, 7, 8, 9};

bool lastState[NB_BUTTONS];
String actions[NB_BUTTONS];

// UUID UART BLE
#define UART_SERVICE_UUID   "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define UART_RX_UUID        "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define UART_TX_UUID        "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

// HIDUSB
USBHIDKeyboard keyboard;

// BLE
NimBLEServer            *pServer = nullptr;
NimBLECharacteristic    *pTxChar = nullptr;
bool bleConnected = false;
Preferences prefs;

// usb hid - send a key
void sendKey(uint8_t modifier, uint8_t keycode)
{
    if (modifier)
        keyboard.press(modifier);
    if (keycode)
        keyboard.press(keycode);
    delay(10);
    keyboard.releaseAll();
    delay(10);
}

// parse "ctrl + z" etc .. 
void executeAction(String action)
{
    action.toUpperCase();
    action.trim();

    uint8_t modifier = 0;
    uint8_t keycode = 0;

    int lastPlus = action.lastIndexOf('+');
    String mods = (lastPlus >= 0) ? action.substring(0, lastPlus) : "";
    String key = (lastPlus >= 0) ? action.substring(lastPlus + 1) : action;

    if (mods.indexOf("CTRL") >= 0) modifier = KEY_LEFT_CTRL;
    if (mods.indexOf("SHIFT") >= 0) modifier = KEY_LEFT_SHIFT;
    if (mods.indexOf("ALT") >= 0) modifier = KEY_LEFT_ALT;
    if (mods.indexOf("GUI") >= 0) modifier = KEY_LEFT_GUI;
    if (mods.indexOf("GUI") >= 0) modifier = KEY_LEFT_GUI;

    if (key.length() == 1 && key[0] >= 'A' && key[0] <= 'Z') keycode = key[0];
    else if (key == "F1")           keycode = KEY_F1;
    else if (key == "F2")           keycode = KEY_F2;
    else if (key == "F3")           keycode = KEY_F3;
    else if (key == "F4")           keycode = KEY_F4;
    else if (key == "F5")           keycode = KEY_F5;
    else if (key == "F6")           keycode = KEY_F6;
    else if (key == "F7")           keycode = KEY_F7;
    else if (key == "F8")           keycode = KEY_F8;
    else if (key == "F9")           keycode = KEY_F9;
    else if (key == "F10")          keycode = KEY_F10;
    else if (key == "F11")          keycode = KEY_F11;
    else if (key == "F12")          keycode = KEY_F12;
    else if (key == "ESC")          keycode = KEY_ESC;
    else if (key == "DEL")          keycode = KEY_DELETE;
    else if (key == "SPACE")        keycode = ' ';
    else if (key == "ENTER")        keycode = KEY_RETURN;
    else if (key == "UP")           keycode = KEY_UP_ARROW;
    else if (key == "DOWN")         keycode = KEY_DOWN_ARROW;
    else if (key == "LEFT")         keycode = KEY_LEFT_ARROW;
    else if (key == "RIGHT")        keycode = KEY_RIGHT_ARROW;

    if (keycode || modifier) sendKey(modifier, keycode);
}

// flasging
void loadConfig(void) 
{
    prefs.begin("macropad", true);
    for (int i = 0; i < NB_BUTTONS; i++)
        actions[i] = prefs.getString(("btn" + String(i)).c_str(), "");
    prefs.end();
    Serial.println("Config loaded from flash");
}

void saveConfig(void)
{
    prefs.begin("macropad", true);
    for (int i = 0; i < NB_BUTTONS; i++)
        prefs.putString(("btn" + String(i)).c_str(), actions[i]);
    prefs.end();
    Serial.println("Config saved in flash memory");
}

// ble - sending replies
void sendReply(String json)
{
    if (pTxChar && bleConnected) 
    {
        pTxChar->setValue(json.c_str());
        pTxChar->notify();
    }
    Serial.println("BLE TX: " + json);
}

// ble - handlecmd
void handleCommand(String payload)
{
    Serial.println("BLE RX: " + payload);
    StaticJsonDocument<512> doc;
    if (deserializeJson(doc, payload)) 
    {
        sendReply("{\"ok\":false,\"error\":\"json invalid\"}");
        return ;
    }
    
    String cmd = doc["cmd"].as<String>();

    if (cmd == "set")
    {
        int btn = doc["btn"].as<int>() - 1;
        if (btn < 0 || btn >= NB_BUTTONS)
        {
            sendReply("{\"ok\":false,\"error\":\"invalid btn\"}");
            return ; 
        }
        actions[btn] = doc["actions"].as<String>();
        sendReply("{\"ok\":true,\"btn\":" + String(btn+1) + "}");
    }
    else if (cmd == "get")
    {
        int btn = doc["btn"].as<int>() - 1;
        if (btn < 0 || btn >= NB_BUTTONS)
        {
            sendReply("{\"ok\":false,\"error\":\"invalid btn\"}");
            return ;
        }
        sendReply("{\"ok\":true,\"btn\":" + String(btn+1) + ",\"action\":\"" + actions[btn] + "\"}");
    }
    else if (cmd == "getall") 
    {
        String reply = "{\"ok\":true,\"buttons\":[";
        for (int i = 0; i < NB_BUTTONS; i++)
        {
            reply += "{\"btn\":" + String(i+1) + ",\"action\":\"" + actions[i] + "\"}";
            if (i < NB_BUTTONS - 1)
                reply += ",";
        }
        reply += "]}";
        sendReply(reply);
    }
    else if (cmd == "save")
    {
        saveConfig();
        sendReply("{\"ok\":true,\"saved\":true}");
    }
    else 
        sendReply("{\"ok\":false,\"error\":\"unknown cmd\"}");
}

// ble callbacks
class ServerCallBacks : public NimBLEServerCallbacks 
{
    void onConnect(NimBLEServer *pSvr) override {
        bleConnected = true;
        Serial.println("BLE Client connected");
    }
    void onDisconnect(NimBLEServer *pSvr) override {
        bleConnected = false;
        Serial.println("BLE Client disconnected - restarting advertising");
        NimBLEDevice::startAdvertising();
    }
};

class RxCallBacks : public NimBLECharacteristicCallbacks 
{
    void onWrite(NimBLECharacteristic *pChar) override {
        String val = pChar->getValue().c_str();
        if (val.length() > 0)
            handleCommand(val);
    }
};

// setup / config 
void setup(void)
{
    Serial.begin(115200);
    loadConfig();

    for (int i = 0; i < NB_BUTTONS; i++)
    {
        pinMode(PINS[i], INPUT_PULLUP);
        lastState[i] = HIGH;
    }

    // usb hid
    keyboard.begin();
    USB.begin();

    // ble uart
    NimBLEDevice::init("MacroPad");
    NimBLEDevice::setSecurityAuth(false, false, true);
    pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks());

    NimBLEService *pUart->createCharacteristic(UART_TX_UUID, NIMBLE_PROPERTY::NOTIFY);
    NimBLECharacteristic *pRxChar = pUart->createCharacteristic(UART_RX_UUID, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR);
    pRxChar->setCallbacks(new RxCallbacks());
    pUart->start();

    NimBLEAdvertising *adv = NimBLEDevice::getAdvertising();
    adv->addServiceUUID(UART_SERVICE_UUID);
    adv->start();

    Serial.println("MacroPad ready, USB HID + BLE UART services has started");
}

// routine
void loop(void)
{
    for (int i = 0; i < NB_BUTTONS; i++)
    {
        bool currentState = digitalRead(PINS[i]);
        if (lastState[i] == HIGH && currentState == LOW)
        {
            Serial.println("Button " + String(i + 1) + " -> " + actions[i]);
            if (actions[i].length() > 0)
                executeAction(actions[i]);
        }
        lastState[i] = currentState;
    }
    delay(20);
}