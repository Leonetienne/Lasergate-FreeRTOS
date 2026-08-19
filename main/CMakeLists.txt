idf_component_register(SRCS
                       "src/main.cpp"
                       "src/StateMachine.cpp"
                       "src/System.cpp"
                       "src/GpioPinRegister.cpp"
                       "src/SettingsManager.cpp"
                       "src/UrlEncodedForm.cpp"
                       "src/ApiController.cpp"
                       "src/Diode.cpp"
                       "src/LaserSensor.cpp"
                       "src/hal/IAdcOneshot.cpp"
                       "src/platform/GpioEsp32.cpp"
                       "src/platform/GpioDigitalWritePin.cpp"
                       "src/platform/AdcAnalogReadPin.cpp"
                       "src/platform/AdcOneshot.cpp"
                       "src/platform/TimeEsp32.cpp"
                       "src/platform/NVSEsp32.cpp"
                       "src/platform/MqttEsp32.cpp"
                       "src/platform/EthernetManagerEsp32.cpp"
                       "src/platform/HttpServerEsp32.cpp"
                       "src/platform/SystemEsp32.cpp"
                       PRIV_REQUIRES spi_flash
                       REQUIRES esp_driver_gpio esp_driver_spi esp_adc nvs_flash mqtt esp_eth esp_netif esp_event esp_http_server
                       INCLUDE_DIRS "include"
                       EMBED_FILES "web/index.html" "web/style.css" "web/settings.html" "web/advanced.html")

target_compile_options(${COMPONENT_LIB} PRIVATE -std=gnu++23)
