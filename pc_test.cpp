// simple mocking of arduino library
#define PC_TEST

#include <iostream>
#include <utility>
#include <array>
#include <vector>
#include <chrono>
#include <thread>
#include <algorithm>
#include <functional>

// Define pin modes
enum PinMode { INPUT, OUTPUT };

// Define pin states
enum PinState { LOW, HIGH };

// A structure to represent each pin's state and mode
struct Pin {
    PinState state;
    PinMode mode;
};

// Create an array to represent 1000 pins
std::array<Pin, 1000> pins;
#define LED_BUILTIN 999

// Mock implementation of pinMode
void pinMode(int pinNumber, PinMode mode) {
    pins[pinNumber].mode = mode;
}

// Mock implementation of digitalWrite
void digitalWrite(int pinNumber, PinState state) {
    pins[pinNumber].state = state;
}

void digitalWriteFast(int pinNumber, PinState state) {
    pins[pinNumber].state = state;
}

// Mock implementation of digitalRead
PinState digitalRead(int pinNumber) {
    return pins[pinNumber].state;
}

// Mock implementation of analogRead
int analogRead(int pinNumber) {
    // Return a mock value, can be adjusted as needed
    return 512; // Middle value of analog read range
}

class IntervalTimer;
std::vector<IntervalTimer*> timers;

struct MockSerial {
    void begin(int baudRate) {}
    template<typename T> void print(const T& message) { std::cout << message; }
    template<typename T> void println(const T& message) { std::cout << message << std::endl; }
};
MockSerial Serial;

class IntervalTimer {
private:
    std::chrono::microseconds interval;
    std::chrono::steady_clock::time_point lastRun;
    std::function<void()> callback;
    bool isRunning;

public:
    IntervalTimer() : isRunning(false) {
        timers.push_back(this);
    }

    ~IntervalTimer() {
        auto it = std::find(timers.begin(), timers.end(), this);
        if (it != timers.end()) {
            timers.erase(it);
        }
    }

    void begin(std::function<void()> func, long intervalMs) {
        interval = std::chrono::microseconds(intervalMs);
        callback = func;
        lastRun = std::chrono::steady_clock::now();
        isRunning = true;
    }

    void stop() {
        isRunning = false;
    }

    void maybeRun() {
        if (!isRunning) return;

        auto now = std::chrono::steady_clock::now();
        if (now - lastRun > interval) {
            callback();
            lastRun = now;
        }
    }
};

// Global timer object

#include "swing_clock_deduped.ino"

int main() {
    setup();

    while (true) {
        loop();
        for (auto& timer : timers) {
            timer->maybeRun();
        }
        // Sleep for a short period to prevent maxing out CPU usage
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }

    return 0;
}

