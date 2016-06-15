#include <wiringPi.h>
#include "gpio.h"

Debouncer *Debouncer::ISRinstance = nullptr;
void Debouncer::DebouncerISR()
{
    if (Debouncer::ISRinstance) Debouncer::ISRinstance->Update();
}

// WiringPi setup must be called only once globally
static bool wiringPiInitialized = false;

Debouncer::Debouncer(int GPIO_input_pin, unsigned int required_stable_interval, bool use_interrupt) :
    gpio(GPIO_input_pin),
    requiredStable(required_stable_interval),
    currentInterval(0),
    lastInterval(0),
    interruptmode(false)
{
    if (!wiringPiInitialized && wiringPiSetup() < 0)
    {
        throw std::runtime_error("wiringPiSetup() failed");
    }

    pinMode(GPIO_input_pin, INPUT);
    pullUpDnControl(GPIO_input_pin, PUD_OFF);

    tmpChangeTime = millis();
    tmpState = digitalRead(GPIO_input_pin);

    currentState = 1 - tmpState;
    lastState = tmpState;
    currentChangeTime = tmpChangeTime - requiredStable;

    ISRinstance = nullptr;
    if (use_interrupt)
    {
        ISRinstance = this;
        if (!wiringPiInitialized && wiringPiISR(gpio, INT_EDGE_BOTH, DebouncerISR) < 0)
        {
            throw std::runtime_error("wiringPiISR() failed");
        }
        interruptmode = true;
        mx.lock();
        waiting = true;
    }
    wiringPiInitialized = true;
}

Debouncer::~Debouncer()
{
    ISRinstance = nullptr;
}

bool Debouncer::Update()
{
    // Unsigned integer arithmetic will also work correctly with timer wraparound
    unsigned int now = millis();
    unsigned int stable = now - tmpChangeTime;
    bool changed = false;
    if (stable >= requiredStable)
    {
        if (tmpState != currentState)
        {
            lastState = currentState;
            currentState = tmpState;
            lastInterval = tmpChangeTime - currentChangeTime;
            currentChangeTime = tmpChangeTime;
            changed = true;
        }
    }
    currentInterval = now - currentChangeTime;

    int pinState = digitalRead(gpio);
    if (pinState != tmpState)
    {
        tmpState = pinState;
        tmpChangeTime = now;
    }
    if (interruptmode)
    {
        printf("%c", pinState == 0 ? '_' : '^');
    }

    if (interruptmode && waiting && changed)
    {
        waiting = false;
        mx.unlock();
    }

    return changed;
}

bool Debouncer::WaitForInterrupt(int timeout)
{
    if (interruptmode)
    {
        // If interrupt has already happened, try_lock_for will return immediately
        if (mx.try_lock_for(std::chrono::milliseconds(timeout)))
        {
            waiting = true;
            return true;
        }
        else // Timeout, do a polling update to get the current state
        {
            Update();
        }
    }
    return false;
}

static const int mark_width  =  60; /* ms */
static const int space_width =  30; /* ms */
static const int pause_width = 143; /* ms */
static const int relay_delay =   7; /* ms */

Selector::Selector(int GPIO_output_pin, bool inverted) :
    gpio(GPIO_output_pin),
    logic_high(inverted ? LOW : HIGH),
    logic_low(inverted ? HIGH : LOW)
{
    if (!wiringPiInitialized && wiringPiSetup() < 0)
    {
        throw std::runtime_error("wiringPiSetup() failed");
    }

    pinMode(GPIO_output_pin, OUTPUT);
    digitalWrite(GPIO_output_pin, logic_low);

    wiringPiInitialized = true;
}

void Selector::WritePulse()
{
    digitalWrite(gpio, logic_high);
    delay(mark_width + relay_delay);
    digitalWrite(gpio, logic_low);
}

void Selector::WriteSpace()
{
    delay(space_width - relay_delay);
}

void Selector::WritePause()
{
    delay(pause_width - relay_delay);
}

void Selector::Select(int page /* 1 - 10 */, int button /* 1 - 20 */)
{
    if (page < 1 || page > 10 || button < 1 || button > 20)
        throw std::invalid_argument("Invalid page or button");

    for (int i=0; i <page+1; ++i)
    {
        WriteSpace();
        WritePulse();
    }

    WritePause();

    for (int i=0; i<button; ++i)
    {
        WritePulse();
        WriteSpace();
    }
}
