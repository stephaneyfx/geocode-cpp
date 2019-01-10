// Copyright (C) 2018 Stephane Raux. Distributed under the MIT license.

#include "service.hpp"
#include <boost/asio.hpp>
#include <boost/program_options.hpp>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>

void print_help_header() {
    auto help = "Geocoding proxy service\n\n"
        "Usage: geocode [OPTIONS] --config=<CONFIG>\n";
    std::cout << help << std::endl;
}

int main(int argc, char** argv) {
    try {
        boost::program_options::options_description options("Options");
        options.add_options()
            ("address,a", boost::program_options::value<std::string>(),
                "IP address to start service on")
            ("config,c", boost::program_options::value<std::string>(),
                "Path to configuration file")
            ("help,h", "Display help message")
            ("port,p", boost::program_options::value<unsigned short>(),
                "Port to start service on");
        boost::program_options::variables_map vm;
        boost::program_options::store(
            boost::program_options::parse_command_line(argc, argv, options),
            vm);
        boost::program_options::notify(vm);
        if (vm.count("help")) {
            print_help_header();
            std::cout << options << std::endl;
            return 0;
        }
        if (!vm.count("config")) {
            std::cerr << "Missing \"config\" parameter" << std::endl;
            return 1;
        }
        auto config_path = vm["config"].as<std::string>();
        auto config = load_service_config(config_path);
        if (vm.count("address")) {
            config.endpoint.address(boost::asio::ip::make_address(
                vm["address"].as<std::string>()));
        }
        if (vm.count("port")) {
            config.endpoint.port(vm["port"].as<unsigned short>());
        }
        std::cout << "Geocoding service starting on " << config.endpoint
            << std::endl;
        boost::asio::io_context ioc;
        run_service(ioc, config);
        ioc.run();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Exiting with exception: " << e.what() << std::endl;
        return 1;
    }
}
