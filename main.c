#include<avr/io.h>
#include<util/delay.h>
#include<stdbool.h>;
#include<avr/interrupt.h>

volatile bool test = false;


ISR(PCINT0_vect){
    test = !test;
}


ISR(PCINT1_vect){
    test = !test;
}


ISR(INT0_vect){
    test = !test;
}


int main() {
    DDRC |=(63);
    DDRD |= (0b11111000);


    DDRB &= ~((1 << PINB0) | (1 << PINB1));
    PORTB |= ((1 << PORTB0) | (1 << PORTB1));

    PORTD |= (1 << PORTD2);

    DDRD &= ~(1 << PIND2);

    PCICR |= (1 << PCIE0);
    PCMSK0 |= ((1 << PCINT0) | (1 << PCINT1));

    EIMSK |= (1 << INT0);
    EICRA |= (1 << ISC00);

    sei();
    while (1) {

        if(test){
            PORTC |=(63) ;
            PORTD |=(0b11111000);
        }else{
            PORTC &=(0);
            PORTD &= (1 << PORTD2);
        }

    }
    return 0;
}
