#ifndef AUTHORIZATION_MQTT_HPP
#define AUTHORIZATION_MQTT_HPP


#include <mosquittopp.h>
#include <cstring>
#include <cstdio>
#include <string>
#include <functional>
#include <thread>
#include <vector>
#include <uuid/uuid.h>
#include <sstream>

#ifdef DEBUG
#include <iostream>
#endif

#define DEFAULT_KEEP_ALIVE 60

class mqtt_client : public mosqpp::mosquittopp
{
    std::function<void(std::string const &)> m_onMessage;
    std::string m_host;
    int m_keepalive{};
    int m_port{};

public:
    mqtt_client(std::string id, std::string host, int port,
                std::function<void(std::string)> onMessage);

    void on_connect(int rc) final;

    void on_subscribe(int mid, int qos_count, const int *granted_qos) final;

    void on_message(const struct mosquitto_message *message) final;

    void on_error() final;

    ~mqtt_client();
};

#endif //AUTHORIZATION_MQTT_HPP
