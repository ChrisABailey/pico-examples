add_executable(LAD_test
        main.c
        i2c.c
        led.c
        bit.c
        lcd.c
        )

# pull in common dependencies and additional i2c hardware support
target_link_libraries(LAD_test pico_stdlib hardware_i2c)

# enable usb output, disable uart output
pico_enable_stdio_usb(LAD_test 1)
pico_enable_stdio_uart(LAD_test 0)

# create map/bin/hex file etc.
pico_add_extra_outputs(LAD_test)

# add url via pico_set_program_url
example_auto_set_url(LAD_test)
