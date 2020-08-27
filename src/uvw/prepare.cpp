#ifdef UVW_AS_LIB
#include "prepare.h"
#endif

#include "config.h"


namespace uvw {


UVW_INLINE void PrepareHandle::startCallback(uv_prepare_t *handle) {
    PrepareHandle &prepare = *(static_cast<PrepareHandle *>(handle->data));
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

template struct UVW_EXTERN uvw::Emitter<class uvw::PrepareHandle, struct uvw::PrepareEvent, struct uvw::CloseEvent, struct uvw::ErrorEvent>::Connection<struct uvw::PrepareEvent>::Connection<struct uvw::PrepareEvent>;
template class std::_List_iterator<class std::_List_val<struct std::_List_simple_types<struct std::pair<bool, class std::function<void(struct uvw::PrepareEvent&, class uvw::PrepareHandle&)> > > > >
uvw::Emitter<class uvw::PrepareHandle, struct uvw::PrepareEvent, struct uvw::CloseEvent, struct uvw::ErrorEvent>::Handler<struct uvw::PrepareEvent>::on(class std::function<void(struct uvw::PrepareEvent&, class uvw::PrepareHandle&)>);
template struct UVW_EXTERN uvw::Emitter<class uvw::PrepareHandle, struct uvw::PrepareEvent, struct uvw::CloseEvent, struct uvw::ErrorEvent>::Connection<struct uvw::ErrorEvent>;
template class std::_List_iterator<class std::_List_val<struct std::_List_simple_types<struct std::pair<bool, class std::function<void(struct uvw::ErrorEvent&, class uvw::PrepareHandle&)> > > > >
uvw::Emitter<class uvw::PrepareHandle, struct uvw::PrepareEvent, struct uvw::CloseEvent, struct uvw::ErrorEvent>::Handler<struct uvw::ErrorEvent>::on(class std::function<void(struct uvw::ErrorEvent&, class uvw::PrepareHandle&)>);
}
