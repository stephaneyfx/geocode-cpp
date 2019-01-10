// Copyright (C) 2018 Stephane Raux. Distributed under the MIT license.

#pragma once

#include "protocol.hpp"
#include <boost/asio/connect.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/beast.hpp>
#include <boost/system/error_code.hpp>
#include <functional>
#include <memory>
#include <string>
#include <utility>

template <typename Protocol, typename F>
class Client: public std::enable_shared_from_this<Client<Protocol, F>> {
private:
    struct Priv {};

public:
    explicit Client(boost::asio::io_context& ioc, Protocol protocol,
        std::string location, F cont, Priv
    ):
        protocol(std::move(protocol)),
        location(std::move(location)),
        continuation(std::move(cont)),
        resolver(ioc),
        ssl_context(boost::asio::ssl::context::tlsv12_client),
        stream(ioc, ssl_context)
    {}
    
    static std::shared_ptr<Client> make(boost::asio::io_context& ioc,
        Protocol protocol, std::string location, F cont)
    {
        return std::make_shared<Client>(ioc, std::move(protocol),
            std::move(location), std::move(cont), Priv {});
    }

    void run() {resolve();}

private:
    void finish_with_error(boost::system::error_code ec) {
        finish_with_error(ErrorKind::BackendFailure, ec.message());
    }

    void finish_with_error(ErrorKind kind, std::string cause) {
        Error e{kind, std::move(cause)};
        continuation(ProtocolResult{std::move(e)});
    }
    
    void on_connect(boost::system::error_code ec) {
        auto this_ = this->shared_from_this();
        if (ec) {
            finish_with_error(ec);
            return;
        }
        stream.async_handshake(boost::asio::ssl::stream_base::client,
            std::bind(&Client::on_handshake, this_, std::placeholders::_1));
    }

    void on_handshake(boost::system::error_code ec) {
        auto this_ = this->shared_from_this();
        if (ec) {
            finish_with_error(ec);
            return;
        }
        request = protocol.request(location);
        boost::beast::http::async_write(stream, request, std::bind(
            &Client::on_request_sent, this_, std::placeholders::_1,
            std::placeholders::_2));
    }

    void on_request_sent(boost::system::error_code ec, std::size_t) {
        auto this_ = this->shared_from_this();
        if (ec) {
            finish_with_error(ec);
            return;
        }
        boost::beast::http::async_read(stream, buffer, response, std::bind(
            &Client::on_response, this_, std::placeholders::_1,
            std::placeholders::_2));
    }

    void on_resolve(boost::system::error_code ec,
        boost::asio::ip::tcp::resolver::results_type results)
    {
        auto this_ = this->shared_from_this();
        if (ec) {
            finish_with_error(ec);
            return;
        }
        boost::asio::async_connect(stream.next_layer(), results.begin(),
            results.end(), std::bind(&Client::on_connect, this_,
            std::placeholders::_1));
    }

    void on_response(boost::system::error_code ec, std::size_t) {
        auto this_ = this->shared_from_this();
        if (ec) {
            finish_with_error(ec);
            return;
        }
        stream.async_shutdown(std::bind(&Client::on_shutdown, this_,
            std::placeholders::_1));
    }

    void on_shutdown(boost::system::error_code) {
        Coordinates coords{};
        try {
            coords = protocol.parse(response);
        } catch (const std::exception& e) {
            finish_with_error(ErrorKind::BackendFailure, e.what());
            return;
        }
        continuation(ProtocolResult {coords});
    }

    void resolve() {
        auto this_ = this->shared_from_this();
        resolver.async_resolve(protocol.host(), "https",
            std::bind(&Client::on_resolve, this_, std::placeholders::_1,
            std::placeholders::_2));
    }

private:
    Protocol protocol;
    std::string location;
    F continuation;
    boost::asio::ip::tcp::resolver resolver;
    boost::asio::ssl::context ssl_context;
    boost::asio::ssl::stream<boost::asio::ip::tcp::socket> stream;
    Request request;
    boost::beast::flat_buffer buffer;
    Response response;
};

template <typename Protocol, typename F>
std::shared_ptr<Client<Protocol, F>> make_client(boost::asio::io_context& ioc,
    Protocol protocol, std::string location, F cont)
{
    return Client<Protocol, F>::make(ioc, std::move(protocol),
        std::move(location), std::move(cont));
}

template <typename Protocol, typename F>
void async_find_lat_long(boost::asio::io_context& ioc, Protocol protocol,
    std::string location, F cont)
{
    auto client = make_client(ioc, std::move(protocol), std::move(location),
        std::move(cont));
    client->run();
}
