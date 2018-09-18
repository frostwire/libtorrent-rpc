/*
 * Created by Angel Leon (@gubatron), Alden Torres (aldenml)
 * Copyright (c) 2018, FrostWire(R). All rights reserved.
 */

#ifndef LIBTORRENT_RPC_HANDLE_REQUEST_HPP
#define LIBTORRENT_RPC_HANDLE_REQUEST_HPP

#include <libtorrent/aux_/disable_warnings_push.hpp>

#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>

#include <libtorrent/aux_/disable_warnings_pop.hpp>

namespace ltrpc { namespace aux
{

namespace http = boost::beast::http;

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

}} // ltrpc::aux


#endif // LIBTORRENT_RPC_HANDLE_REQUEST_HPP
