#ifdef UVW_AS_LIB
#include "pipe.h"
#endif

#include <utility>

#include "config.h"


namespace uvw {


UVW_INLINE PipeHandle::PipeHandle(ConstructorAccess ca, std::shared_ptr<Loop> ref, bool pass)
    : StreamHandle{ca, std::move(ref)}, ipc{pass}
{}


UVW_INLINE bool PipeHandle::init() {
    return initialize(&uv_pipe_init, ipc);
}


UVW_INLINE void PipeHandle::open(FileHandle file) {
    invoke(&uv_pipe_open, get(), file);
}


UVW_INLINE void PipeHandle::bind(std::string name) {
    invoke(&uv_pipe_bind, get(), name.data());
}


UVW_INLINE void PipeHandle::connect(std::string name) {
    auto listener = [ptr = shared_from_this()](const auto &event, const auto &) {
        ptr->publish(event);
    };

    auto connect = loop().resource<details::ConnectReq>();
    connect->once<ErrorEvent>(listener);
    connect->once<ConnectEvent>(listener);
    connect->connect(&uv_pipe_connect, get(), name.data());
}


UVW_INLINE std::string PipeHandle::sock() const noexcept {
    return details::tryRead(&uv_pipe_getsockname, get());
}


UVW_INLINE std::string PipeHandle::peer() const noexcept {
    return details::tryRead(&uv_pipe_getpeername, get());
}


UVW_INLINE void PipeHandle::pending(int count) noexcept {
    uv_pipe_pending_instances(get(), count);
}


UVW_INLINE int PipeHandle::pending() noexcept {
    return uv_pipe_pending_count(get());
}


UVW_INLINE HandleType PipeHandle::receive() noexcept {
    HandleCategory category = uv_pipe_pending_type(get());
    return Utilities::guessHandle(category);
}


UVW_INLINE bool PipeHandle::chmod(Flags<Chmod> flags) noexcept {
    return (0 == uv_pipe_chmod(get(), flags));
}

    // template<> uvw::Emitter<uvw::PipeHandle, uvw::ConnectEvent, uvw::DataEvent, uvw::EndEvent, uvw::ListenEvent, uvw::WriteEvent, uvw::ShutdownEvent, uvw::CloseEvent, uvw::ErrorEvent>::Emitter() = default;
    template class UVW_EXTERN Resource<PipeHandle, uv_pipe_t, ConnectEvent, DataEvent, EndEvent, ListenEvent, WriteEvent, ShutdownEvent, CloseEvent, ErrorEvent>;

    template struct UVW_EXTERN Emitter<PipeHandle, ConnectEvent, DataEvent, EndEvent, ListenEvent, WriteEvent, ShutdownEvent, CloseEvent, ErrorEvent>::Connection<ConnectEvent>;
    template struct UVW_EXTERN Emitter<PipeHandle, ConnectEvent, DataEvent, EndEvent, ListenEvent, WriteEvent, ShutdownEvent, CloseEvent, ErrorEvent>::Connection<DataEvent>;
    template struct UVW_EXTERN Emitter<PipeHandle, ConnectEvent, DataEvent, EndEvent, ListenEvent, WriteEvent, ShutdownEvent, CloseEvent, ErrorEvent>::Connection<EndEvent>;
    template struct UVW_EXTERN Emitter<PipeHandle, ConnectEvent, DataEvent, EndEvent, ListenEvent, WriteEvent, ShutdownEvent, CloseEvent, ErrorEvent>::Connection<ListenEvent>;
    template struct UVW_EXTERN Emitter<PipeHandle, ConnectEvent, DataEvent, EndEvent, ListenEvent, WriteEvent, ShutdownEvent, CloseEvent, ErrorEvent>::Connection<WriteEvent>;
    template struct UVW_EXTERN Emitter<PipeHandle, ConnectEvent, DataEvent, EndEvent, ListenEvent, WriteEvent, ShutdownEvent, CloseEvent, ErrorEvent>::Connection<ShutdownEvent>;
    template struct UVW_EXTERN Emitter<PipeHandle, ConnectEvent, DataEvent, EndEvent, ListenEvent, WriteEvent, ShutdownEvent, CloseEvent, ErrorEvent>::Connection<CloseEvent>;
    template struct UVW_EXTERN Emitter<PipeHandle, ConnectEvent, DataEvent, EndEvent, ListenEvent, WriteEvent, ShutdownEvent, CloseEvent, ErrorEvent>::Connection<ErrorEvent>;

    template struct UVW_EXTERN Emitter<PipeHandle, ConnectEvent, DataEvent, EndEvent, ListenEvent, WriteEvent, ShutdownEvent, CloseEvent, ErrorEvent>::Handler<ConnectEvent>;
    template struct UVW_EXTERN Emitter<PipeHandle, ConnectEvent, DataEvent, EndEvent, ListenEvent, WriteEvent, ShutdownEvent, CloseEvent, ErrorEvent>::Handler<DataEvent>;
    template struct UVW_EXTERN Emitter<PipeHandle, ConnectEvent, DataEvent, EndEvent, ListenEvent, WriteEvent, ShutdownEvent, CloseEvent, ErrorEvent>::Handler<EndEvent>;
    template struct UVW_EXTERN Emitter<PipeHandle, ConnectEvent, DataEvent, EndEvent, ListenEvent, WriteEvent, ShutdownEvent, CloseEvent, ErrorEvent>::Handler<ListenEvent>;
    template struct UVW_EXTERN Emitter<PipeHandle, ConnectEvent, DataEvent, EndEvent, ListenEvent, WriteEvent, ShutdownEvent, CloseEvent, ErrorEvent>::Handler<WriteEvent>;
    template struct UVW_EXTERN Emitter<PipeHandle, ConnectEvent, DataEvent, EndEvent, ListenEvent, WriteEvent, ShutdownEvent, CloseEvent, ErrorEvent>::Handler<ShutdownEvent>;
    template struct UVW_EXTERN Emitter<PipeHandle, ConnectEvent, DataEvent, EndEvent, ListenEvent, WriteEvent, ShutdownEvent, CloseEvent, ErrorEvent>::Handler<CloseEvent>;
    template struct UVW_EXTERN Emitter<PipeHandle, ConnectEvent, DataEvent, EndEvent, ListenEvent, WriteEvent, ShutdownEvent, CloseEvent, ErrorEvent>::Handler<ErrorEvent>;

    template void StreamHandle<PipeHandle, uv_pipe_s, ConnectEvent, DataEvent, EndEvent, ListenEvent, WriteEvent, ShutdownEvent, CloseEvent, ErrorEvent>::shutdown();
    template bool StreamHandle<PipeHandle, uv_pipe_s, ConnectEvent, DataEvent, EndEvent, ListenEvent, WriteEvent, ShutdownEvent, CloseEvent, ErrorEvent>::writable() const noexcept;
    template bool StreamHandle<PipeHandle, uv_pipe_s, ConnectEvent, DataEvent, EndEvent, ListenEvent, WriteEvent, ShutdownEvent, CloseEvent, ErrorEvent>::readable() const noexcept;
    template void StreamHandle<PipeHandle, uv_pipe_s, ConnectEvent, DataEvent, EndEvent, ListenEvent, WriteEvent, ShutdownEvent, CloseEvent, ErrorEvent>::listen(int);
    template void StreamHandle<PipeHandle, uv_pipe_s, ConnectEvent, DataEvent, EndEvent, ListenEvent, WriteEvent, ShutdownEvent, CloseEvent, ErrorEvent>::read();
    template int uvw::StreamHandle<uvw::PipeHandle, uv_pipe_s, uvw::ConnectEvent, uvw::DataEvent, uvw::EndEvent, uvw::ListenEvent, uvw::WriteEvent, uvw::ShutdownEvent, uvw::CloseEvent, uvw::ErrorEvent>::tryWrite(std::unique_ptr<char[]>, unsigned int);
    
    template void 
    uvw::StreamHandle<uvw::PipeHandle, uv_pipe_s, uvw::ConnectEvent, uvw::DataEvent, uvw::EndEvent, uvw::ListenEvent, uvw::WriteEvent, uvw::ShutdownEvent, uvw::CloseEvent, uvw::ErrorEvent>
    ::write(char*, unsigned int);
}
