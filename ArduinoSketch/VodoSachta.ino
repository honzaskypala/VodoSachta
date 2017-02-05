/*
  Vodomerna sachta

  © Honza Skypala 2017 - WTFPL
  function ntpUnixTime © Francesco Potortì 2013 - GPLv3 - Revision: 1.13

 Circuit:
 * Module -> Arduino
 * GND    -> GNDardu
 * VCC    -> 3V3
 * CS     -> D10
 * SCK    -> D13
 * SI     -> D11
 * SO     -> D12
 */

#define DEBUG 1
#define debug_print(msg)   if (DEBUG) Serial.print(msg);
#define debug_println(msg) if (DEBUG) Serial.println(msg);

#include <UIPEthernet.h>
#include <TimeLib.h>

// Enter a MAC address for your controller below.
// Newer Ethernet shields have a MAC address printed on a sticker on the shield
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

// Set the static IP address to use if the DHCP fails to assign
IPAddress ip(192, 168, 2, 41);

// Initialize the Ethernet client library
// with the IP address and port of the server
// that you want to connect to (port 80 is default for HTTP):
EthernetClient client;
EthernetUDP udp;

#define STATUS_COLD_START 0
#define STATUS_GET_NTP_TIME 1
#define STATUS_OPERATIONS -1

const int timeZone = 1;     // Central European Time
//const int timeZone = -5;  // Eastern Standard Time (USA)
//const int timeZone = -4;  // Eastern Daylight Time (USA)
//const int timeZone = -8;  // Pacific Standard Time (USA)
//const int timeZone = -7;  // Pacific Daylight Time (USA)

int status = STATUS_COLD_START;

void setup() {
  #if DEBUG == 1
    Serial.begin(9600);       // Open serial communications and wait for port to open:
    while (!Serial) { };      // wait for serial port to connect. Needed for native USB port only
    debug_println("Cold start");
  #else
    delay(60000);   // wait 1 minute -- in case of cold start, it may be after power failure and the router may be booting, so wait for it
  #endif
}

void inline init_ethernet() {
  debug_println("Init ethernet");
  // start the Ethernet connection:
  if (Ethernet.begin(mac) == 0) {
    debug_println("Failed to configure Ethernet using DHCP");
    // try to congifure using IP address instead of DHCP:
    Ethernet.begin(mac, ip);
  }
  debug_print("  localIP: ");
  debug_println(Ethernet.localIP());
  debug_print("  subnetMask: ");
  debug_println(Ethernet.subnetMask());
  debug_print("  gatewayIP: ");
  debug_println(Ethernet.gatewayIP());
  debug_print("  dnsServerIP: ");
  debug_println(Ethernet.dnsServerIP());
  
  // give the Ethernet shield a second to initialize:
  delay(2000);
  status = STATUS_GET_NTP_TIME;
}

unsigned long inline ntpUnixTime(UDP &udp) {
  static int udpInited = udp.begin(123); // open socket on arbitrary port

  const char timeServer[] = "pool.ntp.org";  // NTP server

  // Only the first four bytes of an outgoing NTP packet need to be set
  // appropriately, the rest can be whatever.
  const long ntpFirstFourBytes = 0xEC0600E3; // NTP request header

  // Fail if WiFiUdp.begin() could not init a socket
  if (! udpInited)
    return 0;

  // Clear received data from possible stray received packets
  udp.flush();

  // Send an NTP request
  if (! (udp.beginPacket(timeServer, 123) // 123 is the NTP port
   && udp.write((byte *)&ntpFirstFourBytes, 48) == 48
   && udp.endPacket()))
    return 0;       // sending request failed

  // Wait for response; check every pollIntv ms up to maxPoll times
  const int pollIntv = 150;   // poll every this many ms
  const byte maxPoll = 15;    // poll up to this many times
  int pktLen;       // received packet length
  for (byte i=0; i<maxPoll; i++) {
    if ((pktLen = udp.parsePacket()) == 48)
      break;
    delay(pollIntv);
  }
  if (pktLen != 48)
    return 0;       // no correct packet received

  // Read and discard the first useless bytes
  // Set useless to 32 for speed; set to 40 for accuracy.
  const byte useless = 40;
  for (byte i = 0; i < useless; ++i)
    udp.read();

  // Read the integer part of sending time
  unsigned long time = udp.read();  // NTP time
  for (byte i = 1; i < 4; i++)
    time = time << 8 | udp.read();

  // Round to the nearest second if we want accuracy
  // The fractionary part is the next byte divided by 256: if it is
  // greater than 500ms we round to the next second; we also account
  // for an assumed network delay of 50ms, and (0.5-0.05)*256=115;
  // additionally, we account for how much we delayed reading the packet
  // since its arrival, which we assume on average to be pollIntv/2.
  time += (udp.read() > 115 - pollIntv/8);

  // Discard the rest of the packet
  udp.flush();

  return time - 2208988800ul + timeZone * SECS_PER_HOUR;   // convert NTP time to Unix time
}

void loop() {
  unsigned long unixTime;
  switch (status) {
  case STATUS_COLD_START:
    init_ethernet();
    break;
  case STATUS_GET_NTP_TIME:
    debug_println("Retrieve NTP time...");
    unixTime = ntpUnixTime(udp);
    debug_print("  retrieved: ");
    debug_println(unixTime);
    if (unixTime) {
      setTime(unixTime);
      status = STATUS_OPERATIONS;
    } else {
      delay(60000);
    }
    break;
  default:
    debug_println("Operations...");
    Ethernet.maintain();
    #if DEBUG == 1
      debug_print("  time: ");
      digitalClockDisplay();
    #endif
    delay(5000);
    break;
  }
    
}

#if DEBUG == 1
  void digitalClockDisplay() {
    // digital clock display of the time
    debug_print(hour());
    printDigits(minute());
    printDigits(second());
    debug_print(" ");
    debug_print(day());
    debug_print(" ");
    debug_print(month());
    debug_print(" ");
    debug_print(year()); 
    debug_println(); 
  }
  
  void printDigits(int digits){
    // utility function for digital clock display: prints preceding colon and leading 0
    debug_print(":");
    if (digits < 10)
      debug_print('0');
    debug_print(digits);
  }
#endif

