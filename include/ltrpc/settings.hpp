/*
 * Created by Angel Leon (@gubatron), Alden Torres (aldenml)
 * Copyright (c) 2018, FrostWire(R). All rights reserved.
 */

#ifndef LIBTORRENT_RPC_SETTINGS_HPP
#define LIBTORRENT_RPC_SETTINGS_HPP

#include <string>

namespace ltrpc
{

struct setting_keys
{
    static std::string const rpc_listen_address;
    static std::string const rpc_listen_port;
    static std::string const rpc_num_threads;
};

} // ltrpc

#endif // LIBTORRENT_RPC_SETTINGS_HPP
