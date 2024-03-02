#include <WiFiNINA.h>

int status = WL_IDLE_STATUS;             // the Wi-Fi radio's status
unsigned long previousMillisInfo = 0;     //will store last time Wi-Fi information was updated
const int intervalInfo = 5000;            // interval at which to update the board information

arduino::IPAddress myIP;


void connectWifi(char* ssid, char* pass) {
  Serial.print("Attempting to connect to network: ");
  // attempt to connect to Wi-Fi network:
  while (status != WL_CONNECTED) {
    Serial.println(ssid);
    // Connect to WPA/WPA2 network:
    status = WiFi.begin(ssid, pass);

    // wait 10ms  for connection:
    delay(10);
  }

  // you're connected now, so print out the data:
  Serial.println("You're connected to the network");
  Serial.println("---------------------------------------");
}

void getWifiStatus() {
//  unsigned long currentMillisInfo = millis();

  // check if the time after the last update is bigger the interval
  // if (currentMillisInfo - previousMillisInfo >= intervalInfo) {
    // previousMillisInfo = currentMillisInfo;

    Serial.println("Board Information:");
    //i print your board's IP address:
    myIP = WiFi.localIP();
    Serial.print("IP Address: ");
    Serial.println(myIP);

    // print your network's SSID:
    Serial.println();
    Serial.println("Network Information:");
    Serial.print("SSID: ");
    Serial.println(WiFi.SSID());

    byte mac[6];                     // the MAC address of your Wifi shield

    WiFi.macAddress(mac);
    Serial.print("MAC: ");
    Serial.print(mac[5],HEX);
    Serial.print(":");
    Serial.print(mac[4],HEX);
    Serial.print(":");
    Serial.print(mac[3],HEX);
    Serial.print(":");
    Serial.print(mac[2],HEX);
    Serial.print(":");
    Serial.print(mac[1],HEX);
    Serial.print(":");
    Serial.println(mac[0],HEX);
    // print the received signal strength:
    long rssi = WiFi.RSSI();
    Serial.print("signal strength (RSSI):");
    Serial.println(rssi);
    Serial.println(myIP);
    Serial.println("---------------------------------------");
  // }


}