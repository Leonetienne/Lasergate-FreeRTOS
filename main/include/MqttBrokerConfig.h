#ifndef LASERGATE_V2_MQTTBROKERCONFIG_H
#define LASERGATE_V2_MQTTBROKERCONFIG_H

#include <string>

struct MqttBrokerConfig {
    std::string uri;
    std::string username;
    std::string password;
};

#endif //LASERGATE_V2_MQTTBROKERCONFIG_H
