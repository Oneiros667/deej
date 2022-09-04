// General
const int DELAY = 100;

// Sliders Settings
const int NUM_SLIDERS = 5;
const int channelAnalogInputs[NUM_SLIDERS] = { A1, A2, A3, A4, 0}; 
const int channelButtonInputs[NUM_SLIDERS] = { 11, 8, 7, 4, 2 }; 
const int channelLedOutputs[NUM_SLIDERS] = { 10, 9, 6, 5, 3 }; 

const int flashTime = 500;
const int flashMin = 0;
const int flashMax = 200;

const int pulseMin = 0;
const int pulseMax = 180;
const int pulseCycle = 25;
const int pulseStep = 20;

// const int NUM_PAIRS = 1;
// const int pairedButtonInputs[NUM_PAIRS] = { 2};
// const int pairedLedOutputs[NUM_PAIRS] = { 3};

// Button Settings - MAX 16
// const int NUM_BUTTONS = 0;
// const int singleButtonInputs[NUM_BUTTONS] = {}; 

//LED Settings - MAX 16
// const int NUM_LEDS = 0;
// const int  singleLedOutputs[NUM_LEDS] = {}; 

int flashLeds = 0;
int flashLevel = 0;
unsigned long nextFlashChange = 0;

int pulseLeds = 0;
int pulseLevel = 0; 
int currentPulseStep = pulseStep;
unsigned long nextPulseChange = 0;

unsigned long currentMillis = 0;

int channelLedLighting[NUM_SLIDERS] = { 0, 0, 0, 0}; 


String incomingLEDState;
int ledIdx;
int ledLighting;
int lastLighting;
int seperatorPos;

// Slider Variables
int channelSliderValues[NUM_SLIDERS];

// Button variables
int buttonCurrentState[NUM_SLIDERS];

int reading; 
int tempVal;

void setup() { 

  pinMode(13, OUTPUT);

  for (int i = 0; i < NUM_SLIDERS; i++) {
    pinMode(channelAnalogInputs[i], INPUT);
  }

  for (int i = 0; i < NUM_SLIDERS; i++) {
    pinMode(channelButtonInputs[i], INPUT_PULLUP);
  }

  for (int i = 0; i < NUM_SLIDERS; i++) {
    pinMode(channelLedOutputs[i], OUTPUT);
  }

  for (int i = 0; i < NUM_SLIDERS; i++) {
    digitalWrite(channelLedOutputs[i], HIGH);
    delay(100);
    digitalWrite(channelLedOutputs[i], LOW);
  }

  Serial.begin(9600);

  // for(int thisButton = 0; thisButton < NUM_BUTTONS; thisButton++){
  //   buttonLastState[thisButton] = 0;
  //   buttonLastBounceTime[thisButton] = 0;
  // }

  for(int thisLED = 0; thisLED < NUM_SLIDERS; thisLED++){
    digitalWrite(channelLedOutputs[thisLED], LOW);
  }
}

void loop() {
  updateSliderValues();
  updateButtonValues();
  sendSliderValues(); // Actually send data (all the time)
  //printSliderValues(); // For debug

  readLedInputs();

  delay(DELAY);
}

void updateSliderValues() {
  // for each slider
  for (int i = 0; i < NUM_SLIDERS; i++) {
    if(channelAnalogInputs[i] == 0){
      channelSliderValues[i] = 0;
    } else {
      channelSliderValues[i] = analogRead(channelAnalogInputs[i]);
    }
  }
}

void updateButtonValues() {
  //buttonState = 0;
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
    if(NUM_SLIDERS > i){
      if(buttonCurrentState[i] == LOW){ // Use LOW for INPUT_PULLUP
        tempVal = tempVal + 1024;   // Set byte 11 to 1
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


void readLedInputs(){
  if(Serial.available() > 0){

    digitalWrite(13, HIGH);

    incomingLEDState = Serial.readStringUntil('\n');
    seperatorPos = incomingLEDState.indexOf('|');
    ledIdx = incomingLEDState.substring(0,seperatorPos).toInt();

    if(ledIdx < 0 || ledIdx >= NUM_SLIDERS){
      return;
    }

    ledLighting = incomingLEDState.substring(seperatorPos + 1).toInt();
    
    if(ledLighting < 0){
      ledLighting = 0;
    }
    else if(ledLighting > 3){
      ledLighting = 1;
    }

    lastLighting = channelLedLighting[ledIdx];
    if(ledLighting != lastLighting){

      if(lastLighting == 2){
        pulseLeds = pulseLeds - 1;
        if(pulseLeds < 0){
          pulseLeds = 0;
        }
      } 
      else if(lastLighting == 3){
        flashLeds = flashLeds - 1;
        if(flashLeds < 0){
          flashLeds = 0;
        }
      }

      if(ledLighting == 0){
        analogWrite(channelLedOutputs[ledIdx], 0);
      } 
      else if(ledLighting == 1){
        analogWrite(channelLedOutputs[ledIdx], 255);
      } 
      else if(ledLighting == 2){
        pulseLeds = pulseLeds + 1;
        if(pulseLeds > NUM_SLIDERS){
          pulseLeds = NUM_SLIDERS;
        }
        if(pulseLeds == 1){
          nextFlashChange = 0;
          flashLevel = flashMax;
        }
      } 
      else if(ledLighting == 3){
        flashLeds = flashLeds + 1;
        if(flashLeds > NUM_SLIDERS){
          flashLeds = NUM_SLIDERS;
        }
        if(flashLeds == 1){
          nextPulseChange = 0;
          pulseLevel = 0;
          currentPulseStep = pulseStep;
        }
      }
    }

    channelLedLighting[ledIdx] = ledLighting;

    digitalWrite(13, LOW);

  }

  if (pulseLeds == 0 && flashLeds == 0){
    return;
  }

  currentMillis = millis();

  if (pulseLeds > 0){
    if (nextPulseChange < currentMillis){

      pulseLevel = pulseLevel + currentPulseStep;

      for (int i = 0; i < NUM_SLIDERS; i++) {
        if(channelLedLighting[i] == 2){
          analogWrite(channelLedOutputs[i], pulseLevel);
        }
      }
      if(pulseLevel > pulseMax || pulseLevel < pulseMin){
        currentPulseStep = -currentPulseStep;
      }
      nextPulseChange = currentMillis + pulseCycle;
    }
  }
    
  if (flashLeds > 0){

    if (nextFlashChange < currentMillis){

      if(flashLevel == flashMin){
        flashLevel = flashMax;
        
      }
      else {
        flashLevel = flashMin;
      }

      for (int i = 0; i < NUM_SLIDERS; i++) {
        if(channelLedLighting[i] == 3){
          analogWrite(channelLedOutputs[i], flashLevel);
        }
      }
      nextFlashChange = currentMillis + flashTime;
    }
  }

    
  


}

void printSliderValues() {
  for (int i = 0; i < NUM_SLIDERS; i++) {
    tempVal = channelSliderValues[i];
    if(tempVal >= 1024){
      String printedString = String("Slider #") + String(i + 1) + String(": ") + String(tempVal - 1024) + String(" mV ") + String("DOWN");
      Serial.write(printedString.c_str());
    }
    else {
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
