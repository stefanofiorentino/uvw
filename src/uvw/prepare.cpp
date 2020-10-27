#ifdef UVW_AS_LIB
#include "prepare.h"
#endif

#include "config.h"


namespace uvw {


UVW_INLINE void PrepareHandle::startCallback(uv_prepare_t *handle) {
    // PrepareHandle &prepare = *(static_cast<PrepareHandle *>(handle->data));
    auto &resource = *(static_cast<Resource<PrepareHandle, uv_prepare_t, PrepareEvent, CloseEvent, ErrorEvent>*>(handle->data));
    PrepareHandle &prepare = static_cast<PrepareHandle &>(resource);
    prepare.publish(PrepareEvent{});
}


UVW_INLINE bool PrepareHandle::init() {
    return initialize(&uv_prepare_init);
}


UVW_INLINE void PrepareHandle::start() {
    invoke(&uv_prepare_start, get(), &startCallback);
}


UVW_INLINE void PrepareHandle::stop() {
    invoke(&uv_prepare_stop, get());
}

template struct UVW_EXTERN uvw::Emitter<uvw::PrepareHandle, uvw::PrepareEvent, uvw::CloseEvent, uvw::ErrorEvent>::Connection<uvw::PrepareEvent>::Connection<uvw::PrepareEvent>;
template uvw::Emitter<uvw::PrepareHandle, uvw::PrepareEvent, uvw::CloseEvent, uvw::ErrorEvent>::Handler<uvw::PrepareEvent>::Connection
uvw::Emitter<uvw::PrepareHandle, uvw::PrepareEvent, uvw::CloseEvent, uvw::ErrorEvent>::Handler<uvw::PrepareEvent>
::on(std::function<void(struct uvw::PrepareEvent&, uvw::PrepareHandle&)>);
template struct UVW_EXTERN uvw::Emitter<uvw::PrepareHandle, uvw::PrepareEvent, uvw::CloseEvent, uvw::ErrorEvent>::Connection<uvw::ErrorEvent>;
template uvw::Emitter<uvw::PrepareHandle, uvw::PrepareEvent, uvw::CloseEvent, uvw::ErrorEvent>::Handler<uvw::ErrorEvent>::Connection
uvw::Emitter<uvw::PrepareHandle, uvw::PrepareEvent, uvw::CloseEvent, uvw::ErrorEvent>::Handler<uvw::ErrorEvent>
::on(std::function<void(struct uvw::ErrorEvent&, uvw::PrepareHandle&)>);
// template<> uvw::Emitter<uvw::PrepareHandle, uvw::PrepareEvent, uvw::CloseEvent, uvw::ErrorEvent>::Emitter() = default;
}
