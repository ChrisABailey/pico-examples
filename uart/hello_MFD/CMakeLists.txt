add_executable(hello_MFD
        hello_MFD.c
        )

# pull in common dependencies
target_link_libraries(hello_MFD pico_stdlib)

# create map/bin/hex file etc.
pico_add_extra_outputs(hello_MFD)

# add url via pico_set_program_url
example_auto_set_url(hello_MFD)
