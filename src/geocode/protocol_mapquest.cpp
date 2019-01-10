// Copyright (C) 2018 Stephane Raux. Distributed under the MIT license.

#include "protocol_mapquest.hpp"
#include <boost/beast.hpp>
#include <boost/format.hpp>
#include <nlohmann/json.hpp>
#include <sstream>

using json = nlohmann::json;

ProtocolMapQuest::ProtocolMapQuest(std::string key):
    key(std::move(key))
{}

Request ProtocolMapQuest::request(const std::string& location) const {
    auto uri = (boost::format("/geocoding/v1/address?key=%1%"
        "&location=%2%") % key % location).str();
    Request req(boost::beast::http::verb::get, uri, HTTP_VERSION);
    req.set(boost::beast::http::field::host, host());
    return req;
}

std::string ProtocolMapQuest::host() const {
    return "www.mapquestapi.com";
}

Coordinates ProtocolMapQuest::parse(const Response& resp) const {
    auto j = json::parse(resp.body());
    const auto& coords = j.at("/results/0/locations/0/latLng"_json_pointer);
    auto latitude = coords.at("lat").get<double>();
    auto longitude = coords.at("lng").get<double>();
    return Coordinates {latitude, longitude};
}
