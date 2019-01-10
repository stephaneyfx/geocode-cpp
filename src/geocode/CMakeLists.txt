add_executable(geocode
    client.hpp
    main.cpp
    protocol.hpp
    protocol_here.cpp
    protocol_here.hpp
    protocol_mapquest.cpp
    protocol_mapquest.hpp
    service.cpp
    service.hpp
)
recmkConfigureTarget(geocode)
target_include_directories(geocode PRIVATE ${CMAKE_SOURCE_DIR}/external)
target_link_libraries(geocode PUBLIC
    Boost::boost
    Boost::date_time
    Boost::filesystem
    Boost::program_options
    Boost::regex
    Boost::thread
    OpenSSL::Crypto
    OpenSSL::SSL
)
if(MSVC)
    target_compile_definitions(geocode PRIVATE
        _WIN32_WINNT=0x502
        _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS # Boost triggers this.
    )
endif()
recmkInstallTarget(geocode)
