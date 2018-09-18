/*
 * Created by Angel Leon (@gubatron), Alden Torres (aldenml)
 * Copyright (c) 2018, FrostWire(R). All rights reserved.
 */

#ifndef LIBTORRENT_RPC_JSON_TYPES_HPP
#define LIBTORRENT_RPC_JSON_TYPES_HPP

#include <libtorrent/aux_/disable_warnings_push.hpp>

#include <libtorrent/settings_pack.hpp>

#include "ltrpc/aux/json.hpp"

#include <libtorrent/aux_/disable_warnings_pop.hpp>

using nlohmann::json;

namespace ltrpc { namespace aux
{

void to_json(json &j, lt::settings_pack const &sp);
void from_json(json const &j, lt::settings_pack &sp);

}} // ltrpc::aux

namespace nlohmann
{

template <>
struct adl_serializer<lt::settings_pack>
{
    static void to_json(json& j, lt::settings_pack const& sp)
    {
        ltrpc::aux::to_json(j, sp);
    }

    static void from_json(json const& j, lt::settings_pack& sp)
    {
        ltrpc::aux::from_json(j, sp);
    }
};

} // nlohmann

#endif // LIBTORRENT_RPC_JSON_TYPES_HPP
