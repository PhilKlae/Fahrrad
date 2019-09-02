#include <mcp_can.h>
#include <SPI.h>
#include <PN532_SPI.h>
#include "PN532.h"
PN532_SPI pn532spi(SPI, 6);
PN532 nfc(pn532spi);

#ifdef ARDUINO_SAMD_VARIANT_COMPLIANCE
  #define SERIAL SerialUSB
#else
  #define SERIAL Serial
#endif

// If using the breakout with SPI, define the pins for SPI communication.
#define PN532_SCK  (13)
#define PN532_MOSI (11)
#define PN532_SS   (6)
#define PN532_MISO (12)

#if defined(ARDUINO_ARCH_SAMD)
// for Zero, output on USB Serial console, remove line below if using programming port to program the Zero!
// also change #define in Adafruit_PN532.cpp library file
   #define Serial SerialUSB
#endif

//input konstanten---------------------------------------------------------------------------

const int taster_out = 4;
const int blinker_links_input = 5;
const int blinker_rechts_input = 7;
const int warnblinker_input = 8; 
const int richtung_input = 14;
const int licht_input_abblend = 15;
const int licht_input_fern = 16;

int i;


//----------------------------------------------------------------------------------------//

void setup() 
{
  // put your setup code here, to run once:
  Serial.begin(115200);
  digitalWrite(4,1);
  pinMode(taster_out,OUTPUT);
  pinMode(warnblinker_input,INPUT);
  pinMode(blinker_rechts_input,INPUT);
  pinMode(blinker_links_input,INPUT);
  pinMode(richtung_input,INPUT);
  pinMode(licht_input_abblend,INPUT);
  pinMode(licht_input_fern,INPUT);
}

//----------------------------------------------------------------------------------------//

void loop() 
{
  // put your main code here, to run repeatedly:
  setupnfc2();
  readnfc2();
  SPI.endTransaction();
  setupcan();

//funktionsaufrufe schalter/bordnetz

  sendBordnetz();
  sendcan();
  sendBlinker();
  sendRichtung();
  sendLicht();

//-----
  SPI.endTransaction();
}

//konstanten für uebergabe---------------------------------------------------------------------//

const int SPI_CS_PIN = 10;
MCP_CAN CAN(SPI_CS_PIN);

const int can_id_blinker = 0x01;
const int can_id_richtung = 0x02;
const int can_id_licht = 0x03;
const int can_id_bordnetz = 0x04;

uint8_t PWD[16] = { 't','h','k','o','e','l','n','r','u','l','e','z','z','z',0,0};
uint8_t data[16];
//-------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------
  
//botschaften-------------------------------------------------------------------------------//
unsigned char stmp[8] = {0, 0, 0, 0, 0, 0, 0, 0};
unsigned char blinker[1] = {0};
unsigned char richtung[1] = {0};
unsigned char licht[1] = {0};
unsigned char bordnetz[1] = {0};


void setupcan()
{
  SERIAL.begin(115200);

  while (CAN_OK != CAN.begin(CAN_500KBPS)) // init can bus : baudrate = 500k
  {             
    SERIAL.println("CAN BUS Shield init fail");
    SERIAL.println(" Init CAN BUS Shield again");
    delay(100);
  }
  
  SERIAL.println("CAN BUS Shield init ok!");
}

void sendcan()
{
    // send data:  id = 0x00, standrad frame, data len = 8, stmp: data buf
  stmp[7] = stmp[7]+1;
  if(stmp[7] == 100)
  {
        stmp[7] = 0;
        stmp[6] = stmp[6] + 1;
        
        if(stmp[6] == 100)
        {
            stmp[6] = 0;
            stmp[5] = stmp[6] + 1;
        }
    }
    
    CAN.sendMsgBuf(0x00, 0, 8, stmp);
    delay(100);                       // send data per 100ms

}
//eigene funktionen-----------------------------------------------------------------//

void sendLicht()
{
  // standardbelegung 0 = licht aus
  licht[0] = 0;
  if(digitalRead(licht_input_abblend) == 1)
  {
    Serial.println("abblendlicht");
    //abblendlicht
    licht[0] = 1;
  }
  if(digitalRead(licht_input_fern) == 1)
  {
    //fernlicht
    Serial.println("fernlicht");
    licht[0] = 2;
  }  
  
  CAN.sendMsgBuf(can_id_licht, 0, 1, licht);
  delay(100); 
}




void sendRichtung()
{
  // standardbelegung 0 = richtung standard vorwaerts
  if(digitalRead(richtung_input) == 0)
  {
    Serial.println("beispiel vorwaerts");
        
    //vorwaerts
    richtung[0] = 0;
  }
  else
  {
    //rueckwaerts
    Serial.println("beispiel rueckwaerts");
    richtung[0] = 1;
  }
      
  CAN.sendMsgBuf(can_id_richtung, 0, 1, richtung);
  delay(100);    
}

void sendBlinker()
{
  // standardbelegung 0 = blinker aus
  blinker[0] = 0;
  if(digitalRead(blinker_links_input) == 1)
  {
    Serial.println("links");
    //blinker links
    blinker[0] = 1;
  }
  if(digitalRead(blinker_rechts_input) == 1)
  {
    //blinker rechts
    Serial.println("rechts");
    blinker[0] = 2;
  }  
  if(digitalRead(warnblinker_input) == 1)
  {
    //warnblinker
    Serial.println("warnblicker");
    blinker[0] = 3;
  }
  
  CAN.sendMsgBuf(can_id_blinker, 0, 1, blinker);
  delay(100); 
}

 void setupnfc2()
{
  Serial.begin(115200);
  Serial.println("Hello!");

  nfc.begin();

  uint32_t versiondata = nfc.getFirmwareVersion();
  if (! versiondata)
  {
    Serial.print("Didn't find PN53x board");
    while (1); // halt
  }
  // Got ok data, print it out!
  Serial.print("Found chip PN5"); Serial.println((versiondata>>24) & 0xFF, HEX); //bitverschiebung
  Serial.print("Firmware ver. "); Serial.print((versiondata>>16) & 0xFF, DEC); 
  Serial.print('.'); Serial.println((versiondata>>8) & 0xFF, DEC);
  
  // configure board to read RFID tags
  nfc.SAMConfig();
  
  Serial.println("Waiting for an ISO14443A Card ..."); 
}
 
 void readnfc2()
{
  uint8_t success;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
  uint8_t uidLength;                        // Length of the UID (4 or 7 bytes depending on ISO14443A card type)  
  // Wait for an ISO14443A type cards (Mifare, etc.).  When one is found
  // 'uid' will be populated with the UID, and uidLength will indicate
  // if the uid is 4 bytes (Mifare Classic) or 7 bytes (Mifare Ultralight)
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);
  if (success) 
  {
    // Display some basic information about the card
    Serial.println("Found an ISO14443A card");
    Serial.print("  UID Length: ");Serial.print(uidLength, DEC);Serial.println(" bytes");
    Serial.print("  UID Value: ");
    nfc.PrintHex(uid, uidLength);
    Serial.println("");
    
    if (uidLength == 4)
    {
      // We probably have a Mifare Classic card ... 
      Serial.println("Seems to be a Mifare Classic card (4 byte UID)");
   
      // Now we need to try to authenticate it for read/write access
      // Try with the factory default KeyA: 0xFF 0xFF 0xFF 0xFF 0xFF 0xFF
      Serial.println("Trying to authenticate block 12 with NEW KEYA value");
      uint8_t keya[6] = { 'h','e','a','d','u','p' }; //{0xFF,0xFF,0xFF,0xFF,0xFF,0xFF };
    
      // Start with block 4 (the first block of sector 1) since sector 0
      // contains the manufacturer data and it's probably better just
      // to leave it alone unless you know what you're doing

      //we work with Block 12
      
      success = nfc.mifareclassic_AuthenticateBlock(uid, uidLength, 12, 0, keya);
    
      if (success)
      {
        Serial.println("Sector 3 (Blocks 12..15) has been authenticated");
       // uint8_t data[16];
    
        // If you want to write something to block 4 to test with, uncomment
        // the following line and this text should be read back in a minute
        // data = { 'a', 'd', 'a', 'f', 'r', 'u', 'i', 't', '.', 'c', 'o', 'm', 0, 0, 0, 0};
        // success = nfc.mifareclassic_WriteDataBlock (4, data);

        // Try to read the contents of block 4
        success = nfc.mifareclassic_ReadDataBlock(12, data);
    
        if (success)
        {
          // Data seems to have been read ... spit it out
          Serial.println("Reading Block 12:");
          nfc.PrintHexChar(data, 16);
          Serial.println("");
      
          // Wait a bit before reading the card again
          delay(1000);
        }
        else
        {
          Serial.println("Ooops ... unable to read the requested block.  Try another key?");
        }
      }
      else
      {
        Serial.println("Ooops ... authentication failed: Try another key?");
      }
    }
    
    if (uidLength == 7)
    {
      // We probably have a Mifare Ultralight card ...
      Serial.println("Seems to be a Mifare Ultralight tag (7 byte UID)");
    
      // Try to read the first general-purpose user page (#4)
      Serial.println("Reading page 4");
      uint8_t data[32];
      success = nfc.mifareultralight_ReadPage (4, data);
      if (success)
      {
        // Data seems to have been read ... spit it out
        nfc.PrintHexChar(data, 4);
        Serial.println("");
    
        // Wait a bit before reading the card again
        delay(1000);
      }
      else
      {
        Serial.println("Ooops ... unable to read the requested page!?");
      }
    }
  }   
}


void sendBordnetz ()
{
  // standardbelegung 0 = Bordnetz ausgeschaltet
  bordnetz[0] = 0;
  for(i=0; i<16; i=i+1)
    if(data[i] != PWD[i])
  {
    Serial.println("bordnetz aus");
    bordnetz[0] = 0;
  }
  else
  {
    Serial.println("bordnetz an");
    bordnetz[0] = 1;
    data[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
  }
  CAN.sendMsgBuf(can_id_bordnetz, 0, 1, bordnetz);
  delay(100); 
}
