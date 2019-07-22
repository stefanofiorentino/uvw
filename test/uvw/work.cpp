#include <gtest/gtest.h>
#include <condition_variable>
#include <mutex>
#include <uvw.hpp>
#include "mqtt.h"


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
        std::this_thread::sleep_for(10s);
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

TEST(Work, mosquitto) {
    auto loop = uvw::Loop::getDefault();
    auto checker = loop->resource<uvw::CheckHandle>();
    auto checker_timer = loop->resource<uvw::TimerHandle>();
    auto fake_delay_timer = loop->resource<uvw::TimerHandle>();

    bool checkWorkEvent = false;
    bool checkTask = false;

    auto waiting_for_commands = loop->resource<uvw::WorkReq>([&checkTask]() {
        ASSERT_FALSE(checkTask);
        checkTask = true;
    });

    waiting_for_commands->on<uvw::ErrorEvent>([](const auto &, auto &) {
        FAIL();
    });

    mqtt_client server("Work.mosquitto.server", "localhost", 60000, [&checkWorkEvent](std::string const &message)
    {
        ASSERT_FALSE(checkWorkEvent);
        checkWorkEvent = true;
    });

    waiting_for_commands->on<uvw::WorkEvent>([&checkWorkEvent, &server](const auto &, auto &waiting_for_commands_handler) {
        ASSERT_FALSE(checkWorkEvent);

        int sub_mid;
        std::string basicString = "TOPIC";
        server.subscribe(&sub_mid, basicString.c_str());
    });

    checker->on<uvw::CheckEvent>([&checkWorkEvent, &server, &checker_timer, &fake_delay_timer](const auto &, auto &checker_handler) {
        server.loop();
        if(checkWorkEvent) {
            checker_handler.stop();
            checker_handler.close();
            checker_timer->stop();
            checker_timer->close();
            fake_delay_timer->stop();
            fake_delay_timer->close();
        }
    });

    fake_delay_timer->on<uvw::TimerEvent>([](const auto &, auto &hndl) {
        mqtt_client client("Work.mosquitto.client", "localhost", 60000, nullptr);
        int pub_mid;
        std::string basicString = "TOPIC";
        client.publish(&pub_mid, basicString.c_str(), 2, "OK");
        client.loop();
    });

    checker_timer->start(uvw::TimerHandle::Time{100}, uvw::TimerHandle::Time{100});
    fake_delay_timer->start(uvw::TimerHandle::Time{5000}, uvw::TimerHandle::Time{0});
    waiting_for_commands->queue();
    checker->start();
    loop->run();

    ASSERT_TRUE(checkWorkEvent);
    ASSERT_TRUE(checkTask);
}