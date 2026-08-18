#ifndef LASERGATE_V2_IETHERNETMANAGER_H
#define LASERGATE_V2_IETHERNETMANAGER_H

#include <functional>
#include "enum/EthernetConnectionState.h"

/**
 * Abstract interface to manage a wired ethernet connection
 */
class IEthernetManager {
public:
    IEthernetManager() = default;
    IEthernetManager(const IEthernetManager&) = delete;
    IEthernetManager& operator=(const IEthernetManager&) = delete;
    IEthernetManager(IEthernetManager&&) = delete;
    IEthernetManager& operator=(IEthernetManager&&) = delete;
    virtual ~IEthernetManager() = default;

    /**
     * Brings up the ethernet interface and starts DHCP
     * @return Success state
     */
    virtual bool begin() noexcept = 0;

    /**
     * Will release the resources acquired by begin()
     * @return Success state
     */
    virtual bool free() noexcept = 0;

    /**
     * @return The ethernet connection state
     */
    [[nodiscard]] virtual EthernetConnectionState getState() const noexcept = 0;

    /**
     * Callback setter. Fired once the link is up and an IP address has been acquired.
     * @param callback
     */
    virtual void setOnConnected(std::function<void()> callback) noexcept = 0;

    /**
     * Callback setter. Fired once a previously established connection (link or IP) is lost.
     * @param callback
     */
    virtual void setOnDisconnected(std::function<void()> callback) noexcept = 0;

    /**
     * @return Whether begin() has been called successfully without a matching free() yet
     */
    [[nodiscard]] bool isReady() const noexcept { return isInitialized; }

protected:
    bool isInitialized = false;
};

#endif //LASERGATE_V2_IETHERNETMANAGER_H
