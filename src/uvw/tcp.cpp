#ifdef UVW_AS_LIB
#include "tcp.h"
#endif

#include "config.h"


namespace uvw {


UVW_INLINE TCPHandle::TCPHandle(ConstructorAccess ca, std::shared_ptr<Loop> ref, unsigned int f)
    : StreamHandle{ca, std::move(ref)}, tag{f ? FLAGS : DEFAULT}, flags{f}
{}


UVW_INLINE bool TCPHandle::init() {
    return (tag == FLAGS) ? initialize(&uv_tcp_init_ex, flags) : initialize(&uv_tcp_init);
}


UVW_INLINE void TCPHandle::open(OSSocketHandle socket) {
    invoke(&uv_tcp_open, get(), socket);
}


UVW_INLINE bool TCPHandle::noDelay(bool value) {
    return (0 == uv_tcp_nodelay(get(), value));
}


UVW_INLINE bool TCPHandle::keepAlive(bool enable, TCPHandle::Time time) {
    return (0 == uv_tcp_keepalive(get(), enable, time.count()));
}


UVW_INLINE bool TCPHandle::simultaneousAccepts(bool enable) {
    return (0 == uv_tcp_simultaneous_accepts(get(), enable));
}


UVW_INLINE void TCPHandle::bind(const sockaddr &addr, Flags<Bind> opts) {
    invoke(&uv_tcp_bind, get(), &addr, opts);
}


template<typename I>
UVW_INLINE void TCPHandle::bind(std::string ip, unsigned int port, Flags<Bind> opts)
{
    typename details::IpTraits<I>::Type addr;
    details::IpTraits<I>::addrFunc(ip.data(), port, &addr);
    bind(reinterpret_cast<const sockaddr &>(addr), std::move(opts));
}


template<typename I>
UVW_INLINE void TCPHandle::bind(Addr addr, Flags<Bind> opts) {
    bind<I>(std::move(addr.ip), addr.port, std::move(opts));
}


template<typename I>
UVW_INLINE Addr TCPHandle::sock() const noexcept {
    return details::address<I>(&uv_tcp_getsockname, get());
}


template<typename I>
UVW_INLINE Addr TCPHandle::peer() const noexcept {
    return details::address<I>(&uv_tcp_getpeername, get());
}


template<typename I>
UVW_INLINE void TCPHandle::connect(std::string ip, unsigned int port) {
    typename details::IpTraits<I>::Type addr;
    details::IpTraits<I>::addrFunc(ip.data(), port, &addr);
    connect(reinterpret_cast<const sockaddr &>(addr));
}


template<typename I>
UVW_INLINE void TCPHandle::connect(Addr addr) {
    connect<I>(std::move(addr.ip), addr.port);
}


UVW_INLINE void TCPHandle::connect(const sockaddr &addr) {
    auto listener = [ptr = shared_from_this()](const auto &event, const auto &) {
        ptr->publish(event);
    };

    auto req = loop().resource<details::ConnectReq>();
    req->once<ErrorEvent>(listener);
    req->once<ConnectEvent>(listener);
    req->connect(&uv_tcp_connect, get(), &addr);
}


UVW_INLINE void TCPHandle::closeReset() {
    invoke(&uv_tcp_close_reset, get(), &this->closeCallback);
}


// explicit instantiations
template<> uvw::Emitter<uvw::TCPHandle, uvw::ConnectEvent, uvw::DataEvent, uvw::EndEvent, uvw::ListenEvent, uvw::WriteEvent, uvw::ShutdownEvent, uvw::CloseEvent, uvw::ErrorEvent>::Emitter() = default;
template void TCPHandle::bind<IPv4>(std::string, unsigned int, Flags<Bind>);
template void TCPHandle::bind<IPv6>(std::string, unsigned int, Flags<Bind>);

template void TCPHandle::bind<IPv4>(Addr, Flags<Bind>);
template void TCPHandle::bind<IPv6>(Addr, Flags<Bind>);

template Addr TCPHandle::sock<IPv4>() const noexcept;
template Addr TCPHandle::sock<IPv6>() const noexcept;

template Addr TCPHandle::peer<IPv4>() const noexcept;
template Addr TCPHandle::peer<IPv6>() const noexcept;

template void TCPHandle::connect<IPv4>(std::string, unsigned int);
template void TCPHandle::connect<IPv6>(std::string, unsigned int);

template void TCPHandle::connect<IPv4>(Addr addr);
template void TCPHandle::connect<IPv6>(Addr addr);

template struct UVW_EXTERN uvw::Emitter<uvw::TCPHandle, uvw::ConnectEvent, uvw::DataEvent, uvw::EndEvent, uvw::ListenEvent, uvw::WriteEvent, uvw::ShutdownEvent, uvw::CloseEvent, uvw::ErrorEvent>::Connection<uvw::ConnectEvent>;
template struct UVW_EXTERN uvw::Emitter<uvw::TCPHandle, uvw::ConnectEvent, uvw::DataEvent, uvw::EndEvent, uvw::ListenEvent, uvw::WriteEvent, uvw::ShutdownEvent, uvw::CloseEvent, uvw::ErrorEvent>::Connection<uvw::ShutdownEvent>;
template struct UVW_EXTERN uvw::Emitter<uvw::TCPHandle, uvw::ConnectEvent, uvw::DataEvent, uvw::EndEvent, uvw::ListenEvent, uvw::WriteEvent, uvw::ShutdownEvent, uvw::CloseEvent, uvw::ErrorEvent>::Connection<uvw::WriteEvent>;
template struct UVW_EXTERN uvw::Emitter<uvw::TCPHandle, uvw::ConnectEvent, uvw::DataEvent, uvw::EndEvent, uvw::ListenEvent, uvw::WriteEvent, uvw::ShutdownEvent, uvw::CloseEvent, uvw::ErrorEvent>::Connection<uvw::CloseEvent>;
template struct UVW_EXTERN uvw::Emitter<uvw::TCPHandle, uvw::ConnectEvent, uvw::DataEvent, uvw::EndEvent, uvw::ListenEvent, uvw::WriteEvent, uvw::ShutdownEvent, uvw::CloseEvent, uvw::ErrorEvent>::Connection<uvw::ListenEvent>;
template struct UVW_EXTERN uvw::Emitter<uvw::TCPHandle, uvw::ConnectEvent, uvw::DataEvent, uvw::EndEvent, uvw::ListenEvent, uvw::WriteEvent, uvw::ShutdownEvent, uvw::CloseEvent, uvw::ErrorEvent>::Connection<uvw::EndEvent>;
template struct UVW_EXTERN uvw::Emitter<uvw::TCPHandle, uvw::ConnectEvent, uvw::DataEvent, uvw::EndEvent, uvw::ListenEvent, uvw::WriteEvent, uvw::ShutdownEvent, uvw::CloseEvent, uvw::ErrorEvent>::Connection<uvw::ErrorEvent>;

template uvw::Emitter<uvw::TCPHandle, uvw::ConnectEvent, uvw::DataEvent, uvw::EndEvent, uvw::ListenEvent, uvw::WriteEvent, uvw::ShutdownEvent, uvw::CloseEvent, uvw::ErrorEvent>::Handler<uvw::ErrorEvent>::Connection
uvw::Emitter<uvw::TCPHandle, uvw::ConnectEvent, uvw::DataEvent, uvw::EndEvent, uvw::ListenEvent, uvw::WriteEvent, uvw::ShutdownEvent, uvw::CloseEvent, uvw::ErrorEvent>::Handler<uvw::ErrorEvent>
::on(std::function<void(struct uvw::ErrorEvent&, uvw::TCPHandle&)>);
template uvw::Emitter<uvw::TCPHandle, uvw::ConnectEvent, uvw::DataEvent, uvw::EndEvent, uvw::ListenEvent, uvw::WriteEvent, uvw::ShutdownEvent, uvw::CloseEvent, uvw::ErrorEvent>::Handler<uvw::CloseEvent>::Connection
uvw::Emitter<uvw::TCPHandle, uvw::ConnectEvent, uvw::DataEvent, uvw::EndEvent, uvw::ListenEvent, uvw::WriteEvent, uvw::ShutdownEvent, uvw::CloseEvent, uvw::ErrorEvent>::Handler<uvw::CloseEvent>
::on(std::function<void(struct uvw::CloseEvent&, uvw::TCPHandle&)>);
template uvw::Emitter<uvw::TCPHandle, uvw::ConnectEvent, uvw::DataEvent, uvw::EndEvent, uvw::ListenEvent, uvw::WriteEvent, uvw::ShutdownEvent, uvw::CloseEvent, uvw::ErrorEvent>::Handler<uvw::ShutdownEvent>::Connection
uvw::Emitter<uvw::TCPHandle, uvw::ConnectEvent, uvw::DataEvent, uvw::EndEvent, uvw::ListenEvent, uvw::WriteEvent, uvw::ShutdownEvent, uvw::CloseEvent, uvw::ErrorEvent>::Handler<uvw::ShutdownEvent>
::once(std::function<void(struct uvw::ShutdownEvent&, uvw::TCPHandle&)>);
template uvw::Emitter<uvw::TCPHandle, uvw::ConnectEvent, uvw::DataEvent, uvw::EndEvent, uvw::ListenEvent, uvw::WriteEvent, uvw::ShutdownEvent, uvw::CloseEvent, uvw::ErrorEvent>::Handler<uvw::EndEvent>::Connection
uvw::Emitter<uvw::TCPHandle, uvw::ConnectEvent, uvw::DataEvent, uvw::EndEvent, uvw::ListenEvent, uvw::WriteEvent, uvw::ShutdownEvent, uvw::CloseEvent, uvw::ErrorEvent>::Handler<uvw::EndEvent>
::on(std::function<void(struct uvw::EndEvent&, uvw::TCPHandle&)>);

template void uvw::StreamHandle<uvw::TCPHandle, uv_tcp_s, uvw::ConnectEvent, uvw::DataEvent, uvw::EndEvent, uvw::ListenEvent, uvw::WriteEvent, uvw::ShutdownEvent, uvw::CloseEvent, uvw::ErrorEvent>::listen(int);

template uvw::Flags<enum uvw::details::UVTCPFlags>::Flags(void);
template bool uvw::StreamHandle<uvw::TCPHandle, uv_tcp_s, uvw::ConnectEvent, uvw::DataEvent, uvw::EndEvent, uvw::ListenEvent, uvw::WriteEvent, uvw::ShutdownEvent, uvw::CloseEvent, uvw::ErrorEvent>::writable(void) const noexcept;
template int uvw::StreamHandle<uvw::TCPHandle, uv_tcp_s, uvw::ConnectEvent, uvw::DataEvent, uvw::EndEvent, uvw::ListenEvent, uvw::WriteEvent, uvw::ShutdownEvent, uvw::CloseEvent, uvw::ErrorEvent>::tryWrite(std::unique_ptr<char[]>, unsigned int);

template uvw::Emitter<uvw::TCPHandle, uvw::ConnectEvent, uvw::DataEvent, uvw::EndEvent, uvw::ListenEvent, uvw::WriteEvent, uvw::ShutdownEvent, uvw::CloseEvent, uvw::ErrorEvent>::Handler<uvw::ConnectEvent>::Connection
uvw::Emitter<uvw::TCPHandle, uvw::ConnectEvent, uvw::DataEvent, uvw::EndEvent, uvw::ListenEvent, uvw::WriteEvent, uvw::ShutdownEvent, uvw::CloseEvent, uvw::ErrorEvent>::Handler<uvw::ConnectEvent>
::once(std::function<void(struct uvw::ConnectEvent&, uvw::TCPHandle&)>);

template void uvw::Emitter<uvw::TCPHandle, uvw::ConnectEvent, uvw::DataEvent, uvw::EndEvent, uvw::ListenEvent, uvw::WriteEvent, uvw::ShutdownEvent, uvw::CloseEvent, uvw::ErrorEvent>::Handler<uvw::WriteEvent>::publish(struct uvw::WriteEvent, uvw::TCPHandle&);

template uvw::Emitter<uvw::TCPHandle, uvw::ConnectEvent, uvw::DataEvent, uvw::EndEvent, uvw::ListenEvent, uvw::WriteEvent, uvw::ShutdownEvent, uvw::CloseEvent, uvw::ErrorEvent>::Handler<uvw::ListenEvent>::Connection
uvw::Emitter<uvw::TCPHandle, uvw::ConnectEvent, uvw::DataEvent, uvw::EndEvent, uvw::ListenEvent, uvw::WriteEvent, uvw::ShutdownEvent, uvw::CloseEvent, uvw::ErrorEvent>::Handler<uvw::ListenEvent>
::once(std::function<void(struct uvw::ListenEvent&, uvw::TCPHandle&)>);

template uvw::Emitter<uvw::TCPHandle, uvw::ConnectEvent, uvw::DataEvent, uvw::EndEvent, uvw::ListenEvent, uvw::WriteEvent, uvw::ShutdownEvent, uvw::CloseEvent, uvw::ErrorEvent>::Handler<uvw::WriteEvent>::Connection
uvw::Emitter<uvw::TCPHandle, uvw::ConnectEvent, uvw::DataEvent, uvw::EndEvent, uvw::ListenEvent, uvw::WriteEvent, uvw::ShutdownEvent, uvw::CloseEvent, uvw::ErrorEvent>::Handler<uvw::WriteEvent>
::once(std::function<void(struct uvw::WriteEvent&, uvw::TCPHandle&)>);

template uvw::Emitter<uvw::TCPHandle, uvw::ConnectEvent, uvw::DataEvent, uvw::EndEvent, uvw::ListenEvent, uvw::WriteEvent, uvw::ShutdownEvent, uvw::CloseEvent, uvw::ErrorEvent>::Handler<uvw::CloseEvent>::Connection
uvw::Emitter<uvw::TCPHandle, uvw::ConnectEvent, uvw::DataEvent, uvw::EndEvent, uvw::ListenEvent, uvw::WriteEvent, uvw::ShutdownEvent, uvw::CloseEvent, uvw::ErrorEvent>::Handler<uvw::CloseEvent>
::once(std::function<void(struct uvw::CloseEvent&, uvw::TCPHandle&)>);

template struct UVW_EXTERN uvw::Emitter<uvw::TCPHandle, uvw::ConnectEvent, uvw::DataEvent, uvw::EndEvent, uvw::ListenEvent, uvw::WriteEvent, uvw::ShutdownEvent, uvw::CloseEvent, uvw::ErrorEvent>::Handler<uvw::DataEvent>;

template void uvw::StreamHandle<uvw::TCPHandle, uv_tcp_s, uvw::ConnectEvent, uvw::DataEvent, uvw::EndEvent, uvw::ListenEvent, uvw::WriteEvent, uvw::ShutdownEvent, uvw::CloseEvent, uvw::ErrorEvent>::read(void);

template uvw::Emitter<uvw::TCPHandle, uvw::ConnectEvent, uvw::DataEvent, uvw::EndEvent, uvw::ListenEvent, uvw::WriteEvent, uvw::ShutdownEvent, uvw::CloseEvent, uvw::ErrorEvent>::Handler<uvw::ErrorEvent>::Connection
uvw::Emitter<uvw::TCPHandle, uvw::ConnectEvent, uvw::DataEvent, uvw::EndEvent, uvw::ListenEvent, uvw::WriteEvent, uvw::ShutdownEvent, uvw::CloseEvent, uvw::ErrorEvent>::Handler<uvw::ErrorEvent>
::once(std::function<void(struct uvw::ErrorEvent&, uvw::TCPHandle&)>);

template void uvw::StreamHandle<uvw::TCPHandle, uv_tcp_s, uvw::ConnectEvent, uvw::DataEvent, uvw::EndEvent, uvw::ListenEvent, uvw::WriteEvent, uvw::ShutdownEvent, uvw::CloseEvent, uvw::ErrorEvent>::shutdown(void);
template int uvw::StreamHandle<uvw::TCPHandle, uv_tcp_s, uvw::ConnectEvent, uvw::DataEvent, uvw::EndEvent, uvw::ListenEvent, uvw::WriteEvent, uvw::ShutdownEvent, uvw::CloseEvent, uvw::ErrorEvent>::tryWrite(char*, unsigned int);
template bool uvw::StreamHandle<uvw::TCPHandle, uv_tcp_s, uvw::ConnectEvent, uvw::DataEvent, uvw::EndEvent, uvw::ListenEvent, uvw::WriteEvent, uvw::ShutdownEvent, uvw::CloseEvent, uvw::ErrorEvent>::readable(void)const noexcept;

template void uvw::StreamHandle<uvw::TCPHandle, uv_tcp_s, uvw::ConnectEvent, uvw::DataEvent, uvw::EndEvent, uvw::ListenEvent, uvw::WriteEvent, uvw::ShutdownEvent, uvw::CloseEvent, uvw::ErrorEvent>::write(char*, unsigned int);
template struct UVW_EXTERN uvw::Emitter<class uvw::TCPHandle, struct uvw::ConnectEvent, struct uvw::DataEvent, struct uvw::EndEvent, struct uvw::ListenEvent, struct uvw::WriteEvent, struct uvw::ShutdownEvent, struct uvw::CloseEvent, struct uvw::ErrorEvent>::Connection<struct uvw::DataEvent>;
}
