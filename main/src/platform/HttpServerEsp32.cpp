#include "platform/HttpServerEsp32.h"
#include "ApiController.h"
#include "UrlEncodedForm.h"

namespace {

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
    StateMachine& stateMachine
) noexcept :
    i_ethernetMan(i_ethernetMan),
    i_mqtt(i_mqtt),
    settings(settings),
    stateMachine(stateMachine)
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
    };
    static const httpd_uri_t postSettingsUri = {
        .uri = "/settings*",
        .method = HTTP_POST,
        .handler = handlePostSettings,
        .user_ctx = this,
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

    isInitialized = true;
    return true;
}

bool HttpServerEsp32::free() noexcept {
    if (!isInitialized) {
        return false;
    }

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
    if (uri == "/api/status") {
        report = ApiController::buildStatusReport(self->i_ethernetMan, self->i_mqtt, self->settings);
    } else if (uri == "/api/settings") {
        report = ApiController::buildSettingsReport(self->settings);
    } else if (uri == "/api/settings/advanced") {
        report = ApiController::buildAdvancedSettingsReport(self->settings);
    } else {
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, "text/plain");
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

esp_err_t HttpServerEsp32::handleSettingsForm(httpd_req_t* req) noexcept {
    std::string body;
    if (!readRequestBody(req, body)) {
        httpd_resp_set_status(req, "400 Bad Request");
        httpd_resp_send(req, nullptr, 0);
        return ESP_FAIL;
    }

    const auto form = UrlEncodedForm::parse(body);
    auto* self = static_cast<HttpServerEsp32*>(req->user_ctx);
    if (!ApiController::applySettingsForm(self->settings, self->stateMachine, form)) {
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
    if (!ApiController::applyAdvancedSettingsForm(self->settings, self->stateMachine, form)) {
        httpd_resp_set_status(req, "400 Bad Request");
        httpd_resp_send(req, nullptr, 0);
        return ESP_FAIL;
    }

    httpd_resp_send(req, nullptr, 0);
    return ESP_OK;
}
