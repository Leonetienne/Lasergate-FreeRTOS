#include "test/stubs/EthernetManagerStub.h"

EthernetManagerStub::EthernetManagerStub(
    gpio_num_t indicatorGpioPin,
    IGpio& gpio,
    GpioPinRegister& pinRegister,
    const ITime& i_time
) noexcept :
    gpio(gpio),
    pinRegister(pinRegister),
    i_time(i_time),
    indicatorPin(pinRegister, gpio, indicatorGpioPin)
{
    if (indicatorPin.getGpioNum() != GPIO_NUM_NC) {
        indicatorPin.initialize();
        indicatorPin.setState(PIN_STATE_DIGITAL::LOW);
    }
}

EthernetManagerStub::EthernetManagerStub(EthernetManagerStub&& other) noexcept :
    gpio(other.gpio),
    pinRegister(other.pinRegister),
    i_time(other.i_time),
    state(other.state),
    onConnected(std::move(other.onConnected)),
    onDisconnected(std::move(other.onDisconnected)),
    beginCallCount(other.beginCallCount),
    indicatorPin(std::move(other.indicatorPin))
{
    other.state = EthernetConnectionState::Disconnected;
    other.beginCallCount = 0;
}

bool EthernetManagerStub::begin() noexcept {
    ++beginCallCount;
    isInitialized = true;
    state = EthernetConnectionState::Connecting;
    return true;
}

bool EthernetManagerStub::free() noexcept {
    isInitialized = false;
    state = EthernetConnectionState::Disconnected;
    setIndicatorState(PIN_STATE_DIGITAL::LOW);
    return true;
}

EthernetConnectionState EthernetManagerStub::getState() const noexcept {
    return state;
}

void EthernetManagerStub::setOnConnected(std::function<void()> callback) noexcept {
    onConnected = std::move(callback);
}

void EthernetManagerStub::setOnDisconnected(std::function<void()> callback) noexcept {
    onDisconnected = std::move(callback);
}

void EthernetManagerStub::setIndicatorState(PIN_STATE_DIGITAL pinState) noexcept {
    if (indicatorPin.isReady()) {
        indicatorPin.setState(pinState);
    }
}

void EthernetManagerStub::simulateConnected() {
    state = EthernetConnectionState::Connected;
    setIndicatorState(PIN_STATE_DIGITAL::HIGH);
    if (onConnected) onConnected();
}

void EthernetManagerStub::simulateDisconnected() {
    state = EthernetConnectionState::Disconnected;
    setIndicatorState(PIN_STATE_DIGITAL::LOW);
    if (onDisconnected) onDisconnected();
}

int EthernetManagerStub::getBeginCallCount() const {
    return beginCallCount;
}
