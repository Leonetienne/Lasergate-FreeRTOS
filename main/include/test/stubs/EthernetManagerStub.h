#ifndef LASERGATE_V2_ETHERNETMANAGERSTUB_H
#define LASERGATE_V2_ETHERNETMANAGERSTUB_H

#include "hal/IEthernetManager.h"
#include "hal/IGpio.h"
#include "hal/ITime.h"
#include "GpioPinRegister.h"
#include "platform/GpioDigitalWritePin.h"

class EthernetManagerStub : public IEthernetManager {
public:
    EthernetManagerStub(gpio_num_t indicatorGpioPin, IGpio& i_gpio, GpioPinRegister& pinRegister, const ITime& i_time) noexcept;
    EthernetManagerStub(const EthernetManagerStub&) = delete;
    EthernetManagerStub& operator=(const EthernetManagerStub&) = delete;
    EthernetManagerStub(EthernetManagerStub&&) noexcept;
    EthernetManagerStub& operator=(EthernetManagerStub&&) = delete;

    bool begin() noexcept override;
    bool free() noexcept override;
    [[nodiscard]] EthernetConnectionState getState() const noexcept override;
    void setOnConnected(std::function<void()> callback) noexcept override;
    void setOnDisconnected(std::function<void()> callback) noexcept override;

    /**
     * fires callbacks, simulates a real link+ip / link-drop event
     */
    void simulateConnected();
    void simulateDisconnected();

    [[nodiscard]] int getBeginCallCount() const;

private:
    void setIndicatorState(PIN_STATE_DIGITAL pinState) noexcept;

    IGpio& i_gpio;
    GpioPinRegister& pinRegister;
    const ITime& i_time;

    EthernetConnectionState state = EthernetConnectionState::Disconnected;
    std::function<void()> onConnected;
    std::function<void()> onDisconnected;
    int beginCallCount = 0;

    GpioDigitalWritePin indicatorPin;
};

#endif //LASERGATE_V2_ETHERNETMANAGERSTUB_H
