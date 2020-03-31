#include <uvw.hpp>
#include <cassert>
#include <iostream>
#include <memory>
#include <chrono>

int main() {
    using namespace std::chrono_literals;

    // get the default loop
    auto loop = uvw::Loop::getDefault();

    // to keep loop running
    auto idle_handle = loop->resource<uvw::IdleHandle>();
    idle_handle->start();

    // here an async handle is created
    auto async_handle = loop->resource<uvw::AsyncHandle>();
    async_handle->on<uvw::CloseEvent>([loop](auto const&, auto&){
        loop->stop();
    });

    auto thread_handle = loop->resource<uvw::Thread>([](std::shared_ptr<void> data){
        auto async_handle = std::static_pointer_cast<uvw::AsyncHandle>(data);
        uv_sleep(1000);
        async_handle->close();
    }, async_handle);
    thread_handle->run();


    loop->run();
    return 0;
}
