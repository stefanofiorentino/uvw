#ifdef UVW_AS_LIB
#include "check.h"
#endif

#include "config.h"


namespace uvw {


UVW_INLINE void CheckHandle::startCallback(uv_check_t *handle) {
    // CheckHandle &check = *(static_cast<CheckHandle *>(handle->data));
    auto &resource = *(static_cast<Resource<CheckHandle, uv_check_t, CheckEvent, CloseEvent, ErrorEvent>*>(handle->data));
    CheckHandle &check = static_cast<CheckHandle &>(resource);
    check.publish(CheckEvent{});
}


UVW_INLINE bool CheckHandle::init() {
    return initialize(&uv_check_init);
}


UVW_INLINE void CheckHandle::start() {
    invoke(&uv_check_start, get(), &startCallback);
}


UVW_INLINE void CheckHandle::stop() {
    invoke(&uv_check_stop, get());
}

// template<> uvw::Emitter<uvw::CheckHandle, uvw::CheckEvent, uvw::CloseEvent, uvw::ErrorEvent>::Emitter() = default;
template struct UVW_EXTERN Emitter<CheckHandle, CheckEvent, CloseEvent, ErrorEvent>::Connection<CheckEvent>;
template struct UVW_EXTERN Emitter<CheckHandle, CheckEvent, CloseEvent, ErrorEvent>::Connection<ErrorEvent>;
template struct UVW_EXTERN Emitter<CheckHandle, CheckEvent, CloseEvent, ErrorEvent>::Handler<CheckEvent>;
template struct UVW_EXTERN Emitter<CheckHandle, CheckEvent, CloseEvent, ErrorEvent>::Handler<ErrorEvent>;


}
