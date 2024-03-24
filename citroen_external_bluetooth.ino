/*
  Sources:

  Tuner:        0x16
  CD:           0x32
  CD Changer:   0x48
  AUX 1:        0x64
  AUX 2:        0x80
  USB:          0x96
  Bluetooth:    0x112
*/

#include <mcp_can.h>
#include <SPI.h>

struct BUTTON {
  unsigned long id;
  int byteNum;
  byte byteValue;
  int pin;
  unsigned long pressedOn;
};

struct SCROLL {
  unsigned long id;
  byte position;
  int byteNum;
  int up;
  int down;
  unsigned long pressedOn;
};

struct SOURCE {
  unsigned long id;
  int byteNum;
  byte *byteValue;
};

byte allowedSources[] = {0x64, 0x80};
SOURCE ALLOWED_SOURCES = {0x165, 2, allowedSources};
int sourcesCount = sizeof(allowedSources) / sizeof(byte);

SCROLL SCROLL;//             = {0x0A2, NULL, 0, NULL, NULL};

BUTTON LIST               = {0x21F, 0, 0x01, NULL};
BUTTON VOL_UP             = {0x21F, 0, 0x08, NULL};
BUTTON VOL_DOWN           = {0x21F, 0, 0x04, NULL};
BUTTON MUTE               = {0x21F, 0, 0x0C, NULL};
BUTTON NEXT               = {0x21F, 0, 0x40, 3};
BUTTON PREVIUOS           = {0x21F, 0, 0x80, 4};

BUTTON SOURCE             = {0x0A2, 1, 0x04, NULL};
BUTTON BACK               = {0x0A2, 1, 0x10, NULL};
BUTTON HOME               = {0x0A2, 1, 0x08, NULL};
BUTTON SCROLL_PRESSED     = {0x0A2, 1, 0xA0, 5};
BUTTON PHONE              = {0x0A2, 2, 0x80, NULL};

BUTTON VOICE_ASSIST       = {0x221, 0, 0x01, NULL};
BUTTON NIGHT_MODE         = {0x036, 3, 0x36, NULL};

BUTTON WHEEL_BUTTON[] = {
  // VOL_UP, 
  // VOL_DOWN, 
  // MUTE, 
  NEXT, 
  PREVIUOS,
  // BACK, 
  // HOME, 
  SCROLL_PRESSED
  // PHONE, 
  // NIGHT_MODE,
  // LIST,
  // SOURCE, 
  // VOICE_ASSIST
};

int btnReleaseDelay = 150;
bool isAllowedSourceSelected = false;

int buttonsCount = sizeof(WHEEL_BUTTON) / sizeof(BUTTON);
int pressedPin = 0;

// can package data
unsigned long rxId;
byte len;
byte rxBuf[8];

MCP_CAN CAN0(10);                // CAN0 interface usins CS on digital pin 2

#define CAN0_INT 2               //define interrupt pin for CAN0 recieve buffer

void setup(){
  Serial.begin(115200);

  pinMode(CAN0_INT, INPUT);

  // init CAN0 bus, baudrate: 125k@8MHz
  if(CAN0.begin(MCP_ANY, CAN_125KBPS, MCP_8MHZ) == CAN_OK){
    Serial.print("CAN0: Init OK!\r\n");
    CAN0.setMode(MCP_NORMAL);
  } else Serial.print("CAN0: Init Fail!!!\r\n");
}

void processKey(){
  if (isAllowedSourceSelected == false){
    return;
  }

  for (int i = 0; i < buttonsCount; i++){
    if (rxId == WHEEL_BUTTON[i].id && rxBuf[WHEEL_BUTTON[i].byteNum] & WHEEL_BUTTON[i].byteValue){
      pressKey(WHEEL_BUTTON[i].pin);
      WHEEL_BUTTON[i].pressedOn = millis();
    }
  }

  if (rxId == SCROLL.id){
    if (!SCROLL.position){
      SCROLL.position = rxBuf[SCROLL.byteNum];
    }

    if (SCROLL.position != rxBuf[SCROLL.byteNum]){
      if (rxBuf[SCROLL.byteNum] > SCROLL.position){
        pressKey(SCROLL.up);
      } else {
        pressKey(SCROLL.down);
      }
      SCROLL.pressedOn = millis();
      SCROLL.position = rxBuf[SCROLL.byteNum];
    }
  }
}

void pressKey(int pin){
  digitalWrite(pin, HIGH);
  
  // Serial.println("pressed key: " + String(pin));
}

void releaseKey(){
  for (int i = 0; i < buttonsCount; i++){
    if (millis() - WHEEL_BUTTON[i].pressedOn > btnReleaseDelay && WHEEL_BUTTON[i].pressedOn > 0){
      digitalWrite(WHEEL_BUTTON[i].pin, LOW);
      WHEEL_BUTTON[i].pressedOn = 0;

      // Serial.println("released key: " + String(WHEEL_BUTTON[i].pin));
    }
  }

  if (millis() - SCROLL.pressedOn > btnReleaseDelay && SCROLL.pressedOn > 0){
    digitalWrite(SCROLL.up, LOW);
    digitalWrite(SCROLL.down, LOW);
    SCROLL.pressedOn = 0;

    // Serial.println("released key: " + String(SCROLL.up));
    // Serial.println("released key: " + String(SCROLL.down));
  }
}

void checkSource(){
  if (rxId == ALLOWED_SOURCES.id){
    for (int i = 0; i < sourcesCount; i++){
      if (rxBuf[ALLOWED_SOURCES.byteNum] == ALLOWED_SOURCES.byteValue[i]){
        isAllowedSourceSelected = true;

        // Serial.println("selected source: " + String(ALLOWED_SOURCES.byteValue[i], HEX) + ", allowed");
        return;
      }
    }
    isAllowedSourceSelected = false;

    // Serial.println("selected source: " + String(rxBuf[ALLOWED_SOURCES.byteNum], HEX) + ", not allowed");
  }
}

void loop(){
  if(!digitalRead(CAN0_INT) && CAN0.readMsgBuf(&rxId, &len, rxBuf) == CAN_OK){

    // Serial.println("received: " + String(rxId, HEX));

    checkSource();
    processKey();
  }
  releaseKey();
}