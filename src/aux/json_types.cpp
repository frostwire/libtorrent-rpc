/*
 * Created by Angel Leon (@gubatron), Alden Torres (aldenml)
 * Copyright (c) 2018, FrostWire(R). All rights reserved.
 */

#include "ltrpc/aux/json_types.hpp"

#include <string>

namespace ltrpc { namespace aux
{

void to_json(json& j, lt::settings_pack const& sp)
{
    j = json{};

    for (int i = 0; i < lt::settings_pack::num_string_settings; ++i)
    {
        int const index = lt::settings_pack::string_type_base + i;
        if (sp.has_val(index))
        {
            std::string name = lt::name_for_setting(index);
            std::string value = sp.get_str(index);

            j.emplace(std::move(name), std::move(value));
        }
    }

    for (int i = 0; i < lt::settings_pack::num_int_settings; ++i)
    {
        int const index = lt::settings_pack::int_type_base + i;
        if (sp.has_val(index))
        {
            std::string name = lt::name_for_setting(index);
            int value = sp.get_int(index);

            j.emplace(std::move(name), value);
        }
    }

    for (int i = 0; i < lt::settings_pack::num_bool_settings; ++i)
    {
        int const index = lt::settings_pack::bool_type_base + i;
        if (sp.has_val(index))
        {
            std::string name = lt::name_for_setting(index);
            bool value = sp.get_bool(index);

            j.emplace(std::move(name), value);
        }
    }
}

void from_json(json const& j, lt::settings_pack& sp)
{
    for (auto it = j.begin(); it != j.end(); ++it)
    {
        std::string const& name = it.key();
        json const value = it.value();

        int const index = lt::setting_by_name(name);
        if (index == -1) continue;

        if (value.is_string())
            sp.set_str(index, value);
        else if (value.is_number_integer())
            sp.set_int(index, value);
        else if (value.is_boolean())
            sp.set_bool(index, value);
    }
}

}} // ltrpc::aux
