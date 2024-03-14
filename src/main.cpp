#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

class Time {
public:
    uint8_t hours;
    uint8_t minutes;
    uint8_t seconds;

    Time(uint8_t h, uint8_t m, uint8_t s) : hours(h), minutes(m), seconds(s) {}

    void increase() {
        seconds++;
        if (seconds == 60) {
            seconds = 0;
            minutes++;
            if (minutes == 60) {
                minutes = 0;
                hours++;
                if (hours == 24) {
                    hours = 0;
                }
            }
        }
    }
};

Time currentTime(1, 1, 1);

class PortPin {
public:
    volatile uint8_t *port;
    uint8_t pin;

    PortPin(volatile uint8_t *p, uint8_t pn) : port(p), pin(pn) {}

    void set(bool value) const {
        if (value) {
            *port |= (1 << pin);
        } else {
            *port &= ~(1 << pin);
        }
    }
};

PortPin hourPins[] = {
    PortPin(&PORTD, PD0),
    PortPin(&PORTD, PD1),
    PortPin(&PORTD, PD4),
    PortPin(&PORTD, PD5),
    PortPin(&PORTD, PD6),
};

PortPin minutePins[] = {
    PortPin(&PORTC, PC0),
    PortPin(&PORTC, PC1),
    PortPin(&PORTC, PC2),
    PortPin(&PORTC, PC3),
    PortPin(&PORTC, PC4),
    PortPin(&PORTC, PC5),
};

void timeToPins(const Time& time) {
    for (int i = 0; i < 5; i++) {
        hourPins[i].set(time.hours & (1 << i));
    }
    for (int i = 0; i < 6; i++) {
        minutePins[i].set(time.minutes & (1 << i));
    }
}

volatile bool sleepEnabled = false; // Zustandsvariable für den Schlafmodus

ISR(INT1_vect) {
    sleepEnabled = !sleepEnabled;
    Time time(0, 0, 0);
    timeToPins(time);
}

ISR(TIMER2_COMPA_vect) {
    currentTime.increase();
}

int main() {
    // Set the pins to output for minutes and hours
    DDRC |= (1 << PC0) | (1 << PC1) | (1 << PC2) | (1 << PC3) | (1 << PC4) | (1 << PC5);
    DDRD |= (1 << PD0) | (1 << PD1) | (1 << PD4) | (1 << PD5) | (1 << PD6);

    // Configure buttons on PD2 and PD3
    DDRD &= ~((1 << DDD2) | (1 << DDD3));
    PORTD |= (1 << PD2) | (1 << PD3);

    // Configure INT1 (PD3) to respond to the falling edge
    EICRA |= (1 << ISC11);
    EIMSK |= (1 << INT1);

    // Configure Timer2 for asynchronous operation
    // ... (Timer configuration remains the same)

    set_sleep_mode(SLEEP_MODE_PWR_SAVE);

    sei(); // Enable global interrupts

    while (true) {
        if (sleepEnabled) {
            sleep_mode(); // Enter sleep mode
        } else {
            currentTime.increase();
            timeToPins(currentTime);
        }
    }
}