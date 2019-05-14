#include <gtest/gtest.h>
#include <uvw.hpp>

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <sample/libuv-tls/uv_tls.h>
#include "evt_tls.h"

namespace{
    typedef void (*uv_tls_close_cb)(uv_tls_t*);
    void on_write(evt_tls_t *tls, int status) {
        evt_tls_close(tls, reinterpret_cast<evt_close_cb>(free));
    }

    void uv_rd_cb(evt_tls_t *strm, ssize_t nrd, uv_buf_t *bfr) {
        if ( nrd <= 0 ) return;
        std::cout.write(bfr->base, bfr->len);
//        evt_tls_write(strm, bfr->base, bfr->len, on_write);
    }

    void on_uv_handshake(uv_tls_t *ut, int status) {
        if ( 0 == status )
            uv_tls_read(ut, reinterpret_cast<uv_tls_read_cb>(uv_rd_cb));
        else
            uv_tls_close(ut, reinterpret_cast<uv_tls_close_cb>(free));
    }
}

TEST(TCP, ReadWrite) {

    const std::string address = std::string{"127.0.0.1"};
    const unsigned int port = 4242;

    auto loop = uvw::Loop::getDefault();
    auto server = loop->resource<uvw::TCPHandle>();

    evt_ctx_t ctx;
    ctx.ctx = nullptr;

    auto rc = evt_ctx_init_ex(&ctx, "agent.pem", "key.pem");
    if (rc != 1)
    {
        FAIL();
    }
    evt_ctx_set_nio(&ctx, nullptr, uv_tls_writer);

    server->data(static_cast<std::shared_ptr<void>>(&ctx));

    server->on<uvw::ErrorEvent>([](const auto &, auto &) {
        FAIL();
    });

    server->on<uvw::ListenEvent>([](const uvw::ListenEvent &, uvw::TCPHandle &server_hdl) {
        uv_tcp_t *tcp = static_cast<uv_tcp_t *>(malloc(sizeof(*tcp))); //freed on uv_close callback
        uv_tcp_init(uv_default_loop(), tcp);
        if (uv_accept(reinterpret_cast<uv_stream_t *>(server_hdl.raw()), reinterpret_cast<uv_stream_t *>(tcp))) {
            return;
        }

        uv_tls_t *sclient = static_cast<uv_tls_t *>(malloc(sizeof(*sclient))); //freed on uv_close callback
        if(0 > uv_tls_init(reinterpret_cast<evt_ctx_t *>(server_hdl.data().get()), tcp, sclient)) {
            free(sclient);
            return;
        }
        uv_tls_accept(sclient, on_uv_handshake);
    });

    server->bind(address, port);
    server->listen();

    loop->run();
}
