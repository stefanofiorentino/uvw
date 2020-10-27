#ifdef UVW_AS_LIB
#include "signal.h"
#endif

#include "config.h"


namespace uvw {


UVW_INLINE SignalEvent::SignalEvent(int sig) noexcept
    : signum{sig}
{}


UVW_INLINE void SignalHandle::startCallback(uv_signal_t *handle, int signum) {
    SignalHandle &signal = *(static_cast<SignalHandle *>(handle->data));
    signal.publish(SignalEvent{signum});
}


UVW_INLINE bool SignalHandle::init() {
    return initialize(&uv_signal_init);
}


UVW_INLINE void SignalHandle::start(int signum) {
    invoke(&uv_signal_start, get(), &startCallback, signum);
}


UVW_INLINE void SignalHandle::oneShot(int signum) {
    invoke(&uv_signal_start_oneshot, get(), &startCallback, signum);
}


UVW_INLINE void SignalHandle::stop() {
    invoke(&uv_signal_stop, get());
}


UVW_INLINE int SignalHandle::signal() const noexcept {
    return get()->signum;
}

template struct UVW_EXTERN uvw::Emitter<uvw::SignalHandle, uvw::SignalEvent, uvw::CloseEvent, uvw::ErrorEvent>::Connection<uvw::ErrorEvent>;
template 
uvw::Emitter<uvw::SignalHandle, uvw::SignalEvent, uvw::CloseEvent, uvw::ErrorEvent>::Handler<uvw::ErrorEvent>::Connection
uvw::Emitter<uvw::SignalHandle, uvw::SignalEvent, uvw::CloseEvent, uvw::ErrorEvent>::Handler<uvw::ErrorEvent>
::on(std::function<void(struct uvw::ErrorEvent&, uvw::SignalHandle&)>);
// template<> uvw::Emitter<uvw::SignalHandle, uvw::SignalEvent, uvw::CloseEvent, uvw::ErrorEvent>::Emitter() = default;
}
