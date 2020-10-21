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
    
    template void details::WriteReq<struct std::default_delete<char[]> >::write(struct uv_stream_s*);
    template details::WriteReq<struct std::default_delete<char[]> >::WriteReq(struct UnderlyingType<class details::WriteReq<struct std::default_delete<char[]> >, struct uv_write_s>::ConstructorAccess, class std::shared_ptr<class Loop>, class std::unique_ptr<char[]>&&, unsigned int);

    template Emitter<class details::WriteReq<struct std::default_delete<char[]> >, struct WriteEvent, struct ErrorEvent>
        ::Handler<struct WriteEvent>::Connection
        Emitter<class details::WriteReq<struct std::default_delete<char[]> >, struct WriteEvent, struct ErrorEvent>
        ::Handler<struct WriteEvent>
        ::once(class std::function<void(struct WriteEvent&, class details::WriteReq<struct std::default_delete<char[]> >&)>);

    template          Emitter<class details::WriteReq<struct std::default_delete<char[]> >, struct WriteEvent, struct ErrorEvent>
        ::Handler<struct WriteEvent>::Connection
        Emitter<class details::WriteReq<struct std::default_delete<char[]> >, struct WriteEvent, struct ErrorEvent>
        ::Handler<struct WriteEvent>
        ::once(class std::function<void(struct WriteEvent&, class details::WriteReq<struct std::default_delete<char[]> >&)>);

    template  class std::_List_iterator<class std::_List_val<struct std::_List_simple_types<struct std::pair<bool, class std::function<void(struct WriteEvent&, class details::WriteReq<struct std::default_delete<char[]> >&)> > > > >
        Emitter<class details::WriteReq<struct std::default_delete<char[]> >, struct WriteEvent, struct ErrorEvent>::Handler<struct WriteEvent>
        ::once(class std::function<void(struct WriteEvent&, class details::WriteReq<struct std::default_delete<char[]> >&)>);

    template    class std::_List_iterator<class std::_List_val<struct std::_List_simple_types<struct std::pair<bool, class std::function<void(struct ErrorEvent&, class details::WriteReq<struct std::default_delete<char[]> >&)> > > > >
        Emitter<class details::WriteReq<struct std::default_delete<char[]> >, struct WriteEvent, struct ErrorEvent>
        ::Handler<struct ErrorEvent>
        ::once(class std::function<void(struct ErrorEvent&, class details::WriteReq<struct std::default_delete<char[]> >&)>);    

}
