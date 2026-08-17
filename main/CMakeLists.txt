idf_component_register(SRCS
                       "src/main.cpp"
                       "src/StateMachine.cpp"
                       "src/System.cpp"
                       "src/GpioPinRegister.cpp"
                       "src/SettingsManager.cpp"
                       "src/hal/IAdcOneshot.cpp"
                       "src/platform/GpioEsp32.cpp"
                       "src/platform/GpioDigitalWritePin.cpp"
                       "src/platform/AdcAnalogReadPin.cpp"
                       "src/platform/AdcOneshot.cpp"
                       "src/platform/TimeEsp32.cpp"
                       "src/platform/NVSEsp32.cpp"
                       "src/platform/MqttEsp32.cpp"
                       "src/platform/SystemEsp32.cpp"
                       PRIV_REQUIRES spi_flash
                       REQUIRES esp_driver_gpio esp_adc nvs_flash mqtt
                       INCLUDE_DIRS "include")

target_compile_options(${COMPONENT_LIB} PRIVATE -std=gnu++23)
