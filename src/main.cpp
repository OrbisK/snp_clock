#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

#define HZ 128
#define POWER_SAVE_PROPORTION 3
#define NORMAL_PROPORTION 1
#define LONG_PRESS_COUNT 40

enum POWER_MODE {
    NORMAL,
    POWER_SAVE,
    SLEEP
};

class Time {
public:
    uint8_t hours;
    uint8_t minutes;
    uint8_t seconds;

    Time(const uint8_t h, const uint8_t m, const uint8_t s) : hours(h), minutes(m), seconds(s) {
    }

    void increaseHours() {
        hours++;
        if (hours == 24) {
            hours = 0;
        }
    }

    void increseMinutes() {
        minutes++;
        if (minutes == 60) {
            minutes = 0;
            increaseHours();
        }
    }

    void increaseSeconds() {
        seconds++;
        if (seconds == 60) {
            seconds = 0;
            increseMinutes();
        }
    }

    void decreaseHours() {
        if (hours == 0) {
            hours = 23;
        } else {
            hours--;
        }
    }

    void decreaseMinutes() {
        if (minutes == 0) {
            minutes = 59;
            decreaseHours();
        } else {
            minutes--;
        }
    }

    void decreaseSeconds() {
        if (seconds == 0) {
            seconds = 59;
            decreaseMinutes();
        } else {
            seconds--;
        }
    }
};

class PortPin {
public:
    volatile uint8_t *port;
    uint8_t pin;

    PortPin(volatile uint8_t *p, const uint8_t pn) : port(p), pin(pn) {
    }

    void set(const bool value) const {
        if (value) {
            *port |= (1 << pin);
        } else {
            *port &= ~(1 << pin);
        }
    }
};

class HardwareController {
public:
    HardwareController() {
        PRR = 0; // Enable all power reduction

        // Set the pins to output for minutes and hours
        DDRC |= (1 << PC0) | (1 << PC1) | (1 << PC2) | (1 << PC3) | (1 << PC4) | (1 << PC5);
        DDRD |= (1 << PD0) | (1 << PD1) | (1 << PD4) | (1 << PD5) | (1 << PD6);

        DDRB |= (1 << PB0); // Set PB0 as output for seconds

        // Configure buttons on PD2 and PD3
        DDRD &= ~((1 << DDD2) | (1 << DDD3));
        PORTD |= (1 << PD2) | (1 << PD3) | (1 << PD7);

        PCICR |= (1 << PCIE2); // Enable PCINT23..16 pin change interrupt
        PCMSK2 |= (1 << PCINT23); // Enable PCINT23

        // Konfigurieren Sie den Pin, an dem PCINT1 angeschlossen ist, als Eingang
        DDRB &= ~(1 << DDB1); // Set PB1 as input
        PORTB |= (1 << PB1); // Enable pull-up resistor on PB1

        // Aktivieren Sie den Pin-Change-Interrupt für PCINT1
        PCICR |= (1 << PCIE0); // Enable PCINT7..0 pin change interrupt
        PCMSK0 |= (1 << PCINT1); // Enable PCINT1

        // Configure INT1 (PD3) to respond to the falling edge
        EICRA |= (1 << ISC11);
        EIMSK |= (1 << INT1);

        // Configure Timer2 for asynchronous operation
        // Set up Timer 2 for asynchronous operation
        ASSR |= (1 << AS2); // Enable asynchronous mode

        // Set up Timer 2 control register A and B
        TCCR2A |= (1 << WGM21); // Configure timer for CTC mode
        TCCR2B |= (1 << CS20); // Set prescaler to no prescaling

        // Set the compare match register to the value that generates an interrupt every second - 255 is 1 second
        // calculate the value for given HZ
        OCR2A = (32768 / HZ) - 1;

        // Enable Timer 2 compare match interrupt
        TIMSK2 |= (1 << OCIE2A);

        while (ASSR & ((1 << TCR2BUB) | (1 << TCR2AUB) | (1 << OCR2BUB) | (1 << OCR2AUB) | (1 << TCN2UB)));

        set_sleep_mode(SLEEP_MODE_PWR_SAVE);

        sei(); // Enable global interrupts

        powerMode = NORMAL;
        applyPowerMode();
    }

    void updatePowerMode() {
        switch (powerMode) {
            case NORMAL:
                powerMode = POWER_SAVE;
                break;
            case POWER_SAVE:
                powerMode = SLEEP;
                break;
            case SLEEP:
                powerMode = NORMAL;
                break;
        }
        applyPowerMode();
    }

    void applyPowerMode() {
        switch (powerMode) {
            case NORMAL:
                led_off_proportion = NORMAL_PROPORTION;
                sleepEnabled = false;
                break;
            case POWER_SAVE:
                led_off_proportion = POWER_SAVE_PROPORTION;
                sleepEnabled = false;
                break;
            case SLEEP:
                sleepEnabled = true;
                break;
        }
    }

    bool isSleepEnabled() const {
        return sleepEnabled;
    }

    unsigned short getLedOffProportion() const {
        return led_off_proportion;
    }

    void timeToPins(const Time &time) const {
        for (int i = 0; i < 5; i++) {
            hourPins[i].set(time.hours & (1 << i));
        }
        for (int i = 0; i < 6; i++) {
            minutePins[i].set(time.minutes & (1 << i));
        }

        secondPin.set(time.seconds % 2);
    }

    void handleSleepMode() const {
        Time time = {0, 0, 0};
        timeToPins(time);
        sleep_mode(); // Enter sleep mode
    }

    void handleInterrupt1() {
        updatePowerMode();
    }

    void handlePInterrupt23() {
        const bool currentPinState = PIND & (1 << PD7); // Read the current state of PD7
        if (currentPinState && !lastPinState0) {
            currentTime.increseMinutes();
            upPressed = false;
            // Rising edge detected
            // Handle rising edge
        } else if (!currentPinState && lastPinState0) {
            upPressed = true;

            // Falling edge detected
            // Handle falling edge
        }

        lastPinState0 = currentPinState;
    }

    void handlePInterrupt1() {
        const bool currentPinState = PINB & (1 << PB1); // Read the current state of PB1
        if (currentPinState && !lastPinState1) {
            currentTime.decreaseMinutes();
            downPressed = false;
            // Rising edge detected
            // Handle rising edge
        } else if (!currentPinState && lastPinState1) {
            downPressed = true;
            // Falling edge detected
            // Handle falling edge
        }

        lastPinState1 = currentPinState;
    }

    void handleSecondPassed() {
        currentTime.increaseSeconds();
    }

    void handleFastForwardUp() {
        if (count % 2 == 0) {
            currentTime.increseMinutes();
        }
    }

    void handleFastForwardDown() {
        if (count % 2 == 0) {
            currentTime.decreaseMinutes();
        }
    }

    void handleTickPassed() {
        if (upPressed) {
            downCounter = 0;
            upCounter += 1;
            if (upCounter > LONG_PRESS_COUNT) {
                handleFastForwardUp();
            }
        } else {
            upCounter = 0;
        }

        if (downPressed) {
            upCounter = 0;
            downCounter += 1;
            if (downCounter > LONG_PRESS_COUNT) {
                handleFastForwardDown();
            }
        } else {
            downCounter = 0;
        }
    }

    void handleTimer2() {
        count++;
        // once per second
        if (count % HZ == 0) {
            handleSecondPassed();
        }
        handleTickPassed();

        count = count % HZ;
    }

    void handleUpdate() const {
        if (sleepEnabled) {
            handleSleepMode();
        } else {
            if (count % led_off_proportion == 0) {
                timeToPins(currentTime);
            } else {
                const Time time = {0, 0, 0};
                timeToPins(time);
            }
        }
    }

private:
    POWER_MODE powerMode;
    unsigned short led_off_proportion = POWER_SAVE_PROPORTION;
    volatile bool sleepEnabled = false; // Zustandsvariable für den Schlafmodus
    PortPin hourPins[5] = {
        PortPin(&PORTD, PD0),
        PortPin(&PORTD, PD1),
        PortPin(&PORTD, PD4),
        PortPin(&PORTD, PD5),
        PortPin(&PORTD, PD6),
    };
    PortPin minutePins[6] = {
        PortPin(&PORTC, PC0),
        PortPin(&PORTC, PC1),
        PortPin(&PORTC, PC2),
        PortPin(&PORTC, PC3),
        PortPin(&PORTC, PC4),
        PortPin(&PORTC, PC5),
    };

    PortPin secondPin = PortPin(&PORTB, PB0);
    bool lastPinState0 = true;
    bool lastPinState1 = true;
    unsigned short count = HZ;
    Time currentTime = Time(0, 10, 0);
    bool upPressed = false;
    bool downPressed = false;
    unsigned short upCounter = 0;
    unsigned short downCounter = 0;
};

HardwareController hardwareController;

ISR(INT1_vect) {
    hardwareController.handleInterrupt1();
}

ISR(PCINT2_vect) {
    hardwareController.handlePInterrupt23();
}

ISR(TIMER2_COMPA_vect) {
    hardwareController.handleTimer2();
}

ISR(PCINT0_vect) {
    hardwareController.handlePInterrupt1();
}


int main() {
    while (true) {
        hardwareController.handleUpdate();
    }
}
