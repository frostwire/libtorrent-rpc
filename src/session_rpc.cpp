/*
 * Created by Angel Leon (@gubatron), Alden Torres (aldenml)
 * Copyright (c) 2018, FrostWire(R). All rights reserved.
 */

#include "ltrpc/session_rpc.hpp"

#include <thread>
#include <vector>
#include <memory>

#include "ltrpc/aux/json_types.hpp"
#include "ltrpc/aux/handle_request.hpp"

#include <libtorrent/aux_/disable_warnings_push.hpp>

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/bind_executor.hpp>

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>

#include <libtorrent/session.hpp>
#include <libtorrent/address.hpp>
#include <libtorrent/socket.hpp>
#include <libtorrent/settings_pack.hpp>

#include "ltrpc/aux/json.hpp"

#include <libtorrent/aux_/disable_warnings_pop.hpp>

using namespace std::placeholders;

using tcp = boost::asio::ip::tcp;

namespace http = boost::beast::http;

using json = nlohmann::json;

namespace ltrpc
{

struct session_logger
{
    virtual void on_error(int ec, std::string const& msg) const = 0;

protected:
    ~session_logger() = default;
};

namespace
{

std::string const rpc_listen_address_key = "rpc_listen_address";
std::string const rpc_listen_port_key = "rpc_listen_port";
std::string const rpc_num_threads_key = "rpc_num_threads";

std::string const rpc_listen_address_default = "127.0.0.1";
int const rpc_listen_port_default = 8181;
int const rpc_num_threads_default = 2;

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

    explicit http_session(tcp::socket socket, session_logger& logger)
        : m_socket{std::move(socket)}
        , m_logger{logger}
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
        {
            m_logger.on_error(ec.value(), "read");
            return;
        }

        // send the response
        aux::handle_request(std::move(m_req), m_lambda);
    }

    void on_write(boost::system::error_code ec, std::size_t bytes_transferred
        , bool const close)
    {
        boost::ignore_unused(bytes_transferred);

        if (ec)
        {
            m_logger.on_error(ec.value(), "write");
            return;
        }

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
    session_logger& m_logger;
    boost::asio::strand<boost::asio::io_context::executor_type> m_strand;
    boost::beast::flat_buffer m_buffer;
    http::request<http::string_body> m_req;
    std::shared_ptr<void> m_res;
    send_lambda m_lambda;
};

class http_listener : public std::enable_shared_from_this<http_listener>
{
public:

    http_listener(boost::asio::io_context& ioc, tcp::endpoint const& endp
        , session_logger& logger)
        : m_acceptor{ioc}
        , m_socket{ioc}
        , m_logger{logger}
    {
        boost::system::error_code ec;

        m_acceptor.open(endp.protocol(), ec);
        if (ec)
        {
            m_logger.on_error(ec.value(), "open");
            return;
        }

        m_acceptor.set_option(boost::asio::socket_base::reuse_address(true), ec);
        if (ec)
        {
            m_logger.on_error(ec.value(), "set_option");
            return;
        }

        m_acceptor.bind(endp, ec);
        if (ec)
        {
            m_logger.on_error(ec.value(), "bind");
            return;
        }

        m_acceptor.listen(boost::asio::socket_base::max_listen_connections, ec);
        if (ec)
        {
            m_logger.on_error(ec.value(), "listen");
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
            m_logger.on_error(ec.value(), "accept");
        }
        else
        {
            auto session = std::make_shared<http_session>(std::move(m_socket)
                , m_logger);
            session->run();
        }

        do_accept();
    }

private:

    tcp::acceptor m_acceptor;
    tcp::socket m_socket;
    session_logger& m_logger;
};

} // anonymous namespace

class session_rpc::impl final : public session_logger
{
public:

    impl() = default;

    void set_error_cb(std::function<void(int, std::string const&)> cb);

    void listen(std::string const settings);

    void on_error(int ec, std::string const& msg) const override;

private:

    std::function<void(int, std::string const&)> m_error_cb;
    std::unique_ptr<lt::session> m_session;
};

void session_rpc::impl::set_error_cb(
    std::function<void(int, std::string const&)> cb)
{
    m_error_cb = std::move(cb);
}

void session_rpc::impl::listen(std::string const settings)
{
    auto sett = json::parse(settings);

    auto const listen_address = sett.value(rpc_listen_address_key
        , rpc_listen_address_default);
    int const listen_port = sett.value(rpc_listen_port_key
        , rpc_listen_port_default);
    int const num_threads = sett.value(rpc_num_threads_key
        , rpc_num_threads_default);

    lt::settings_pack sp = sett;

    // in C++ 17 use std::make_unique
    m_session = std::unique_ptr<lt::session>(new lt::session(std::move(sp)));

    boost::asio::io_context ioc{num_threads};
    tcp::endpoint endp{lt::make_address(listen_address)
        , std::uint16_t(listen_port)};

    auto listener = std::make_shared<http_listener>(ioc, endp, *this);
    listener->run();

    // run the I/O service on the requested number of threads
    std::vector<std::thread> v;
    v.reserve(std::size_t(num_threads - 1));
    for (int i = num_threads - 1; i > 0; --i)
        v.emplace_back([&ioc] { ioc.run(); });
    ioc.run();
}

void session_rpc::impl::on_error(int ec, std::string const& msg) const
{
    if (m_error_cb)
        m_error_cb(ec, msg);
}

session_rpc::session_rpc()
    : m_impl{new impl()}
{}

session_rpc::~session_rpc() = default;

session_rpc::session_rpc(session_rpc&&) noexcept = default;
session_rpc& session_rpc::operator=(session_rpc&&) noexcept = default;

void session_rpc::set_error_cb(std::function<void(int, std::string const&)> cb)
{
    m_impl->set_error_cb(std::move(cb));
}

void session_rpc::listen(std::string settings)
{
    m_impl->listen(std::move(settings));
}

} // ltrpc
