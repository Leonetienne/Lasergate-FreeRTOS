idf_component_register(SRCS
                        "main.cpp"
                        "StateMachine.cpp"
                        "System.cpp"
                        "GpioDigitalWritePin.cpp"
                        "GpioPinRegister.cpp"
                        PRIV_REQUIRES spi_flash
                        REQUIRES driver esp_adc
                        INCLUDE_DIRS "")

target_compile_options(${COMPONENT_LIB} PRIVATE -std=gnu++23)
