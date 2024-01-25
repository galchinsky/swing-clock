#include <Bounce2.h>
//Bounce2::Button button = Bounce2::Button();

#define CLOCK1 21
#define CLOCK2 15
#define START1 23
#define START2 16
#define GROOVE1 5 //a5 invert
#define GROOVE2 4 //a4 invert
#define BUTTON_PIN 12
#define SPEED_MAIN 3 //not inverted
#define SPEED_FINE 0 //not inverted

int butstate = LOW;
int clockstate = LOW;
int spd_main = 0;
int spd_fine = 0;
int fine_range = 2000;

// let's use hardware based timer for better precision
// minimal interval will be 1 ms : enough for musical purporses
IntervalTimer timer;

int ledState = LOW;

void timerFunc();

void setup() {
  //button.attach( BUTTON_PIN ,  INPUT_PULLUP );
  //button.interval(20);
  //button.setPressedState(LOW);

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWriteFast(LED_BUILTIN, HIGH);
  pinMode(CLOCK1, OUTPUT);
  pinMode(CLOCK2, OUTPUT);
  pinMode(START1, OUTPUT);
  pinMode(START2, OUTPUT);

  Serial.begin(115200);

  timer.begin(timerFunc, 1000);

}

float mapFloat(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

float map10bit(float x, float out_min, float out_max) {
  return mapFloat(x, 0.0f, 1023.0f, out_min, out_max);
}

float mapAnalogRead(uint8_t pin, float out_min, float out_max) {
  return map10bit(float(analogRead(pin)), out_min, out_max);
}


float analogReadToBpmToMs(int analogReadValue_) {
  float analogReadValue = analogReadValue_;
  float bpm = mapAnalogRead(analogReadValue, 30.0f, 270.0f);
  float ms = 60000.0f / bpm;
  return ms;
}

// pattern is 16 quoter notes

int patternTick = 0;
int beat1Tick = 0;
int impulse1Tick = 0;
int beat2Tick = 0;
int impulse2Tick = 0;

int pattern1_index = 0;
float pattern1[] = {1.0, 1.0, 1.0, 1.0, 10000};

int pattern2_index = 0;
float pattern2[] = {1.0, 1.0, 1.0, 1.0, 10000};

int numberOfBeatsInPattern = sizeof(pattern1) / sizeof(pattern1[0]) - 1;

int impulseLength = 10;

// 1/16th note is 1/4 of the beat
float note16th = 0.25f;

void timerFunc() {
  float beatTime = analogReadToBpmToMs(analogRead(SPEED_MAIN)) * note16th;
  float patternTime = beatTime * numberOfBeatsInPattern;

  float amountOfSwing1 = mapAnalogRead(GROOVE1, -0.9f, 0.9f);
  float amountOfSwing2 = mapAnalogRead(GROOVE2, -0.9f, 0.9f);

  pattern1[0] = 1.0f + amountOfSwing1;
  pattern1[1] = 1.0f - amountOfSwing1;
  pattern1[2] = 1.0f + amountOfSwing1;
  pattern1[3] = 1.0f - amountOfSwing1;

  pattern2[0] = 1.0f + amountOfSwing2;
  pattern2[1] = 1.0f - amountOfSwing2;
  pattern2[2] = 1.0f + amountOfSwing2;
  pattern2[3] = 1.0f - amountOfSwing2;

  // pattern tick is common for all sequences, so 100% sync

  if (patternTick >= patternTime) {
    patternTick = 0;

    pattern1_index = 0;
    impulse1Tick = 0;
    beat1Tick = 0;

    pattern2_index = 0;
    impulse2Tick = 0;
    beat2Tick = 0;
  }

  // beat ticks are swinging depending on the patterns

  if (beat1Tick > beatTime * pattern1[pattern1_index]) {
    pattern1_index = pattern1_index + 1;
    impulse1Tick = 0;
    beat1Tick = 0;
    // don't output if the pattern is over
    if (pattern1_index >= numberOfBeatsInPattern) {
      pattern1_index = numberOfBeatsInPattern;
      impulse1Tick = impulseLength;
    }
  }

  if (beat2Tick > beatTime * pattern2[pattern2_index]) {
    pattern2_index = pattern2_index + 1;
    impulse2Tick = 0;
    beat2Tick = 0;
    // don't output if the pattern is over
    if (pattern2_index >= numberOfBeatsInPattern) {
      pattern2_index = numberOfBeatsInPattern;
      impulse2Tick = impulseLength;
    }
  }


  // one impulse is always 10 ms
  if (impulse1Tick == 0) {
    digitalWrite(CLOCK1, HIGH);
  }
  if (impulse1Tick == impulseLength) {
    digitalWrite(CLOCK1, LOW);
  }

  if (impulse2Tick == 0) {
    digitalWrite(CLOCK2, HIGH);
  }
  if (impulse2Tick == impulseLength) {
    digitalWrite(CLOCK2, LOW);
  }

  ++patternTick;
  ++beat1Tick;
  ++impulse1Tick;


}


void loop() {

}
