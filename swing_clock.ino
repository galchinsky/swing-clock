#ifndef PC_TEST
#include <Bounce2.h>
Bounce2::Button button = Bounce2::Button();
#endif

#define CLOCK1 21
#define CLOCK2 15
#define START1 23
#define START2 16
#define GROOVE1 5 //a5 invert
#define GROOVE2 4 //a4 invert
#define BUTTON_PIN 12
#define SPEED_MAIN 3 //not inverted
#define SPEED_FINE 0 //not inverted

#define MAX_PATTERN_SIZE 32

// unit of measure is 1 timer tick aka 1 ms
// trigger impulse length in ms
#define IMPULSE_LENGTH 10

IntervalTimer timer;

void timerFunc();

void setup() {
  #ifndef PC_TEST
  button.attach( BUTTON_PIN ,  INPUT_PULLUP );
  button.interval(5);
  button.setPressedState(LOW);
  #endif

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWriteFast(LED_BUILTIN, HIGH);
  pinMode(CLOCK1, OUTPUT);
  pinMode(CLOCK2, OUTPUT);
  pinMode(START1, OUTPUT);
  pinMode(START2, OUTPUT);

  Serial.begin(115200);

  // let's use a hardware timer for better precision
  // minimal interval is 1000 microseconds, enough for musical purporses
  timer.begin(timerFunc, 1000);

}

// teensy is 32 bit, so no need to suffer with int's lack of precision
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
  float leftmostKnobPosition = 30.0f; //minimum bpm
  float rightmostKnobPosition = 250.0f; //maximum bpm
  float bpm = mapAnalogRead(analogReadValue, leftmostKnobPosition, rightmostKnobPosition);
  float ms = 60000.0f / bpm;
  return ms;
}

class SwingingClock {
private:
    float pattern[MAX_PATTERN_SIZE + 1];
    int numberOfBeatsInPattern;
    int patternIndex;
    int impulseTick;
    int beatTick;
    int outputPin;

    void incrementTicks() {
        ++beatTick;
        ++impulseTick;
    }

public:
    SwingingClock(const float initPattern[], int size, int pin)
        : numberOfBeatsInPattern(size), patternIndex(0), impulseTick(0), beatTick(0), outputPin(pin) {
        if (size > MAX_PATTERN_SIZE) {
            size = MAX_PATTERN_SIZE;
        }
        for (int i = 0; i < size; ++i) {
            pattern[i] = initPattern[i];
        }
        pattern[size] = 999999;
    }

    void update(float beatTime, float amountOfSwing) {
        // when we apply swing, total time mustn't change
        // so if we increase the length of the first beat
        // the length of the second beat must be decreased so that L1 + L2 = 2
        // so in case of no swing:
        //      1   +     1     = 2
        // [--------][--------]
        // in case of swing
        //     1.2  +   0.8     = 2
        // [----------][------]
        // so we increase even beats (0, 2, 4) and decrease odd beats (1, 3, 5)

        float swingMultiplier = pattern[patternIndex];
        if (patternIndex % 2 == 0) {
            swingMultiplier += amountOfSwing;
        } else {
            swingMultiplier -= amountOfSwing;
        }

        // if beat time is over, go to the next one in the array
        if (beatTick > beatTime * swingMultiplier) {
            ++patternIndex;
            impulseTick = 0;
            beatTick = 0;

            // if the last beat is over, get stuck in the LOW state
            // until we got reset from the outside
            if (patternIndex >= numberOfBeatsInPattern) {
                patternIndex = numberOfBeatsInPattern - 1;
                impulseTick = IMPULSE_LENGTH;
            }
        }

        // it can be impulseTick >= 0 and impulseTick >= 10
        // which is more robust, but I don't want to spam digitalWrite
        if (impulseTick == 0) {
            digitalWrite(outputPin, HIGH);
        }
        if (impulseTick == IMPULSE_LENGTH) {
            digitalWrite(outputPin, LOW);
        }

        incrementTicks();
    }

    void reset() {
        patternIndex = 0;
        impulseTick = 0;
        beatTick = 0;
    }
};

// we don't really need arrays for a vanilla swing, so the references to the patterns
// can be replaced with 1.0f constant
// but having the lengths in the array allows to use groove patterns
float pattern1Array[] = {1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
                         1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
                         1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
                         1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0};
int pattern1Length = sizeof(pattern1Array) / sizeof(pattern1Array[0]);

// Funky Drummer
float pattern2Array[] = {1.05,  1.0,   1.033, 0.883, 1.017, 1.0, 1.083,   0.9,
                         1.017, 1.05,  1.05,  0.883, 1.017, 1.033, 0.95,  1.0,
                         1.083, 0.983, 1.017, 0.917, 1.017, 1.067, 1.033, 0.883,
                         1.017, 1.05,  1.033, 0.9,   1.067, 1.0,   0.933, 1.033999999999999};
int pattern2Length = sizeof(pattern2Array) / sizeof(pattern2Array[0]);

// the patterns must have the same length
int numberOfBeatsInPattern = pattern1Length < pattern2Length ? pattern1Length : pattern2Length;

int startTick = 0;
int patternTick = 0;
SwingingClock pattern1(pattern1Array, pattern1Length, CLOCK1);
SwingingClock pattern2(pattern2Array, pattern2Length, CLOCK2);

void timerFunc() {

    // if the button is hold, output nothing
    // this mode can be used to use knobs to tweak something
    // for example, select different groove patterns
    if (startTick == -1) {
        digitalWrite(START1, LOW);
        digitalWrite(START2, LOW);
        digitalWrite(CLOCK1, LOW);
        digitalWrite(CLOCK2, LOW);
        return;
    }

    if (startTick == 0) {
        digitalWrite(START1, HIGH);
        digitalWrite(START2, HIGH);
        pattern1.reset();
        pattern2.reset();
        patternTick = 0;
    }
    if (startTick == IMPULSE_LENGTH) {
        digitalWrite(START1, LOW);
        digitalWrite(START2, LOW);
    }
    // without this condition the device may reset after 49 days
    if (startTick > 10 * IMPULSE_LENGTH) {
        startTick = 10 * IMPULSE_LENGTH;
    }

    float note16th = 0.25f;
    float beatTime = analogReadToBpmToMs(analogRead(SPEED_MAIN)) * note16th;
    beatTime = beatTime * mapAnalogRead(SPEED_FINE, 0.9, 1.1);

    float patternTime = beatTime * numberOfBeatsInPattern;

    float amountOfSwing1 = mapAnalogRead(GROOVE1, -0.5f, 0.5f);
    float amountOfSwing2 = mapAnalogRead(GROOVE2, -0.5f, 0.5f);

    if (patternTick >= patternTime) {
        pattern1.reset();
        pattern2.reset();
        patternTick = 0;
    }

    pattern1.update(beatTime, amountOfSwing1);
    pattern2.update(beatTime, amountOfSwing2);

    ++startTick;
    ++patternTick;

    #ifdef PC_TEST
    std::cout << digitalRead(CLOCK1) << " " << digitalRead(CLOCK2) << std::endl;
    #endif
}

void loop() {
#ifndef PC_TEST
  button.update();

  if (button.pressed()) {
    // start on leaving the button
    startTick = -1;
  }
  delay(4);
#endif
}
