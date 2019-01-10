// Copyright (C) 2018 Stephane Raux. Distributed under the MIT license.

#pragma once

#include "protocol_here.hpp"
#include "protocol_mapquest.hpp"
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/filesystem/path.hpp>
#include <variant>
#include <vector>

using AnyProtocol = std::variant<ProtocolHere, ProtocolMapQuest>;

struct ServiceConfiguration {
    boost::asio::ip::tcp::endpoint endpoint;
    std::vector<AnyProtocol> protocols;
};

ServiceConfiguration load_service_config(const boost::filesystem::path& path);
void run_service(boost::asio::io_context& ioc,
    const ServiceConfiguration& config);
