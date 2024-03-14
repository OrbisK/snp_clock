#ifndef TIME_H
#define TIME_H


class Time {
private:
    int hours{};
    int minutes{};
    int seconds{};
public:
    Time();
    void increaseTime();
    int getHours() const;
    int getMinutes() const;
    int getSeconds() const;

};


#endif //TIME_H
