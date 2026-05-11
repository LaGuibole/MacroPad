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
#define UART_RX_UUID        "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define UART_TX_UUID        "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"

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