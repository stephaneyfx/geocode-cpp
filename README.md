**A similar service was implemented in Rust and is much easier to build:** [Geocode in Rust](https://github.com/stephaneyfx/geocode-rs.git)

# Purpose

This is a crude REST geocode proxy service. It consists of a command-line application starting a HTTP server providing a REST API to geocode addresses. Geocoding consists in finding the latitude and longitude of an address or part of an address.

Note that this service does not geocode by itself and merely forwards requests to other online services. It currently delegates to the following services:

- [Here](https://developer.here.com/documentation/geocoder/topics/quick-start-geocode.html)
- [MapQuest](https://developer.mapquest.com/documentation/geocoding-api/)

# Building

The following dependencies are required:

- [boost](https://www.boost.org/) [1.68.0 or newer]
- [OpenSSL](https://www.openssl.org/) [1.1.1a or newer]
- [nlohmann's JSON library](https://github.com/nlohmann/json) [Already part of the repository under "external"]

The following tools need to be installed:

- [git](https://git-scm.com/) [2.16.2 or newer]
- [CMake](https://cmake.org/download/) [3.13.2 or newer]
- A C++17 compiler is required; one of the following:
  - Visual Studio 2017
  - clang++ 7.0.1
  - g++ 8.2.1

Clone the repository:

```sh
git clone https://github.com/stephaneyfx/geocode-cpp.git
```

Run CMake at the root of the repository. The following CMake variables may need to be set to help CMake find boost and OpenSSL, especially on Windows:

- `BOOST_ROOT`
- `BOOST_LIBRARYDIR`
- `OPENSSL_INCLUDE_DIR`

For example, to build with ninja on Linux, assuming the working directory is the root of the repository:

```sh
mkdir build
cd build
cmake -G Ninja -D CMAKE_BUILD_TYPE=Release ..
ninja
```

# Configuration

The proxy service needs some keys to use the services it delegates the geocoding to. These keys must be provided through a configuration file with the following format:

```json
{
    "sock_addr": "127.0.0.1:8080",
    "finder": {
        "protocols": [
            {
                "Here": {
                    "app_id": "...",
                    "app_code": "..."
                }
            },
            {
                "MapQuest": {
                    "key": "..."
                }
            }
        ]
    }
}
```

The `protocols` array can contain either or both backend services.

# Running the service

To start the service, execute the following (adapt syntax for Windows):

```sh
./bin/geocode -c /path/to/config.json
```

Help is available by running:

```sh
./bin/geocode --help
```

# REST API
## Geocode

### Request
`http://hostname/geocode?location=350+w+georgia+st,+Vancouver`

### Response
#### Success
```json
{
  "Ok": {
    "latitude": 49.2801599,
    "longitude": -123.1147572
  }
}
```

#### Error
```json
{
  "Err": {
    "kind": "BadRequest",
    "cause": []
  }
}
```

If the location could not be found, the error kind is `LocationNotFound`.

# Supported platforms

Tested on Windows 10 and ArchLinux.
