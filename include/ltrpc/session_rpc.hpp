/*
 * Created by Angel Leon (@gubatron), Alden Torres (aldenml)
 * Copyright (c) 2018, FrostWire(R). All rights reserved.
 */

#ifndef LIBTORRENT_RPC_SESSION_RPC_HPP
#define LIBTORRENT_RPC_SESSION_RPC_HPP

#include <memory>
#include <string>
#include <functional>

namespace ltrpc
{

/**
 * This class is not thread-safe.
 */
class session_rpc
{
public:

    session_rpc();
    ~session_rpc();

    // movable
    session_rpc(session_rpc&&) noexcept;
    session_rpc& operator=(session_rpc&&) noexcept;
    // non-copyable
    session_rpc(session_rpc const&) = delete;
    session_rpc& operator=(session_rpc const&) = delete;

    void set_error_cb(std::function<void(int, std::string const&)> cb);

    /**
     * @param settings a json string with the setting values
     */
    void listen(std::string settings = "{}");

private:

    class impl;
    std::unique_ptr<impl> m_impl;
};

} // ltrpc

#endif // LIBTORRENT_RPC_SESSION_RPC_HPP
