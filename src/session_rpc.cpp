/*
 * Created by Angel Leon (@gubatron), Alden Torres (aldenml)
 * Copyright (c) 2018, FrostWire(R). All rights reserved.
 */

#include "ltrpc/session_rpc.hpp"

#include <libtorrent/aux_/disable_warnings_push.hpp>
#include <libtorrent/session.hpp>
#include <libtorrent/aux_/disable_warnings_pop.hpp>

namespace ltrpc
{

class session_rpc::impl
{

private:

    std::unique_ptr<lt::session> m_session;
};

} // ltrpc
