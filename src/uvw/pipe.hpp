#pragma once


#include <type_traits>
#include <utility>
#include <memory>
#include <string>
#include <uv.h>
#include "event.hpp"
#include "request.hpp"
#include "stream.hpp"
#include "util.hpp"


namespace uvw {


class Pipe final: public Stream<Pipe> {
    explicit Pipe(std::shared_ptr<Loop> ref)
        : Stream{HandleType<uv_pipe_t>{}, std::move(ref)}
    { }

public:
    template<typename... Args>
    static std::shared_ptr<Pipe> create(Args&&... args) {
        return std::shared_ptr<Pipe>{new Pipe{std::forward<Args>(args)...}};
    }

    bool init(bool ipc = false) { return initialize<uv_pipe_t>(&uv_pipe_init, ipc); }

    void bind(std::string name) {
        invoke(&uv_pipe_bind, get<uv_pipe_t>(), name.data());
    }

    void connect(std::string name) {
        auto listener = [ptr = this->shared_from_this()](const auto &event, details::Connect &) {
            ptr->publish(event);
        };

        auto connect = loop().resource<details::Connect>();
        connect->once<ErrorEvent>(listener);
        connect->once<ConnectEvent>(listener);
        connect->connect(&uv_pipe_connect, get<uv_pipe_t>(), name.data());
    }

    std::string sock() const noexcept { return details::path(&uv_pipe_getsockname, get<uv_pipe_t>()); }
    std::string peer() const noexcept { return details::path(&uv_pipe_getpeername, get<uv_pipe_t>()); }

    void pending(int count) noexcept { uv_pipe_pending_instances(get<uv_pipe_t>(), count); }
    int pending() noexcept { return uv_pipe_pending_count(get<uv_pipe_t>()); }

    // TODO uv_pipe_pending_type
};


}