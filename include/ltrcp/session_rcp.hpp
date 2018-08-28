/*
 * Created by Angel Leon (@gubatron), Alden Torres (aldenml)
 * Copyright (c) 2018, FrostWire(R). All rights reserved.
 */

#ifndef LIBTORRENT_RPC_SESSION_RCP_HPP
#define LIBTORRENT_RPC_SESSION_RCP_HPP

#include <memory>

#include <libtorrent/aux_/disable_warnings_push.hpp>
#include <libtorrent/session.hpp>
#include <libtorrent/aux_/disable_warnings_pop.hpp>

namespace ltrcp
{

class session_rcp
{

private:

    std::unique_ptr<lt::session> m_session;
};

}

#endif // LIBTORRENT_RPC_SESSION_RCP_HPP
