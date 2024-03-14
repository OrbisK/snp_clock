#include<avr/io.h>
#include<avr/interrupt.h>
#include <avr/sleep.h>

#include "./Time.h"

static Time currentTime;

struct PortPin {
    volatile uint8_t *port;
    uint8_t pin;
};

struct PortPin hourPins[] = {
    {&PORTD, PD0},
    {&PORTD, PD1},
    {&PORTD, PD4},
    {&PORTD, PD5},
    {&PORTD, PD6},
};

struct PortPin minutePins[] = {
    {&PORTC, PC0},
    {&PORTC, PC1},
    {&PORTC, PC2},
    {&PORTC, PC3},
    {&PORTC, PC4},
    {&PORTC, PC5},
};

volatile bool test = false;

void setPins(struct PortPin p, uint8_t value) {
    if (value) {
        *p.port |= (1 << p.pin);
    } else {
        *p.port &= ~(1 << p.pin);
    }
}

uint8_t getBits(uint8_t number, uint8_t start, uint8_t end) {
    return (number >> start) & ((1 << (end - start + 1)) - 1);
}

void timeToPins(Time time) {
    for (int i = 0; i < 5; i++) {
        setPins(hourPins[i], getBits(time.getHours(), i, i));
    }
    for (int i = 0; i < 6; i++) {
        setPins(minutePins[i], getBits(time.getMinutes(), i, i));
    }
}

ISR(PCINT0_vect) {
    test = !test;
}


ISR(PCINT1_vect) {
}


volatile bool sleepEnabled = false; // Zustandsvariable für den Schlafmodus

ISR(INT1_vect) {
    // Umschalten des Schlafmodus-Zustands bei jedem Interrupt
    sleepEnabled = !sleepEnabled;
    // disable all pins
    const Time time;
    timeToPins(time);
}

ISR(TIMER2_COMPA_vect){
    currentTime.increaseTime();
}


int main() {
    // Set the pins to output for minutes
    DDRC |= (1 << PC0) | (1 << PC1) | (1 << PC2) | (1 << PC3) | (1 << PC4) | (1 << PC5);
    // Set the pins to output for hours
    DDRD |= (1 << PD0) | (1 << PD1) | (1 << PD4) | (1 << PD5) | (1 << PD6);

    // Taster an PD2
    DDRD &= ~(1 << DDD2);
    PORTD |= (1 << PD2);

    // Taster an PD3
    DDRD &= ~(1 << DDD3);
    PORTD |= (1 << PD3);

    // Konfigurieren Sie INT1 (PD3), um auf die fallende Flanke zu reagieren
    EICRA |= (1 << ISC11); // INT1 auf fallende Flanke einstellen
    EIMSK |= (1 << INT1); // INT1-Interrupt aktivieren

    // Konfiguration von Timer2 für asynchronen Betrieb
    ASSR |= (1 << AS2); // Taktquelle als Kristall auf TOSCn-Pins auswählen
    TCCR2A = (1 << WGM21); // CTC-Modus
    TCCR2B = (1 << CS22) | (1 << CS20); // Timer Clock = 32768 Hz / 128
    OCR2A = 255; // Output Compare Value für 1 Sekunde Interrupt

    while (ASSR & ((1<<TCN2UB)|(1<<OCR2AUB)|(1<<TCR2AUB)|(1<<TCR2BUB))); // Warten, bis Timer aktualisiert ist

    TIMSK2 |= (1 << OCIE2A); // Timer/Counter2 Output Compare Match A Interrupt aktivieren

    set_sleep_mode(SLEEP_MODE_PWR_SAVE);

    sei();
    while (1) {
        if (sleepEnabled) {
            sleep_mode();
            // Schlafmodus aktivieren
            // sleep_enable();
            //
            //
            // // Kritischen Abschnitt betreten (Interrupts deaktivieren)
            // cli();
            //
            // // Überprüfen, ob der INT1-Interrupt aufgetreten ist, während Interrupts deaktiviert waren
            // if (!(EIFR & (1 << INTF1))) {
            //     // Interrupts aktivieren und in den Schlafmodus gehen
            //     sei();
            //     sleep_cpu();
            //
            //     // CPU wird hier fortgesetzt, wenn sie aufwacht
            // }
            //
            // // Kritischen Abschnitt verlassen (Interrupts aktivieren)
            // sei();
            //
            // // Schlafmodus deaktivieren
            // sleep_disable();
        }else {
            currentTime.increaseTime();
            timeToPins(currentTime);
        }

    }
}
