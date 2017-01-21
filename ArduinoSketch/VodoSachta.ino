/*
  Vodomerna sachta

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
#define debug_print(msg)   do { if (DEBUG) Serial.print(msg);   } while (0)
#define debug_println(msg) do { if (DEBUG) Serial.println(msg); } while (0)

#include <UIPEthernet.h>

// Enter a MAC address for your controller below.
// Newer Ethernet shields have a MAC address printed on a sticker on the shield
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

// Set the static IP address to use if the DHCP fails to assign
IPAddress ip(192, 168, 2, 41);

// Initialize the Ethernet client library
// with the IP address and port of the server
// that you want to connect to (port 80 is default for HTTP):
EthernetClient client;

#define STATUS_COLD_START 0
#define STATUS_OPERATIONS -1

int status = STATUS_COLD_START;

void setup() {
  do {
    if (DEBUG) {
      Serial.begin(9600);       // Open serial communications and wait for port to open:
      while (!Serial) { };      // wait for serial port to connect. Needed for native USB port only
      debug_println("Cold start");
    }
  } while (0);
  do { 
    if (!DEBUG) delay(60000);   // wait 1 minute -- in case of cold start, it may be after power failure and the router may be booting, so wait for it
  } while (0);
}

void loop() {
  switch (status) {
  case STATUS_COLD_START:
    init_ethernet();
    break;
  default:
    debug_println("Operations...");
    delay(5000);
    break;
  }
    
}

void init_ethernet() {
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
  status = STATUS_OPERATIONS;
}
