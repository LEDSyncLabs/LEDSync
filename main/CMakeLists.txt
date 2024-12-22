idf_component_register(SRCS "main.cpp" "modules/gatt_client/gatt_client.cpp" "modules/http_server/http_server.cpp" "modules/wifi/wifi_manager.cpp" "modules/utils/utils.cpp"
                    INCLUDE_DIRS "." "modules/gatt_client" "modules/wifi" "modules/utils" "modules/http_server")
