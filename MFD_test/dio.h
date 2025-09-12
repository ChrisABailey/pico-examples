#ifndef _DIO_H_
#define _DIO_H_
#include "pico/stdlib.h"
#include "hardware/pwm.h"

class DigitalIn
{
public:
    enum PinMode
    {
        PullNone = 0,
        PullUp = 1,
        PullDown = 2
    };
    DigitalIn(uint8_t pin)
        : _pin(pin)
    {
        gpio_init(_pin);
        gpio_set_dir(_pin, GPIO_IN);
    }

    DigitalIn(uint8_t pin, PinMode _mode)
        : _pin(pin)
    {
        gpio_init(_pin);
        gpio_set_dir(_pin, GPIO_IN);
        mode(_mode);
    }

    int read()
    {
        // Underlying read is thread safe
        return (gpio_get(_pin)?1:0);
    }
    void mode(PinMode _mode)
    {
        switch (_mode)
        {
            case PullNone:
                gpio_set_pulls(_pin, false, false);
                break;
            case PullUp:
                gpio_set_pulls(_pin, true, false);
                break;
            case PullDown:
                gpio_set_pulls(_pin, false, true);
                break;
            default:
                break;
        }
    }
    ~DigitalIn() {gpio_deinit(_pin);}

    /** An operator shorthand for read()
     * \sa DigitalIn::read()
     * @code
     *      DigitalIn  button(BUTTON1);
     *      DigitalOut led(LED1);
     *      led = button;   // Equivalent to led.write(button.read())
     * @endcode
     */
    operator int()
    {
        // Underlying read is thread safe
        return read();
    }

protected:
    uint8_t _pin;
};

class DigitalOut
{
public:
    DigitalOut(uint8_t pin, uint8_t value = 1)
        : _pin(pin)
    {
        gpio_init(_pin);
        gpio_set_dir(_pin, GPIO_OUT);
        write(value);
    }

    void write(uint8_t value)
    {
        // Underlying write is thread safe
        gpio_put(_pin, value!=0);
    }

    uint8_t read()
    {
        // Underlying read is thread safe
        return gpio_get_out_level(_pin);
    }

    void toggle()
    {
        // Underlying write is thread safe
        gpio_xor_mask(1 << _pin);
    }

    ~DigitalOut() {gpio_deinit(_pin);}
 
    /** A shorthand for write()
     * \sa DigitalOut::write()
     * @code
     *      DigitalIn  button(BUTTON1);
     *      DigitalOut led(LED1);
     *      led = button;   // Equivalent to led.write(button.read())
     * @endcode
     */
    DigitalOut &operator= (int value)
    {
        // Underlying write is thread safe
        write(value);
        return *this;
    }

    /** A shorthand for write() using the assignment operator which copies the
     * state from the DigitalOut argument.
     * \sa DigitalOut::write()
     */
    DigitalOut &operator= (DigitalOut &rhs);

    /** A shorthand for read()
     * \sa DigitalOut::read()
     * @code
     *      DigitalIn  button(BUTTON1);
     *      DigitalOut led(LED1);
     *      led = button;   // Equivalent to led.write(button.read())
     * @endcode
     */
    operator int()
    {
        // Underlying call is thread safe
        return read();
    }
    protected:
    uint8_t _pin;
};

class PwmOut
{
public:
    PwmOut(uint8_t pin)
        : _pin(pin)
    {
        //gpio_init(_pin);
        gpio_set_function(_pin, GPIO_FUNC_PWM);
    }

    void period_ms(uint32_t period_ms)
    {
        const float CLOCK_HZ= 125000000.0f;
        uint slice_num = pwm_gpio_to_slice_num(_pin);
        _wrap = (uint16_t)(period_ms/1000.0f * CLOCK_HZ);
        pwm_set_wrap(slice_num, _wrap);
    }

    // Set the Duty cycle of the PWM signal
    // value is between 0.0 and 1.0
    void write(float duty)
    {
        uint slice_num = pwm_gpio_to_slice_num(_pin);
        uint channel = pwm_gpio_to_channel(_pin);
        pwm_set_chan_level(slice_num,channel, (int16_t)(_wrap * duty));
        //printf("Set PWM on pin %d slice %d channel %d to %2.2f%%\r\n",_pin,slice_num,channel,duty*100);
        pwm_set_enabled(slice_num, true);
    }

    ~PwmOut() {gpio_deinit(_pin);}
    /** A shorthand for write()
     * \sa PwmOut::write()
     * @code
     *      PwmOut  pwm(PWM1);
     *      pwm = 0.5;   // Equivalent to pwm.write(0.5)
     * @endcode
     */
    PwmOut &operator= (float duty)
    {
        // Underlying write is thread safe
        write(duty);
        return *this;
    }
protected:
    uint8_t _pin;
    uint16_t _wrap;
};
#endif