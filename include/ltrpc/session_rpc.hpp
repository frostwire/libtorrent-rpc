/*
 * Created by Angel Leon (@gubatron), Alden Torres (aldenml)
 * Copyright (c) 2018, FrostWire(R). All rights reserved.
 */

#ifndef LIBTORRENT_RPC_SESSION_RPC_HPP
#define LIBTORRENT_RPC_SESSION_RPC_HPP

#include <memory>
#include <string>

namespace ltrpc
{

class session_rpc
{
public:

    session_rpc(std::string address, int port, int threads);
    session_rpc();
    ~session_rpc();

    void run();

private:

    class impl;
    std::unique_ptr<impl> m_impl;
};

} // ltrpc

#endif // LIBTORRENT_RPC_SESSION_RPC_HPP
