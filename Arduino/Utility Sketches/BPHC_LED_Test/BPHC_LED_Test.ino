#include "Arduino.h"

#define localControlLED  GPIO2
#define toggleButton     GPIO3
#define pumpControl      GPIO5

bool localControl = false;
bool toggleButtonFlag = false;
bool pumpOn = false;
bool localMessagePrinted = false;
bool remoteMessagePrinted = false;
uint32_t risingInterruptTime = 0;
uint32_t fallingInterruptTime = 0;
bool vextOn = false;
uint32_t currentTime = 0;
uint32_t previousTime = 0;
const uint32_t vextToggleInterval = 1000; // milliseconds
bool overrideTimerActive = false;
const uint32_t overrideTriggerInterval = 3000; // milliseconds

// Timers
static TimerEvent_t overrideTimer;

typedef enum {
  sequenceOne,  // LED test sequence
  sequenceTwo   // Local control test sequence
} tests;

tests testProgram;

void setup() {
//  testProgram = sequenceOne;
  testProgram = sequenceTwo;

  Serial.begin(115200);
  delay(500);
  uint64_t chipID=getID();
  Serial.printf("ChipID:%04X%08X\r\n",(uint32_t)(chipID>>32),(uint32_t)chipID);

  pinMode(Vext,OUTPUT);
  digitalWrite(Vext,HIGH);     // Turn the external power supply OFF
  pinMode(pumpControl,OUTPUT);
  digitalWrite(pumpControl,LOW); // Turn the pump OFF

// Local Override Timer

  TimerInit( &overrideTimer, setControlMode );
  Serial.println("[setup] Override timer initialised");
  TimerSetValue( &overrideTimer, overrideTriggerInterval );

  switch ( testProgram ) {
    case sequenceOne: {
      Serial.println("[setup] Test Sequence One");
      pinMode(localControlLED,OUTPUT);
      digitalWrite(localControlLED,LOW);     // Turn Activate LED OFF
      break;
    }
    case sequenceTwo: {
      Serial.println("[setup] Test Sequence Two");

      // Interrupt to toggle pump state

      PINMODE_INPUT_PULLDOWN( toggleButton );
      attachInterrupt( toggleButton, toggleButtonPressed, RISING );

      pinMode(localControlLED,OUTPUT);
      break;
    }
  }
}

void loop() {
  switch ( testProgram ) {
    case sequenceOne: {
      digitalWrite(Vext,LOW);     // Turn the external power supply ON
      Serial.println("[loop] Vext ON");
      delay(500);
      digitalWrite(Vext,HIGH);     // Turn the external power supply OFF
      Serial.println("[loop] Vext OFF");
      delay(1000);
      digitalWrite(Vext,LOW);     // Turn the external power supply ON
      Serial.println("[loop] Vext ON");
      delay(500);
      digitalWrite(Vext,HIGH);     // Turn the external power supply OFF
      Serial.println("[loop] Vext OFF");
      delay(1000);
      digitalWrite(localControlLED,HIGH);     // Turn the Activate LED ON
      Serial.println("[loop] Activate Local Control");
      delay(500);
      digitalWrite(pumpControl,HIGH);     // Turn the pump ON
      Serial.println("[loop] Pump ON");
      delay(2000);
      digitalWrite(pumpControl,LOW);     // Turn the pump OFF
      Serial.println("[loop] Pump OFF");
      delay(500);
      digitalWrite(localControlLED,LOW);     // Turn the Activate LED OFF
      Serial.println("[loop] Deactivate Local Control");
      delay(2000);
      break;
    }
    case sequenceTwo: {
      if ( localControl ) {
        if ( !localMessagePrinted ) {
          Serial.println("[loop] Local control...");
          localMessagePrinted = true;
          remoteMessagePrinted = false;
        }
        if ( toggleButtonFlag ) {
          Serial.println("[loop] Toggle pump state...");
          pumpOn = !pumpOn;
          if ( pumpOn ) {
            digitalWrite(pumpControl,HIGH);
            Serial.println("[loop] Pump ON");
          } else {
            digitalWrite(pumpControl,LOW);
            Serial.println("[loop] Pump OFF");
          }
          toggleButtonFlag = false;
        }
      } else {
        if ( !remoteMessagePrinted ) {
          Serial.println("[loop] Remote control...");
          remoteMessagePrinted = true;
          localMessagePrinted = false;
        }
      }
      if ( pumpOn ) {
        currentTime = millis();
        if ( currentTime - previousTime > vextToggleInterval ) {
          vextOn = !vextOn;
          if ( vextOn ) {
            digitalWrite(Vext,LOW);
          } else {
            digitalWrite(Vext,HIGH);
          }
          previousTime = currentTime;
        }
      } else {
        if ( vextOn ) {
          digitalWrite(Vext,HIGH);
          vextOn = false;
        }
      }
      break;
    }
    default: {
      break;
    }
  }
}

void toggleButtonPressed() {
  Serial.println("[toggleButtonPressed] RISING Interrupt...");
  risingInterruptTime = millis();
  startOverrideTimer();
  attachInterrupt( toggleButton, toggleButtonReleased, FALLING );
}

void toggleButtonReleased() {
  Serial.println("[toggleButtonReleased] FALLING Interrupt...");
  fallingInterruptTime = millis();
  Serial.print("[toggleButtonReleased] Elapsed time: ");
  Serial.println(fallingInterruptTime - risingInterruptTime);
  attachInterrupt( toggleButton, toggleButtonPressed, RISING );
  if ( fallingInterruptTime - risingInterruptTime > overrideTriggerInterval ) {
    Serial.println("[toggleButtonReleased] Trigger time elapsed...");
  } else {
    if ( overrideTimerActive ) {
      TimerStop( &overrideTimer );
      overrideTimerActive = false;
      Serial.println("[toggleButtonReleased] Stop override timer");
    }
    if ( localControl ) {
      toggleButtonFlag = true;
      Serial.println("[toggleButtonReleased] Local mode, toggle pump state...");
    } else {
      Serial.println("[toggleButtonReleased] Remote mode, toggle request ignored...");
    }
  }
}

void setControlMode() {
  if ( localControl ) {
    localControl = false;
    Serial.println("[setControlMode] Set Remote Control Mode");
    digitalWrite(localControlLED,LOW);     // Turn Local LED OFF
  } else {
    localControl = true;
    digitalWrite(localControlLED,HIGH);     // Turn Local LED ON
    Serial.println("[setControlMode] Set Local Control Mode");
  }
  overrideTimerActive = false;
}

void startOverrideTimer() {
  if ( overrideTimerActive ) {
    TimerReset( &overrideTimer );
    Serial.println("[startOverrideTimer] Override timer reset");
  } else {
    TimerStart( &overrideTimer );
    Serial.println("[startOverrideTimer] Override timer started");
    overrideTimerActive = true;
  }
}