//*******************************************************
// mLRS Wireless Bridge for ESP32
// Copyright (c) www.olliw.eu, OlliW, OlliW42
// License: GPL v3
// https://www.gnu.org/licenses/gpl-3.0.de.html
//*******************************************************
// Basic but effective & reliable transparent WiFi and Bluetooth<->serial bridge
//*******************************************************
// 23. Aug. 2023
//*********************************************************/
// inspired by examples from Arduino
// ArduinoIDE 2.0.3, esp32 by Espressif Systems 2.0.6
// use upload speed 115200 if serial passthrough shall be used for flashing
// this can be useful: https://github.com/espressif/arduino-esp32/blob/master/libraries
/*
Definitions:
- "module" refers to the physical hardware
- "board" refers to the board you need to select in the menu Tools->Board

For more details on the modules see mlrs-wifi-bridge-boards.h

List of supported modules, and board which need to be selected

- Espressif ESP32-DevKitC V4      board: ESP32 Dev Module
- NodeMCU ESP32-Wroom-32          board: ESP32 Dev Module
- Espressif ESP32-PICO-KIT        board: ESP32 PICO-D4
- Adafruit QT Py S2               board: Adafruit QT Py ESP32-S2
- Lilygo TTGO-MICRO32             board: ESP32 PICO-D4
- M5Stack M5Stamp C3 Mate         board: ESP32C3 Dev Module
  ATTENTION: when the 5V pin is used, one MUST not also use the USB port, since they are connected internally!!
- M5Stack M5Stamp Pico            board: ESP32 PICO-D4
- M5Stack M5Stamp C3U Mate        board: ESP32C3 Dev Module
  ATTENTION: when the 5V pin is used, one MUST not also use the USB port, since they are connected internally!!
- M5Stack ATOM Lite               board: M5Stack-ATOM
- Ai-Thinker ESP-01S              board: Generic ESP8266 Module. Note WiFi only, no Bluetooth
*/

//-------------------------------------------------------
// User configuration
//-------------------------------------------------------

// Module
// uncomment what you want
//#define MODULE_GENERIC
//#define MODULE_ESP32_DEVKITC_V4
//#define MODULE_NODEMCU_ESP32_WROOM32
//#define MODULE_ESP32_PICO_KIT
//#define MODULE_ADAFRUIT_QT_PY_ESP32_S2
//#define MODULE_TTGO_MICRO32
//#define MODULE_M5STAMP_C3_MATE
//#define MODULE_M5STAMP_C3U_MATE
//#define MODULE_M5STAMP_C3U_MATE_FOR_FRSKY_R9M // uses inverted serial
//#define MODULE_M5STAMP_PICO
//#define MODULE_M5STAMP_PICO_FOR_FRSKY_R9M // uses inverted serial
//#define MODULE_M5STACK_ATOM_LITE
//#define MODULE_ESP01S

// Serial level
// uncomment, if you need inverted serial for a supported module
// (for the generic module also SERIAL_RXD and SERIAL_TXD need to be defined)
//#define USE_SERIAL_INVERTED

// Wireless protocol
// 0 = WiFi TCP, 1 = WiFi UDP, 2 = Wifi UDPCl, 3 = Bluetooth (not available for all boards)
#define WIRELESS_PROTOCOL  1

#if defined(MODULE_ESP01S)
  #include <ESP8266WiFi.h>
#endif

//**********************//
//*** WiFi settings ***//

// for TCP, UDP
String ssid = "mLRS AP"; // Wifi name
String password = ""; // "thisisgreat"; // WiFi password, "" makes it an open AP

IPAddress ip(192, 168, 4, 55); // connect to this IP // MissionPlanner default is 127.0.0.1, so enter

int port_tcp = 5760; // connect to this port per TCP // MissionPlanner default is 5760
int port_udp = 14550; // connect to this port per UDP // MissionPlanner default is 14550

// for UDPCl
String network_ssid = "****"; // name of your WiFi network
String network_password = "****"; // password to access your WiFi network

IPAddress ip_udpcl(192, 168, 0, 164); // connect to this IP // MissionPlanner default is 192.168.0.164

int port_udpcl = 14550; // connect to this port per UDPCL // MissionPlanner default is 14550

// WiFi channel
// 1 is the default, 13 (2461-2483 MHz) has the least overlap with mLRS 2.4 GHz frequencies.
// Note: Channel 13 is generally not available in the US, where 11 is the maximum.
int wifi_channel = 6;

// WiFi power
// comment out for default setting
// Note: In order to find the possible options, right click on WIFI_POWER_19_5dBm and choose "Go To Definiton"
#define WIFI_POWER_ESP32  WIFI_POWER_2dBm // WIFI_POWER_MINUS_1dBm is the lowest possible, WIFI_POWER_19_5dBm is the max

// comment out for default setting
#define WIFI_POWER_ESP8266 0 // 0 (min) to 20.5 (max)

//**************************//
//*** Bluetooth settings ***//

String bluetooth_device_name = "mLRS BT"; // Bluetooth device name


//************************//
//*** General settings ***//

// baudrate
int baudrate = 115200;

// serial port usage (only effective for the generic module)
// comment all for default behavior, which is using only Serial port
//#define USE_SERIAL_DBG1 // use Serial for communication and flashing, and Serial1 for debug output
//#define USE_SERIAL1_DBG // use Serial1 for communication, and Serial for debug output and flashing
//#define USE_SERIAL2_DBG // use Serial2 for communication, and Serial for debug output and flashing

// LED pin (only effective for the generic module)
// uncomment if you want a LED
//#define LED_IO  13


//-------------------------------------------------------
// Includes
//-------------------------------------------------------

#if (WIRELESS_PROTOCOL <= 2) // WiFi
  #if defined(MODULE_ESP01S)
    #if WIRELESS_PROTOCOL > 0 // UDP
      #include <WiFiUdp.h>
    #endif
  #else
    #include <WiFi.h>
  #endif
#elif (WIRELESS_PROTOCOL == 3) // Bluetooth
  #if !defined(CONFIG_BT_ENABLED)
    #error Bluetooth is not enabled !
  #endif
  #include <BluetoothSerial.h>
#else
  #error Invalid WIRELESS_PROTOCOL chosen !
#endif


//-------------------------------------------------------
// Module details
//-------------------------------------------------------

#include "mlrs-wireless-bridge-boards.h"


//-------------------------------------------------------
// Internals
//-------------------------------------------------------

#if (WIRELESS_PROTOCOL <= 1) // WiFi TCP, UDP

IPAddress ip_udp(ip[0], ip[1], ip[2], ip[3]+1); // speculation: it seems that MissionPlanner wants it +1
IPAddress netmask(255, 255, 255, 0);
  #if WIRELESS_PROTOCOL == 1 // UDP
    WiFiUDP udp;
  #else // TCP
    WiFiServer server(port_tcp);
    WiFiClient client;
  #endif

#elif (WIRELESS_PROTOCOL == 2) // WiFi UDPCl

IPAddress ip_gateway(ip_udpcl[0], ip_udpcl[1], ip_udpcl[2], 1); // x.x.x.1 is usually the router IP
IPAddress netmask(255, 255, 255, 0);
WiFiUDP udp;

#elif (WIRELESS_PROTOCOL == 3) || !defined(MODULE_ESP01S) // Bluetooth

BluetoothSerial SerialBT;

#endif

bool led_state;
unsigned long led_tlast_ms;
bool is_connected;
unsigned long is_connected_tlast_ms;
unsigned long serial_data_received_tfirst_ms;


void serialFlushRx(void)
{
    while (SERIALx.available() > 0) { uint8_t c = SERIALx.read(); }
}


//-------------------------------------------------------
// setup() and loop()
//-------------------------------------------------------

void setup()
{
    led_init();
    dbg_init();
    delay(500);

    size_t rxbufsize = SERIALx.setRxBufferSize(2*1024); // must come before uart started, retuns 0 if it fails
#if !defined(MODULE_ESP01S)
    size_t txbufsize = SERIALx.setTxBufferSize(512); // must come before uart started, retuns 0 if it fails
#endif    
#ifdef SERIAL_RXD // if SERIAL_TXD is not defined the compiler will complain, so all good
  #ifdef USE_SERIAL_INVERTED
    SERIALx.begin(baudrate, SERIAL_8N1, SERIAL_RXD, SERIAL_TXD, true);
  #else
    SERIALx.begin(baudrate, SERIAL_8N1, SERIAL_RXD, SERIAL_TXD);
  #endif
#else
    SERIALx.begin(baudrate);
#endif
//????used to work    pinMode(U1_RXD, INPUT_PULLUP); // important, at least in older versions Arduino serial lib did not do it

    DBG_PRINTLN(rxbufsize);
    DBG_PRINTLN(txbufsize);

#if (WIRELESS_PROTOCOL <= 1)
//-- WiFi TCP, UDP

    // AP mode
    WiFi.mode(WIFI_AP); // seems not to be needed, done by WiFi.softAP()?
    WiFi.softAPConfig(ip, ip, netmask);
  #if (WIRELESS_PROTOCOL == 1)
    String ssid_full = ssid + " UDP";
  #else
    String ssid_full = ssid + " TCP";
  #endif
    WiFi.softAP(ssid_full.c_str(), (password.length()) ? password.c_str() : NULL, wifi_channel); // channel = 1 is default
    DBG_PRINT("ap ip address: ");
    DBG_PRINTLN(WiFi.softAPIP()); // comes out as 192.168.4.1
    DBG_PRINT("channel: ");
    DBG_PRINTLN(WiFi.channel());

  #if defined(MODULE_ESP01S)
    #ifdef WIFI_POWER_ESP8266
      WiFi.setOutputPower(WIFI_POWER_ESP8266); // set WiFi power, AP or STA must have been started, returns false if it fails
    #endif
  #else // ESP32
    #ifdef WIFI_POWER_ESP32
      WiFi.setTxPower(WIFI_POWER_ESP32); // set WiFi power, AP or STA must have been started, returns false if it fails
    #endif
  #endif

  #if (WIRELESS_PROTOCOL == 1)
    udp.begin(port_udp);
  #else
    server.begin();
    server.setNoDelay(true);
  #endif    

#elif (WIRELESS_PROTOCOL == 2)
//-- Wifi UDPCl

    // STA mode
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    WiFi.config(ip_udpcl, ip_gateway, netmask);
    WiFi.begin(network_ssid.c_str(), network_password.c_str());

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        DBG_PRINTLN("connecting to WiFi network...");
    }
    DBG_PRINTLN("connected");
    DBG_PRINT("network ip address: ");
    DBG_PRINTLN(WiFi.localIP());

  #if defined(MODULE_ESP01S)
    #ifdef WIFI_POWER_ESP8266
      WiFi.setOutputPower(WIFI_POWER_ESP8266); // set WiFi power, AP or STA must have been started, returns false if it fails
    #endif
  #else // ESP32
    #ifdef WIFI_POWER_ESP32
      WiFi.setTxPower(WIFI_POWER_ESP32); // set WiFi power, AP or STA must have been started, returns false if it fails
    #endif
  #endif
    udp.begin(port_udpcl);

#elif (WIRELESS_PROTOCOL == 3)
//-- Bluetooth

    SerialBT.begin(bluetooth_device_name);

#endif

    led_tlast_ms = 0;
    led_state = false;

    is_connected = false;
    is_connected_tlast_ms = 0;

    serial_data_received_tfirst_ms = 0;

    serialFlushRx();
}


void loop()
{
    unsigned long tnow_ms = millis();

    if (is_connected && (tnow_ms - is_connected_tlast_ms > 2000)) { // nothing from GCS for 2 secs
        is_connected = false;
    }

    if (tnow_ms - led_tlast_ms > (is_connected ? 500 : 200)) {
        led_tlast_ms = tnow_ms;
        led_state = !led_state;
        if (led_state) led_on(is_connected); else led_off();
    }

    //-- here comes the core code, handle WiFi or Bluetooth connection and do the bridge

    uint8_t buf[256]; // working buffer

#if (WIRELESS_PROTOCOL == 0)
//-- WiFi TCP

    if (server.hasClient()) {
        if (!client.connected()) {
            client.stop(); // doesn't appear to make a difference
            client = server.available();
            DBG_PRINTLN("connection");
        } else { // is already connected, so reject, doesn't seem to ever happen
            server.available().stop();
            DBG_PRINTLN("connection rejected");
        }
    }

    if (!client.connected()) { // nothing to do
        client.stop();
        serialFlushRx();
        is_connected = false;
        return;
    }

    while (client.available()) {
        //uint8_t c = (uint8_t)client.read();
        //SERIALx.write(c);
        int len = client.read(buf, sizeof(buf));
        SERIALx.write(buf, len);
        is_connected = true;
        is_connected_tlast_ms = millis();
    }

    tnow_ms = millis(); // update it
    int avail = SERIALx.available();
    if (avail <= 0) {
        serial_data_received_tfirst_ms = tnow_ms;
    } else
    if ((tnow_ms - serial_data_received_tfirst_ms) > 10 || avail > 128) { // 10 ms at 57600 bps corresponds to 57 bytes, no chance for 128 bytes
        serial_data_received_tfirst_ms = tnow_ms;

        int len = SERIALx.read(buf, sizeof(buf));
        client.write(buf, len);
    }

#elif (WIRELESS_PROTOCOL == 1)
//-- WiFi UDP

    int packetSize = udp.parsePacket();
    if (packetSize) {
        int len = udp.read(buf, sizeof(buf));
        SERIALx.write(buf, len);
        is_connected = true;
        is_connected_tlast_ms = millis();
    }

    tnow_ms = millis(); // may not be relevant, but just update it
    int avail = SERIALx.available();
    if (avail <= 0) {
        serial_data_received_tfirst_ms = tnow_ms;
    } else
    if ((tnow_ms - serial_data_received_tfirst_ms) > 10 || avail > 128) { // 10 ms at 57600 bps corresponds to 57 bytes, no chance for 128 bytes
        serial_data_received_tfirst_ms = tnow_ms;

        int len = SERIALx.read(buf, sizeof(buf));
        udp.beginPacket(ip_udp, port_udp);
        udp.write(buf, len);
        udp.endPacket();
    }

#elif (WIRELESS_PROTOCOL == 2)
//-- WiFi UDPCl (STA mode)

    int packetSize = udp.parsePacket();
    if (packetSize) {
        int len = udp.read(buf, sizeof(buf));
        SERIALx.write(buf, len);
        is_connected = true;
        is_connected_tlast_ms = millis();
    }

    if (!is_connected) {
        // remote's ip and port not known, so jump out
        serialFlushRx();
        return;
    }

    tnow_ms = millis(); // may not be relevant, but just update it
    int avail = SERIALx.available();
    if (avail <= 0) {
        serial_data_received_tfirst_ms = tnow_ms;
    } else
    if ((tnow_ms - serial_data_received_tfirst_ms) > 10 || avail > 128) { // 10 ms at 57600 bps corresponds to 57 bytes, no chance for 128 bytes
        serial_data_received_tfirst_ms = tnow_ms;

        int len = SERIALx.read(buf, sizeof(buf));
        udp.beginPacket(udp.remoteIP(), udp.remotePort());
        udp.write(buf, len);
        udp.endPacket();
    }

#elif (WIRELESS_PROTOCOL == 3)
//-- Bluetooth

    int len = SerialBT.available();
    if (len > 0) {
        if (len > sizeof(buf)) len = sizeof(buf);
        for (int i = 0; i < len; i++) buf[i] = SerialBT.read();
        SERIALx.write(buf, len);
        is_connected = true;
        is_connected_tlast_ms = millis();
    }

    tnow_ms = millis(); // may not be relevant, but just update it
    int avail = SERIALx.available();
    if (avail <= 0) {
        serial_data_received_tfirst_ms = tnow_ms;
    } else
    if ((tnow_ms - serial_data_received_tfirst_ms) > 10 || avail > 128) { // 10 ms at 57600 bps corresponds to 57 bytes, no chance for 128 bytes
        serial_data_received_tfirst_ms = tnow_ms;

        int len = SERIALx.read(buf, sizeof(buf));
        SerialBT.write(buf, len);
    }

#endif // (WIRELESS_PROTOCOL == 3)
}

