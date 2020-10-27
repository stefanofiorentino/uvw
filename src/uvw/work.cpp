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
    auto &resource = *(static_cast<Resource<WorkReq, uv_work_t, WorkEvent, ErrorEvent>*>(req->data));
    WorkReq &work = static_cast<WorkReq &>(resource);
    work.task();
}


UVW_INLINE void WorkReq::queue() {
    invoke(&uv_queue_work, parent(), get(), &workCallback, &defaultCallback<WorkEvent>);
}

template uvw::Emitter<uvw::WorkReq, uvw::WorkEvent, uvw::ErrorEvent>::Handler<uvw::WorkEvent>::Connection
    uvw::Emitter<uvw::WorkReq, uvw::WorkEvent, uvw::ErrorEvent>::Handler<uvw::WorkEvent>
::on(std::function<void(struct uvw::WorkEvent&, uvw::WorkReq&)>);
template uvw::Emitter<uvw::WorkReq, uvw::WorkEvent, uvw::ErrorEvent>::Handler<uvw::ErrorEvent>::Connection
    uvw::Emitter<uvw::WorkReq, uvw::WorkEvent, uvw::ErrorEvent>::Handler<uvw::ErrorEvent>
    ::on(std::function<void(struct uvw::ErrorEvent&, uvw::WorkReq&)>);

template struct UVW_EXTERN uvw::Emitter<class uvw::WorkReq, struct uvw::WorkEvent, struct uvw::ErrorEvent>::Connection<struct uvw::ErrorEvent>;
template struct UVW_EXTERN uvw::Emitter<class uvw::WorkReq, struct uvw::WorkEvent, struct uvw::ErrorEvent>::Connection<struct uvw::WorkEvent>;
template bool uvw::Request<class uvw::WorkReq, struct uv_work_s, struct uvw::WorkEvent, struct uvw::ErrorEvent>::cancel();
template uint64_t uvw::Request<class uvw::WorkReq, struct uv_work_s, struct uvw::WorkEvent, struct uvw::ErrorEvent>::size(void)const noexcept;
// template<> uvw::Emitter<uvw::WorkReq, uvw::WorkEvent, uvw::ErrorEvent>::Emitter() = default;
}

