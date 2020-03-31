#include <uvw.hpp>
#include <cassert>
#include <iostream>
#include <memory>
#include <chrono>
#include <thread>

int main() {
    using namespace std::chrono_literals;

    // get the default loop
    auto loop = uvw::Loop::getDefault();

    // create a variable for a thread outside of the following scope (next line)
    std::thread loopThread;

    // here an async handle is created
    auto asyncHandle = loop->resource<uvw::AsyncHandle>();

    asyncHandle->on<uvw::CloseEvent>([](auto const&, auto&){
        std::cout << "closing the async handler" << std::endl;
    });

    // now the thread for the loop gets started
    loopThread = std::thread{[loop] {
        loop->run();
    }};

    // just a little delay
    std::this_thread::sleep_for(1s);

    asyncHandle->close();


    // joining the thread (here, the std::this_thread blocks because loopThread will not terminate)
    if(loopThread.joinable()) {
        loopThread.join();
    }
    return 0;
}
