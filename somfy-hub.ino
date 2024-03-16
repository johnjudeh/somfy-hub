/*   This sketch allows you to emulate a Somfy RTS or Simu HZ remote.
   If you want to learn more about the Somfy RTS protocol, check out https://pushstack.wordpress.com/somfy-rts-protocol/
   
   The rolling code will be stored in EEPROM, so that you can power the Arduino off.
   
   Easiest way to make it work for you:
    - Choose a remote number
    - Choose a starting point for the rolling code. Any unsigned int works, 1 is a good start
    - Upload the sketch
    - Long-press the program button of YOUR ACTUAL REMOTE until your blind goes up and down slightly
    - send 'p' to the serial terminal
  To make a group command, just repeat the last two steps with another blind (one by one)
  
  Then:
    - m, u or h will make it to go up
    - s make it stop
    - b, or d will make it to go down
    - you can also send a HEX number directly for any weird command you (0x9 for the sun and wind detector for instance)
*/

#include <EEPROM.h>
#define RF_DATA_OUT_PIN 13

#define BTN_UP_PIN 12
#define BTN_STOP_PIN 14
#define BTN_DOWN_PIN 27

#define SYMBOL 640
#define UP 0x2
#define STOP 0x1
#define DOWN 0x4
#define PROG 0x8

#define EEPROM_ADDR_ROLLING_CODE 0
#define EEPROM_ADDR_PROGRAMMED 2
#define EEPROM_TOTAL_MEMORY 3

#define REMOTE 0x121303    //<-- Change it!

uint16_t rollingCode;
bool programmed;

byte frame[7];
byte checksum;

void BuildFrame(byte *frame, byte button);
void SendCommand(byte *frame, byte sync);


void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("");
  
  pinMode(RF_DATA_OUT_PIN, OUTPUT);
  digitalWrite(RF_DATA_OUT_PIN, LOW);
  
  pinMode(BTN_UP_PIN, INPUT);
  digitalWrite(BTN_UP_PIN, LOW);
  
  pinMode(BTN_STOP_PIN, INPUT);
  digitalWrite(BTN_STOP_PIN, LOW);
  
  pinMode(BTN_DOWN_PIN, INPUT);
  digitalWrite(BTN_DOWN_PIN, LOW);
  
  if (EEPROM.begin(EEPROM_TOTAL_MEMORY)) {
    Serial.println("EEPROM initialised");
  }

  // Fetch non-volatile values from EEPROM
  EEPROM.get(EEPROM_ADDR_ROLLING_CODE, rollingCode);
  if (EEPROM.read(EEPROM_ADDR_PROGRAMMED) == 0xFF) {
    programmed = false;
  } else {
    EEPROM.get(EEPROM_ADDR_PROGRAMMED, programmed);
  }
  Serial.print("Initial rolling code    : "); Serial.println(rollingCode);
  Serial.print("Programmed              : "); Serial.println(programmed);

  if (!programmed) {
    Serial.println("Resetting rolling code to 0");
    rollingCode = 0;
    EEPROM.put(EEPROM_ADDR_ROLLING_CODE, rollingCode);
    EEPROM.commit();
  }
  
  Serial.print("Simulated remote number : "); Serial.println(REMOTE, HEX);
  Serial.print("Current rolling code    : "); Serial.println(rollingCode);
}

void loop() {
  byte btnUp = digitalRead(BTN_UP_PIN);
  byte btnStop = digitalRead(BTN_STOP_PIN);
  byte btnDown = digitalRead(BTN_DOWN_PIN);

  bool btnActivated = btnUp == HIGH || btnStop == HIGH || btnDown == HIGH;
  
  char serie = '\0';
  
  if (Serial.available() > 0) {
    serie = (char)Serial.read();
    Serial.println("");
    Serial.print("serie = "); Serial.println(serie);
  }

  if (serie != '\0' || btnActivated) {
    if(serie == 'm'||serie == 'u'||serie == 'h' || btnUp == HIGH) {
      Serial.println("Monte"); // Somfy is a French company, after all.
      BuildFrame(frame, UP);
    }
    else if(serie == 's' || btnStop == HIGH) {
      Serial.println("Stop");
      BuildFrame(frame, STOP);
    }
    else if(serie == 'b'||serie == 'd' || btnDown == HIGH) {
      Serial.println("Descend");
      BuildFrame(frame, DOWN);
    }
    else if(serie == 'p') {
      Serial.println("Prog");
      BuildFrame(frame, PROG);
    }
    else {
      Serial.println("Custom code");
      BuildFrame(frame, serie);
    }
  
    Serial.println("");
    SendCommand(frame, 2);
    for(int i = 0; i<2; i++) {
      SendCommand(frame, 7);
    }
  
    if(serie == 'p' and !programmed) {
      programmed = true;
      EEPROM.put(EEPROM_ADDR_PROGRAMMED, programmed);
      EEPROM.commit();
    }
  }
}


void BuildFrame(byte *frame, byte button) {
  frame[0] = 0xA7;             // Encryption key. Doesn't matter much
  frame[1] = button << 4;      // Which button did  you press? The 4 LSB will be the checksum
  frame[2] = rollingCode >> 8; // Rolling code (big endian)
  frame[3] = rollingCode;      // Rolling code
  frame[4] = REMOTE >> 16;     // Remote address
  frame[5] = REMOTE >>  8;     // Remote address
  frame[6] = REMOTE;           // Remote address

  Serial.print("Frame         : ");
  for(byte i = 0; i < 7; i++) {
    if(frame[i] >> 4 == 0) { //  Displays leading zero in case the most significant
      Serial.print("0");     // nibble is a 0.
    }
    Serial.print(frame[i],HEX); Serial.print(" ");
  }
  
// Checksum calculation: a XOR of all the nibbles
  checksum = 0;
  for(byte i = 0; i < 7; i++) {
    checksum = checksum ^ frame[i] ^ (frame[i] >> 4);
  }
  checksum &= 0b1111; // We keep the last 4 bits only


//Checksum integration
  frame[1] |= checksum; //  If a XOR of all the nibbles is equal to 0, the blinds will
                        // consider the checksum ok.

  Serial.println(""); Serial.print("With checksum : ");
  for(byte i = 0; i < 7; i++) {
    if(frame[i] >> 4 == 0) {
      Serial.print("0");
    }
    Serial.print(frame[i],HEX); Serial.print(" ");
  }

  
// Obfuscation: a XOR of all the bytes
  for(byte i = 1; i < 7; i++) {
    frame[i] ^= frame[i-1];
  }

  Serial.println(""); Serial.print("Obfuscated    : ");
  for(byte i = 0; i < 7; i++) {
    if(frame[i] >> 4 == 0) {
      Serial.print("0");
    }
    Serial.print(frame[i],HEX); Serial.print(" ");
  }
  Serial.println("");
  Serial.print("Rolling Code  : "); Serial.println(rollingCode);
  
  //  We store the value of the rolling code in the
  // EEPROM. It should take up to 2 adresses but the
  // Arduino function takes care of it.
  rollingCode += 1;
  EEPROM.put(EEPROM_ADDR_ROLLING_CODE, rollingCode);
  EEPROM.commit();
}



void SendCommand(byte *frame, byte sync) {
  if(sync == 2) { // Only with the first frame.
  //Wake-up pulse & Silence
    digitalWrite(RF_DATA_OUT_PIN, HIGH);
    delayMicroseconds(9415);
    digitalWrite(RF_DATA_OUT_PIN, LOW);
    delayMicroseconds(89565);
  }

// Hardware sync: two sync for the first frame, seven for the following ones.
  for (int i = 0; i < sync; i++) {
    digitalWrite(RF_DATA_OUT_PIN, HIGH);
    delayMicroseconds(4*SYMBOL);
    digitalWrite(RF_DATA_OUT_PIN, LOW);
    delayMicroseconds(4*SYMBOL);
  }

// Software sync
  digitalWrite(RF_DATA_OUT_PIN, HIGH);
  delayMicroseconds(4550);
  digitalWrite(RF_DATA_OUT_PIN, LOW);
  delayMicroseconds(SYMBOL);
  
  
//Data: bits are sent one by one, starting with the MSB.
  for(byte i = 0; i < 56; i++) {
    if(((frame[i/8] >> (7 - (i%8))) & 1) == 1) {
      digitalWrite(RF_DATA_OUT_PIN, LOW);
      delayMicroseconds(SYMBOL);
      digitalWrite(RF_DATA_OUT_PIN, HIGH);
      delayMicroseconds(SYMBOL);
    }
    else {
      digitalWrite(RF_DATA_OUT_PIN, HIGH);
      delayMicroseconds(SYMBOL);
      digitalWrite(RF_DATA_OUT_PIN, LOW);
      delayMicroseconds(SYMBOL);
    }
  }
  
  digitalWrite(RF_DATA_OUT_PIN, LOW);
  delayMicroseconds(30415); // Inter-frame silence
}