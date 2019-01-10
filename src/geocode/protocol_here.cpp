// Copyright (C) 2018 Stephane Raux. Distributed under the MIT license.

#include "protocol_here.hpp"
#include <boost/beast.hpp>
#include <boost/format.hpp>
#include <nlohmann/json.hpp>
#include <sstream>

using json = nlohmann::json;

ProtocolHere::ProtocolHere(std::string app_id, std::string app_code):
    app_id(std::move(app_id)),
    app_code(std::move(app_code))
{}

Request ProtocolHere::request(const std::string& location) const {
    auto uri = (boost::format("/6.2/geocode.json?app_id=%1%&app_code=%2%"
        "&searchtext=%3%") % app_id % app_code % location).str();
    Request req(boost::beast::http::verb::get, uri, HTTP_VERSION);
    req.set(boost::beast::http::field::host, host());
    return req;
}

std::string ProtocolHere::host() const {
    return "geocoder.api.here.com";
}

Coordinates ProtocolHere::parse(const Response& resp) const {
    auto j = json::parse(resp.body());
    const auto& coords = j.at("/Response/View/0/Result/0/Location/"
        "DisplayPosition"_json_pointer);
    auto latitude = coords.at("Latitude").get<double>();
    auto longitude = coords.at("Longitude").get<double>();
    return Coordinates {latitude, longitude};
}
