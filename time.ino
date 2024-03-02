// #include <NTPClient.h>

#include <TimeLib.h>
WiFiUDP ntpUDP;

// NTPClient timeClient(ntpUDP, "time.nist.gov", -8 * 60 * 60);  // -7 hours offset

IPAddress timeServer(132, 163, 97, 1); // time-a-wwv.nist.gov
// IPAddress timeServer(132, 163, 4, 102); // time-b.timefreq.bldrdoc.gov
// IPAddress timeServer(132, 163, 4, 103); // time-c.timefreq.bldrdoc.gov


// const int timeZone = 1;     // Central European Time
// const int timeZone = -5;  // Eastern Standard Time (USA)
// const int timeZone = -4;  // Eastern Daylight Time (USA)
const int timeZone = -8;  // Pacific Standard Time (USA)
//const int timeZone = -7;  // Pacific Daylight Time (USA)
unsigned int localPort = 1023;  // local port to listen for UDP packets


void setupTime(){
  // timeClient.begin();
  ntpUDP.begin(localPort);
  Serial.println("waiting for sync");
  setSyncProvider(getNtpTime);
  setSyncInterval(120);
}

unsigned long epoch() {
  // Serial.println(timeClient.getDay());
  // return timeClient.getFormattedTime();
  return  0;

}


void digitalClockDisplay(){
  // digital clock display of the time
  Serial.print(hour());
  printDigits(minute());
  printDigits(second());
  Serial.print(" ");
  Serial.print(dayShortStr(weekday()));
  Serial.print(" ");
  Serial.print(day());
  Serial.print(" ");
  Serial.print(monthShortStr(month()));
  Serial.print(" ");
  Serial.print(year()); 
  Serial.println(); 
}

void printDigits(int digits){
  // utility for digital clock display: prints preceding colon and leading 0
  Serial.print(":");
  if(digits < 10)
    Serial.print('0');
  Serial.print(digits);
}

// buffer needs to be hhmmsswwwddmmm
// for 123456monjan02
void getDateTimeString(char * buffer) {
  uint8_t hr = hour();

  if (hr > 12) {hr-=12;}

  if (hr == 0){
      sprintf (buffer, "12");
  } else if (hr < 10) {
      sprintf (buffer, "0%d", hr);
  } else {
      sprintf (buffer, "%d", hr);
  } 

  uint16_t min = minute();
  if (min < 10) {
      sprintf (buffer + 2, "0%d", min);
  } else {
      sprintf (buffer + 2, "%d", min);
  }

  uint16_t sec = second();
  if (sec < 10) {
      sprintf (buffer + 4, "0%d", sec);
  } else {
      sprintf (buffer + 4, "%d", sec);
  }

  sprintf (buffer + 6, dayShortStr(weekday()));
    if (day() < 10) {
      sprintf (buffer + 9, "0%d", day());
  } else {
      sprintf (buffer + 9, "%d", day());
  }

  sprintf (buffer + 11, monthShortStr(month()));
}


time_t lastUpdate = 0; // when the digital clock was displayed
void updateTime() {
  // Serial.print(".");
  // time_t current = now();
  // if (current - lastUpdate > 1){
  //   digitalClockDisplay();  
  //   lastUpdate = current;
  // }
}
/*-------- NTP code ----------*/

const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets

time_t getNtpTime()
{
  while (ntpUDP.parsePacket() > 0) ; // discard any previously received packets
  Serial.println("Transmit NTP Request");
  sendNTPpacket(timeServer);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = ntpUDP.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      Serial.println("Receive NTP Response");
      ntpUDP.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
    }
  }
  Serial.println("No NTP Response :-(");
  return 0; // return 0 if unable to get the time
}

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress &address)
{
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:                 
  ntpUDP.beginPacket(address, 123); //NTP requests are to port 123
  ntpUDP.write(packetBuffer, NTP_PACKET_SIZE);
  ntpUDP.endPacket();
}

