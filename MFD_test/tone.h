// Create a class to play a tone on a piezo buzzer using the pwmout from dio.h

#include "dio.h"
#include "hardware/pwm.h"
#include "pico/stdlib.h"
#include <cmath>

class Tone
{
public:

    enum notes
    {
        C4 = 261,
        D4 = 294,
        E4 = 329,
        F4 = 349,
        G4 = 392,
        A4 = 440,
        B4 = 493,
        C5 = 523,
        D5 = 587,
        E5 = 659,
        F5 = 698,
        G5 = 784,
        A5 = 880,
        B5 = 987
    };

    enum durations
    {
        WHOLE = 1000,
        HALF = 500,
        QUARTER = 250,
        EIGHTH = 125,
        SIXTEENTH = 62
    };

    Tone(uint8_t pin) : 
    _pwm(pin)
    {
    }

    void play(uint16_t frequency, uint8_t volume = 10)
    {
        // Calculate the period in milliseconds
        uint32_t period = 1000 / frequency;
        _pwm.period_ms(period);
        _pwm.write(pow(0.5, ((5-(volume/2))))); // 50% duty cycle
    }

    void playNote(uint16_t frequency, uint32_t duration, uint8_t volume = 10)
    {
        play(frequency, volume);
        sleep_ms(duration);
        stop();
    }

    void playNote(notes note, durations dur, uint8_t volume = 10)
    {
        playNote((uint16_t)note, (uint32_t)dur, volume);
    }

    void playTune(notes notelist[], durations durlist[], size_t length, uint8_t volume = 10)
    {
        for (size_t i = 0; i < length; i++)
        {
            printf("Playing note %d for %d ms\n", (uint16_t)notelist[i], (uint32_t)durlist[i]);
            playNote(notelist[i], durlist[i], volume);
            sleep_ms(50); // brief pause between notes
        }
    }

    void stop()
    {
        _pwm.write(0.0f);
    }

private:
    PwmOut _pwm;
};  