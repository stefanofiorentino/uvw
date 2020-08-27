#ifdef UVW_AS_LIB
#include "work.h"
#endif

#include <utility>

#include "config.h"


namespace uvw {


UVW_INLINE WorkReq::WorkReq(ConstructorAccess ca, std::shared_ptr<Loop> ref, InternalTask t)
    : Request{ca, std::move(ref)}, task{t}
{}


UVW_INLINE void WorkReq::workCallback(uv_work_t *req) {
    static_cast<WorkReq*>(req->data)->task();
}


UVW_INLINE void WorkReq::queue() {
    invoke(&uv_queue_work, parent(), get(), &workCallback, &defaultCallback<WorkEvent>);
}

template struct UVW_EXTERN uvw::Emitter<class uvw::WorkReq, struct uvw::WorkEvent, struct uvw::ErrorEvent>::Connection<struct uvw::WorkEvent>;
template struct UVW_EXTERN uvw::Emitter<class uvw::WorkReq, struct uvw::WorkEvent, struct uvw::ErrorEvent>::Connection<struct uvw::ErrorEvent>;
template bool uvw::Request<class uvw::WorkReq, struct uv_work_s, struct uvw::WorkEvent, struct uvw::ErrorEvent>::cancel(void);
template class UVW_EXTERN std::_List_iterator<class std::_List_val<struct std::_List_simple_types<struct std::pair<bool, class std::function<void(struct uvw::WorkEvent&, class uvw::WorkReq&)> > > > >
    uvw::Emitter<class uvw::WorkReq, struct uvw::WorkEvent, struct uvw::ErrorEvent>
::Handler<struct uvw::WorkEvent>::on(class std::function<void(struct uvw::WorkEvent&, class uvw::WorkReq&)>);
template class UVW_EXTERN std::_List_iterator<class std::_List_val<struct std::_List_simple_types<struct std::pair<bool, class std::function<void(struct uvw::ErrorEvent&, class uvw::WorkReq&)> > > > >
    uvw::Emitter<class uvw::WorkReq, struct uvw::WorkEvent, struct uvw::ErrorEvent>
    ::Handler<struct uvw::ErrorEvent>::on(class std::function<void(struct uvw::ErrorEvent&, class uvw::WorkReq&)>);
template unsigned __int64 uvw::Request<class uvw::WorkReq, struct uv_work_s, struct uvw::WorkEvent, struct uvw::ErrorEvent>::size(void)const noexcept;
}

