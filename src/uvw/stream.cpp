#ifdef UVW_AS_LIB
#include "stream.h"
#endif

#include "config.h"


namespace uvw {


    UVW_INLINE DataEvent::DataEvent(std::unique_ptr<char[]> buf, std::size_t len) noexcept
        : data{ std::move(buf) }, length{ len }
    {}


    UVW_INLINE void details::ShutdownReq::shutdown(uv_stream_t* handle) {
        invoke(&uv_shutdown, get(), handle, &defaultCallback<ShutdownEvent>);
    }
    
    template void details::WriteReq<std::default_delete<char[]> >::write(struct uv_stream_s*);
    template details::WriteReq<std::default_delete<char[]> >::WriteReq(struct UnderlyingType<details::WriteReq<std::default_delete<char[]> >, uv_write_s>::ConstructorAccess, std::shared_ptr<Loop>, std::unique_ptr<char[]>&&, unsigned int);

    template Emitter<details::WriteReq<std::default_delete<char[]> >, WriteEvent, ErrorEvent>::Handler<WriteEvent>::Connection
        Emitter<details::WriteReq<std::default_delete<char[]> >, WriteEvent, ErrorEvent>::Handler<WriteEvent>
        ::once(std::function<void(struct WriteEvent&, details::WriteReq<std::default_delete<char[]> >&)>);

    template  Emitter<details::WriteReq<std::default_delete<char[]> >, WriteEvent, ErrorEvent>::Handler<ErrorEvent>::Connection
        Emitter<details::WriteReq<std::default_delete<char[]> >, WriteEvent, ErrorEvent>::Handler<ErrorEvent>
        ::once(std::function<void(struct ErrorEvent&, details::WriteReq<std::default_delete<char[]> >&)>);

}
