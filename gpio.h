#ifndef GPIO_H
#define GPIO_H

#include <mutex>

class Debouncer
{
public:
    Debouncer(int GPIO_input_pin, unsigned int required_stable_interval, bool use_interrupt = false);
    ~Debouncer();

    bool Update();  // Return true if stably changed

    // Return true if interrupt received
    // Return false if timeout happened or interrupt not active
    bool WaitForInterrupt(int timeout);
    bool IsWaiting() const
        { return waiting; }

    int CurrentState() const
        { return currentState; }
    int HWState() const
        { return tmpState; }
    int operator()() const
        { return CurrentState(); }
    unsigned int CurrentInterval() const
        { return currentInterval; }

    int LastState() const
        { return lastState; }
    unsigned int LastInterval() const
        { return lastInterval; }

public:
    static Debouncer* ISRinstance;
    static void DebouncerISR();

private:
    const int gpio;
    const unsigned int requiredStable;

    unsigned int tmpChangeTime;
    int tmpState;
    int currentState;
    int currentInterval;
    unsigned int currentChangeTime;
    int lastState;
    int lastInterval;

    bool interruptmode;
    std::timed_mutex mx;
    bool waiting;
};

class Selector
{
public:
    Selector(int GPIO_output_pin, bool inverted = false);

    void Select(int page /* 1 - 10 */, int button /* 1 - 20 */);

private:
    void WritePulse();
    void WriteSpace();
    void WritePause();

private:
    const int gpio;
    const int logic_high;
    const int logic_low;
};

#endif // GPIO_H

/*
AMI WQ200
Page pulses
2  |        ppppppppp |
3  | p       pppppppp |
4  | pp       ppppppp |
5  | ppp       pppppp |
6  | pppp       ppppp |
7  | ppppp       pppp |
8  | pppppp       ppp |
9  | ppppppp       pp |
10 | pppppppp       p |
11 | ppppppppp        |

Button pulses
1   *  *  11
2   *  *  12
3   o  o  13
4   o  o  14
5   *  *  15
6   *  *  16
7   o  o  17
8   o  o  18
9   *  *  19
10  *  *  20

*/
