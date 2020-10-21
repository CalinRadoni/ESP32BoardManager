set(c_SOURCE_FILES
    "src/Board.cpp"
    "src/Configuration.cpp"
    "src/Events.cpp"
    "src/pax_http_server.cpp"
    "src/WiFiManager.cpp"
    "src/WiFiConfig.cpp"
)

set(c_PRIVATE_REQUIREMENTS
    nvs_flash
)

set(c_REQUIREMENTS
    esp_http_server
    esp_netif
    json
    mdns
    ESP32HAL
    ESP32SimpleOTA
)

set(c_EMBED_FILES
    "${CMAKE_SOURCE_DIR}/html/web/index.html.gz"
)

idf_component_register(
    SRCS ${c_SOURCE_FILES}
    INCLUDE_DIRS "src"
    REQUIRES ${c_REQUIREMENTS}
    PRIV_REQUIRES ${c_PRIVATE_REQUIREMENTS}
    EMBED_FILES ${c_EMBED_FILES}
)
