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
        GpioDigitalWritePin.cpp

        ../main/src/StateMachine.cpp
        ../main/src/System.cpp
        ../main/src/platform/GpioDigitalWritePin.cpp
        ../main/src/GpioPinRegister.cpp
        ../main/src/hal/IAdcOneshot.cpp
        ../main/src/test/stubs/GpioStub.cpp
        ../main/src/test/stubs/AdcOneshotStub.cpp
)


target_link_libraries(lasergate_tests PRIVATE Catch2::Catch2WithMain)

target_include_directories(lasergate_tests PRIVATE
        ../main/include
)

target_compile_definitions(lasergate_tests PRIVATE HOST_BUILD=1)
target_compile_features(lasergate_tests PRIVATE cxx_std_23)

include(CTest)
include(${catch2_SOURCE_DIR}/extras/Catch.cmake)
catch_discover_tests(lasergate_tests)
