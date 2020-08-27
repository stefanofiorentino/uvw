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

template struct UVW_EXTERN uvw::Emitter<class uvw::TCPHandle, struct uvw::ConnectEvent, struct uvw::DataEvent, struct uvw::EndEvent, struct uvw::ListenEvent, struct uvw::WriteEvent, struct uvw::ShutdownEvent, struct uvw::CloseEvent, struct uvw::ErrorEvent>::Connection<struct uvw::ConnectEvent>;
template struct UVW_EXTERN uvw::Emitter<class uvw::TCPHandle, struct uvw::ConnectEvent, struct uvw::DataEvent, struct uvw::EndEvent, struct uvw::ListenEvent, struct uvw::WriteEvent, struct uvw::ShutdownEvent, struct uvw::CloseEvent, struct uvw::ErrorEvent>::Connection<struct uvw::ShutdownEvent>;
template struct UVW_EXTERN uvw::Emitter<class uvw::TCPHandle, struct uvw::ConnectEvent, struct uvw::DataEvent, struct uvw::EndEvent, struct uvw::ListenEvent, struct uvw::WriteEvent, struct uvw::ShutdownEvent, struct uvw::CloseEvent, struct uvw::ErrorEvent>::Connection<struct uvw::WriteEvent>;
template struct UVW_EXTERN uvw::Emitter<class uvw::TCPHandle, struct uvw::ConnectEvent, struct uvw::DataEvent, struct uvw::EndEvent, struct uvw::ListenEvent, struct uvw::WriteEvent, struct uvw::ShutdownEvent, struct uvw::CloseEvent, struct uvw::ErrorEvent>::Connection<struct uvw::CloseEvent>;
template struct UVW_EXTERN uvw::Emitter<class uvw::TCPHandle, struct uvw::ConnectEvent, struct uvw::DataEvent, struct uvw::EndEvent, struct uvw::ListenEvent, struct uvw::WriteEvent, struct uvw::ShutdownEvent, struct uvw::CloseEvent, struct uvw::ErrorEvent>::Connection<struct uvw::ListenEvent>;
template struct UVW_EXTERN uvw::Emitter<class uvw::TCPHandle, struct uvw::ConnectEvent, struct uvw::DataEvent, struct uvw::EndEvent, struct uvw::ListenEvent, struct uvw::WriteEvent, struct uvw::ShutdownEvent, struct uvw::CloseEvent, struct uvw::ErrorEvent>::Connection<struct uvw::EndEvent>;
template struct UVW_EXTERN uvw::Emitter<class uvw::TCPHandle, struct uvw::ConnectEvent, struct uvw::DataEvent, struct uvw::EndEvent, struct uvw::ListenEvent, struct uvw::WriteEvent, struct uvw::ShutdownEvent, struct uvw::CloseEvent, struct uvw::ErrorEvent>::Connection<struct uvw::ErrorEvent>;

template class std::_List_iterator<class std::_List_val<struct std::_List_simple_types<struct std::pair<bool, class std::function<void(struct uvw::ErrorEvent&, class uvw::TCPHandle&)> > > > > uvw::Emitter<class uvw::TCPHandle, struct uvw::ConnectEvent, struct uvw::DataEvent, struct uvw::EndEvent, struct uvw::ListenEvent, struct uvw::WriteEvent, struct uvw::ShutdownEvent, struct uvw::CloseEvent, struct uvw::ErrorEvent>::Handler<struct uvw::ErrorEvent>::on(class std::function<void(struct uvw::ErrorEvent&, class uvw::TCPHandle&)>);
template class std::_List_iterator<class std::_List_val<struct std::_List_simple_types<struct std::pair<bool, class std::function<void(struct uvw::CloseEvent&, class uvw::TCPHandle&)> > > > > uvw::Emitter<class uvw::TCPHandle, struct uvw::ConnectEvent, struct uvw::DataEvent, struct uvw::EndEvent, struct uvw::ListenEvent, struct uvw::WriteEvent, struct uvw::ShutdownEvent, struct uvw::CloseEvent, struct uvw::ErrorEvent>::Handler<struct uvw::CloseEvent>::on(class std::function<void(struct uvw::CloseEvent&, class uvw::TCPHandle&)>);
template class std::_List_iterator<class std::_List_val<struct std::_List_simple_types<struct std::pair<bool, class std::function<void(struct uvw::ShutdownEvent&, class uvw::TCPHandle&)> > > > > uvw::Emitter<class uvw::TCPHandle, struct uvw::ConnectEvent, struct uvw::DataEvent, struct uvw::EndEvent, struct uvw::ListenEvent, struct uvw::WriteEvent, struct uvw::ShutdownEvent, struct uvw::CloseEvent, struct uvw::ErrorEvent>::Handler<struct uvw::ShutdownEvent>::once(class std::function<void(struct uvw::ShutdownEvent&, class uvw::TCPHandle&)>);
template class std::_List_iterator<class std::_List_val<struct std::_List_simple_types<struct std::pair<bool, class std::function<void(struct uvw::EndEvent&, class uvw::TCPHandle&)> > > > > uvw::Emitter<class uvw::TCPHandle, struct uvw::ConnectEvent, struct uvw::DataEvent, struct uvw::EndEvent, struct uvw::ListenEvent, struct uvw::WriteEvent, struct uvw::ShutdownEvent, struct uvw::CloseEvent, struct uvw::ErrorEvent>::Handler<struct uvw::EndEvent>::on(class std::function<void(struct uvw::EndEvent&, class uvw::TCPHandle&)>);

template void uvw::StreamHandle<class uvw::TCPHandle, struct uv_tcp_s, struct uvw::ConnectEvent, struct uvw::DataEvent, struct uvw::EndEvent, struct uvw::ListenEvent, struct uvw::WriteEvent, struct uvw::ShutdownEvent, struct uvw::CloseEvent, struct uvw::ErrorEvent>::listen(int);

template uvw::Flags<enum uvw::details::UVTCPFlags>::Flags<enum uvw::details::UVTCPFlags>(void);
template bool uvw::StreamHandle<class uvw::TCPHandle, struct uv_tcp_s, struct uvw::ConnectEvent, struct uvw::DataEvent, struct uvw::EndEvent, struct uvw::ListenEvent, struct uvw::WriteEvent, struct uvw::ShutdownEvent, struct uvw::CloseEvent, struct uvw::ErrorEvent>::writable(void) const noexcept;
template int uvw::StreamHandle<class uvw::TCPHandle, struct uv_tcp_s, struct uvw::ConnectEvent, struct uvw::DataEvent, struct uvw::EndEvent, struct uvw::ListenEvent, struct uvw::WriteEvent, struct uvw::ShutdownEvent, struct uvw::CloseEvent, struct uvw::ErrorEvent>::tryWrite(class std::unique_ptr<char[]>, unsigned int);
template class std::_List_iterator<class std::_List_val<struct std::_List_simple_types<struct std::pair<bool, class std::function<void(struct uvw::ConnectEvent&, class uvw::TCPHandle&)> > > > > uvw::Emitter<class uvw::TCPHandle, struct uvw::ConnectEvent, struct uvw::DataEvent, struct uvw::EndEvent, struct uvw::ListenEvent, struct uvw::WriteEvent, struct uvw::ShutdownEvent, struct uvw::CloseEvent, struct uvw::ErrorEvent>::Handler<struct uvw::ConnectEvent>::once(class std::function<void(struct uvw::ConnectEvent&, class uvw::TCPHandle&)>);
template void uvw::Emitter<class uvw::TCPHandle, struct uvw::ConnectEvent, struct uvw::DataEvent, struct uvw::EndEvent, struct uvw::ListenEvent, struct uvw::WriteEvent, struct uvw::ShutdownEvent, struct uvw::CloseEvent, struct uvw::ErrorEvent>::Handler<struct uvw::WriteEvent>::publish(struct uvw::WriteEvent, class uvw::TCPHandle&);
template class std::_List_iterator<class std::_List_val<struct std::_List_simple_types<struct std::pair<bool, class std::function<void(struct uvw::ListenEvent&, class uvw::TCPHandle&)> > > > > uvw::Emitter<class uvw::TCPHandle, struct uvw::ConnectEvent, struct uvw::DataEvent, struct uvw::EndEvent, struct uvw::ListenEvent, struct uvw::WriteEvent, struct uvw::ShutdownEvent, struct uvw::CloseEvent, struct uvw::ErrorEvent>::Handler<struct uvw::ListenEvent>::once(class std::function<void(struct uvw::ListenEvent&, class uvw::TCPHandle&)>);
template class std::_List_iterator<class std::_List_val<struct std::_List_simple_types<struct std::pair<bool, class std::function<void(struct uvw::WriteEvent&, class uvw::TCPHandle&)> > > > > uvw::Emitter<class uvw::TCPHandle, struct uvw::ConnectEvent, struct uvw::DataEvent, struct uvw::EndEvent, struct uvw::ListenEvent, struct uvw::WriteEvent, struct uvw::ShutdownEvent, struct uvw::CloseEvent, struct uvw::ErrorEvent>::Handler<struct uvw::WriteEvent>::once(class std::function<void(struct uvw::WriteEvent&, class uvw::TCPHandle&)>);
template class std::_List_iterator<class std::_List_val<struct std::_List_simple_types<struct std::pair<bool, class std::function<void(struct uvw::CloseEvent&, class uvw::TCPHandle&)> > > > > uvw::Emitter<class uvw::TCPHandle, struct uvw::ConnectEvent, struct uvw::DataEvent, struct uvw::EndEvent, struct uvw::ListenEvent, struct uvw::WriteEvent, struct uvw::ShutdownEvent, struct uvw::CloseEvent, struct uvw::ErrorEvent>::Handler<struct uvw::CloseEvent>::once(class std::function<void(struct uvw::CloseEvent&, class uvw::TCPHandle&)>);
template uvw::Emitter<class uvw::TCPHandle, struct uvw::ConnectEvent, struct uvw::DataEvent, struct uvw::EndEvent, struct uvw::ListenEvent, struct uvw::WriteEvent, struct uvw::ShutdownEvent, struct uvw::CloseEvent, struct uvw::ErrorEvent>::Connection<struct uvw::DataEvent>;
template class std::_List_iterator<class std::_List_val<struct std::_List_simple_types<struct std::pair<bool, class std::function<void(struct uvw::DataEvent&, class uvw::TCPHandle&)> > > > > uvw::Emitter<class uvw::TCPHandle, struct uvw::ConnectEvent, struct uvw::DataEvent, struct uvw::EndEvent, struct uvw::ListenEvent, struct uvw::WriteEvent, struct uvw::ShutdownEvent, struct uvw::CloseEvent, struct uvw::ErrorEvent>::Handler<struct uvw::DataEvent>::on(class std::function<void(struct uvw::DataEvent&, class uvw::TCPHandle&)>);
template void __cdecl uvw::StreamHandle<class uvw::TCPHandle, struct uv_tcp_s, struct uvw::ConnectEvent, struct uvw::DataEvent, struct uvw::EndEvent, struct uvw::ListenEvent, struct uvw::WriteEvent, struct uvw::ShutdownEvent, struct uvw::CloseEvent, struct uvw::ErrorEvent>::read(void);
template class std::_List_iterator<class std::_List_val<struct std::_List_simple_types<struct std::pair<bool, class std::function<void __cdecl(struct uvw::ErrorEvent&, class uvw::TCPHandle&)> > > > > __cdecl uvw::Emitter<class uvw::TCPHandle, struct uvw::ConnectEvent, struct uvw::DataEvent, struct uvw::EndEvent, struct uvw::ListenEvent, struct uvw::WriteEvent, struct uvw::ShutdownEvent, struct uvw::CloseEvent, struct uvw::ErrorEvent>::Handler<struct uvw::ErrorEvent>::once(class std::function<void __cdecl(struct uvw::ErrorEvent&, class uvw::TCPHandle&)>);
template void __cdecl uvw::StreamHandle<class uvw::TCPHandle, struct uv_tcp_s, struct uvw::ConnectEvent, struct uvw::DataEvent, struct uvw::EndEvent, struct uvw::ListenEvent, struct uvw::WriteEvent, struct uvw::ShutdownEvent, struct uvw::CloseEvent, struct uvw::ErrorEvent>::shutdown(void);
template int __cdecl uvw::StreamHandle<class uvw::TCPHandle, struct uv_tcp_s, struct uvw::ConnectEvent, struct uvw::DataEvent, struct uvw::EndEvent, struct uvw::ListenEvent, struct uvw::WriteEvent, struct uvw::ShutdownEvent, struct uvw::CloseEvent, struct uvw::ErrorEvent>::tryWrite(char*, unsigned int);
template bool __cdecl uvw::StreamHandle<class uvw::TCPHandle, struct uv_tcp_s, struct uvw::ConnectEvent, struct uvw::DataEvent, struct uvw::EndEvent, struct uvw::ListenEvent, struct uvw::WriteEvent, struct uvw::ShutdownEvent, struct uvw::CloseEvent, struct uvw::ErrorEvent>::readable(void)const noexcept;

template void uvw::StreamHandle<class uvw::TCPHandle, struct uv_tcp_s, struct uvw::ConnectEvent, struct uvw::DataEvent, struct uvw::EndEvent, struct uvw::ListenEvent, struct uvw::WriteEvent, struct uvw::ShutdownEvent, struct uvw::CloseEvent, struct uvw::ErrorEvent>::write(char*, unsigned int);
}
