add_executable(pico_crsf
    main.c
    crsf.c
    crsf_telemetry.c
    crsf_stream_buffer.c
    crsf_crc.c
)

# Add dependencies
target_link_libraries(pico_crsf
    pico_stdlib
	hardware_dma
    pico_time
    hardware_uart
    hardware_gpio
)

# Route stdin/stdout to USB rather than UART
pico_enable_stdio_usb(pico_crsf 1)
pico_enable_stdio_uart(pico_crsf 0)

# create map/bin/hex/uf2 file in addition to ELF.
pico_add_extra_outputs(pico_crsf)
