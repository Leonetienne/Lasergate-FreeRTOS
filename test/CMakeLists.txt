cmake_minimum_required(VERSION 3.16)
project(lasergate_tests CXX)

include(FetchContent)

FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.13.0
)
FetchContent_MakeAvailable(Catch2)

add_executable(lasergate_tests
        GpioPinRegister.cpp
        GpioStub.cpp
        AdcOneshotStub.cpp
        RandomStub.cpp
        GpioDigitalWritePin.cpp
        AdcAnalogReadPin.cpp
        LaserSensor.cpp
        StateMachine.cpp
        TimeStub.cpp
        NVSStub.cpp
        MqttStub.cpp
        EthernetManagerStub.cpp
        HttpServerStub.cpp
        SettingsManager.cpp
        UrlEncodedForm.cpp
        ApiController.cpp
        System.cpp
        Diode.cpp
        LaserDiode.cpp
        LightEmittingDiode.cpp
        GateModule.cpp
        LdrThreshCalibration.cpp
        Gate.cpp
        PulseRingBuffer.cpp
        LdrPhysicsSim.cpp

        ../main/src/StateMachine.cpp
        ../main/src/System.cpp
        ../main/src/GpioPinRegister.cpp
        ../main/src/SettingsManager.cpp
        ../main/src/UrlEncodedForm.cpp
        ../main/src/ApiController.cpp
        ../main/src/Diode.cpp
        ../main/src/platform/GpioDigitalWritePin.cpp
        ../main/src/platform/AdcAnalogReadPin.cpp
        ../main/src/platform/TimeEsp32.cpp
        ../main/src/hal/IAdcOneshot.cpp
        ../main/src/hal/AdcGpioMapping.cpp
        ../main/src/hal/IRandom.cpp
        ../main/include/hal/IRandom.h
        ../main/src/test/stubs/RandomStub.cpp
        ../main/include/test/stubs/RandomStub.h
        ../main/src/test/stubs/GpioStub.cpp
        ../main/src/test/stubs/AdcOneshotStub.cpp
        ../main/src/test/stubs/TimeStub.cpp
        ../main/src/test/stubs/NVSStub.cpp
        ../main/src/test/stubs/MqttStub.cpp
        ../main/src/test/stubs/EthernetManagerStub.cpp
        ../main/src/test/stubs/HttpServerStub.cpp
        ../main/src/test/stubs/SystemStub.cpp
        ../main/include/platform/AdcOneshot.h
        ../main/src/LaserSensor.cpp
        ../main/include/LaserSensor.h
        ../main/src/GateModule.cpp
        ../main/include/GateModule.h
        ../main/include/SettingsManager.h
        ../main/include/LdrThreshCalibConfig.h
        ../main/include/LaserPulseFreqCalibConfig.h
        ../main/include/GateModuleConfig.h
        ../main/src/Gate.cpp
        ../main/include/Gate.h
        ../main/src/PulseRingBuffer.cpp
        ../main/include/PulseRingBuffer.h
        ../main/src/test/stubs/LdrPhysicsSim.cpp
        ../main/include/test/stubs/LdrPhysicsSim.h
)


target_link_libraries(lasergate_tests PRIVATE Catch2::Catch2WithMain)

target_include_directories(lasergate_tests PRIVATE
        ../main/include
)

target_compile_definitions(lasergate_tests PRIVATE HOST_BUILD=1)
target_compile_features(lasergate_tests PRIVATE cxx_std_23)

target_compile_options(lasergate_tests PRIVATE
        -Wall
        -Wextra
        -Werror
        -Wno-unused
)

include(CTest)
include(${catch2_SOURCE_DIR}/extras/Catch.cmake)
catch_discover_tests(lasergate_tests)
