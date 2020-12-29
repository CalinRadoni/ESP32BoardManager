cmake_minimum_required(VERSION 3.5)

set(EXTRA_COMPONENT_DIRS
        "$ENV{CodeLib_PATH}/ESP32")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

set(IDF_Dev_Version "4.1")
if (NOT "${IDF_VERSION_MAJOR}.${IDF_VERSION_MINOR}" STREQUAL "${IDF_Dev_Version}")
    message(WARNING "The project was tested with ESP-IDF v${IDF_Dev_Version}, it may not work with v${IDF_VERSION_MAJOR}.${IDF_VERSION_MINOR}.${IDF_VERSION_PATCH}")
endif()

set(SUPPORTED_TARGETS esp32)

project(Example-ESP32BoardManager)
