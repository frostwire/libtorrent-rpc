/*
 * Created by Angel Leon (@gubatron), Alden Torres (aldenml)
 * Copyright (c) 2018, FrostWire(R). All rights reserved.
 */

#include <iostream>

#include <ltrpc/session_rpc.hpp>

using namespace ltrpc;

int main()
{
    session_rpc server;

    std::cout << "Hello client_test (server@" << server.address() << ":" << server.port() << ")" << std::endl;
    server.run();

    std::cout << "Hello client_test over" << std::endl;
    return 0;
}
