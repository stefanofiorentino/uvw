#ifndef _EVENT_TYPE_H_
#define _EVENT_TYPE_H_ 1

namespace uvw
{

    enum event_type
    {
        error = 0,
        close,
        open,
        async
    };
}

#endif