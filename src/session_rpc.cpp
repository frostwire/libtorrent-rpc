/*
 * Created by Angel Leon (@gubatron), Alden Torres (aldenml)
 * Copyright (c) 2018, FrostWire(R). All rights reserved.
 */

#include "ltrpc/session_rpc.hpp"

#include <thread>
#include <vector>
#include <iostream>

#include <libtorrent/aux_/disable_warnings_push.hpp>

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/bind_executor.hpp>

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>

#include <libtorrent/session.hpp>

#include <libtorrent/aux_/disable_warnings_pop.hpp>

using namespace std::placeholders;
using tcp = boost::asio::ip::tcp;
namespace http = boost::beast::http;

namespace ltrpc
{

namespace
{

void fail(boost::system::error_code ec, char const* what)
{
    std::cerr << what << ": " << ec.message() << "\n";
}

template<class Body, class Allocator, class Send>
void handle_request(
    http::request<Body, http::basic_fields<Allocator>>&& req,
    Send&& send)
{
    auto const ok_message = [&req](boost::beast::string_view msg)
    {
        http::response<http::string_body> res{http::status::ok, req.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "text/html");
        res.keep_alive(req.keep_alive());
        res.body() = msg.to_string();
        res.prepare_payload();
        return res;
    };

    return send(ok_message("Hello from server"));
}

class http_session : public std::enable_shared_from_this<http_session>
{
    struct send_lambda
    {
        http_session& m_self;

        explicit send_lambda(http_session& self)
            : m_self(self)
        {}

        template<bool isRequest, class Body, class Fields>
        void operator()(http::message<isRequest, Body, Fields>&& msg) const
        {
            // The lifetime of the message has to extend
            // for the duration of the async operation so
            // we use a shared_ptr to manage it.
            auto sp = std::make_shared<
                http::message<isRequest, Body, Fields>>(std::move(msg));

            // Store a type-erased version of the shared
            // pointer in the class to keep it alive.
            m_self.m_res = sp;

            // Write the response
            http::async_write(
                m_self.m_socket,
                *sp,
                boost::asio::bind_executor(
                    m_self.m_strand,
                    std::bind(
                        &http_session::on_write,
                        m_self.shared_from_this(),
                        std::placeholders::_1,
                        std::placeholders::_2,
                        sp->need_eof())));
        }
    };

public:

    explicit http_session(tcp::socket socket)
        : m_socket{std::move(socket)}
        , m_strand{m_socket.get_executor()}
        , m_lambda{*this}
    {}

    void run()
    {
        do_read();
    }

    void do_read()
    {
        m_req = {};

        http::async_read(m_socket, m_buffer, m_req
            , boost::asio::bind_executor(m_strand
                , std::bind(&http_session::on_read
                    , shared_from_this(), _1, _2)));
    }

    void on_read(boost::system::error_code ec, std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        // this means they closed the connection
        if (ec == http::error::end_of_stream)
            return do_close();

        if (ec)
            return fail(ec, "read");

        // send the response
        handle_request(std::move(m_req), m_lambda);
    }

    void on_write(boost::system::error_code ec, std::size_t bytes_transferred
        , bool const close)
    {
        boost::ignore_unused(bytes_transferred);

        if (ec)
            return fail(ec, "write");

        if (close)
        {
            // this means we should close the connection, usually because
            // the response indicated the "Connection: close" semantic.
            return do_close();
        }

        // we're done with the response so delete it
        m_res.reset();

        do_read();
    }

    void do_close()
    {
        boost::system::error_code ec;
        m_socket.shutdown(tcp::socket::shutdown_send, ec);
    }

private:

    tcp::socket m_socket;
    boost::asio::strand<boost::asio::io_context::executor_type> m_strand;
    boost::beast::flat_buffer m_buffer;
    http::request<http::string_body> m_req;
    std::shared_ptr<void> m_res;
    send_lambda m_lambda;
};

class http_listener : public std::enable_shared_from_this<http_listener>
{
public:

    http_listener(boost::asio::io_context& ioc, tcp::endpoint const& endp)
        : m_acceptor{ioc}
        , m_socket{ioc}
    {
        boost::system::error_code ec;

        m_acceptor.open(endp.protocol(), ec);
        if (ec)
        {
            fail(ec, "open");
            return;
        }

        m_acceptor.set_option(boost::asio::socket_base::reuse_address(true), ec);
        if (ec)
        {
            fail(ec, "set_option");
            return;
        }

        m_acceptor.bind(endp, ec);
        if (ec)
        {
            fail(ec, "bind");
            return;
        }

        m_acceptor.listen(boost::asio::socket_base::max_listen_connections, ec);
        if (ec)
        {
            fail(ec, "listen");
            return;
        }
    }

    void run()
    {
        if (!m_acceptor.is_open())
            return;
        do_accept();
    }

    void do_accept()
    {
        m_acceptor.async_accept(m_socket
            , std::bind(&http_listener::on_accept, shared_from_this(), _1));
    }

    void on_accept(boost::system::error_code ec)
    {
        if (ec)
        {
            fail(ec, "accept");
        }
        else
        {
            std::make_shared<http_session>(std::move(m_socket))->run();
        }

        do_accept();
    }

private:

    tcp::acceptor m_acceptor;
    tcp::socket m_socket;
};

} // anonymous namespace

class session_rpc::impl
{
public:

    impl(std::string address, int port, int threads)
        : m_address{std::move(address)}
        , m_port{port}
        , m_threads{threads}
    {}

    void run()
    {
        boost::asio::io_context ioc{m_threads};
        tcp::endpoint endp{boost::asio::ip::make_address(m_address)
            , std::uint16_t(m_port)};

        std::make_shared<http_listener>(ioc, endp)->run();

        std::vector<std::thread> v;
        v.reserve(std::size_t(m_threads - 1));
        for (int i = m_threads - 1; i > 0; --i)
            v.emplace_back([&ioc] { ioc.run(); });
        ioc.run();
    }

    std::string address() const
    {
        return m_address;
    }

    int port() const 
    {
        return m_port;
    }

private:

    std::string m_address;
    int m_port;
    int m_threads;
    std::unique_ptr<lt::session> m_session;
};

session_rpc::session_rpc(std::string address, int port, int threads)
    : m_impl{new session_rpc::impl(std::move(address), port, threads)}
{}

session_rpc::session_rpc()
    : session_rpc("127.0.0.1", 8181, 2)
{}

session_rpc::~session_rpc() = default;

void session_rpc::run()
{
    m_impl->run();
}

std::string session_rpc::address() const
{
    return m_impl->address();
}

int session_rpc::port() const
{
    return m_impl->port();
}

} // ltrpc
