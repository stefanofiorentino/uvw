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
    void uv_rd_cb(evt_tls_t *tls_client, ssize_t nrd, uv_buf_t *bfr) {
        if ( nrd <= 0 ) return;
        std::cout.write(bfr->base, bfr->len) << std::endl;
        uv_tls_close(reinterpret_cast<uv_tls_t *>(tls_client), reinterpret_cast<uv_tls_close_cb>(free));
    }

    void on_uv_handshake(uv_tls_t *tls_client, int status) {
        if ( 0 == status )
            uv_tls_read(tls_client, reinterpret_cast<uv_tls_read_cb>(uv_rd_cb));
        else
            uv_tls_close(tls_client, reinterpret_cast<uv_tls_close_cb>(free));
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
    evt_ctx_set_nio(&ctx, uv_tls_reader, uv_tls_writer);

    server->data(static_cast<std::shared_ptr<void>>(&ctx));

    server->on<uvw::ErrorEvent>([](const auto &, auto &) {
        FAIL();
    });

    server->on<uvw::ListenEvent>([](const uvw::ListenEvent &, uvw::TCPHandle &server_hdl) {
        uv_tcp_t *tcp_client = static_cast<uv_tcp_t *>(malloc(sizeof(*tcp_client))); //freed on uv_close callback
        uv_tcp_init(uv_default_loop(), tcp_client);
        if (uv_accept(reinterpret_cast<uv_stream_t *>(server_hdl.raw()), reinterpret_cast<uv_stream_t *>(tcp_client))) {
            return;
        }

        uv_tls_t *tls_client = static_cast<uv_tls_t *>(malloc(sizeof(*tls_client))); //freed on uv_close callback
        if(0 > uv_tls_init(reinterpret_cast<evt_ctx_t *>(server_hdl.data().get()), tcp_client, tls_client)) {
            free(tls_client);
            return;
        }
        uv_tls_accept(tls_client, on_uv_handshake);
    });

    server->bind(address, port);
    server->listen();

    loop->run();
}
