#ifndef LASERGATE_V2_HTTPSERVERESP32_H
#define LASERGATE_V2_HTTPSERVERESP32_H

#include "esp_http_server.h"
#include "esp_timer.h"
#include "hal/IHttpServer.h"
#include "hal/IEthernetManager.h"
#include "hal/IMqtt.h"
#include "ApiController.h"
#include "SettingsManager.h"

/**
 * Esp32-Implementation of the web ui / api http server. Serves the embedded static
 * frontend, the text/JSON api, and a websocket (/ws) that periodically broadcasts the
 * live system/module state built by ApiController::publishSnapshot().
 */
class HttpServerEsp32 : public IHttpServer {
public:
    HttpServerEsp32(IEthernetManager& i_ethernetMan, IMqtt& i_mqtt, SettingsManager& settings, ApiController& apiController) noexcept;
    HttpServerEsp32(const HttpServerEsp32&) = delete;
    HttpServerEsp32& operator=(const HttpServerEsp32&) = delete;
    HttpServerEsp32(HttpServerEsp32&&) = delete;
    HttpServerEsp32& operator=(HttpServerEsp32&&) = delete;
    ~HttpServerEsp32() noexcept override;

    /**
     * Starts the http server, registers all uri handlers and starts the websocket broadcast timer
     * @return Success state
     */
    bool begin() noexcept override;

    /**
     * Stops the broadcast timer and the http server, and releases the resources acquired by begin()
     * @return Success state
     */
    bool free() noexcept override;

private:
    /**
     * Serves the embedded static asset (html/css/js) matching req->uri, or 404
     */
    static esp_err_t handleGetStatic(httpd_req_t* req) noexcept;

    /**
     * Routes GET requests under /api
     */
    static esp_err_t handleGetApi(httpd_req_t* req) noexcept;

    /**
     * Routes POST /settings* requests
     */
    static esp_err_t handlePostSettings(httpd_req_t* req) noexcept;

    /**
     * Sends a 400 whose plain text body is message
     */
    static void sendBadRequest(httpd_req_t* req, const std::string& message) noexcept;

    /**
     * Saves the settings form submitted via the web ui and requests a shutdown
     */
    static esp_err_t handleSettingsForm(httpd_req_t* req) noexcept;

    /**
     * Saves the advanced settings form submitted via the web ui and requests a shutdown
     */
    static esp_err_t handleAdvancedSettingsForm(httpd_req_t* req) noexcept;

    /**
     * Routes POST /api/state {state=NAME}: requests a state transition if currently allowed
     */
    static esp_err_t handlePostApiState(httpd_req_t* req) noexcept;

    /**
     * Erases all user settings and requests a shutdown
     */
    static esp_err_t handlePostApiSettingsReset(httpd_req_t* req) noexcept;

    /**
     * Routes POST /api/modules/{0..3}: saves a single gate module's manual configuration
     */
    static esp_err_t handlePostApiModule(httpd_req_t* req) noexcept;

    /**
     * Routes POST /api/gpio-discovery/pin {pin=N,level=0|1}: queues a pin write, only
     * accepted while the system is in STATE::DIAGNOSTIC_GPIO_DISCOVERY
     */
    static esp_err_t handlePostApiGpioDiscoveryPin(httpd_req_t* req) noexcept;

    /**
     * Upgrades GET /ws to a websocket for server push, driven by the broadcast timer.
     * Client frames are discarded.
     */
    static esp_err_t handleWs(httpd_req_t* req) noexcept;

    /**
     * Pushes the current snapshot to every open websocket connection.
     * Queued by onBroadcastTimer, so it runs on the httpd task.
     */
    static void broadcastToWebsockets(void* arg) noexcept;

    /**
     * esp_timer callback: queues broadcastToWebsockets on the httpd task
     */
    static void onBroadcastTimer(void* arg) noexcept;

    bool isInitialized = false;
    httpd_handle_t server = nullptr;
    esp_timer_handle_t broadcastTimer = nullptr;
    IEthernetManager& i_ethernetMan;
    IMqtt& i_mqtt;
    SettingsManager& settings;
    ApiController& apiController;
};

#endif //LASERGATE_V2_HTTPSERVERESP32_H
