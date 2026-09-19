#include "platform/HttpServerEsp32.h"
#include "ApiController.h"
#include "StateMachine.h"
#include "UrlEncodedForm.h"
#include <array>
#include <vector>

namespace {

constexpr uint16_t MAX_OPEN_SOCKETS = 7; // LWIP_MAX_SOCKETS(10) - 3 reserved internally by httpd
constexpr uint16_t MAX_URI_HANDLERS = 10;
constexpr uint64_t BROADCAST_INTERVAL_US = 500 * 1000; // how often the /ws snapshot is pushed

bool readRequestBody(httpd_req_t* req, std::string& outBody) noexcept {
    char buf[512] = {};
    const int contentLength = req->content_len < sizeof(buf) - 1
        ? static_cast<int>(req->content_len)
        : static_cast<int>(sizeof(buf) - 1);

    const int received = httpd_req_recv(req, buf, contentLength);
    if (received <= 0) {
        return false;
    }

    outBody.assign(buf, static_cast<std::size_t>(received));
    return true;
}

esp_err_t serveEmbedded(httpd_req_t* req, const uint8_t* start, const uint8_t* end, const char* contentType) noexcept {
    httpd_resp_set_type(req, contentType);
    return httpd_resp_send(req, reinterpret_cast<const char*>(start), static_cast<ssize_t>(end - start));
}

// trailing "/api/modules/<n>" index, n is a single digit 0-3
std::optional<std::size_t> parseModuleIndex(std::string_view uri) noexcept {
    const auto lastSlash = uri.rfind('/');
    if (lastSlash == std::string_view::npos || lastSlash + 1 >= uri.size()) {
        return std::nullopt;
    }

    const std::string_view segment = uri.substr(lastSlash + 1);
    if (segment.size() != 1 || segment[0] < '0' || segment[0] > '3') {
        return std::nullopt;
    }

    return static_cast<std::size_t>(segment[0] - '0');
}

}

extern const uint8_t index_html_start[] asm("_binary_index_html_start");
extern const uint8_t index_html_end[] asm("_binary_index_html_end");
extern const uint8_t style_css_start[] asm("_binary_style_css_start");
extern const uint8_t style_css_end[] asm("_binary_style_css_end");
extern const uint8_t settings_html_start[] asm("_binary_settings_html_start");
extern const uint8_t settings_html_end[] asm("_binary_settings_html_end");
extern const uint8_t advanced_html_start[] asm("_binary_advanced_html_start");
extern const uint8_t advanced_html_end[] asm("_binary_advanced_html_end");

HttpServerEsp32::HttpServerEsp32(
    IEthernetManager& i_ethernetMan,
    IMqtt& i_mqtt,
    SettingsManager& settings,
    ApiController& apiController
) noexcept :
    i_ethernetMan(i_ethernetMan),
    i_mqtt(i_mqtt),
    settings(settings),
    apiController(apiController)
{ }

HttpServerEsp32::~HttpServerEsp32() noexcept {
    if (isInitialized) {
        HttpServerEsp32::free();
    }
}

bool HttpServerEsp32::begin() noexcept {
    if (isInitialized) {
        return false;
    }

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.uri_match_fn = httpd_uri_match_wildcard;
    config.max_open_sockets = MAX_OPEN_SOCKETS;
    config.max_uri_handlers = MAX_URI_HANDLERS;

    if (httpd_start(&server, &config) != ESP_OK) {
        return false;
    }

    static const httpd_uri_t getIndexUri = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = handleGetIndex,
        .user_ctx = nullptr,
    };
    static const httpd_uri_t getStyleUri = {
        .uri = "/style.css",
        .method = HTTP_GET,
        .handler = handleGetStyle,
        .user_ctx = nullptr,
    };
    static const httpd_uri_t getSettingsPageUri = {
        .uri = "/settings",
        .method = HTTP_GET,
        .handler = handleGetSettingsPage,
        .user_ctx = nullptr,
    };
    static const httpd_uri_t getAdvancedPageUri = {
        .uri = "/settings/advanced",
        .method = HTTP_GET,
        .handler = handleGetAdvancedPage,
        .user_ctx = nullptr,
    };
    static const httpd_uri_t getApiUri = {
        .uri = "/api/*",
        .method = HTTP_GET,
        .handler = handleGetApi,
        .user_ctx = this,
        .is_websocket = false,
        .handle_ws_control_frames = false,
        .supported_subprotocol = nullptr,
    };
    static const httpd_uri_t postSettingsUri = {
        .uri = "/settings*",
        .method = HTTP_POST,
        .handler = handlePostSettings,
        .user_ctx = this,
        .is_websocket = false,
        .handle_ws_control_frames = false,
        .supported_subprotocol = nullptr,
    };
    static const httpd_uri_t postApiStateUri = {
        .uri = "/api/state",
        .method = HTTP_POST,
        .handler = handlePostApiState,
        .user_ctx = this,
        .is_websocket = false,
        .handle_ws_control_frames = false,
        .supported_subprotocol = nullptr,
    };
    static const httpd_uri_t postApiSettingsResetUri = {
        .uri = "/api/settings/reset",
        .method = HTTP_POST,
        .handler = handlePostApiSettingsReset,
        .user_ctx = this,
        .is_websocket = false,
        .handle_ws_control_frames = false,
        .supported_subprotocol = nullptr,
    };
    static const httpd_uri_t postApiModuleUri = {
        .uri = "/api/modules/*",
        .method = HTTP_POST,
        .handler = handlePostApiModule,
        .user_ctx = this,
        .is_websocket = false,
        .handle_ws_control_frames = false,
        .supported_subprotocol = nullptr,
    };
    static const httpd_uri_t postApiGpioDiscoveryPinUri = {
        .uri = "/api/gpio-discovery/pin",
        .method = HTTP_POST,
        .handler = handlePostApiGpioDiscoveryPin,
        .user_ctx = this,
        .is_websocket = false,
        .handle_ws_control_frames = false,
        .supported_subprotocol = nullptr,
    };
    static const httpd_uri_t wsUri = {
        .uri = "/ws",
        .method = HTTP_GET,
        .handler = handleWs,
        .user_ctx = this,
        .is_websocket = true,
        .handle_ws_control_frames = false,
        .supported_subprotocol = nullptr,
    };

    if (httpd_register_uri_handler(server, &getIndexUri) != ESP_OK) {
        return false;
    }
    if (httpd_register_uri_handler(server, &getStyleUri) != ESP_OK) {
        return false;
    }
    if (httpd_register_uri_handler(server, &getSettingsPageUri) != ESP_OK) {
        return false;
    }
    if (httpd_register_uri_handler(server, &getAdvancedPageUri) != ESP_OK) {
        return false;
    }
    if (httpd_register_uri_handler(server, &getApiUri) != ESP_OK) {
        return false;
    }
    if (httpd_register_uri_handler(server, &postSettingsUri) != ESP_OK) {
        return false;
    }
    if (httpd_register_uri_handler(server, &postApiStateUri) != ESP_OK) {
        return false;
    }
    if (httpd_register_uri_handler(server, &postApiSettingsResetUri) != ESP_OK) {
        return false;
    }
    if (httpd_register_uri_handler(server, &postApiModuleUri) != ESP_OK) {
        return false;
    }
    if (httpd_register_uri_handler(server, &postApiGpioDiscoveryPinUri) != ESP_OK) {
        return false;
    }
    if (httpd_register_uri_handler(server, &wsUri) != ESP_OK) {
        return false;
    }

    const esp_timer_create_args_t timerArgs = {
        .callback = onBroadcastTimer,
        .arg = this,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "ws_broadcast",
        .skip_unhandled_events = false,
    };
    if (esp_timer_create(&timerArgs, &broadcastTimer) != ESP_OK) {
        return false;
    }
    if (esp_timer_start_periodic(broadcastTimer, BROADCAST_INTERVAL_US) != ESP_OK) {
        return false;
    }

    isInitialized = true;
    return true;
}

bool HttpServerEsp32::free() noexcept {
    if (!isInitialized) {
        return false;
    }

    esp_timer_stop(broadcastTimer);
    esp_timer_delete(broadcastTimer);
    broadcastTimer = nullptr;

    if (httpd_stop(server) != ESP_OK) {
        return false;
    }

    server = nullptr;
    isInitialized = false;
    return true;
}

esp_err_t HttpServerEsp32::handleGetIndex(httpd_req_t* req) noexcept {
    return serveEmbedded(req, index_html_start, index_html_end, "text/html");
}

esp_err_t HttpServerEsp32::handleGetStyle(httpd_req_t* req) noexcept {
    return serveEmbedded(req, style_css_start, style_css_end, "text/css");
}

esp_err_t HttpServerEsp32::handleGetSettingsPage(httpd_req_t* req) noexcept {
    return serveEmbedded(req, settings_html_start, settings_html_end, "text/html");
}

esp_err_t HttpServerEsp32::handleGetAdvancedPage(httpd_req_t* req) noexcept {
    return serveEmbedded(req, advanced_html_start, advanced_html_end, "text/html");
}

esp_err_t HttpServerEsp32::handleGetApi(httpd_req_t* req) noexcept {
    const std::string_view uri = req->uri;
    auto* self = static_cast<HttpServerEsp32*>(req->user_ctx);

    std::string report;
    const char* contentType = "text/plain";

    if (uri == "/api/status") {
        report = ApiController::buildStatusReport(self->i_ethernetMan, self->i_mqtt, self->settings);
    } else if (uri == "/api/settings") {
        report = ApiController::buildSettingsReport(self->settings);
    } else if (uri == "/api/settings/advanced") {
        report = ApiController::buildAdvancedSettingsReport(self->settings);
    } else if (uri == "/api/state") {
        report = ApiController::buildStateJson(self->apiController.getSnapshot());
        contentType = "application/json";
    } else {
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, contentType);
    return httpd_resp_send(req, report.c_str(), static_cast<ssize_t>(report.size()));
}

esp_err_t HttpServerEsp32::handlePostSettings(httpd_req_t* req) noexcept {
    const std::string_view uri = req->uri;

    if (uri == "/settings/advanced") {
        return handleAdvancedSettingsForm(req);
    }
    if (uri == "/settings") {
        return handleSettingsForm(req);
    }

    httpd_resp_send_404(req);
    return ESP_FAIL;
}

void HttpServerEsp32::sendBadRequest(httpd_req_t* req, const std::string& message) noexcept {
    httpd_resp_set_status(req, "400 Bad Request");
    httpd_resp_set_type(req, "text/plain");
    httpd_resp_send(req, message.c_str(), static_cast<ssize_t>(message.size()));
}

esp_err_t HttpServerEsp32::handleSettingsForm(httpd_req_t* req) noexcept {
    std::string body;
    if (!readRequestBody(req, body)) {
        httpd_resp_set_status(req, "400 Bad Request");
        httpd_resp_send(req, nullptr, 0);
        return ESP_FAIL;
    }

    const auto form = UrlEncodedForm::parse(body);
    auto* self = static_cast<HttpServerEsp32*>(req->user_ctx);
    if (!self->apiController.applySettingsForm(self->settings, form)) {
        httpd_resp_set_status(req, "400 Bad Request");
        httpd_resp_send(req, nullptr, 0);
        return ESP_FAIL;
    }

    httpd_resp_send(req, nullptr, 0);
    return ESP_OK;
}

esp_err_t HttpServerEsp32::handleAdvancedSettingsForm(httpd_req_t* req) noexcept {
    std::string body;
    if (!readRequestBody(req, body)) {
        httpd_resp_set_status(req, "400 Bad Request");
        httpd_resp_send(req, nullptr, 0);
        return ESP_FAIL;
    }

    const auto form = UrlEncodedForm::parse(body);
    auto* self = static_cast<HttpServerEsp32*>(req->user_ctx);
    if (const auto result = self->apiController.applyAdvancedSettingsForm(self->settings, form); !result) {
        sendBadRequest(req, result.error());
        return ESP_FAIL;
    }

    httpd_resp_send(req, nullptr, 0);
    return ESP_OK;
}

esp_err_t HttpServerEsp32::handlePostApiState(httpd_req_t* req) noexcept {
    std::string body;
    if (!readRequestBody(req, body)) {
        httpd_resp_set_status(req, "400 Bad Request");
        httpd_resp_send(req, nullptr, 0);
        return ESP_FAIL;
    }

    const auto form = UrlEncodedForm::parse(body);
    const auto it = form.find("state");
    const auto requested = it != form.end() ? StateMachine::fromString(it->second) : std::nullopt;

    auto* self = static_cast<HttpServerEsp32*>(req->user_ctx);
    const STATE current = self->apiController.getSnapshot().state;

    if (!requested.has_value() || !self->apiController.requestStateIfAllowed(current, *requested)) {
        httpd_resp_set_status(req, "400 Bad Request");
        httpd_resp_send(req, nullptr, 0);
        return ESP_FAIL;
    }

    httpd_resp_send(req, nullptr, 0);
    return ESP_OK;
}

esp_err_t HttpServerEsp32::handlePostApiSettingsReset(httpd_req_t* req) noexcept {
    auto* self = static_cast<HttpServerEsp32*>(req->user_ctx);
    const STATE current = self->apiController.getSnapshot().state;

    if (const auto result = self->apiController.resetSettingsToDefaults(self->settings, current); !result) {
        sendBadRequest(req, result.error());
        return ESP_FAIL;
    }

    httpd_resp_send(req, nullptr, 0);
    return ESP_OK;
}

esp_err_t HttpServerEsp32::handlePostApiModule(httpd_req_t* req) noexcept {
    const auto moduleIndex = parseModuleIndex(req->uri);
    if (!moduleIndex.has_value()) {
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }

    std::string body;
    if (!readRequestBody(req, body)) {
        httpd_resp_set_status(req, "400 Bad Request");
        httpd_resp_send(req, nullptr, 0);
        return ESP_FAIL;
    }

    const auto form = UrlEncodedForm::parse(body);
    auto* self = static_cast<HttpServerEsp32*>(req->user_ctx);
    if (const auto result = self->apiController.applyModuleConfigForm(self->settings, *moduleIndex, form); !result) {
        sendBadRequest(req, result.error());
        return ESP_FAIL;
    }

    httpd_resp_send(req, nullptr, 0);
    return ESP_OK;
}

esp_err_t HttpServerEsp32::handlePostApiGpioDiscoveryPin(httpd_req_t* req) noexcept {
    std::string body;
    if (!readRequestBody(req, body)) {
        httpd_resp_set_status(req, "400 Bad Request");
        httpd_resp_send(req, nullptr, 0);
        return ESP_FAIL;
    }

    const auto form = UrlEncodedForm::parse(body);
    auto* self = static_cast<HttpServerEsp32*>(req->user_ctx);

    if (self->apiController.getSnapshot().state != STATE::DIAGNOSTIC_GPIO_DISCOVERY ||
        !self->apiController.requestGpioDiscoveryPinLevel(form)) {
        httpd_resp_set_status(req, "400 Bad Request");
        httpd_resp_send(req, nullptr, 0);
        return ESP_FAIL;
    }

    httpd_resp_send(req, nullptr, 0);
    return ESP_OK;
}

esp_err_t HttpServerEsp32::handleWs(httpd_req_t* req) noexcept {
    // push only, client frames are drained and discarded
    httpd_ws_frame_t frame{};
    frame.type = HTTPD_WS_TYPE_TEXT;

    esp_err_t ret = httpd_ws_recv_frame(req, &frame, 0);
    if (ret != ESP_OK || frame.len == 0) {
        return ret;
    }

    std::vector<uint8_t> payload(frame.len);
    frame.payload = payload.data();
    return httpd_ws_recv_frame(req, &frame, frame.len);
}

void HttpServerEsp32::onBroadcastTimer(void* arg) noexcept {
    auto* self = static_cast<HttpServerEsp32*>(arg);
    httpd_queue_work(self->server, broadcastToWebsockets, self);
}

void HttpServerEsp32::broadcastToWebsockets(void* arg) noexcept {
    auto* self = static_cast<HttpServerEsp32*>(arg);
    std::string json = ApiController::buildStateJson(self->apiController.getSnapshot());

    httpd_ws_frame_t frame{};
    frame.type = HTTPD_WS_TYPE_TEXT;
    frame.payload = reinterpret_cast<uint8_t*>(json.data());
    frame.len = json.size();

    std::array<int, MAX_OPEN_SOCKETS> clientFds{};
    std::size_t fdCount = clientFds.size();
    if (httpd_get_client_list(self->server, &fdCount, clientFds.data()) != ESP_OK) {
        return;
    }

    for (std::size_t i = 0; i < fdCount; ++i) {
        if (httpd_ws_get_fd_info(self->server, clientFds[i]) == HTTPD_WS_CLIENT_WEBSOCKET) {
            httpd_ws_send_frame_async(self->server, clientFds[i], &frame);
        }
    }
}
