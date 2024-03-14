#include "Time.h"

Time::Time() {
    this->hours = 0;
    this->minutes = 0;
    this->seconds = 0;
}

void Time::increaseTime() {
    this->seconds++;
    if (this->seconds == 60) {
        this->seconds = 0;
        this->minutes++;
        if (this->minutes == 60) {
            this->minutes = 0;
            this->hours++;
            if (this->hours == 24) {
                this->hours = 0;
            }
        }
    }
}

int Time::getHours() const {
    return this->hours;
}

int Time::getMinutes() const {
    return this->minutes;
}

int Time::getSeconds() const {
    return this->seconds;
}
