// MACROPAD => NimBLE based
// HID MacroPad via BT + UART config service
#include <NimBLEDevice.h>
#include <NimBLEServer.h>
#include <NimBLEUtils.h>
#include <NimBLEHIDDevice.h>
#include <Preferences.h>
#include <ArduinoJson.h>

// Pins
const int NB_BUTTONS = 9;
const int PINS[NB_BUTTONS] = {13, 21, 4, 27, 25, 5, 32, 22, 18};
bool lastState[NB_BUTTONS];
String actions[NB_BUTTONS]; // ig: ctrl z ctrls s etc

// UUIds UART
#define UART_SERVICE_UUID   "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define UART_RX_UUID        "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define UART_TX_UUID        "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

// modifiers
#define MOD_CTRL    0x01
#define MOD_SHIFT   0x02
#define MOD_ALT     0x04
#define MOD_GUI     0x08    // windows 

// keycodes
#define KEY_A       0x04
#define KEY_B       0x05
#define KEY_C       0x06
#define KEY_D       0x07
#define KEY_E       0x08
#define KEY_F       0x09
#define KEY_G       0x0A
#define KEY_H       0x0B
#define KEY_I       0x0C
#define KEY_J       0x0D
#define KEY_K       0x0E
#define KEY_L       0x0F
#define KEY_M       0x10
#define KEY_N       0x11
#define KEY_O       0x12
#define KEY_P       0x13
#define KEY_Q       0x14
#define KEY_R       0x15
#define KEY_S       0x16
#define KEY_T       0x17
#define KEY_U       0x18
#define KEY_V       0x19
#define KEY_W       0x1A
#define KEY_X       0x1B
#define KEY_Y       0x1C
#define KEY_Z       0x1D
#define KEY_F1      0x3A
#define KEY_F2      0x3B
#define KEY_F3      0x3C
#define KEY_F4      0x3D
#define KEY_F5      0x3E
#define KEY_F6      0x3F
#define KEY_F7      0x40
#define KEY_F8      0x41
#define KEY_F9      0x42
#define KEY_F10     0x43
#define KEY_F11     0x44
#define KEY_F12     0x45
#define KEY_ESC     0x29
#define KEY_TAB     0x2B
#define KEY_DEL     0x4C
#define KEY_SPACE   0x2C

// globals for BLE
NimBLEHIDDevice         *hid;
NimBLECharacteristic    *inputReport;
NimBLECharacteristic    *pTxChar = nullptr;
NimBLEServer            *pServer = nullptr;
bool bleConnected = false;
Preferences prefs;

// HID Report descriptor 
static const uint8_t hidReportDescriptor[] = {
    0x05, 0x01, // usage page
    0x09, 0x06, // usage keyboard
    0xA1, 0x01, // collection
    0x85, 0x01, // report ID
    // modifiers
    0x05, 0x07, // Usage page
    0x19, 0xE0, // Usage minimum
    0x29, 0xE7, // useage max
    0x15, 0x00, // logical min
    0x25, 0x01, // logical max
    0x75, 0x01, // report size
    0x95, 0x08, // report count
    0x81, 0x02, // input type (data, variable)
    // reserved byte
    0x95, 0x01, // report count
    0x75, 0x08, // report size
    0x81, 0x03, // input (const)
    // keycodes
    0x95, 0x06, // report count
    0x75, 0x08, // report size
    0x15, 0x00, // logical min
    0x26, 0xFF, 0x00, // logical max
    0x05, 0x07, // usage page
    0x19, 0x00, // usage min
    0x29, 0xFF, // usage max
    0x81, 0x00, // input
    0xC0 // end collection
};

// send HID button
// report[0] = modifiers, report[1] = reserved, report[2, ..., 7] = keycodes
void sendKey(uint8_t modifier, uint8_t keycode)
{
    if (!bleConnected) 
        return;
    uint8_t report[8] = {0};
    report[0] = modifier;
    report[2] = keycode;
    inputReport->setValue(report, sizeof(report));
    inputReport->notify();
    delay(10);
    // release :
    memset(report, 0, sizeof(report));
    inputReport->setValue(report, sizeof(report));
    inputReport->notify();
    delay(10);
}

// parse and return modifiers + keycode
void executeAction(String action) 
{
    action.toUpperCase();
    action.trim();

    uint8_t modifier = 0;
    uint8_t keycode = 0;

    // build : <modifier> '+' <keycode> 
    int lastPlus = action.lastIndexOf('+');
    String modifiers = (lastPlus >= 0) ? action.substring(0, lastPlus) : "";
    String key       = (lastPlus >= 0) ? action.substring(lastPlus + 1) : action;

    // modifiers
    if (modifiers.indexOf("CTRL") >= 0) modifier |= MOD_CTRL;
    if (modifiers.indexOf("SHIFT") >= 0) modifier |= MOD_SHIFT;
    if (modifiers.indexOf("ALT") >= 0) modifier |= MOD_ALT;
    if (modifiers.indexOf("GUI") >= 0) modifier |= MOD_GUI;
    if (modifiers.indexOf("WIN") >= 0) modifier |= MOD_GUI;

    // keycodes
    if      (key == "A") keycode = KEY_A;
    else if (key == "B") keycode = KEY_B;
    else if (key == "C") keycode = KEY_C;
    else if (key == "D") keycode = KEY_D;
    else if (key == "E") keycode = KEY_E;
    else if (key == "F") keycode = KEY_F;
    else if (key == "G") keycode = KEY_G;
    else if (key == "H") keycode = KEY_H;
    else if (key == "I") keycode = KEY_I;
    else if (key == "J") keycode = KEY_J;
    else if (key == "K") keycode = KEY_K;
    else if (key == "L") keycode = KEY_L;
    else if (key == "M") keycode = KEY_M;
    else if (key == "N") keycode = KEY_N;
    else if (key == "O") keycode = KEY_O;
    else if (key == "P") keycode = KEY_P;
    else if (key == "Q") keycode = KEY_Q;
    else if (key == "R") keycode = KEY_R;
    else if (key == "S") keycode = KEY_S;
    else if (key == "T") keycode = KEY_T;
    else if (key == "U") keycode = KEY_U;
    else if (key == "V") keycode = KEY_V;
    else if (key == "W") keycode = KEY_W;
    else if (key == "X") keycode = KEY_X;
    else if (key == "Y") keycode = KEY_Y;
    else if (key == "Z") keycode = KEY_Z;
    else if (key == "F1")  keycode = KEY_F1;
    else if (key == "F2")  keycode = KEY_F2;
    else if (key == "F3")  keycode = KEY_F3;
    else if (key == "F4")  keycode = KEY_F4;
    else if (key == "F5")  keycode = KEY_F5;
    else if (key == "F6")  keycode = KEY_F6;
    else if (key == "F7")  keycode = KEY_F7;
    else if (key == "F8")  keycode = KEY_F8;
    else if (key == "F9")  keycode = KEY_F9;
    else if (key == "F10") keycode = KEY_F10;
    else if (key == "F11") keycode = KEY_F11;
    else if (key == "F12") keycode = KEY_F12;
    else if (key == "ESC") keycode = KEY_ESC;
    else if (key == "TAB") keycode = KEY_TAB;
    else if (key == "DEL") keycode = KEY_DEL;
    else if (key == "SPACE") keycode = KEY_SPACE;

    if (keycode > 0 || modifier > 0) {
        sendKey(modifier, keycode);
    }
}

// flash esp32
void loadConfig(void) 
{
    prefs.begin("macropad", true);
    for (int i = 0; i < NB_BUTTONS; i++)
    {
        actions[i] = prefs.getString(("btn" + String(i)).c_str(), "");
    }
    prefs.end();
    Serial.println("Config loaded from flash memory");
}

void saveConfig(void)
{
    prefs.begin("macropad", false);
    for (int i = 0; i < NB_BUTTONS; i++)
    {
        prefs.putString(("btn" + String(i)).c_str(), actions[i]);
    }
    prefs.end();
    Serial.println("Config saved in flash memory");
}

// UART BLE sendReply()
void sendReply(String json)
{
    if (pTxChar && bleConnected)
    {
        pTxChar->setValue(json.c_str());
        pTxChar->notify();
    }
    Serial.println("TX: " + json);
}

void handleCommand(String payload)
{
    Serial.println("RX: " + payload);
    StaticJsonDocument<512> doc;
    if (deserializeJson(doc, payload))
    {
        sendReply("{\"ok\":false,\"error\":\"invalid btn\"}");
        return;
    }

    String cmd = doc["cmd"].as<String>();

    if(cmd == "set")
    {
        int btn = doc["btn"].as<int>() - 1;
        if (btn < 0 || btn >= NB_BUTTONS)
        {
            sendReply("{\"ok\":false,\"error\":\"invalid btn\"}");
            return;
        }
        actions[btn] = doc["action"].as<String>();
        sendReply("{\"ok\":true,\"btn\":" + String(btn+1) + "}");
    }

    else if (cmd == "get")
    {
        int btn = doc["btn"].as<int>() - 1;
        if (btn < 0 || btn >= NB_BUTTONS) 
        {
            sendReply("{\"ok\":false,\"error\":\"invalid btn\"}");
            return;
        }
        sendReply("{\"ok\":true,\"btn\":" + String(btn+1) + ",\"action\":\"" + actions[btn] + "\"}");
    }

    else if (cmd == "getall")
    {
        String reply = "{\"ok\":true,\"buttons\":[";
        for (int i = 0; i < NB_BUTTONS; i++)
        {
            reply += "{\"btn\":" + String(i+1) + ",\"action\":\"" + actions[i] + "\"}";
            if (i < NB_BUTTONS - 1) reply += ",";
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

// BLE callbacks
class ServerCallbacks : public NimBLEServerCallbacks 
{
    void onConnect(NimBLEServer* pSvr) {
        bleConnected = true;
        Serial.println("Client connected");
    }
    void onDisconnect(NimBLEServer* pSvr) {
        bleConnected = false;
        Serial.println("Client disconnected - advertising");
        NimBLEDevice::startAdvertising(); 
    }
};

class RxCallbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* pChar) {
        String val = pChar->getValue().c_str();
        if (val.length() > 0) 
            handleCommand(val);
    }
};

// setup
void setup(void)
{
    Serial.begin(115200);
    loadConfig();

    for (int i = 0; i < NB_BUTTONS; i++)
    {
        pinMode(PINS[i], INPUT_PULLUP);
        lastState[i] = HIGH;
    }

    NimBLEDevice::init("MyMacroPad");
    NimBLEDevice::setSecurityAuth(false, false, true);
    pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks());

    // HID service
    hid = new NimBLEHIDDevice(pServer);
    inputReport = hid->getInputReport(1);
    hid->setManufacturer("LaGuibole");
    hid->setPnp(0x02, 0x045E, 0x0000, 0x0110);
    hid->setHidInfo(0x00, 0x01);
    hid->setReportMap((uint8_t*)hidReportDescriptor, sizeof(hidReportDescriptor));
    hid->setBatteryLevel(100);
    hid->startServices();

    // UART config service
    NimBLEService* pUart = pServer->createService(UART_SERVICE_UUID);
    pTxChar = pUart->createCharacteristic(UART_TX_UUID, NIMBLE_PROPERTY::NOTIFY);
    NimBLECharacteristic* pRxChar = pUart->createCharacteristic(
        UART_RX_UUID,
        NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR
    );
    pRxChar->setCallbacks(new RxCallbacks());
    pUart->start();

    // advertising
    NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();
    adv->setAppearance(HID_KEYBOARD);
    adv->addServiceUUID(hid->getHidService()->getUUID());
    adv->addServiceUUID(UART_SERVICE_UUID);
    adv->start();

    Serial.println("BT MacroPad is ready !");
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
            if (bleConnected && actions[i].length() > 0)
                executeAction(actions[i]);
        }
        lastState[i] = currentState;
    }
    delay(20);
}