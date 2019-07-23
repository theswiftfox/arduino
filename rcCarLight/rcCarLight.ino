#define IN_PIN_SWC 19 // light - highbeam
#define IN_PIN_SWB 3 // hazard light
#define IN_PIN_SWA 20 // turn left
#define IN_PIN_SWD 18 // turn right
#define IN_PIN_THROTTLE 21 // drive 

#define OUT_PIN_SWC_1 24 // light
#define OUT_PIN_SWC_2 27 // high-beam
#define OUT_PIN_SWD 23 // turn right
#define OUT_PIN_SWA 22 // turn left
#define OUT_PIN_THROTTLE_1 25 // drive back
#define OUT_PIN_THROTTLE_2 4 // break 

#define ulong unsigned long

#define NOISE 50u


uint16_t brightness_light = 24;
uint16_t brightness_break = 255;

struct SWC {
  uint16_t stateOff = 1000;
  uint16_t stateOne = 1500;
  uint16_t stateTwo = 2000;
} swc;

struct SWA {
  uint16_t stateOff = 1000;
  uint16_t stateOn = 2000;
}swa, swb, swd;

struct THROTTLE {
  uint16_t accelMin = 1000;
  uint16_t neutral = 1500;
  uint16_t breaking = 2000;
} throttle;

struct THROTTLE_STATE {
  uint16_t accel = 1;
  uint16_t neutral = 0;
  uint16_t breaking = 2;
} throttle_state;

volatile ulong startSWC;
volatile ulong pulseSWC;
volatile boolean newSignalSWC = false; // set in the interrupt and read in the loop

void signalSWC() {
    //record the interrupt time so that we can tell if the receiver has a signal from the transmitter 
    ulong lastInterruptTime = micros(); 
    //if the pin has gone HIGH, record the microseconds since the Arduino started up 
    if(digitalRead(IN_PIN_SWC) == HIGH) 
    { 
        startSWC = lastInterruptTime;
    } 
    //otherwise, the pin has gone LOW 
    else
    { 
        //only worry about this if the timer has actually started
        if(startSWC != 0)
        { 
            //record the pulse time
            pulseSWC = lastInterruptTime - startSWC;
            //restart the timer
            startSWC = 0;
            newSignalSWC = true;
        }
    } 
}

volatile ulong startThrottle;
volatile ulong pulseThrottle;
volatile boolean newSignalThrottle = false; // set in the interrupt and read in the loop

void signalThrottle() {
    //record the interrupt time so that we can tell if the receiver has a signal from the transmitter 
    ulong lastInterruptTime = micros(); 
    //if the pin has gone HIGH, record the microseconds since the Arduino started up 
    if(digitalRead(IN_PIN_THROTTLE) == HIGH) 
    { 
        startThrottle = lastInterruptTime;
    } 
    //otherwise, the pin has gone LOW 
    else
    { 
        //only worry about this if the timer has actually started
        if(startThrottle != 0)
        { 
            //record the pulse time
            pulseThrottle = lastInterruptTime - startThrottle;
            //restart the timer
            startThrottle = 0;
            newSignalThrottle = true;
        }
    } 
}

volatile ulong startSWA;
volatile ulong pulseSWA;
volatile boolean newSignalSWA = false; // set in the interrupt and read in the loop

void signalSWA() {
    //record the interrupt time so that we can tell if the receiver has a signal from the transmitter 
    ulong lastInterruptTime = micros(); 
    //if the pin has gone HIGH, record the microseconds since the Arduino started up 
    if(digitalRead(IN_PIN_SWA) == HIGH) 
    { 
        startSWA = lastInterruptTime;
    } 
    //otherwise, the pin has gone LOW 
    else
    { 
        //only worry about this if the timer has actually started
        if(startSWA != 0)
        { 
            //record the pulse time
            pulseSWA = lastInterruptTime - startSWA;
            //restart the timer
            startSWA = 0;
            newSignalSWA = true;
        }
    } 
}
volatile ulong startSWD;
volatile ulong pulseSWD;
volatile boolean newSignalSWD = false; // set in the interrupt and read in the loop

void signalSWD() {
    //record the interrupt time so that we can tell if the receiver has a signal from the transmitter 
    ulong lastInterruptTime = micros(); 
    //if the pin has gone HIGH, record the microseconds since the Arduino started up 
    if(digitalRead(IN_PIN_SWD) == HIGH) 
    { 
        startSWD = lastInterruptTime;
    } 
    //otherwise, the pin has gone LOW 
    else
    { 
        //only worry about this if the timer has actually started
        if(startSWD != 0)
        { 
            //record the pulse time
            pulseSWD = lastInterruptTime - startSWD;
            //restart the timer
            startSWD = 0;
            newSignalSWD = true;
        }
    } 
}

#define TURN_FREQ 2 // in Hz
#define ONE_SEC_IN_MICROS 1000000
ulong turnLightPeriod;
bool turnLightOn = false;
bool offCycle = false;

struct TurnLight {
  bool left = false;
  bool right = false;
} turnLightsState;

ulong lastTime;

bool checkFreq(ulong dt, uint16_t target) {
  if (dt > target - NOISE && dt < target + NOISE) {
    return true;
  }
  return false;
}

void resetTurnPeriod() {
  turnLightPeriod = ONE_SEC_IN_MICROS / TURN_FREQ;
}

void setup() {
    lastTime = 0;
    startSWC = 0;
    startThrottle = 0;
    resetTurnPeriod();
    attachInterrupt(digitalPinToInterrupt(IN_PIN_SWC), signalSWC, CHANGE);
    attachInterrupt(digitalPinToInterrupt(IN_PIN_SWA), signalSWA, CHANGE);
    attachInterrupt(digitalPinToInterrupt(IN_PIN_SWD), signalSWD, CHANGE);
    attachInterrupt(digitalPinToInterrupt(IN_PIN_THROTTLE), signalThrottle, CHANGE);
    pinMode(OUT_PIN_SWC_1, OUTPUT);
    pinMode(OUT_PIN_SWC_2, OUTPUT);
    pinMode(OUT_PIN_SWA, OUTPUT);
    pinMode(OUT_PIN_SWD, OUTPUT);
    pinMode(OUT_PIN_THROTTLE_1, OUTPUT);
    pinMode(OUT_PIN_THROTTLE_2, OUTPUT);
  //  Serial.begin(115200);
} 

uint16_t lastThrottleState = 0;
ulong lastThrottleStateChange = 0;
ulong timeDeltaBreaking = 100000; // 1s

void loop() {
    ulong timeNow = micros();
    ulong deltaT = lastTime > 0 ? timeNow - lastTime : timeNow;

    digitalWrite(OUT_PIN_THROTTLE_1, LOW);

    if (newSignalSWC) {
      ulong dt = pulseSWC;
      newSignalSWC = false;
      // SWC - Three Way Switch
      if (checkFreq(dt, swc.stateOff)) {
        digitalWrite(OUT_PIN_SWC_1, LOW);
        digitalWrite(OUT_PIN_SWC_2, LOW);
        analogWrite(OUT_PIN_THROTTLE_2, 0);
      } else if (checkFreq(dt, swc.stateOne)) {
        digitalWrite(OUT_PIN_SWC_1, HIGH);
        digitalWrite(OUT_PIN_SWC_2, LOW);
        analogWrite(OUT_PIN_THROTTLE_2, brightness_light);
      } else if (checkFreq(dt,swc.stateTwo)) {
        digitalWrite(OUT_PIN_SWC_2, HIGH);
        digitalWrite(OUT_PIN_SWC_1, HIGH);
        analogWrite(OUT_PIN_THROTTLE_2, brightness_light);
      }
    }

    if (newSignalSWA) {
      ulong dt = pulseSWA;
      newSignalSWA = false;

      if (checkFreq(dt, swa.stateOff)) {
        turnLightsState.left = false;
      } else if (checkFreq(dt, swa.stateOn)) {
        turnLightsState.left = true;
      } 
    }
    if (newSignalSWD) {
      ulong dt = pulseSWD;
      newSignalSWD = false;

      if (checkFreq(dt, swd.stateOff)) {
        turnLightsState.right = false;
      } else if (checkFreq(dt, swd.stateOn)) {
        turnLightsState.right = true;
      }
    }
    
    if (turnLightsState.left || turnLightsState.right) {
      if (turnLightPeriod > 0) {
          turnLightPeriod = turnLightPeriod > deltaT ? turnLightPeriod - deltaT : 0;
      }
      else {
        offCycle = !offCycle;
        resetTurnPeriod();
      }
    }
    if (turnLightsState.left && !offCycle) {
      digitalWrite(OUT_PIN_SWA, HIGH);
    } else {
      digitalWrite(OUT_PIN_SWA, LOW);
    }
    if (turnLightsState.right && !offCycle) {
      digitalWrite(OUT_PIN_SWD, HIGH);
    } else {
      digitalWrite(OUT_PIN_SWD, LOW);
    }
    
    if (newSignalThrottle) {
      ulong dt = pulseThrottle;
      newSignalSWC = false;
      if (dt < (throttle.neutral - NOISE)) {
        lastThrottleState = throttle_state.accel;
        lastThrottleStateChange = timeNow;
      } else if (checkFreq(dt, throttle.neutral)) {
        if (timeNow - lastThrottleStateChange >= timeDeltaBreaking) {
          lastThrottleState = throttle_state.neutral;
        }
      } else if (dt > (throttle.neutral + NOISE)) {
        if (lastThrottleState == throttle_state.neutral) {
          digitalWrite(OUT_PIN_THROTTLE_1, HIGH);
        } else if (lastThrottleState == throttle_state.accel) {
          analogWrite(OUT_PIN_THROTTLE_2, brightness_break);
        }
      }
    }
    lastTime = timeNow;
    delay(20);
}
