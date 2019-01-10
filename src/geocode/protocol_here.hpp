// Copyright (C) 2018 Stephane Raux. Distributed under the MIT license.

#pragma once

#include "protocol.hpp"
#include <string>

class ProtocolHere {
public:
    explicit ProtocolHere(std::string app_id, std::string app_code);
    std::string host() const;
    Coordinates parse(const Response& resp) const;
    Request request(const std::string& location) const;

private:
    std::string app_id;
    std::string app_code;
};
