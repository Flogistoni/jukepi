#include "gpio.h"

static const int pollInterval = 5; //ms

bool pulse_ok(int width)
{
    return width >= 50 && width <= 70;
}

bool space_ok(int width)
{
    return width >= 20 && width <= 40;
}

bool pause_ok(int width)
{
    return width >= 120 && width <= 160;
}

void receiver()
{
    Debouncer db(5 /* WiringPi GPIO */, 10 /* ms stable */, true /* interrupt */);
    int page = -1; // There is one more pulse than page number
    int button = 0;
    int expiry = 60000; /*ms*/

    enum { idle, get_page, get_button, complete, read_error = -1} rec_state;
    rec_state = idle;
    bool edge;
    do {
        edge = db.WaitForInterrupt(expiry);
        int duration = db.CurrentInterval();
        int value = db();
        printf("%d/%d: %d ~ %d\n", edge, rec_state, value, duration);
        if (edge)
        {
            switch (rec_state)
            {
            case read_error:
                if (duration > 2000) rec_state = idle;
            case idle:
                if      (value == 0) rec_state = get_page; // First pulse, rising edge
                break;

            case get_page:
                if      (value == 1 && pulse_ok(duration)) ++page;
                else if (value == 0 && space_ok(duration)) /* do nothing */;
                else if (value == 0 && pause_ok(duration)) rec_state = get_button;
                else rec_state = read_error;
                break;

            case get_button:
                if      (value == 1 && pulse_ok(duration)) ++button;
                else if (value == 0 && space_ok(duration)) /* do nothing */;
                else if (value == 0) rec_state = complete;
                else rec_state = read_error;
                expiry = 2000;
                break;

            default:
                break;
            }
        }
        else // timeout expired
        {
            if (rec_state == get_button) rec_state = complete;
            if (rec_state == read_error) rec_state = idle; // reset error
        }
        if (rec_state == read_error) expiry = 1000;
        fflush(stdout);
    }
    while (edge && rec_state != complete);

    printf("page = %d, button = %d, state = %d\n", page, button, rec_state);
}

int main(int argc, char *argv[])
{
    if (argc == 1)
    {
        printf("Listening...\n");
        for (;;) receiver();
    }
    else if (argc == 3)
    {
        int page = std::atoi(argv[1]);
        int button = std::atoi(argv[2]);

        Selector sel(4 /* WiringPi GPIO */);
        printf("Sending page %d button %d\n", page, button);
        sel.Select(page, button);
    }
    else
    {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

void visualReceiver()
{
    Debouncer db(5 /* WiringPi GPIO */, 10 /* ms stable */, true /* interrupt */);
    printf("Pin: %d\n", db());

    bool timeout = false;
    while (!timeout)
    {
        timeout = !db.WaitForInterrupt(60000);
        int duration = db.CurrentInterval();
        char c = (db() == 0) ? '_' : '^';
        char d = (db() == 0) ? '/' : '\\';
        for (int i=0; i < duration; i += 10) printf("%c",c);
        if (!timeout) printf("%c",d);
        fflush(stdout);
    }

    printf("\n");
}
