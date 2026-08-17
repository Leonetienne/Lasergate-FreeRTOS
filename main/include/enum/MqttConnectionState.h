#ifndef LASERGATE_V2_MQTTCONNECTIONSTATE_H
#define LASERGATE_V2_MQTTCONNECTIONSTATE_H

enum class MqttConnectionState {
    Disconnected,
    Connecting,
    Connected,
    Failed
};

#endif //LASERGATE_V2_MQTTCONNECTIONSTATE_H
