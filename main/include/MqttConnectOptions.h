#ifndef LASERGATE_V2_MQTTCONNECTOPTIONS_H
#define LASERGATE_V2_MQTTCONNECTOPTIONS_H

#include <string>

/**
 * Everything an IMqtt implementation needs to open a broker connection
 */
struct MqttConnectOptions {
    std::string brokerUri;
    std::string username;
    std::string password;
    std::string lwtTopic;
    std::string lwtMessage;
};

#endif //LASERGATE_V2_MQTTCONNECTOPTIONS_H
