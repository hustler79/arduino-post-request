#include <EtherCard.h>
#include <avr/pgmspace.h> // to use flash memory (progmem)

#define PATH "/"
#define VARIABLE "test"

static byte mymac[] = { 0x74,0x69,0x69,0x2D,0x30,0x31 };
const char website[] PROGMEM = "SERVER_URL";

byte Ethernet::buffer[700];
uint32_t timer;
Stash stash;

void setup() {
  Serial.begin(57600);
  Serial.println(F("\n[webClient]"));
  
  if (ether.begin(sizeof Ethernet::buffer, mymac, 8) == 0) 
    Serial.println(F("Failed to access Ethernet controller"));

  Serial.println(F("Setting up DHCP"));
  if (!ether.dhcpSetup())
    Serial.println(F("DHCP failed"));
  
  ether.printIp("My IP: ", ether.myip);
  ether.printIp("Netmask: ", ether.netmask);
  ether.printIp("GW IP: ", ether.gwip);
  ether.printIp("DNS IP: ", ether.dnsip);

  if (!ether.dnsLookup(website))
    Serial.println(F("DNS failed"));

  ether.printIp("SRV: ", ether.hisip);
}

void loop() {
  ether.packetLoop(ether.packetReceive());
  static byte session; // save server response ID

  if (millis() > timer) {
    timer = millis() + 10000;

    byte sd = stash.create();
    stash.print("variable=");
    stash.print(VARIABLE);
    stash.save();

    // generate the header with payload - note that the stash size is used,
    // and that a "stash descriptor" is passed in as argument using "$H"
    Stash::prepare(PSTR("POST http://$F/$F HTTP/1.0" "\r\n"
                        "Host: $F" "\r\n"
                        "Content-Length: $D" "\r\n"
                        "Content-Type: application/x-www-form-urlencoded\r\n"
                        "\r\n"
                        "$H"),
            website, PSTR(PATH), website, stash.size(), sd);

    // send the packet - this also releases all stash buffers once done
    // Each new tcpSend() call increases an internal session ID, which consist of a small integer in the range 0..7 (it wraps after 8 calls).
    session = ether.tcpSend();
  }

  // There’s a new tclReply() call which takes that session ID as argument, 
  // and returns a pointer to the received data if there is any, or a null pointer otherwise. Each new reply is only returned once.
  const char* reply = ether.tcpReply(session);
  if (reply != 0) {
    Serial.println(">>> RESPONSE RECEIVED ----");
    Serial.println(reply);
  }
}
