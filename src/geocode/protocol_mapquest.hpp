// Copyright (C) 2018 Stephane Raux. Distributed under the MIT license.

#pragma once

#include "protocol.hpp"
#include <string>

class ProtocolMapQuest {
public:
    explicit ProtocolMapQuest(std::string key);
    std::string host() const;
    Coordinates parse(const Response& resp) const;
    Request request(const std::string& location) const;

private:
    std::string key;
};
