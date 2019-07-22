#include <iostream>
#include "mqtt.h"

mqtt_client::mqtt_client(std::string id, std::string host, int port,
                         std::function<void(std::string)> onMessage) :
        mosquittopp(id.c_str()), m_onMessage(std::move(onMessage)), m_host(std::move(host)), m_port(port), m_keepalive(DEFAULT_KEEP_ALIVE)
{
#ifdef DEBUG
    std::cerr << "mqtt_client constructor called " << std::endl;
#endif
    mosquittopp::threaded_set();
    mosquittopp::connect(m_host.c_str(), m_port, m_keepalive);
}

void mqtt_client::on_connect(int rc)
{
    if (!rc)
    {
#ifdef DEBUG
        std::cout << "Connected - code " << rc << std::endl;
#endif
    }
}

void mqtt_client::on_subscribe(int mid, int qos_count, const int *granted_qos)
{
#ifdef DEBUG
    std::cout << "Subscription succeeded." << std::endl;
#endif
}

void mqtt_client::on_message(const struct mosquitto_message *message)
{
    std::string std_message{strdup(static_cast<const char *>(message->payload))};
#ifdef DEBUG
    std::cout << "Message: " << std_message << std::endl;
#endif
    // call the callback
    if (m_onMessage)
    {
        m_onMessage(std_message);
    }
}

mqtt_client::~mqtt_client()
{
#ifdef DEBUG
    std::cerr << "mqtt_client destructor called " << std::endl;
#endif
}

void mqtt_client::on_error()
{
    mosquittopp::on_error();
#ifdef DEBUG
    std::cerr << "mqtt_client on_error called " << std::endl;
#endif
}
