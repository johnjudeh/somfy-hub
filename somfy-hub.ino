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

// User config - update me
#define RF_DATA_OUT_PIN 13

#define BTN_UP_PIN 12
#define BTN_STOP_PIN 14
#define BTN_DOWN_PIN 27
#define BTN_CYCLE_PIN 26
#define BTN_PROG_PIN 25

#define REMOTE_ID_START 0x121303

const int REMOTE_COUNT = 5;
// End of config

#define SYMBOL 640
#define UP 0x2
#define STOP 0x1
#define DOWN 0x4
#define PROG 0x8

#define EEPROM_BLANK_VAL 0xFF
#define EEPROM_START_ADDR 0

struct Remote {
  bool programmed;
  uint32_t id;
  uint16_t rollingCode;
} typedef Remote;

struct Hub {
  bool initialised;
  int remote_index;
  Remote remotes[REMOTE_COUNT];
} typedef Hub;

Hub somfy_hub;

byte frame[7];
bool btn_was_active = false;

void init_hub(Hub* hub);
void print_hub(Hub hub);
void init_eeprom(Hub* hub);
void program_remote(Hub* hub);
void build_frame(byte* frame, Hub* hub, byte command);
void send_command(byte* frame, byte sync);

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("");

  pinMode(RF_DATA_OUT_PIN, OUTPUT);
  digitalWrite(RF_DATA_OUT_PIN, LOW);

  pinMode(BTN_UP_PIN, INPUT);
  pinMode(BTN_STOP_PIN, INPUT);
  pinMode(BTN_DOWN_PIN, INPUT);
  pinMode(BTN_CYCLE_PIN, INPUT);
  pinMode(BTN_PROG_PIN, INPUT);

  init_eeprom(&somfy_hub);
}

void loop() {
  byte btn_up = digitalRead(BTN_UP_PIN);
  byte btn_stop = digitalRead(BTN_STOP_PIN);
  byte btn_down = digitalRead(BTN_DOWN_PIN);
  byte btn_cycle = digitalRead(BTN_CYCLE_PIN);
  byte btn_prog = digitalRead(BTN_PROG_PIN);

  bool btn_is_active = btn_up == HIGH || btn_stop == HIGH || btn_down == HIGH || btn_cycle == HIGH || btn_prog == HIGH;

  char serial_cmd = '\0';

  if (Serial.available() > 0) {
    serial_cmd = (char)Serial.read();
    Serial.println("");
    Serial.print("serie = "); Serial.println(serial_cmd);
  }

  if (serial_cmd != '\0' || btn_is_active) {
    if (serial_cmd == 'm' || serial_cmd == 'u' || serial_cmd == 'h' || btn_up == HIGH) {
      Serial.println("Monte"); // Somfy is a French company, after all.
      build_frame(frame, &somfy_hub, UP);
    }
    else if (serial_cmd == 's' || btn_stop == HIGH) {
      Serial.println("Stop");
      build_frame(frame, &somfy_hub, STOP);
    }
    else if (serial_cmd == 'b' || serial_cmd == 'd' || btn_down == HIGH) {
      Serial.println("Descend");
      build_frame(frame, &somfy_hub, DOWN);
    }
    else if (serial_cmd == 'p' || btn_prog == HIGH) {
      Serial.println("Prog");
      build_frame(frame, &somfy_hub, PROG);
    }
    else if (btn_cycle) {
      somfy_hub.remote_index = (somfy_hub.remote_index + 1) % REMOTE_COUNT;
      Serial.printf("Remote index = %d\n", somfy_hub.remote_index);
      EEPROM.put(EEPROM_START_ADDR, somfy_hub);
      EEPROM.commit();
    }
    else {
      Serial.println("Custom code");
      build_frame(frame, &somfy_hub, serial_cmd);
    }

    Serial.println("");
    if (!btn_was_active) {
      // The first frame is sent differently to the rest
      send_command(frame, 2);
    }
    for (int i = 0; i < 2; i++) {
      send_command(frame, 7);
    }

    if (serial_cmd == 'p') {
      program_remote(&somfy_hub);
    }
  }

  // We set this so the next loop can check
  btn_was_active = btn_is_active;
}

void init_hub(Hub* hub) {
  EEPROM.get(EEPROM_START_ADDR, *hub);

  if (EEPROM.read(EEPROM_START_ADDR) == EEPROM_BLANK_VAL) {
    Serial.println("Initialising hub from blank EEPROM");
    for (int i = 0; i < REMOTE_COUNT; i++) {
      Remote* premote = &hub->remotes[i];
      premote->id = REMOTE_ID_START + i;
      premote->rollingCode = 0;
      premote->programmed = false;
    }
    hub->initialised = true;
    hub->remote_index = 0;
    EEPROM.put(EEPROM_START_ADDR, *hub);
    EEPROM.commit();
  }
}

void print_hub(Hub hub) {
  Serial.printf("initialised = %s\n", hub.initialised ? "true" : "false");
  Serial.printf("remote_index = %d\n", hub.remote_index);
  Serial.println("remotes");
  for (int i = 0; i < REMOTE_COUNT; i++) {
    Remote remote = hub.remotes[i];
    Serial.printf("|__remote %d:", i + 1);
    Serial.printf("   id: %x; rollingCode: %d; programmed: %d\n", remote.id, remote.rollingCode, remote.programmed);
  }
  Serial.println("");
}

void init_eeprom(Hub* hub) {
  if (EEPROM.begin(sizeof(somfy_hub))) {
    Serial.println("EEPROM initialised");
    init_hub(&somfy_hub);
    print_hub(somfy_hub);
  }
}

void program_remote(Hub* hub) {
  Remote* premote = &hub->remotes[hub->remote_index];
  if (premote->programmed) {
    return;
  }
  // Ideally send signal here
  premote->programmed = true;
  EEPROM.put(EEPROM_START_ADDR, *hub);
  EEPROM.commit();
}

void build_frame(byte* frame, Hub* hub, byte command) {
  Remote* premote = &hub->remotes[hub->remote_index];

  frame[0] = 0xA7;                      // Encryption key. Doesn't matter much
  frame[1] = command << 4;              // Which button did  you press? The 4 LSB will be the checksum
  frame[2] = premote->rollingCode >> 8; // Rolling code (big endian)
  frame[3] = premote->rollingCode;      // Rolling code
  frame[4] = premote->id >> 16;         // Remote address
  frame[5] = premote->id >> 8;          // Remote address
  frame[6] = premote->id;               // Remote address

  Serial.print("Frame         : ");
  for (byte i = 0; i < 7; i++) {
    if (frame[i] >> 4 == 0) { //  Displays leading zero in case the most significant
      Serial.print("0");     // nibble is a 0.
    }
    Serial.print(frame[i], HEX); Serial.print(" ");
  }

  // Checksum calculation: a XOR of all the nibbles
  byte checksum = 0;
  for (byte i = 0; i < 7; i++) {
    checksum = checksum ^ frame[i] ^ (frame[i] >> 4);
  }
  checksum &= 0b1111; // We keep the last 4 bits only


  //Checksum integration
  frame[1] |= checksum; //  If a XOR of all the nibbles is equal to 0, the blinds will
  // consider the checksum ok.

  Serial.println(""); Serial.print("With checksum : ");
  for (byte i = 0; i < 7; i++) {
    if (frame[i] >> 4 == 0) {
      Serial.print("0");
    }
    Serial.print(frame[i], HEX); Serial.print(" ");
  }

  // Obfuscation: a XOR of all the bytes
  for (byte i = 1; i < 7; i++) {
    frame[i] ^= frame[i - 1];
  }

  Serial.println(""); Serial.print("Obfuscated    : ");
  for (byte i = 0; i < 7; i++) {
    if (frame[i] >> 4 == 0) {
      Serial.print("0");
    }
    Serial.print(frame[i], HEX); Serial.print(" ");
  }
  Serial.println("");
  Serial.print("Rolling Code  : "); Serial.println(premote->rollingCode);

  premote->rollingCode += 1;
  EEPROM.put(EEPROM_START_ADDR, *hub);
  EEPROM.commit();
}

void send_command(byte* frame, byte sync) {
  if (sync == 2) { // Only with the first frame.
    //Wake-up pulse & Silence
    digitalWrite(RF_DATA_OUT_PIN, HIGH);
    delayMicroseconds(9415);
    digitalWrite(RF_DATA_OUT_PIN, LOW);
    delayMicroseconds(89565);
  }

  // Hardware sync: two sync for the first frame, seven for the following ones.
  for (int i = 0; i < sync; i++) {
    digitalWrite(RF_DATA_OUT_PIN, HIGH);
    delayMicroseconds(4 * SYMBOL);
    digitalWrite(RF_DATA_OUT_PIN, LOW);
    delayMicroseconds(4 * SYMBOL);
  }

  // Software sync
  digitalWrite(RF_DATA_OUT_PIN, HIGH);
  delayMicroseconds(4550);
  digitalWrite(RF_DATA_OUT_PIN, LOW);
  delayMicroseconds(SYMBOL);


  //Data: bits are sent one by one, starting with the MSB.
  for (byte i = 0; i < 56; i++) {
    if (((frame[i / 8] >> (7 - (i % 8))) & 1) == 1) {
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