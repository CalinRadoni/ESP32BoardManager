set(c_SOURCE_FILES
    "ExampleBoard.cpp"
    "ExampleBoardHTTPSrv.cpp"
)

set(c_PRIVATE_REQUIREMENTS
    ESP32BoardManager
    ESP32HAL
)

idf_component_register(
    SRCS ${c_SOURCE_FILES}
    INCLUDE_DIRS "."
    PRIV_REQUIRES ${c_PRIVATE_REQUIREMENTS}
)
