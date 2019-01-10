# Copyright (C) 2017 Stephane Raux. Distributed under the MIT license.

cmake_minimum_required(VERSION 3.8.2)
include(CMakePackageConfigHelpers)

macro(recmkConfigureProject)
    if(MSVC)
        string(REGEX REPLACE "/W[0-9]" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
        string(REGEX REPLACE "/W[0-9]" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
    endif()
endmacro()

macro(recmkInstallProject)
    set(binConfigDir ${CMAKE_BINARY_DIR}/lib/cmake/${PROJECT_NAME})
    set(configFile ${binConfigDir}/${PROJECT_NAME}Config.cmake)
    set(srcConfigFile ${CMAKE_SOURCE_DIR}/cmake/${PROJECT_NAME}Config.cmake)
    set(packageVersionFile ${binConfigDir}/${PROJECT_NAME}ConfigVersion.cmake)
    if(EXISTS ${srcConfigFile})
        file(COPY ${srcConfigFile} DESTINATION ${binConfigDir})
    else()
        file(WRITE ${configFile} "include(\${CMAKE_CURRENT_LIST_DIR}/${PROJECT_NAME}.cmake)\n")
    endif()
    write_basic_package_version_file(${packageVersionFile}
        COMPATIBILITY SameMajorVersion)
    install(EXPORT ${PROJECT_NAME} DESTINATION lib/cmake/${PROJECT_NAME}
        NAMESPACE ${PROJECT_NAME}:: FILE ${PROJECT_NAME}.cmake)
    install(FILES ${packageVersionFile} ${configFile}
        DESTINATION lib/cmake/${PROJECT_NAME})
    export(EXPORT ${PROJECT_NAME} NAMESPACE ${PROJECT_NAME}::
        FILE ${binConfigDir}/${PROJECT_NAME}.cmake)
endmacro()

macro(recmkPackageProject)
    set(CPACK_GENERATOR 7Z)
    set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
    set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
    set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
    set(CPACK_PACKAGE_FILE_NAME ${PROJECT_NAME}-${PROJECT_VERSION}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR})
endmacro()

function(recmkConfigureTarget target)
    target_compile_features(${target} PRIVATE cxx_std_17)
    set_target_properties(${target} PROPERTIES CXX_EXTENSIONS FALSE)
    target_include_directories(${target}
        PUBLIC $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src>
        PUBLIC $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/src>
        INTERFACE $<INSTALL_INTERFACE:$<INSTALL_PREFIX>/include>
    )
    get_target_property(targetType ${target} TYPE)
    string(TOUPPER ${target} upperCaseTarget)
    set(apiMacro ${upperCaseTarget}_API)
    if(WIN32 AND (targetType STREQUAL SHARED_LIBRARY))
        target_compile_definitions(${target}
            PRIVATE "${apiMacro}=__declspec(dllexport)"
            INTERFACE "${apiMacro}=__declspec(dllimport)"
        )
    else()
        target_compile_definitions(${target} PUBLIC "${apiMacro}=")
    endif()
    if(MSVC)
        target_compile_options(${target} PRIVATE
            /bigobj
            /W4 # Warning level 4.
            /WX # Treat warnings as errors.
            /wd4251 # DLL interface needed.
            /wd4456 # Hiding local declaration.
            /wd4458 # Hiding class member.
            /wd4503 # Decorated name maximum length exceeded.
            /wd4512 # Assignment operator implicitly defined as deleted.
            /wd4913 # Built-in comma operator used.
            /we4062 # Enumerator not handled in switch.
        )
        target_compile_definitions(${target} PRIVATE
            _CRT_NON_CONFORMING_SWPRINTFS
            _CRT_SECURE_NO_WARNINGS
            _SCL_SECURE_NO_WARNINGS
            UNICODE
            _UNICODE
        )
    else()
        target_compile_options(${target} PRIVATE -Wall -Wpedantic -Wextra
            -Werror -Wno-mismatched-tags)
    endif()
endfunction()

function(recmkInstallTarget target)
    install(TARGETS ${target} EXPORT ${PROJECT_NAME}
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin
    )
endfunction()
