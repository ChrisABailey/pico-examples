add_executable(MFD_test
        main.cpp
        SerialUI.cpp
        DUTSerial.cpp
        adv7393.cpp

        )

# pull in common dependencies and additional i2c hardware support
target_link_libraries(MFD_test pico_stdlib hardware_i2c hardware_adc hardware_pwm)

# enable usb output, disable uart output
pico_enable_stdio_usb(MFD_test 1)
pico_enable_stdio_uart(MFD_test 0)

# create map/bin/hex file etc.
pico_add_extra_outputs(MFD_test)

# add url via pico_set_program_url
example_auto_set_url(MFD_test)
