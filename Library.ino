#include <MFRC522.h>
#include <SPI.h>

#define RST_PIN 13
#define SS_1_PIN 12
#define SS_2_PIN 21

#define NUM_OF_READERS 2

byte ssPins[] = {SS_1_PIN, SS_2_PIN};

MFRC522 mfrc522[NUM_OF_READERS];

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  while(!Serial);

  SPI.begin();

  for(uint8_t reader = 0; reader < NUM_OF_READERS; reader++){
    mfrc522[reader].PCD_Init(ssPins[reader], RST_PIN);
    Serial.print(F("Reader "));
    Serial.print(reader);
    Serial.print(F(": "));

    mfrc522[reader].PCD_DumpVersionToSerial();
  }

}

void loop() {

  for (uint8_t reader = 0; reader < NUM_OF_READERS; reader++) {
    // Look for new cards

    if (mfrc522[reader].PICC_IsNewCardPresent() && mfrc522[reader].PICC_ReadCardSerial()) {
      Serial.print(F("Reader "));
      Serial.print(reader);
      // Show some details of the PICC (that is: the tag/card)
      Serial.print(F(": Card UID:"));
      dump_byte_array(mfrc522[reader].uid.uidByte, mfrc522[reader].uid.size);
      Serial.println();
      Serial.print(F("PICC type: "));
      MFRC522::PICC_Type piccType = mfrc522[reader].PICC_GetType(mfrc522[reader].uid.sak);
      Serial.println(mfrc522[reader].PICC_GetTypeName(piccType));

      // Halt PICC
      mfrc522[reader].PICC_HaltA();
      // Stop encryption on PCD
      mfrc522[reader].PCD_StopCrypto1();
    } //if (mfrc522[reader].PICC_IsNewC
  } //for(uint8_t reader
}

/**
 * Helper routine to dump a byte array as hex values to Serial.
 */
void dump_byte_array(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);
  }
}
