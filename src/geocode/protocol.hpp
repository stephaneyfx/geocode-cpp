// Copyright (C) 2018 Stephane Raux. Distributed under the MIT license.

#pragma once

#include <boost/beast/http/message.hpp>
#include <boost/beast/http/string_body.hpp>
#include <stdexcept>
#include <string>
#include <variant>

using Request = boost::beast::http::request<boost::beast::http::string_body>;
using Response = boost::beast::http::response<boost::beast::http::string_body>;

const int HTTP_VERSION = 11;

// Coordinates in degrees.
struct Coordinates {
    double latitude;
    double longitude;
};

enum class ErrorKind {
    BackendFailure,
    BadRequest,
    LocationNotFound
};

struct Error {
    ErrorKind kind;
    std::string cause;
};

using ProtocolResult = std::variant<Coordinates, Error>;

inline std::string to_string(ErrorKind kind) {
    switch (kind) {
    case ErrorKind::BackendFailure: return "BackendFailure";
    case ErrorKind::BadRequest: return "BadRequest";
    case ErrorKind::LocationNotFound: return "LocationNotFound";
    }
    throw std::logic_error("Unreachable");
}
