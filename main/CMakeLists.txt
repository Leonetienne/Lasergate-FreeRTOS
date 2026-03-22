idf_component_register(SRCS
                        "src/main.cpp"
                        "src/StateMachine.cpp"
                        "src/System.cpp"
                        "src/GpioPinRegister.cpp"
                        "src/hal/IGpio.cpp"
                        "src/platform/GpioEsp32.cpp"
                        "src/platform/GpioDigitalWritePin.cpp"
                        PRIV_REQUIRES spi_flash
                        REQUIRES driver esp_adc
                        INCLUDE_DIRS "include")

target_compile_options(${COMPONENT_LIB} PRIVATE -std=gnu++23)
