add_executable(AMR_touch_test
        main.c
        i2c.c
        led.c
        bit.c
        lcd.c
        touch.c
        )

# pull in common dependencies and additional i2c hardware support
target_link_libraries(AMR_touch_test pico_stdlib hardware_i2c)

# enable usb output, disable uart output
pico_enable_stdio_usb(AMR_touch_test 1)
pico_enable_stdio_uart(AMR_touch_test 0)

# create map/bin/hex file etc.
pico_add_extra_outputs(AMR_touch_test)

# add url via pico_set_program_url
example_auto_set_url(AMR_touch_test)
