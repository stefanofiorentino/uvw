#ifdef UVW_AS_LIB
#include "async.h"
#endif

#include "config.h"


namespace uvw {


UVW_INLINE void AsyncHandle::sendCallback(uv_async_t *handle) {
    // AsyncHandle &async = *(static_cast<AsyncHandle*>(handle->data));
    auto &resource = *(static_cast<Resource<AsyncHandle, uv_async_t, AsyncEvent, CloseEvent, ErrorEvent> *>(handle->data));
    AsyncHandle &async = static_cast<AsyncHandle&>(resource);
    async.publish(AsyncEvent{});
}


UVW_INLINE bool AsyncHandle::init() {
    return initialize(&uv_async_init, &sendCallback);
}


UVW_INLINE void AsyncHandle::send() {
    invoke(&uv_async_send, get());
}

    // template<> uvw::Emitter<uvw::AsyncHandle, uvw::AsyncEvent, uvw::CloseEvent, uvw::ErrorEvent>::Emitter() = default;
    template class UVW_EXTERN Resource<AsyncHandle, uv_async_t, AsyncEvent, CloseEvent, ErrorEvent>;
    template struct UVW_EXTERN Emitter<AsyncHandle, AsyncEvent, CloseEvent, ErrorEvent>::Connection<AsyncEvent>;
    template struct UVW_EXTERN Emitter<AsyncHandle, AsyncEvent, CloseEvent, ErrorEvent>::Connection<CloseEvent>;
    template struct UVW_EXTERN Emitter<AsyncHandle, AsyncEvent, CloseEvent, ErrorEvent>::Connection<ErrorEvent>;
    template struct UVW_EXTERN Emitter<AsyncHandle, AsyncEvent, CloseEvent, ErrorEvent>::Handler<AsyncEvent>;
    template struct UVW_EXTERN Emitter<AsyncHandle, AsyncEvent, CloseEvent, ErrorEvent>::Handler<CloseEvent>;
    template struct UVW_EXTERN Emitter<AsyncHandle, AsyncEvent, CloseEvent, ErrorEvent>::Handler<ErrorEvent>;
    template uvw::OSFileDescriptor uvw::Handle<uvw::AsyncHandle, uv_async_s, uvw::AsyncEvent, uvw::CloseEvent, uvw::ErrorEvent>::fd(void)const;
    template bool uvw::Handle<uvw::AsyncHandle, uv_async_s, uvw::AsyncEvent, uvw::CloseEvent, uvw::ErrorEvent>::recvBufferSize(int);
    template uint64_t uvw::Handle<uvw::AsyncHandle, uv_async_s, uvw::AsyncEvent, uvw::CloseEvent, uvw::ErrorEvent>::size(void)const noexcept;
    template int uvw::Handle<uvw::AsyncHandle,struct uv_async_s,struct uvw::AsyncEvent,struct uvw::CloseEvent,struct uvw::ErrorEvent>::sendBufferSize(void);
    template bool uvw::Handle<uvw::AsyncHandle,struct uv_async_s,struct uvw::AsyncEvent,struct uvw::CloseEvent,struct uvw::ErrorEvent>::sendBufferSize(int);
    template int uvw::Handle<uvw::AsyncHandle,struct uv_async_s,struct uvw::AsyncEvent,struct uvw::CloseEvent,struct uvw::ErrorEvent>::recvBufferSize(void);
    template uv_async_t* uvw::UnderlyingType<uvw::AsyncHandle, uv_async_s>::raw(void) noexcept;
}
