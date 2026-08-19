#ifndef LASERGATE_V2_HTTPSERVERESP32_H
#define LASERGATE_V2_HTTPSERVERESP32_H

#include "esp_http_server.h"
#include "hal/IHttpServer.h"
#include "hal/IEthernetManager.h"
#include "hal/IMqtt.h"
#include "SettingsManager.h"
#include "StateMachine.h"

/**
 * Esp32-Implementation of the web ui / api http server.
 */
class HttpServerEsp32 : public IHttpServer {
public:
    HttpServerEsp32(IEthernetManager& i_ethernetMan, IMqtt& i_mqtt, SettingsManager& settings, StateMachine& stateMachine) noexcept;
    HttpServerEsp32(const HttpServerEsp32&) = delete;
    HttpServerEsp32& operator=(const HttpServerEsp32&) = delete;
    HttpServerEsp32(HttpServerEsp32&&) = delete;
    HttpServerEsp32& operator=(HttpServerEsp32&&) = delete;
    ~HttpServerEsp32() noexcept override;

    /**
     * Starts the http server and registers all uri handlers
     * @return Success state
     */
    bool begin() noexcept override;

    /**
     * Stops the http server and releases the resources acquired by begin()
     * @return Success state
     */
    bool free() noexcept override;

private:
    /**
     * Serves the embedded index page
     */
    static esp_err_t handleGetIndex(httpd_req_t* req) noexcept;

    /**
     * Serves the embedded stylesheet
     */
    static esp_err_t handleGetStyle(httpd_req_t* req) noexcept;

    /**
     * Serves the embedded settings page
     */
    static esp_err_t handleGetSettingsPage(httpd_req_t* req) noexcept;

    /**
     * Serves the embedded advanced settings page
     */
    static esp_err_t handleGetAdvancedPage(httpd_req_t* req) noexcept;

    /**
     * Routes GET /api/ requests
     */
    static esp_err_t handleGetApi(httpd_req_t* req) noexcept;

    /**
     * Routes POST /settings* requests
     */
    static esp_err_t handlePostSettings(httpd_req_t* req) noexcept;

    /**
     * Saves the settings form submitted via the web ui and requests a shutdown
     */
    static esp_err_t handleSettingsForm(httpd_req_t* req) noexcept;

    /**
     * Saves the advanced settings form submitted via the web ui and requests a shutdown
     */
    static esp_err_t handleAdvancedSettingsForm(httpd_req_t* req) noexcept;

    bool isInitialized = false;
    httpd_handle_t server = nullptr;
    IEthernetManager& i_ethernetMan;
    IMqtt& i_mqtt;
    SettingsManager& settings;
    StateMachine& stateMachine;
};

#endif //LASERGATE_V2_HTTPSERVERESP32_H
