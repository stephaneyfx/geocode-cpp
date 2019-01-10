// Copyright (C) 2018 Stephane Raux. Distributed under the MIT license.

#include "client.hpp"
#include "protocol.hpp"
#include "service.hpp"
#include <boost/algorithm/string/predicate.hpp>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/system/error_code.hpp>
#include <cstdlib>
#include <iostream>
#include <iterator>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <utility>

using json = nlohmann::json;

namespace {
    class ServiceHandler: public std::enable_shared_from_this<ServiceHandler> {
    private:
        struct Priv {};

    public:
        explicit ServiceHandler(boost::asio::ip::tcp::socket socket,
            ServiceConfiguration config, Priv
        ):
            config(std::move(config)),
            socket(std::move(socket)),
            http_version(11),
            attempts(0)
        {}

        static std::shared_ptr<ServiceHandler> make(
            boost::asio::ip::tcp::socket socket, ServiceConfiguration config)
        {
            return std::make_shared<ServiceHandler>(std::move(socket),
                std::move(config), Priv {});
        }

        void process() {
            auto this_ = shared_from_this();
            boost::beast::http::async_read(socket, buffer, request,
                [this_] (boost::system::error_code ec, std::size_t) {
                    this_->on_request(ec);
                }
            );
        }

    private:
        void finalize_response(Response& response) {
            response.set(boost::beast::http::field::content_type,
                "application/json; charset=utf-8");
        }
        
        Response make_error_response(const Error& error) {
            json j = {
                {"Err", {
                    {"kind", to_string(error.kind)},
                    {"cause", {error.cause}}
                }}
            };
            boost::beast::http::status status = boost::beast::http::status::ok;
            switch (error.kind) {
            case ErrorKind::BackendFailure:
                break;
            case ErrorKind::BadRequest:
                status = boost::beast::http::status::bad_request;
                break;
            case ErrorKind::LocationNotFound:
                break;
            }
            Response resp(status, http_version, j.dump(2));
            finalize_response(resp);
            return resp;
        }

        Response make_success_response(const Coordinates& coords) {
            json j = {
                {"Ok", {
                    {"latitude", coords.latitude},
                    {"longitude", coords.longitude}
                }}
            };
            Response resp(boost::beast::http::status::ok, http_version,
                j.dump(2));
            finalize_response(resp);
            return resp;
        }
        
        void on_coordinates(ProtocolResult result) {
            if (std::get_if<Error>(&result)) {
                ++attempts;
                request_coordinates(location);
                return;
            }
            response = make_success_response(std::get<Coordinates>(result));
            respond();
        }
        
        void on_request(boost::system::error_code ec) {
            if (ec) {return;}
            http_version = request.version();
            const char prefix[] = "/geocode?location=";
            const auto& target = request.target();
            if (request.method() != boost::beast::http::verb::get
                || !boost::starts_with(target, prefix))
            {
                respond_with_error(Error {ErrorKind::BadRequest, ""});
                return;
            }
            location = target.substr(std::size(prefix) - 1).to_string();
            request_coordinates(location);
        }

        void on_responded(boost::system::error_code ec, std::size_t) {
            socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
        }

        void request_coordinates(const std::string& location) {
            if (attempts >= config.protocols.size()) {
                std::string cause;
                if (config.protocols.empty()) {
                    cause = "No backend service configured";
                }
                respond_with_error(Error {ErrorKind::LocationNotFound,
                    std::move(cause)});
                return;
            }
            std::visit([location, this] (auto proto) {
                auto this_ = shared_from_this();
                async_find_lat_long(socket.get_io_context(), proto, location,
                    std::bind(&ServiceHandler::on_coordinates, this_,
                    std::placeholders::_1));
            }, config.protocols[attempts]);
        }

        void respond() {
            auto this_ = shared_from_this();
            boost::beast::http::async_write(socket, response, std::bind(
                &ServiceHandler::on_responded, this_, std::placeholders::_1,
                std::placeholders::_2));
        }

        void respond_with_error(const Error& e) {
            response = make_error_response(e);
            respond();
        }

    private:
        ServiceConfiguration config;
        boost::asio::ip::tcp::socket socket;
        boost::beast::flat_buffer buffer;
        Request request;
        Response response;
        unsigned int http_version;
        unsigned int attempts;
        std::string location;
    };

    class Service: public std::enable_shared_from_this<Service> {
    private:
        struct Priv {};

    public:
        explicit Service(boost::asio::io_context& ioc,
            const ServiceConfiguration& config, Priv
        ):
            config(config),
            acceptor(ioc, config.endpoint)
        {}

        static std::shared_ptr<Service> make(boost::asio::io_context& ioc,
            const ServiceConfiguration& config)
        {
            return std::make_shared<Service>(ioc, config, Priv {});
        }

        void run() {accept();}

    private:
        void accept() {
            auto this_ = shared_from_this();
            auto cfg = config;
            acceptor.async_accept([cfg, this_] (boost::system::error_code ec,
                boost::asio::ip::tcp::socket socket)
            {
                if (!ec) {
                    auto handler = ServiceHandler::make(std::move(socket), cfg);
                    handler->process();
                }
                this_->accept();
            });
        }

    private:
        ServiceConfiguration config;
        boost::asio::ip::tcp::acceptor acceptor;
    };
}

ServiceConfiguration load_service_config(const boost::filesystem::path& path) {
    ServiceConfiguration conf {
        boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), 8080),
        {}
    };
    boost::filesystem::ifstream is(path);
    json j;
    is >> j;
    auto sock_addr_iter = j.find("sock_addr");
    if (sock_addr_iter != j.end()) {
        auto sock_addr = sock_addr_iter->get<std::string>();
        auto sep = sock_addr.find_last_of(':');
        if (sep == std::string::npos) {
            throw std::invalid_argument("Invalid address. Missing colon "
                "and port.");
        }
        auto addr = boost::asio::ip::make_address(sock_addr.substr(0, sep));
        auto port = static_cast<unsigned short>(std::atoi(sock_addr.substr(
            sep + 1).c_str()));
        conf.endpoint.address(addr);
        conf.endpoint.port(port);
    }
    auto finder = j.find("finder");
    if (finder != j.end()) {
        auto protocols = finder->find("protocols");
        if (protocols != finder->end()) {
            for (auto&& protocol: *protocols) {
                if (auto p = protocol.find("Here"); p != protocol.end()) {
                    auto app_id = p->at("app_id").get<std::string>();
                    auto app_code = p->at("app_code").get<std::string>();
                    ProtocolHere proto(std::move(app_id), std::move(app_code));
                    conf.protocols.push_back(std::move(proto));
                } else if (auto p = protocol.find("MapQuest");
                    p != protocol.end())
                {
                    auto key = p->at("key").get<std::string>();
                    ProtocolMapQuest proto(std::move(key));
                    conf.protocols.push_back(std::move(proto));
                }
            }
        }
    }
    return conf;
}

void run_service(boost::asio::io_context& io_ctx,
    const ServiceConfiguration& config)
{
    auto service = Service::make(io_ctx, config);
    service->run();
}
