// General
// Log debug messages to Serial
const bool DEBUG = false;
// Delay between loops
const int DELAY = 25;
//const int SLEEP = 1000;
// LED to use for certain debug indicators
const int DEBUG_LED = 13;
// How long to wait before stopping sending slider and button info, 
// after last message was recieved
const unsigned long RX_TIMEOUT = 5000;


// Sliders Settings
const int NUM_SLIDERS = 4;
const int channelAnalogInputs[NUM_SLIDERS] = { A2, A3, A1, A0 };
const int channelButtonInputs[NUM_SLIDERS] = { 8, 4, 2, 7 };
const int channelLedOutputs[NUM_SLIDERS] = { 9, 5, 3, 6 };

// MIN & MAX lighting levels
const int MIN = 0;
const int MAX = 10;

// How many milliseconds does one lighting cycle last
const int LIGHT_CYCLE = 1600;
// How many times in a cycle should lighting be updated - will affect fade smoothness
// and steps of Pulse mode LED's
const int LIGHT_STEPS = 10;  // Shouldn't be smaller than MAX
// At what level between 0 and 1 in the cycle does the flash toggle state
const float FLASH_TRIGGER = 0.5;

// Current cycle / brighness level
float cycleLevel = 0;

// PULSE MODE:  Fading in and out / breathing with brighness scaling linearly
//              up and down in one cycle
// Current number of LEDs on Pulse mode
int pulseLeds = 0;
// current Pulse mode brightness level
int pulseLevel = 0;

// FLASH MODE:  Turns LED's on when cycleLevel reaches FLASH_TRIGGER as it
//              increases, then turns back of when cycleLevel falls below
//              FLASH_TRIGGER level
// Current number of LEDs on Flash mode
int flashLeds = 0;
// current Flash mode brightness level (Either MIN or MAX)
int flashLevel = 0;

// When next lighting changes should be applied
unsigned long nextLightChange = 0;
// last checked milliseconds since boot
unsigned long currentMillis = 0;
// last received message time
unsigned long lastMessage = 0;


// Current Slider Values
int channelSliderValues[NUM_SLIDERS];
// Current Button States
int buttonCurrentState[NUM_SLIDERS];
// Current LED Lighting State
int channelLedLighting[NUM_SLIDERS] = { 0, 0, 0, 0 };
// 0 - Off:   No Sessions bound to the computer side mapping
// 1 - On:    Mapping is Master or has sessions, and no sessions are muted
// 2 - Pulse: Mapping is Master or has sessions, ans all sessions muted
// 3 - Flash: Mapping has multiple sessions, but they are mixed muted / unmuted

// Variables to store incoming LED messages
String incomingLEDState;
int ledIdx;
int ledLighting;
int lastLighting;
int seperatorPos;

// Variables to store button state readings
int reading;
int tempVal;
bool active;

void setup() {
  // Initialise PINs
  // Debug LED
  pinMode(DEBUG_LED, OUTPUT);

  // Analog inputs
  for (int i = 0; i < NUM_SLIDERS; i++) {
    pinMode(channelAnalogInputs[i], INPUT);
  }

  // Button inputs
  for (int i = 0; i < NUM_SLIDERS; i++) {
    pinMode(channelButtonInputs[i], INPUT_PULLUP);
  }

  // LEDs using PWM to control brightness
  for (int i = 0; i < NUM_SLIDERS; i++) {
    pinMode(channelLedOutputs[i], OUTPUT);
  }

  // Ready flash
  delay(240);
  for (int j = 0; j < 2; j++) {
    for (int i = 0; i < NUM_SLIDERS; i++) {
      analogWrite(channelLedOutputs[i], MAX * 0.4);
    }
    delay(240);
    for (int i = NUM_SLIDERS - 1; i >= 0; i--) {
      analogWrite(channelLedOutputs[i], MIN);
    }
    delay(200);
  }

  // Start Serial comms
  Serial.begin(9600);
}

void loop() {

  // Read available incoming data
  if (Serial.available() > 0) {
    readLedInputs();
  }

  // Dont send if we haven't heard from the computer in RX_TIMEOUT sec
  if (lastMessage + RX_TIMEOUT > millis()) {

    if ((pulseLeds > 0 || flashLeds > 0) && (nextLightChange < millis())) {
      updateLedStates();
    }

    updateSliderValues();
    updateButtonValues();
    sendSliderValues();  // Actually send data (all the time)
    //printSliderValues(); // For debug

    // Wait until next loop time
    delay(DELAY);
  } else {
    // Display 'waiting' pattern to indicate no data is being received from PC
    for (int i = 0; i < NUM_SLIDERS - 1; i++) {
      analogWrite(channelLedOutputs[i], MIN);
    }

    for (int i = 0; i < NUM_SLIDERS - 1; i++) {
      analogWrite(channelLedOutputs[i], MAX * 0.2);
      delay(400);
      analogWrite(channelLedOutputs[i], MIN);
    }
    for (int i = NUM_SLIDERS - 1; i >= 1; i--) {
      analogWrite(channelLedOutputs[i], MAX * 0.2);
      delay(400);
      analogWrite(channelLedOutputs[i], MIN);
    }
    //delay(SLEEP);
  }
}

void updateSliderValues() {
  // for each slider
  for (int i = 0; i < NUM_SLIDERS; i++) {
    if (channelAnalogInputs[i] == 0) {
      channelSliderValues[i] = 0;
    } else {
      channelSliderValues[i] = analogRead(channelAnalogInputs[i]);
    }
  }
}

void updateButtonValues() {
  // Loop through each button
  for (int i = 0; i < NUM_SLIDERS; i++) {
    buttonCurrentState[i] = digitalRead(channelButtonInputs[i]);
  }
}

void sendSliderValues() {

  String builtString = String("");

  // Buttons linked to sliders
  for (int i = 0; i < NUM_SLIDERS; i++) {
    int tempVal = (int)channelSliderValues[i];
    if (NUM_SLIDERS > i) {
      if (buttonCurrentState[i] == LOW) {  // Use LOW for INPUT_PULLUP
        tempVal = tempVal + 1024;          // Set byte 11 to 1
      }
    }

    builtString += String((int)tempVal);

    if (i < NUM_SLIDERS - 1) {
      builtString += String("|");
    }
  }

  builtString += (String());

  Serial.println(builtString);
}


void readLedInputs() {

  digitalWrite(DEBUG_LED, HIGH);

  // Read all the waiting messages, else we only read one message per loop
  while (Serial.available() > 0) {

    lastMessage = millis();
    incomingLEDState = Serial.readStringUntil('\n');
    seperatorPos = incomingLEDState.indexOf('|');
    ledIdx = incomingLEDState.substring(0, seperatorPos).toInt();

    // Ignore out of range sliders
    if (ledIdx < 0 || ledIdx >= NUM_SLIDERS) {
      return;
    }

    ledLighting = incomingLEDState.substring(seperatorPos + 1).toInt();

    // Limit to valid values
    if (ledLighting < 0) {
      ledLighting = 0;
    } else if (ledLighting > 3) {
      ledLighting = 1;
    }

    // Determine if LED Lighting state has changed from last value
    lastLighting = channelLedLighting[ledIdx];
    if (ledLighting != lastLighting) {

      debug(String("LED CHANGE: ") + String(ledIdx) + String(" = ") + String(channelLedLighting[ledIdx]) + String(" -> ") + String(ledLighting));

      // Decrement counts for dynamic lights
      if (lastLighting == 2) {
        pulseLeds--;
        if (pulseLeds < 0) {
          pulseLeds = 0;
        }
      } else if (lastLighting == 3) {
        flashLeds--;
        if (flashLeds < 0) {
          flashLeds = 0;
        }
      }

      // Set static lighting
      if (ledLighting == 0) {
        analogWrite(channelLedOutputs[ledIdx], MIN);
      } else if (ledLighting == 1) {
        analogWrite(channelLedOutputs[ledIdx], MAX);
      }
      // Set Dynamic lighting
      else if (ledLighting == 2) {
        pulseLeds++;
        if (pulseLeds > NUM_SLIDERS) {
          pulseLeds = NUM_SLIDERS;
        }
      } else if (ledLighting == 3) {
        flashLeds++;
        if (flashLeds > NUM_SLIDERS) {
          flashLeds = NUM_SLIDERS;
        }
      }
    }

    channelLedLighting[ledIdx] = ledLighting;
  }

  digitalWrite(DEBUG_LED, LOW);
}

void updateLedStates() {

  debug(String("updateLedStates"));

  for (int i = 0; i < NUM_SLIDERS; i++) {
    debug(String("LED STATUS: ") + String(i) + String(" = ") + String(channelLedLighting[i]));
  }

  currentMillis = millis();

  // Cycle climbs from 0 to max haflway through cycle, then down again, linearly
  // Get current cycle level as if scaled once to max at the end of the cycle
  cycleLevel = (currentMillis % LIGHT_CYCLE) / (float)LIGHT_CYCLE;
  debug(String("LED CL B: ") + String(cycleLevel));

  // Double the scale of the level so that it peaks halway through
  cycleLevel = cycleLevel * 2;
  debug(String("LED CL S: ") + String(cycleLevel));

  // Flip the value going over the max back down
  if (cycleLevel > 1) {
    cycleLevel = cycleLevel - (cycleLevel - 1) * 2;
  }
  debug(String("LED CL F: ") + String(cycleLevel));

  if (pulseLeds > 0) {
    // Set pulse brightness level relative to MAX
    pulseLevel = MAX * cycleLevel;

    for (int i = 0; i < NUM_SLIDERS; i++) {
      if (channelLedLighting[i] == 2) {
        analogWrite(channelLedOutputs[i], pulseLevel);
      }
    }
  }

  if (flashLeds > 0) {

    // Toggle between flash states
    if (cycleLevel >= FLASH_TRIGGER && flashLevel == MIN) {

      flashLevel = MAX;
      for (int i = 0; i < NUM_SLIDERS; i++) {
        if (channelLedLighting[i] == 3) {
          analogWrite(channelLedOutputs[i], flashLevel);
        }
      }

    } else if (cycleLevel < FLASH_TRIGGER && flashLevel == MAX) {

      flashLevel = MIN;
      for (int i = 0; i < NUM_SLIDERS; i++) {
        if (channelLedLighting[i] == 3) {
          analogWrite(channelLedOutputs[i], flashLevel);
        }
      }

    }
  }

  nextLightChange = currentMillis + (LIGHT_CYCLE / LIGHT_STEPS);
}

// Send Debug info over the Serial interface
void debug(String s) {
  if (!DEBUG) {
    return;
  }

  String debugMessage = String("DEBUG: ") + s;
  Serial.write(debugMessage.c_str());
  Serial.write("\n");
}

void printSliderValues() {
  for (int i = 0; i < NUM_SLIDERS; i++) {
    tempVal = channelSliderValues[i];
    if (tempVal >= 1024) {
      String printedString = String("Slider #") + String(i + 1) + String(": ") + String(tempVal - 1024) + String(" mV ") + String("DOWN");
      Serial.write(printedString.c_str());
    } else {
      String printedString = String("Slider #") + String(i + 1) + String(": ") + String(tempVal) + String(" mV ") + String("UP");
      Serial.write(printedString.c_str());
    }

    if (i < NUM_SLIDERS - 1) {
      Serial.write(" | ");
    } else {
      Serial.write("\n");
    }
  }
}