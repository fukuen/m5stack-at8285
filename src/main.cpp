/**
 * Copyright (c) 2020 fukuen. \n\n
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version. \n\n
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
*/
#include <M5Stack.h>
#include <WiFi.h>

TFT_eSprite sprite = TFT_eSprite(&M5.Lcd);

WiFiClient client;

//HardwareSerial serial_ext(2); // Grove
HardwareSerial serial_ext(0); // USB

char msg[200];
char buf[4096];
int recv_length;
int mux_mode = 1;
int echo_mode = 1;
int ipd_mode = 0;
int linkid = 0;
int conn = 0;
//int delayms = 0;
int delayms = 100; // for MaixPy

void disp0(String cmd) {
  sprite.setScrollRect(0, 0, 310, 220, TFT_BLACK);
  sprite.scroll(0, -10);
  sprite.drawString(cmd, 0, 210);
  sprite.pushSprite(5, 15);
}

void disp1(String cmd) {
  sprite.setTextColor(TFT_WHITE);
  disp0(cmd);
}

void disp2(String cmd) {
  sprite.setTextColor(TFT_GREEN);
  disp0(cmd);
}

void error(String cmd) {
  sprite.setTextColor(TFT_RED);
  disp0(cmd);
}

void echo(String cmd) {
  disp1(cmd);
  if (echo_mode) {
    serial_ext.printf("%s\r\n", cmd.c_str());
  }
  delay(delayms);
}

String trimq(String str) {
  if (str.charAt(0) == '"') {
    return str.substring(1, str.length() - 1);
  }
  return str;
}

String get_next_param(String &cmd) {
  String str = "";
  if (cmd.indexOf(",") > -1) {
    str = cmd.substring(0, cmd.indexOf(","));
    cmd = cmd.length() > cmd.indexOf(",") + 1 ? cmd.substring(cmd.indexOf(",") + 1) : "";
  } else {
    str = cmd;
    cmd = "";
  }
  return str;
}

String get_args(String &cmd) {
  String arg = cmd.length() > cmd.indexOf("=") + 1 ? cmd.substring(cmd.indexOf("=") + 1) : "";
  return arg;
}

void send_ok(void) {
  serial_ext.printf("\r\nOK\r\n");
}

void force_close(void) {
  disp2("CLOSED");
  conn = 0;
  if (mux_mode == 1) {
    serial_ext.printf("%d,CLOSED\r\n", linkid);
  } else {
    serial_ext.printf("CLOSED\r\n");
  }
}

void check_conn(void) {
  if (conn == 1) {
    force_close();
  }
}

void eAT(String command) {
  echo(command);
  send_ok();
}

void eDUMMY(String command) {
  eAT(command);
}

void eATRST(String command) {
  echo(command);
  ESP.restart();
}

void eATGMR(String command) {
  echo(command);
  serial_ext.printf("AT version:1.1.0.0(May 11 2016 18:09:56)\r\n");
  serial_ext.printf("SDK version:1.5.4(baaeaebb)\r\n");
  serial_ext.printf("compile time:May 20 2016 15:06:44\r\n");
  send_ok();
}

void eATE(String command) {
  echo(command);
  command = get_args(command);
  String echo = get_next_param(command);
  echo_mode = echo.toInt();
  send_ok();
}

void eATCWMODE(String command) {
  echo(command);
  command = get_args(command);
  String mode = command.substring(command.length() - 1);
  if (mode == "1") {
    WiFi.enableSTA(true);
  } else if (mode == "2") {
    WiFi.enableAP(true);
  } else if (mode == "3") {
    WiFi.enableSTA(true);
    WiFi.enableAP(true);
  }
  send_ok();
}

void qATCWMODE(String command) {
  echo(command);
  wifi_mode_t mode = WiFi.getMode();
  if (mode == WIFI_MODE_AP) {
    serial_ext.printf("+CWMODE:2\r\n");
  } else if (mode == WIFI_MODE_APSTA) {
    serial_ext.printf("+CWMODE:3\r\n");
  } else {
    serial_ext.printf("+CWMODE:1\r\n");
  }
  send_ok();
}

void qATCWMODECUR(String command) {
  echo(command);
  wifi_mode_t mode = WiFi.getMode();
  if (mode == WIFI_MODE_AP) {
    serial_ext.printf("+CWMODE_CUR:2\r\n");
  } else if (mode == WIFI_MODE_APSTA) {
    serial_ext.printf("+CWMODE_CUR:3\r\n");
  } else {
    serial_ext.printf("+CWMODE_CUR:1\r\n");
  }
  send_ok();
}

void eATCWJAPCUR(String command) {
  echo(command);
  command = get_args(command);
  String ssid = get_next_param(command);
  String pass = get_next_param(command);
  String bssid = get_next_param(command);
  ssid = trimq(ssid);
  pass = trimq(pass);
//  echo(ssid.c_str());
//  echo(pass.c_str());
  WiFi.begin(ssid.c_str(), pass.c_str());
  int cnt = 10;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    cnt--;
    if (cnt <= 0) {
      break;
    }
  }
  if (WiFi.status() == WL_CONNECTED) {
    delay(100);
    serial_ext.printf("WIFI CONNECTED\r\n");
    delay(100);
    serial_ext.printf("WIFI GOT IP\r\n");
    send_ok();
  } else {
//    serial_ext.printf("+CWJAP_CUR:4\r\n\r\n");
//    serial_ext.printf("FAIL\r\n");
    delay(100);
    serial_ext.printf("+CWJAP:4\r\n\r\n");
    delay(100);
    serial_ext.printf("FAIL\r\n");
  }
}

void eATCWJAP(String command) {
  echo(command);
  command = get_args(command);
  String ssid = get_next_param(command);
  String pass = get_next_param(command);
  String bssid = get_next_param(command);
  ssid = trimq(ssid);
  pass = trimq(pass);
//  echo(ssid.c_str());
//  echo(pass.c_str());
  WiFi.begin(ssid.c_str(), pass.c_str());
  int cnt = 10;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    cnt--;
    if (cnt <= 0) {
      break;
    }
  }
  if (WiFi.status() == WL_CONNECTED) {
    delay(100);
    serial_ext.printf("WIFI CONNECTED\r\n");
    delay(100);
    serial_ext.printf("WIFI GOT IP\r\n");
    send_ok();
  } else {
//    serial_ext.printf("+CWJAP_CUR:4\r\n\r\n");
//    serial_ext.printf("FAIL\r\n");
    delay(100);
    serial_ext.printf("+CWJAP_CUR:4\r\n\r\n");
    delay(100);
    serial_ext.printf("FAIL\r\n");
  }
}

void qATCWJAP(String command) {
  echo(command);
//        uint8_t bssid[8] = WiFi.BSSID();
  serial_ext.printf("+CWJAP:");
  serial_ext.printf("\"%s\",", WiFi.SSID().c_str());
  serial_ext.printf("\"%s\",", WiFi.BSSIDstr().c_str());
  serial_ext.printf("%d,", WiFi.channel());
  serial_ext.printf("%d\r\n", WiFi.RSSI());
  delay(delayms);
  send_ok();
}

void qATCWJAPCUR(String command) {
  echo(command);
//        uint8_t bssid[8] = WiFi.BSSID();
  serial_ext.printf("+CWJAP_CUR:");
  serial_ext.printf("\"%s\",", WiFi.SSID().c_str());
  serial_ext.printf("\"%s\",", WiFi.BSSIDstr().c_str());
  serial_ext.printf("%d,", WiFi.channel());
  serial_ext.printf("%d\r\n", WiFi.RSSI());
  delay(delayms);
  send_ok();
}

void qATCWLAP(String command) {
  echo(command);
  int num = WiFi.scanNetworks();
  for (int i = 0; i < num; i++) {
    serial_ext.printf("+CWLAP:(");
    serial_ext.printf("%d,", WiFi.encryptionType(i));
    serial_ext.printf("\"%s\",", WiFi.SSID(i).c_str());
    serial_ext.printf("%d,", WiFi.RSSI(i));
    String mac = WiFi.BSSIDstr(i);
    mac.toLowerCase();
    serial_ext.printf("\"%s\",", mac.c_str());
    serial_ext.printf("%d", WiFi.channel(i));
    serial_ext.printf(",-42,0)\r\n");
    delay(200);
  }
  send_ok();
}

void eATCWQAP(String command) {
  echo(command);
  WiFi.disconnect();
  conn = 0;
  send_ok();
  serial_ext.printf("WIFI DISCONNECT\r\n");
}

void eATCWSAP(String command) {
  echo(command);
  command = get_args(command);
  String ssid = get_next_param(command);
  String pass = get_next_param(command);
  String channel = get_next_param(command);
  String ecn = get_next_param(command);
  ssid = trimq(ssid);
  pass = trimq(pass);
  if (WiFi.softAP(ssid.c_str(), pass.c_str())) {
    serial_ext.printf("OK\r\n");
  } else {
    serial_ext.printf("ERROR\r\n");
  }
}

void qATCIPSTA(String command) {
  echo(command);
  serial_ext.printf("+CIPSTA:ip:\"%s\"\r\n", WiFi.localIP().toString().c_str());
  delay(100);
  serial_ext.printf("+CIPSTA:gateway:\"%s\"\r\n", WiFi.gatewayIP().toString().c_str());
  delay(100);
  serial_ext.printf("+CIPSTA:netmask:");
  serial_ext.printf("\"%s\"\r\n", WiFi.subnetMask().toString().c_str());
  delay(100);
  send_ok();
}

void qATCIPSTACUR(String command) {
  echo(command);
  delay(100);
  serial_ext.printf("+CIPSTA_CUR:ip:\"%s\"\r\n", WiFi.localIP().toString().c_str());
  delay(100);
  serial_ext.printf("+CIPSTA_CUR:gateway:\"%s\"\r\n", WiFi.gatewayIP().toString().c_str());
  delay(100);
  serial_ext.printf("+CIPSTA_CUR:netmask:\"%s\"\r\n", WiFi.subnetMask().toString().c_str());
  delay(100);
  send_ok();
}

void qATCIPAP(String command) {
  echo(command);
  serial_ext.printf("+CIPAP:");
  serial_ext.printf("\"%s\"\r\n", WiFi.localIP().toString().c_str());
  serial_ext.printf("+CIPAP:");
  serial_ext.printf("\"%s\"\r\n", WiFi.gatewayIP().toString().c_str());
  serial_ext.printf("+CIPAP:");
  serial_ext.printf("\"%s\"\r\n", WiFi.subnetMask().toString().c_str());
  send_ok();
}

void eATCIPSTATUS(String command) {
  echo(command);
  if (WiFi.isConnected()) {
//    check_conn();
    if (client.connected()) {
      serial_ext.printf("STATUS:3\r\n");
      delay(delayms);
      serial_ext.printf("+CIPSTATUS:%d,", linkid);
      serial_ext.printf("\"TCP\",");
      serial_ext.printf("\"%s\",", client.remoteIP().toString().c_str());
      serial_ext.printf("%d,", client.remotePort());
      serial_ext.printf("%d,", client.localPort());
      wifi_mode_t mode = WiFi.getMode();
      if (mode == WIFI_MODE_STA) {
        serial_ext.printf("0\r\n");
      } else {
        serial_ext.printf("1\r\n");
      }
    } else {
      serial_ext.printf("STATUS:4\r\n");
    }
  } else {
    serial_ext.printf("STATUS:5\r\n");
  }
  send_ok();
}

void qATCIFSR(String command) {
  echo(command);
  wifi_mode_t mode = WiFi.getMode();
  if (mode == WIFI_MODE_STA) {
    serial_ext.printf("+CIFSR:STAIP,");
    serial_ext.printf("\"%s\"\r\n", WiFi.localIP().toString().c_str());
    serial_ext.printf("+CIFSR:STAMAC,");
    serial_ext.printf("\"%s\"\r\n", WiFi.macAddress().c_str());
  } else if (mode == WIFI_MODE_AP) {
    serial_ext.printf("+CIFSR:APIP,");
    serial_ext.printf("\"%s\"\r\n", WiFi.softAPBroadcastIP().toString().c_str());
    serial_ext.printf("+CIFSR:APMAC,");
    serial_ext.printf("\"%s\"\r\n", WiFi.softAPmacAddress().c_str());
  } else if (mode == WIFI_MODE_APSTA) {
    serial_ext.printf("+CIFSR:APIP,");
    serial_ext.printf("\"%s\"\r\n", WiFi.softAPBroadcastIP().toString().c_str());
    serial_ext.printf("+CIFSR:APMAC,");
    serial_ext.printf("\"%s\"\r\n", WiFi.softAPmacAddress().c_str());
    serial_ext.printf("+CIFSR:STAIP,");
    serial_ext.printf("\"%s\"\r\n", WiFi.localIP().toString().c_str());
    serial_ext.printf("+CIFSR:STAMAC,");
    serial_ext.printf("\"%s\"\r\n", WiFi.macAddress().c_str());
  }
  send_ok();
}

void eATCIPSTART(String command) {
  echo(command);
  command = get_args(command);
  String id = "";
  String protocol = "";
  String host = "";
  String port = "";
  if (command.substring(0, 1) == "\"") {
    //
  } else {
    id = get_next_param(command);
    linkid = (int)id.toInt();
    command = command.length() > command.indexOf(",") + 1 ? command.substring(command.indexOf(",") + 1) : "";
  }
  protocol = get_next_param(command);
  protocol = trimq(protocol);
  host = get_next_param(command);
  host = trimq(host);
  port = get_next_param(command);
  int ret = client.connect(host.c_str(), port.toInt(), 10000);
//      Serial.printf(host.c_str());
//      Serial.printf(port.c_str());
//      IPAddress ip = IPAddress((uint8_t *)host.c_str());
//      int ret = client.connect(ip, port.toInt(), 10000);
  if (ret) {
    conn = 1;
    if (mux_mode == 1) {
      serial_ext.printf("%d,CONNECT\r\n", linkid);
    } else {
      serial_ext.printf("CONNECT\r\n");
    }
    serial_ext.printf("OK\r\n");
  } else {
    serial_ext.printf("ERROR\r\n");
  }
}

void eATCIPCLOSE(String command) {
  echo(command);
  client.stop();
//  check_conn();
  force_close();
  send_ok();
}

void eATCIPSEND(String command) {
  echo(command);
  command = get_args(command);
  String linkid = "";
  String length = "";
  if (command.indexOf(",") > -1) {
    linkid = get_next_param(command);
    length = get_next_param(command);
  } else {
    length = command;
    command = "";
  }
  serial_ext.printf(">");
  recv_length = length.toInt();
//  serial_ext.readBytes(buf, length.toInt());
//  client.write(buf, length.toInt());
//  serial_ext.printf("Recv %s bytes\r\n", length);
//  serial_ext.printf("SEND OK\r\n");
}

void eATUARTCUR(String command) {
  echo(command);
  command = get_args(command);
  String baudrate = get_next_param(command);
//  serial_ext.end();
//  serial_ext.begin(baudrate.toInt(), SERIAL_8N1, 21, 22); // M5Stack (Grove)
  serial_ext.updateBaudRate(baudrate.toInt());
  delay(500);
  serial_ext.printf("OK\r\n");
}

void eATCIPDOMAIN(String command) {
  echo(command);
  command = get_args(command);
  String domain = get_next_param(command);
  domain = trimq(domain);
  IPAddress addr = {0, 0, 0, 0};
  int ret = WiFi.hostByName(domain.c_str(), addr);
//      Serial.printf(host.c_str());
//      Serial.printf(port.c_str());
//      IPAddress ip = IPAddress((uint8_t *)host.c_str());
//      int ret = client.connect(ip, port.toInt(), 10000);
  if (ret == 1) {
//    serial_ext.printf("+CIPDOMAIN:\"%s\"\r\n", addr.toString().c_str());
    serial_ext.printf("+CIPDOMAIN:%s\r\n", addr.toString().c_str());
    send_ok();
  } else {
    serial_ext.printf("DNS Fail\r\n");
    serial_ext.printf("ERROR\r\n");
  }
}

void eATCIPMUX(String command) {
  echo(command);
  command = get_args(command);
  String mux = get_next_param(command);
  mux_mode = mux.toInt();
  send_ok();
}

void eATIPDINFO(String command) {
  echo(command);
  command = get_args(command);
  String ipd = get_next_param(command);
  ipd_mode = ipd.toInt();
  send_ok();
}

void setup() {
  M5.begin();
  M5.Lcd.fillScreen(TFT_NAVY);

  M5.Lcd.setTextColor(TFT_CYAN);
  M5.Lcd.drawString("ESP32 WiFi", 0, 0);

  sprite.setColorDepth(8);
  sprite.createSprite(310, 220);
  sprite.setCursor(0, 0);
  sprite.setTextFont(0);

//  sleep(5);
//  Serial.println("hello");

//  serial_ext.begin(115200, SERIAL_8N1, 32, 33); // M5StackC
  serial_ext.begin(115200, SERIAL_8N1, 21, 22); // M5Stack (Grove)
//  serial_ext.begin(115200, SERIAL_8N1, 3, 1); // M5Stack (USB)

//*  delay(500);
  serial_ext.printf("OK\r\n");
}

void loop() {
  // network input
  int avlen = client.available();
//  if (avlen > 1024) {
//    avlen = 1024;
//  }
  if (avlen > 0) {
//    client.readBytes(buf, avlen);
    serial_ext.printf("\r\n");
    serial_ext.printf("+IPD");
    if (mux_mode == 1) {
      serial_ext.printf(",%d", linkid);
    }
    serial_ext.printf(",%d", avlen);
    if (ipd_mode == 1) {
      serial_ext.printf(",%s", client.remoteIP().toString().c_str());
      serial_ext.printf(",%d", client.remotePort());
    }
    serial_ext.printf(":");
    sprintf(msg, "+IPD,%d,%d,%s,%d %d %d:", linkid, avlen, client.remoteIP().toString().c_str(), client.remotePort(), mux_mode, ipd_mode);
    disp2(msg);
    while (client.available() > 0) {
//      char c = client.read();
//      serial_ext.write(c);
      int size = client.readBytes((uint8_t *)buf, 128);
      serial_ext.write((uint8_t *)buf, size);
    }
//    int size = client.readBytes((uint8_t *)buf, avlen);
//    serial_ext.write((uint8_t *)buf, size);
//    delay(100);
    check_conn();
    return;
  }

  // serial input
  if (serial_ext.available() > 0) {
    // data
    if (recv_length > 0) {
      delay(100);
      int len = serial_ext.available();
//      unsigned long start = millis();
//      while (recv_length > 0) {
//        char c = serial_ext.read();
//        client.write(c);
//        recv_length--;
//      }
      int size = serial_ext.readBytes(buf, len);
      client.write(buf, size);
      disp2(buf);
//      recv_length -= len;
      recv_length = 0;
      serial_ext.printf("\r\nRecv %d bytes\r\n", len);
      sprintf(msg, "Recv %d bytes\r\n", len);
      disp2(msg);
      serial_ext.printf("\r\nSEND OK\r\n");
      disp2("SEND OK");
      return;
    }
    // command
    String command = serial_ext.readStringUntil('\n');
    if (command.length() > 0 && command.endsWith("\r")) {
      command.remove(command.length() - 1);
    }
    // echo
//    serial_ext.printf("\"%s\"\r\n", command.c_str());
    if (command == "AT") {
      //Tests AT Startup
      eAT(command);
    } else if (command == "AT+RST") {
      //Restarts the Module
      eATRST(command);
    } else if (command == "AT+GMR") {
      //Checks Version Information
      eATGMR(command);
    } else if (command.length() == 4 && command.substring(0, 3) == "ATE") {
      //AT Commands Echoing
      eATE(command);
    } else if (command.length() == 11 && command.substring(0, 10) == "AT+CWMODE=") {
      //Sets the Current Wi-Fi mode; Configuration Not Saved in the Flash
      eATCWMODE(command);
    } else if (command.length() == 15 && command.substring(0, 14) == "AT+CWMODE_CUR=") {
      //Sets the Current Wi-Fi mode; Configuration Not Saved in the Flash
      eATCWMODE(command);
    } else if (command == "AT+CWMODE?") {
      //Sets the Current Wi-Fi mode; Configuration Not Saved in the Flash
      qATCWMODE(command);
    } else if (command == "AT+CWMODE_CUR?") {
      //Sets the Current Wi-Fi mode; Configuration Not Saved in the Flash
      qATCWMODECUR(command);
    } else if (command.length() > 13 && command.substring(0, 13) == "AT+CWJAP_CUR=") {
      //Connects to an AP; Configuration Not Saved in the Flash
      eATCWJAPCUR(command);
    } else if (command.length() > 9 && command.substring(0, 9) == "AT+CWJAP=") {
      //Connects to an AP; Configuration Not Saved in the Flash
      eATCWJAP(command);
    } else if (command == "AT+CWJAP?") {
      //to query the AP to which the ESP8266 Station is already connected.
      qATCWJAP(command);
    } else if (command == "AT+CWJAP_CUR?") {
      //to query the AP to which the ESP8266 Station is already connected.
      qATCWJAPCUR(command);
    } else if (command == "AT+CWLAP") {
      //Lists Available APs
      qATCWLAP(command);
    } else if (command == "AT+CWQAP") {
      //Disconnects from the AP
      eATCWQAP(command);
    } else if (command.length() > 13 && command.substring(0, 13) == "AT+CWSAP_CUR=") {
      //Configures the ESP8266 SoftAP; Configuration Not Saved in the Flash
      eATCWSAP(command);
    } else if (command.length() == 13 && command.substring(0, 10) == "AT+CWDHCP=") {
      //Enables/Disables DHCP; Configuration Not Saved in the Flash
      eDUMMY(command);
    } else if (command.length() == 17 && command.substring(0, 14) == "AT+CWDHCP_CUR=") {
      //Enables/Disables DHCP; Configuration Not Saved in the Flash
      eDUMMY(command);
    } else if (command.length() == 15 && command.substring(0, 14) == "AT+CWAUTOCONN=") {
      //Auto-Connects to the AP or Not
      eDUMMY(command);
    } else if (command.length() > 14 && command.substring(0, 14) == "AT+CIPSTA_CUR=") {
      //Sets the Current IP Address of the ESP8266 Station; Configuration Not Saved in the Flash
      eDUMMY(command);
    } else if (command == "AT+CIPSTA?") {
      //to obtain the current IP address of the ESP8266 Station.
      qATCIPSTA(command);
    } else if (command == "AT+CIPSTA_CUR?") {
      //to obtain the current IP address of the ESP8266 Station.
      qATCIPSTACUR(command);
    } else if (command.length() > 13 && command.substring(0, 13) == "AT+CIPAP_CUR=") {
      //—Sets the IP Address of the ESP8266 SoftAP; Configuration Not Saved in the Flash
      eDUMMY(command);
    } else if (command == "AT+CIPAP?") {
      //to obtain the current IP address of the ESP8266 SoftAP.
      qATCIPAP(command);
    } else if (command == "AT+CIPSTATUS") {
      //Gets the Connection Status
      eATCIPSTATUS(command);
    } else if (command.length() == 12 && command.substring(0, 11) == "AT+CIPMODE=") {
      //Sets Transmission Mode
      eDUMMY(command);
    } else if (command.length() == 11 && command.substring(0, 10) == "AT+CIPMUX=") {
      //Enable or Disable Multiple Connections
      eATCIPMUX(command);
    } else if (command.length() == 13 && command.substring(0, 12) == "AT+CIPDINFO=") {
      //Shows the Remote IP and Port with +IPD
      eATIPDINFO(command);
    } else if (command == "AT+CIFSR") {
      //Gets the Local IP Address
      qATCIFSR(command);
    } else if (command.length() > 8 && command.substring(0, 8) == "AT+PING=") {
      //Ping Packets
      eDUMMY(command);
    } else if (command.length() > 13 && command.substring(0, 13) == "AT+CIPSERVER=") {
      //Deletes/Creates TCP Server
      eDUMMY(command);
    } else if (command.length() > 14 && command.substring(0, 14) == "AT+CIPSSLSIZE=") {
      //Sets the Size of SSL Buffer
      eDUMMY(command);
    } else if (command.length() > 12 && command.substring(0, 12) == "AT+CIPSTART=") {
      //Establishes TCP Connection, UDP Transmission or SSL Connection
      eATCIPSTART(command);
    } else if (command == "AT+CIPCLOSE") {
      //Closes TCP/UDP/SSL connection
      eATCIPCLOSE(command);
    } else if (command.length() > 12 && command.substring(0, 12) == "AT+CIPCLOSE=") {
      //Closes TCP/UDP/SSL connection
      eATCIPCLOSE(command);
    } else if (command.length() > 11 && command.substring(0, 11) == "AT+CIPSEND=") {
      //Sends Data
      eATCIPSEND(command);
    } else if (command.length() > 12 && command.substring(0, 12) == "AT+UART_CUR=") {
      //Set baud rate
      eATUARTCUR(command);
    } else if (command.length() > 13 && command.substring(0, 13) == "AT+CIPDOMAIN=") {
      //
      eATCIPDOMAIN(command);
    } else {
      error(command);
    }
  }
  // connect info
  if (WiFi.isConnected()) {
    M5.Lcd.setTextColor(TFT_GREEN);
    M5.Lcd.drawString("@", 300, 0);
  } else {
    M5.Lcd.setTextColor(TFT_RED);
    M5.Lcd.drawString("O", 300, 0);
  }
}