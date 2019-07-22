#include <gtest/gtest.h>
#include <condition_variable>
#include <mutex>
#include <uvw.hpp>
#include "mosquitto.h"

using namespace std::chrono_literals;

TEST(Work, RunTask) {
    auto loop = uvw::Loop::getDefault();
    auto handle = loop->resource<uvw::CheckHandle>();

    bool checkWorkEvent = false;
    bool checkTask = false;

    handle->on<uvw::CheckEvent>([&checkWorkEvent](const auto &, auto &hndl) {
        if(checkWorkEvent) {
            hndl.stop();
            hndl.close();
        }
    });

    auto req = loop->resource<uvw::WorkReq>([&checkTask]() {
        ASSERT_FALSE(checkTask);
        checkTask = true;
    });

    req->on<uvw::ErrorEvent>([](const auto &, auto &) {
        FAIL();
    });

    req->on<uvw::WorkEvent>([&checkWorkEvent](const auto &, auto &) {
        ASSERT_FALSE(checkWorkEvent);
        checkWorkEvent = true;
    });

    handle->start();
    req->queue();
    loop->run();

    ASSERT_TRUE(checkWorkEvent);
    ASSERT_TRUE(checkTask);
}

TEST(Work, Cancellation) {
    auto loop = uvw::Loop::getDefault();
    auto handle = loop->resource<uvw::TimerHandle>();

    bool checkErrorEvent = false;

    handle->on<uvw::TimerEvent>([](const auto &, auto &hndl) {
        hndl.stop();
        hndl.close();
    });

    for(auto i = 0; i < 5 /* default uv thread pool size + 1 */; ++i) {
        auto req = loop->resource<uvw::WorkReq>([]() {});

        req->on<uvw::WorkEvent>([](const auto &, auto &) {});
        req->on<uvw::ErrorEvent>([&checkErrorEvent](const auto &, auto &) { checkErrorEvent = true; });

        req->queue();
        req->cancel();
    }

    handle->start(uvw::TimerHandle::Time{500}, uvw::TimerHandle::Time{500});
    loop->run();

    ASSERT_TRUE(checkErrorEvent);
}

static bool checkWorkEvent = false;
static void message_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message)
{
    ASSERT_FALSE(checkWorkEvent);
    checkWorkEvent = true;
}

TEST(Work, mosquitto) {
    mosquitto_lib_init();
    auto loop = uvw::Loop::getDefault();
    auto checker = loop->resource<uvw::CheckHandle>();
    auto tick_timer = loop->resource<uvw::TimerHandle>();
    auto fake_delay_timer = loop->resource<uvw::TimerHandle>();

    bool checkTask = false;

    auto waiting_for_commands = loop->resource<uvw::WorkReq>([&checkTask]() {
        ASSERT_FALSE(checkTask);
        checkTask = true;
    });

    waiting_for_commands->on<uvw::ErrorEvent>([](const auto &, auto &) {
        FAIL();
    });

    struct mosquitto *mosq_server = mosquitto_new("Work.mosquitto.server", true, 0);

    waiting_for_commands->on<uvw::WorkEvent>([mosq_server](const auto &, auto &waiting_for_commands_handler) {
        ASSERT_FALSE(checkWorkEvent);
        auto rc = mosquitto_connect(mosq_server, "localhost", 60000, 60);
        if (rc)
        {
            FAIL();
        }
        mosquitto_message_callback_set(mosq_server, message_callback);
        int sub_mid;
        mosquitto_subscribe(mosq_server, &sub_mid, "TOPIC", 0);
    });

    checker->on<uvw::CheckEvent>([&tick_timer, &fake_delay_timer, mosq_server](const auto &, auto &checker_handler) {
        auto rc = mosquitto_loop(mosq_server, -1, 1);
        if (rc)
        {
            FAIL();
        }
        if(checkWorkEvent) {
            checker_handler.stop();
            checker_handler.close();
            tick_timer->stop();
            tick_timer->close();
            fake_delay_timer->stop();
            fake_delay_timer->close();
        }
    });

    fake_delay_timer->on<uvw::TimerEvent>([](const auto &, auto &hndl) {
        struct mosquitto *mosq_client = mosquitto_new("Work.mosquitto.client", true, 0);
        auto rc = mosquitto_connect(mosq_client, "localhost", 60000, 60);
        if (rc)
        {
            FAIL();
        }
        int pub_mid;
        mosquitto_publish(mosq_client,&pub_mid, "TOPIC", 2, "OK", 0, false);
        mosquitto_loop(mosq_client, -1, 1);
        mosquitto_destroy(mosq_client);
    });

    tick_timer->start(uvw::TimerHandle::Time{100}, uvw::TimerHandle::Time{100});
    fake_delay_timer->start(uvw::TimerHandle::Time{5000}, uvw::TimerHandle::Time{0});
    waiting_for_commands->queue();
    checker->start();
    loop->run();

    ASSERT_TRUE(checkWorkEvent);
    ASSERT_TRUE(checkTask);
    mosquitto_destroy(mosq_server);
    mosquitto_lib_cleanup();
}